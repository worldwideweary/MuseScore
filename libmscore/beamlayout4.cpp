/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2023 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "beam.h"
#include "groups.h"
#include "tuplet.h"
#include "chord.h"
#include "note.h"
#include "measure.h"
#include "score.h"
#include "staff.h"
#include "stem.h"
#include "system.h"
#include "beamlayout4.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Ms {
namespace {

// The algorithms below are adapted from Studio v4.7.5's
// engraving/rendering/score/beamtremololayout.cpp. Keep all temporary
// state local: this code is used in both provisional and final layout.
struct NotePosition {
    int line;
    int staff;
    NotePosition(int l, int s) : line(l), staff(s) {}
    bool operator<(const NotePosition& p) const { return staff != p.staff ? staff < p.staff : line < p.line; }
    bool operator>(const NotePosition& p) const { return p < *this; }
    bool operator==(const NotePosition& p) const { return line == p.line && staff == p.staff; }
    bool operator>=(const NotePosition& p) const { return !(*this < p); }
    bool operator<=(const NotePosition& p) const { return !(p < *this); }
};
enum class SlopeConstraint { NO_CONSTRAINT, FLAT, SMALL_SLOPE };
enum class ChordBeamAnchorType { Start, Middle, End };
enum class CrossStaffBeamPosition { INVALID, ABOVE, BELOW, BETWEEN };
struct LayoutData {
    CrossStaffBeamPosition crossStaffBeamPos = CrossStaffBeamPosition::INVALID;
    Beam* beam;
    bool up;
    bool isGrace;
    qreal spatium;
    qreal beamWidth;
    qreal beamDist;
    int beamSpacing;
    qreal magnitude;
    QPointF startAnchor;
    QPointF endAnchor;
    std::vector<ChordRest*> elements;
    std::vector<NotePosition> notePositions;
    qreal mag() const { return magnitude; }
};
static const int _maxSlopes[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
static int strokeCount(const LayoutData*, const ChordRest* cr) { return qBound(1, cr->beams(), 8); }
static QPointF chordBeamAnchor(const LayoutData*, const ChordRest*, ChordBeamAnchorType);
static double chordBeamAnchorX(const LayoutData*, const ChordRest*, ChordBeamAnchorType);
static double defaultStemLength4(const Chord*);
static int minStemLength(const ChordRest* cr, const LayoutData* ldata);
static void offsetBeamToRemoveCollisions(const Beam* item, const LayoutData* ldata,
                                                     const std::vector<ChordRest*>& chordRests,
                                                     int& dictator,
                                                     int& pointer,
                                                     const double startX, const double endX,
                                                     bool isFlat, bool isStartDictator);
static void offsetBeamWithAnchorShortening(const LayoutData* ldata, const std::vector<ChordRest*>& chordRests,
                                                       int& dictator, int& pointer, int staffLines, bool isStartDictator,
                                                       int stemLengthDictator, const int targetLine);
static bool isBeamInsideStaff(int yPos, int staffLines, bool allowFloater);
static void setSmallInnerBeamPos(const LayoutData* ldata, int& dictator, int& pointer, const int staffLines,
                                             const bool isFlat, const bool isSmall, const Score* score);
static int getOuterBeamPosOffset(const LayoutData* ldata, int innerBeam, int beamCount, int staffLines);
static bool isValidBeamPosition(const bool isUp, int yPos, bool isStart, bool isAscending, bool isFlat, int staffLines,
                                            bool isOuter);
static bool is64thBeamPositionException(const int beamSpacing, int& yPos, int staffLines);
static int findValidBeamOffset(const LayoutData* ldata, int outer, int beamCount, int staffLines, bool isStart,
                                           bool isAscending, bool isFlat);
static void setValidBeamPositions(const LayoutData* ldata, int& dictator, int& pointer, int beamCountD,
                                              int beamCountP,
                                              int staffLines,
                                              bool isStartDictator,
                                              bool isFlat, bool isAscending);
static void addMiddleLineSlant(const LayoutData* ldata, int& dictator, int& pointer, int beamCount, int targetLine,
                                           int interval, int desiredSlant);
static void add8thSpaceSlant(LayoutData* ldata, QPointF& dictatorAnchor, int dictator, int pointer, int beamCount,
                                         int interval, int targetLine, bool isFlat);
static SlopeConstraint getSlopeConstraint(const LayoutData* ldata, const NotePosition& startPos,
                                                      const NotePosition& endPos);
static int getMaxSlope(const LayoutData* ldata);

static int minStemLength(const ChordRest* cr, const LayoutData* ldata)
{
    // min stem lengths in quarter spaces according to how many beams there are (starting with 1)
    static constexpr int BEAMS_COUNT = 8;
    static constexpr int minStemLengths[BEAMS_COUNT] = { 11, 13, 15, 18, 21, 24, 27, 30 };
    int beams = strokeCount(ldata, cr);
    if (beams > BEAMS_COUNT) {
        return minStemLengths[BEAMS_COUNT - 1];
    }

    return minStemLengths[std::max(beams - 1, 0)];
}

static void offsetBeamToRemoveCollisions(const Beam* item, const LayoutData* ldata,
                                                     const std::vector<ChordRest*>& chordRests,
                                                     int& dictator,
                                                     int& pointer,
                                                     const double startX, const double endX,
                                                     bool isFlat, bool isStartDictator)
{
    if (endX == startX) {
        return;
    }

    if (isFlat && dictator != pointer) {
        // This will produce false positives if the pointer & dictator are not flat when they should be
        pointer = dictator;
    }

    // tolerance eliminates all possibilities of floating point rounding errors
    const double tolerance = ldata->beamWidth * 0.25 * (ldata->up ? -1 : 1);
    const bool isSmall = ldata->isGrace || ldata->mag() < 1.;

    double startY = (isStartDictator ? dictator : pointer) * ldata->spatium / 4 + tolerance;
    double endY = (isStartDictator ? pointer : dictator) * ldata->spatium / 4 + tolerance;

    const ChordRest* firstChordRest = chordRests.front();
    const ChordRest* lastChordRest = chordRests.back();

    size_t curChordRest = 0;
    for (ChordRest* chordRest : chordRests) {
        if (!chordRest->isChord() || chordRest == ldata->elements.back() || chordRest == ldata->elements.front()) {
            curChordRest++;
            continue;
        }

        int sameLineException = 0;

        if (chordRest->isChord()
            && ((curChordRest == 1 && firstChordRest->isChord()) || (curChordRest == chordRests.size() - 2 && lastChordRest->isChord()))) {
            const Chord* innerChord = toChord(chordRest);
            const Chord* outerChord = curChordRest == 1 && firstChordRest->isChord() ? toChord(firstChordRest) : toChord(lastChordRest);
            const int innerLine = ldata->up ? innerChord->upNote()->line() : innerChord->downNote()->line();
            const int outerLine = ldata->up ? outerChord->upNote()->line() : outerChord->downNote()->line();

            if (innerLine == outerLine) {
                sameLineException = 1;
            } else if (chordRests.size() == 3) {
                const ChordRest* otherOuterCR = outerChord == firstChordRest ? lastChordRest : firstChordRest;
                const Chord* otherOuterChord = otherOuterCR->isChord() ? toChord(otherOuterCR) : nullptr;
                if (!otherOuterChord) {
                    continue;
                }

                const int otherOuterLine = ldata->up ? otherOuterChord->upNote()->line() : otherOuterChord->downNote()->line();
                if (innerLine == otherOuterLine) {
                    sameLineException = 1;
                }
            }
        }

        QPointF anchor = chordBeamAnchor(ldata, chordRest, ChordBeamAnchorType::Middle) - item->pagePos();

        const int minLen = minStemLength(chordRest, ldata) - sameLineException;

        // avoid division by zero for zero-length beams (can exist as a pre-layout state used
        // for horizontal spacing computations)
        if (!qFuzzyCompare(endX, startX)) {
            const double proportionAlongX = (anchor.x() - startX) / (endX - startX);
            while (true) {
                const int slope = std::abs(dictator - pointer);
                double reduction = 0.0;
                if (!isFlat) {
                    if (slope <= 3) {
                        reduction = 0.25 * ldata->spatium;
                    } else if (slope <= 6) {
                        reduction = 0.5 * ldata->spatium;
                    } else { // slope > 6
                        reduction = 0.75 * ldata->spatium;
                    }
                }
                // Ensure the beam clears the anchor's height and inner note's stem minimum stem length is enforced
                const double desiredY = proportionAlongX * (endY - startY) + startY;    // start note stem len + extra for slope
                const bool beamClearsAnchor = (ldata->up && desiredY <= anchor.y() + reduction + 1e-8)
                                              || (!ldata->up && desiredY >= anchor.y() - reduction - 1e-8);

                const Note* note = ldata->up ? toChord(chordRest)->downNote() : toChord(chordRest)->upNote();
                const double noteAnchor = (ldata->up ? note->stemUpSE().y() : note->stemDownNW().y()) + note->pagePos().y()
                                          - item->pagePos().y();
                // Resultant length in quarter spaces
                const int desiredLen = std::abs(round((desiredY - noteAnchor) / ldata->spatium * 4)) + 1;

                if (beamClearsAnchor && desiredLen >= round(minLen * chordRest->mag())) {
                    break;
                }

                if (isFlat || (isSmall && dictator == pointer)) {
                    dictator += ldata->up ? -1 : 1;
                    pointer += ldata->up ? -1 : 1;
                } else if (std::abs(dictator - pointer) == 1) {
                    dictator += ldata->up ? -1 : 1;
                } else {
                    pointer += ldata->up ? -1 : 1;
                }

                startY = (isStartDictator ? dictator : pointer) * ldata->spatium / 4 + tolerance;
                endY = (isStartDictator ? pointer : dictator) * ldata->spatium / 4 + tolerance;
            }
        }
        curChordRest++;
    }
}

static void offsetBeamWithAnchorShortening(const LayoutData* ldata, const std::vector<ChordRest*>& chordRests,
                                                       int& dictator, int& pointer, int staffLines, bool isStartDictator,
                                                       int stemLengthDictator, const int targetLine)
{
    const Chord* startChord = nullptr;
    const Chord* endChord = nullptr;
    for (const ChordRest* cr : chordRests) {
        if (cr->isChord()) {
            endChord = toChord(cr);
            if (!startChord) {
                startChord = toChord(cr);
            }
        }
    }
    if (!startChord) {
        // beam full of only rests, don't adjust this
        return;
    }
    // min stem lengths according to how many beams there are (starting with 1)
    static const int minStemLengths[] = { 11, 13, 15, 18, 21, 24, 27, 30 };
    int dictatorBeams = strokeCount(ldata, isStartDictator ? startChord : endChord);
    int pointerBeams = strokeCount(ldata, isStartDictator ? endChord : startChord);
    int maxDictatorReduce = stemLengthDictator - minStemLengths[qBound(0, dictatorBeams - 1, 7)];
    maxDictatorReduce = std::min(std::abs(dictator - targetLine), maxDictatorReduce);

    bool isFlat = dictator == pointer;
    const NotePosition startPos = NotePosition(startChord->line(), startChord->vStaffIdx());
    const NotePosition endPos = NotePosition(endChord->line(), endChord->vStaffIdx());
    bool isAscending = startPos > endPos;
    int towardBeam = ldata->up ? -1 : 1;
    int newDictator = dictator;
    int newPointer = pointer;
    int reduce = 0;
    auto fourBeamException = [](int beams, int yPos) {
        yPos += 400; // because there is some weirdness with modular division around zero, add
                     // a large multiple of 4 so that we can guarantee that yPos%4 will be correct
        return beams >= 4 && (yPos % 4 == 2);
    };
    while (!fourBeamException(dictatorBeams, newDictator)
           && !isValidBeamPosition(ldata->up, newDictator, isStartDictator, isAscending, isFlat, staffLines, true)) {
        if (++reduce > maxDictatorReduce) {
            // we can't shorten this stem at all. bring it back to default and start extending
            newDictator = dictator;
            newPointer = pointer;
            while (!isValidBeamPosition(ldata->up, newDictator, isStartDictator, isAscending, isFlat, staffLines, true)) {
                newDictator += towardBeam;
                newPointer += towardBeam;
            }
            break;
        }
        newDictator += -towardBeam;
        newPointer += -towardBeam;
    }

    // newDictator is guaranteed either valid, or ==dictator
    // first, constrain pointer to valid position
    newPointer = ldata->up ? std::min(newPointer, targetLine) : std::max(newPointer, targetLine);
    // walk it back beamwards until we get a position that satisfies both pointer and dictator
    while (!fourBeamException(dictatorBeams, newDictator) && !fourBeamException(pointerBeams, newPointer)
           && (!isValidBeamPosition(ldata->up, newDictator, isStartDictator, isAscending, isFlat, staffLines, true)
               || !isValidBeamPosition(ldata->up, newPointer, !isStartDictator, isAscending, isFlat, staffLines, true))) {
        if (isFlat) {
            newDictator += towardBeam;
            newPointer += towardBeam;
        } else if (std::abs(newDictator - newPointer) == 1) {
            newDictator += towardBeam;
        } else {
            newPointer += towardBeam;
        }
    }
    dictator = newDictator;
    pointer = newPointer;

    if (isFlat) {
        pointer = dictator;
    }
}

static bool isBeamInsideStaff(int yPos, int staffLines, bool allowFloater)
{
    int aboveStaff = allowFloater ? -2 : -3;
    int belowStaff = (staffLines - 1) * 4 + (allowFloater ? 2 : 3);
    return yPos > aboveStaff && yPos < belowStaff;
}

static void setSmallInnerBeamPos(const LayoutData* ldata, int& dictator, int& pointer, const int staffLines,
                                             const bool isFlat, const bool isSmall, const Score* score)
{
    // We only want this adjustment if the beam is small and flat
    if (!isSmall || !isFlat) {
        return;
    }
    bool noteOutsideStaff = false;
    int beamCount = 0;
    for (const ChordRest* cr : ldata->elements) {
        if (!cr->isChord()) {
            continue;
        }
        const Chord* c = toChord(cr);
        beamCount = std::max(beamCount, strokeCount(ldata, cr));
        noteOutsideStaff |= (c->downLine() < -2 && !ldata->up) || (c->upLine() >= 11 && ldata->up);
    }

    // AND stems have been extended to the second stave line when the notes are far enough outside of the stave
    if (!noteOutsideStaff) {
        return;
    }

    // Allow fractions of quarter spaces in inner beam position, as mag can change
    const double mag = ldata->isGrace ? score->styleD(Sid::graceNoteMag) : ldata->mag();
    const double innerBeam = dictator + (beamCount - 1) * (ldata->up ? ldata->beamSpacing : -ldata->beamSpacing) * mag;
    const int aboveStaff = 1;
    const int belowStaff = (staffLines - 1) * 4 - 1;
    int offset = 0;

    // Make sure the innermost beam is at least 0.5sp within the stave
    if (ldata->up) {
        offset = std::min(offset, int(std::floor(belowStaff - innerBeam)));
    } else {
        offset = std::max(offset, int(std::ceil(aboveStaff - innerBeam)));
    }

    // the beam is flat, so dictator and pointer should match
    dictator += offset;
    pointer = dictator;
}

static int getOuterBeamPosOffset(const LayoutData* ldata, int innerBeam, int beamCount, int staffLines)
{
    int spacing = (ldata->up ? -ldata->beamSpacing : ldata->beamSpacing);
    int offset = (beamCount - 1) * spacing;
    bool isInner = false;
    while (offset != 0 && !isBeamInsideStaff(innerBeam + offset, staffLines, isInner)) {
        offset -= spacing;
        isInner = true;
    }
    return offset;
}

static bool isValidBeamPosition(const bool isUp, int yPos, bool isStart, bool isAscending, bool isFlat, int staffLines,
                                            bool isOuter)
{
    // outside the staff
    bool slantsAway = (isUp && isAscending == isStart) || (!isUp && isAscending != isStart);
    if (!isBeamInsideStaff(yPos, staffLines, isOuter && (slantsAway || isFlat))) {
        return true;
    }

    // removes modulo weirdness with negative numbers (i.e., right above staff)
    yPos += 8;
    // is floater
    if (yPos % 4 == 2) {
        return false;
    }
    if (isFlat) {
        return true;
    }
    // is on line
    if (yPos % 4 == 0) {
        return true;
    }
    // is sitting
    if (yPos % 4 == 3) {
        // return true only if we're starting here and descending, or ascending and ending here
        return isAscending != isStart;
    }
    // is hanging
    // return true only if we're starting here and ascending, or decending and ending here
    return isAscending == isStart;
}

static bool is64thBeamPositionException(const int beamSpacing, int& yPos, int staffLines)
{
    if (beamSpacing == 4) {
        return false;
    }
    return yPos == 2 || yPos == staffLines * 4 - 2 || yPos == staffLines * 4 - 6 || yPos == -2;
}

static int findValidBeamOffset(const LayoutData* ldata, int outer, int beamCount, int staffLines, bool isStart,
                                           bool isAscending, bool isFlat)
{
    bool isBeamValid = false;
    int offset = 0;
    int innerBeam = outer + (beamCount - 1) * (ldata->up ? ldata->beamSpacing : -ldata->beamSpacing);
    while (!isBeamValid) {
        while (!isValidBeamPosition(ldata->up, innerBeam + offset, isStart, isAscending, isFlat, staffLines, beamCount < 2)) {
            offset += ldata->up ? -1 : 1;
        }
        int outerMostBeam = innerBeam + offset + getOuterBeamPosOffset(ldata, innerBeam + offset, beamCount, staffLines);
        if (isValidBeamPosition(ldata->up, outerMostBeam, isStart, isAscending, isFlat, staffLines, true)
            || (beamCount == 4 && is64thBeamPositionException(ldata->beamSpacing, outerMostBeam, staffLines))) {
            isBeamValid = true;
        } else {
            offset += ldata->up ? -1 : 1;
        }
    }
    return offset;
}

static void setValidBeamPositions(const LayoutData* ldata, int& dictator, int& pointer, int beamCountD,
                                              int beamCountP,
                                              int staffLines,
                                              bool isStartDictator,
                                              bool isFlat, bool isAscending)
{
    bool areBeamsValid = false;
    bool has3BeamsInsideStaff = beamCountD >= 3 || beamCountP >= 3;
    while (!areBeamsValid && has3BeamsInsideStaff && ldata->beamSpacing != 4) {
        int dictatorInner = dictator + (beamCountD - 1) * (ldata->up ? ldata->beamSpacing : -ldata->beamSpacing);
        // use dictatorInner for both to simulate flat beams
        int outerDictatorOffset = getOuterBeamPosOffset(ldata, dictatorInner, beamCountD, staffLines);
        if (std::abs(outerDictatorOffset) <= ldata->beamSpacing) {
            has3BeamsInsideStaff = false;
            break;
        }
        int offsetD = findValidBeamOffset(ldata, dictator, beamCountD, staffLines, isStartDictator, false, true);
        int offsetP = findValidBeamOffset(ldata, pointer, beamCountP, staffLines, isStartDictator, false, true);
        int offset = (offsetD == 0 ? offsetP : offsetD);
        if (pointer == dictator) {
            dictator += offset;
        }
        pointer = dictator;
        if (offset == 0) {
            areBeamsValid = true;
        }
    }
    if (isFlat) {
        // flat beams need more checks (non-dictator/pointer notes with floater inner beams)
        areBeamsValid = false;
    }
    while (!areBeamsValid) {
        int dictatorOffset = findValidBeamOffset(ldata, dictator, beamCountD, staffLines, isStartDictator, isAscending, isFlat);
        dictator += dictatorOffset;
        pointer += dictatorOffset;
        if (isFlat) {
            pointer = dictator;
            int currOffset = 0;
            for (ChordRest* cr : ldata->elements) {
                if (!cr->isChord() && (cr != ldata->elements.front() && cr != ldata->elements.back())) {
                    continue;
                }
                // we can use dictator beam position because all of the notes have the same beam position
                int beamCount = strokeCount(ldata, cr);
                currOffset = findValidBeamOffset(ldata, dictator, beamCount, staffLines, isStartDictator, isAscending, isFlat);
                if (currOffset) {
                    break;
                }
            }

            if (currOffset == 0) {
                areBeamsValid = true;
            } else {
                dictator += currOffset;
                pointer += currOffset;
            }
        } else {
            pointer += findValidBeamOffset(ldata, pointer, beamCountP, staffLines, !isStartDictator, isAscending, isFlat);
            if ((ldata->up && pointer <= dictator) || (!ldata->up && pointer >= dictator)) {
                dictator = pointer + (ldata->up ? -1 : 1);
            } else {
                areBeamsValid = true;
            }
        }
    }
}

static void addMiddleLineSlant(const LayoutData* ldata, int& dictator, int& pointer, int beamCount, int targetLine,
                                           int interval, int desiredSlant)
{
    bool isSmall = ldata->mag() < 1. || ldata->isGrace;
    if (interval == 0 || (!isSmall && beamCount > 2 && ldata->beamSpacing != 4) || ldata->beam->noSlope()) {
        return;
    }
    bool isOnTargetLine = pointer == targetLine && (std::abs(pointer - dictator) < 2);
    if (isOnTargetLine) {
        if (std::abs(desiredSlant) == 1 || interval == 1 || (beamCount == 2 && ldata->beamSpacing != 4 && !isSmall)) {
            dictator = targetLine + (ldata->up ? -1 : 1);
        } else {
            dictator = targetLine + (ldata->up ? -2 : 2);
        }
    }
}

static void add8thSpaceSlant(LayoutData* ldata, QPointF& dictatorAnchor, int dictator, int pointer, int beamCount,
                                         int interval, int targetLine, bool isFlat)
{
    if (beamCount != 3 || ldata->beam->noSlope() || ldata->beamSpacing != 3) {
        return;
    }
    if ((isFlat && dictator != targetLine) || (dictator != pointer) || interval == 0) {
        return;
    }
    if ((ldata->up && (dictator + 4) % 4 == 3) || (!ldata->up && (dictator + 4) % 4 == 1)) {
        return;
    }
    dictatorAnchor.setY(dictatorAnchor.y() + (ldata->up ? -0.125 * ldata->spatium : 0.125 * ldata->spatium));
    ldata->beamDist += 0.0625 * ldata->spatium;
}

static SlopeConstraint getSlopeConstraint(const LayoutData* ldata, const NotePosition& startPos,
                                                      const NotePosition& endPos)
{
    if (ldata->notePositions.empty()) {
        return SlopeConstraint::NO_CONSTRAINT;
    }

    // 0 to constrain to flat, 1 to constrain to 0.25, <0 for no constraint
    if (startPos == endPos) {
        return SlopeConstraint::FLAT;
    }

    // if a note is more extreme than the endpoints, slope is 0
    // p.s. _notes is a sorted vector
    const std::vector<ChordRest*>& elements = ldata->elements;
    const std::vector<NotePosition>& notePositions = ldata->notePositions;

    if (elements.size() > 2) {
        if (ldata->up) {
            NotePosition higherEnd = std::min(startPos, endPos);
            if (higherEnd > notePositions[0]) {
                return SlopeConstraint::FLAT; // a note is higher in the staff than the highest end
            }
            if (higherEnd == notePositions[0] && higherEnd >= notePositions[1]) {
                if (higherEnd > notePositions[1]) {
                    return SlopeConstraint::FLAT; // a note is higher in the staff than the highest end
                }
                size_t chordCount = elements.size();
                if (chordCount >= 3 && notePositions.size() >= 3) {
                    bool middleNoteHigherThanHigherEnd = higherEnd >= notePositions[2];
                    if (middleNoteHigherThanHigherEnd) {
                        return SlopeConstraint::FLAT; // two notes are the same as the highest end (notes [0] [1] and [2] higher than or same as higherEnd)
                    }
                    Chord* secondNote = elements[1]->isChord() ? toChord(elements[1]) : nullptr;
                    Chord* secondToLastNote
                        = elements[chordCount - 2]->isChord() ? toChord(elements[chordCount - 2]) : nullptr;
                    bool secondNoteSameHeightAsHigherEnd = startPos < endPos && secondNote && NotePosition(
                        secondNote->upLine(), secondNote->vStaffIdx()) == higherEnd;
                    bool secondToLastNoteSameHeightAsHigherEnd = endPos.line < startPos.line && secondToLastNote && NotePosition(
                        secondToLastNote->upLine(), secondToLastNote->vStaffIdx()) == higherEnd;
                    if (!(secondNoteSameHeightAsHigherEnd || secondToLastNoteSameHeightAsHigherEnd)) {
                        return SlopeConstraint::FLAT; // only one note same as higher end, but it is not a neighbor
                    } else {
                        // there is a single note next to the highest one with equivalent height
                        // and they are neighbors. this is our exception, so
                        // the slope may be a max of 0.25.
                        return SlopeConstraint::SMALL_SLOPE;
                    }
                } else {
                    return SlopeConstraint::FLAT; // only two notes in entire beam, in this case startNote == endNote
                }
            }
        } else {
            NotePosition lowerEnd = std::max(startPos, endPos);
            if (lowerEnd < notePositions[notePositions.size() - 1]) {
                return SlopeConstraint::FLAT;
            }
            if (lowerEnd == notePositions[notePositions.size() - 1]
                && lowerEnd <= notePositions[notePositions.size() - 2]) {
                if (lowerEnd < notePositions[notePositions.size() - 2]) {
                    return SlopeConstraint::FLAT;
                }
                size_t chordCount = elements.size();
                if (chordCount >= 3 && notePositions.size() >= 3) {
                    bool middleNoteLowerThanLowerEnd = lowerEnd <= notePositions[notePositions.size() - 3];
                    if (middleNoteLowerThanLowerEnd) {
                        return SlopeConstraint::FLAT;
                    }
                    const Chord* secondNote = elements[1]->isChord() ? toChord(elements[1]) : nullptr;
                    const Chord* secondToLastNote
                        = elements[chordCount - 2]->isChord() ? toChord(elements[chordCount - 2]) : nullptr;
                    const NotePosition secondNotePos = secondNote ? NotePosition(
                        secondNote->downLine(), secondNote->vStaffIdx()) : NotePosition(0, -1);
                    const NotePosition secondToLastNotePos = secondToLastNote ? NotePosition(
                        secondToLastNote->downLine(), secondToLastNote->vStaffIdx()) : NotePosition(0, -1);

                    bool secondNoteSameHeightAsLowerEnd = startPos.line > endPos.line && secondNotePos == lowerEnd;
                    bool secondToLastNoteSameHeightAsLowerEnd = endPos.line > startPos.line && secondToLastNotePos == lowerEnd;
                    if (!(secondNoteSameHeightAsLowerEnd || secondToLastNoteSameHeightAsLowerEnd)) {
                        return SlopeConstraint::FLAT;
                    } else {
                        return SlopeConstraint::SMALL_SLOPE;
                    }
                } else {
                    return SlopeConstraint::FLAT;
                }
            }
        }
    }
    return SlopeConstraint::NO_CONSTRAINT;
}

static int getMaxSlope(const LayoutData* ldata)
{
    // for 2-indexed interval i (seconds, thirds, etc.)
    // maxSlopes[i] = max slope of beam for notes with interval i

    // calculate max slope based on distance between first and last chords
    double endX = chordBeamAnchorX(ldata, ldata->elements[ldata->elements.size() - 1], ChordBeamAnchorType::Start);
    double startX = chordBeamAnchorX(ldata, ldata->elements[0], ChordBeamAnchorType::End);
    double beamWidth = endX - startX;
    beamWidth /= ldata->spatium;
    int maxSlope = _maxSlopes[7];
    if (beamWidth < 3.0) {
        maxSlope = _maxSlopes[1];
    } else if (beamWidth < 5.0) {
        maxSlope = _maxSlopes[2];
    } else if (beamWidth < 7.5) {
        maxSlope = _maxSlopes[3];
    } else if (beamWidth < 10.0) {
        maxSlope = _maxSlopes[4];
    } else if (beamWidth < 15.0) {
        maxSlope = _maxSlopes[5];
    } else if (beamWidth < 20.0) {
        maxSlope = _maxSlopes[6];
    } else {
        maxSlope = _maxSlopes[7];
    }

    return maxSlope;
}

// Beam-only counterpart of Studio 4.7.5 StemLayout::calcDefaultStemLength.
// Unbeamed flags and tremolos retain their existing 3.x handling.
static int minStaffOverlap(bool up, int staffLines, int beams, int spacing, bool fullSize)
{
    int overlap = 8;
    if (fullSize) {
        if (beams == 3)
            overlap = 12;
        else if (beams >= 4)
            overlap = (beams - 4) * spacing + (spacing == 4 ? 16 : 14);
    }
    overlap = std::min(overlap, (staffLines - (fullSize ? 1 : 4)) * 4);
    return up ? (staffLines - 1) * 4 - overlap : overlap;
}

static double defaultStemLength4(const Chord* chord)
{
    const Score* score = chord->score();
    const qreal sp = chord->spatium();
    const qreal lineDistance = chord->staff()->lineDistance(chord->tick());
    const int qPerLine = int(std::floor(lineDistance * 2));
    const int beams = qBound(1, chord->beams(), 8);
    const int lines = chord->staff()->lines(chord->tick());
    // 3.x expresses beam distance relative to beam thickness. Use the
    // closest Studio grid while retaining the actual configured distance.
    const int spacing = score->styleP(Sid::beamWidth) * (1.0 + score->styleD(Sid::beamDistance)) / sp >= 0.875 ? 4 : 3;
    qreal intrinsicMag = chord->isSmall() ? score->styleD(Sid::smallNoteMag) : 1.0;
    if (chord->isGrace())
        intrinsicMag *= score->styleD(Sid::graceNoteMag);
    const bool up = chord->up();
    const bool fullSize = !chord->isSmall() && !chord->isGrace();
    int normal = 14; // Studio stemLength default: 3.5sp
    normal += beams == 3 ? 2 : (beams >= 4 ? (beams - 3) * spacing : 0);
    const Note* near = up ? chord->upNote() : chord->downNote();
    const qreal extraHeight = (up ? near->stemUpSE().y() : near->stemDownNW().y()) / (intrinsicMag * sp);
    qreal shortestSetting = score->styleS(Sid::shortestStem).val();
    // Studio's default is 2.5sp; Evolution's legacy default is 2.25sp.
    // Honour non-default user settings without changing the stored style.
    if (qFuzzyCompare(shortestSetting, 2.25))
        shortestSetting = 2.5;
    const int shortest = spacing == 4 ? 12 : int((shortestSetting + std::abs(extraHeight)) * 4);
    const int target = minStaffOverlap(up, lines, beams, spacing, fullSize);
    const int noteLine = up ? chord->upLine() : chord->downLine();
    const int end = noteLine * qPerLine + (up ? -normal : normal);
    const double endMag = noteLine * qPerLine + (up ? -normal : normal) * intrinsicMag;
    // Studio 4.7.5 shortStemStartLocation defaults to one staff position.
    const int shortStemStart = qPerLine + 1;
    const int shortStart = up ? -shortStemStart : (lines - 1) * 2 * qPerLine + shortStemStart;
    const bool outside = up ? endMag <= shortStart : endMag >= shortStart;
    int ideal = normal;
    if (outside) {
        static const int reductions[4][5] = {
            {1, 2, 3, 4, 4}, {0, 1, 2, 3, 3}, {0, 1, 1, 1, 1}, {0, 0, 0, 1, 1}
        };
        const int extension = up ? std::abs(int(std::floor(endMag)) + shortStemStart)
                                 : std::abs(int(std::ceil(endMag)) - shortStart);
        const int halfSpaces = qBound(0, int(std::floor(extension / 2.0)), 4);
        int reduce = beams >= 4 ? 0 : reductions[beams][halfSpaces];
        if (!score->styleB(Sid::shortenStem))
            reduce = 0;
        if (score->styleB(Sid::shortenStem) && intrinsicMag >= 1.0) {
            if (beams == 1 && halfSpaces < 2)
                reduce = 2;
            else if (beams == 3 && halfSpaces == 3)
                reduce = 0;
        }
        ideal = std::max(ideal - reduce, shortest);
    }
    else if (!outside && !(up ? end > target : end < target)) {
        if (beams <= 2 && std::fmod(end + 4, 4) == 2)
            --ideal;
        ideal = std::max(ideal, shortest);
    }
    static const int innerLengths[] = {10, 9, 8, 7};
    int minLength = innerLengths[std::min(beams, 3)] + beams * spacing - 1;
    const bool noteInStaff = up ? noteLine > 0 : noteLine < (lines - 1) * 2;
    if (beams >= 4 && noteInStaff)
        ++minLength;
    int stemLength = std::max(ideal, minLength);
    if (beams == 4) {
        const int difference = up ? std::max(0, chord->upLine() - (lines - 1) * 2)
                                  : std::max(0, -chord->downLine());
        if (difference == 2)
            stemLength = std::max(stemLength, 21);
        else if (difference == 3 || difference == 4)
            stemLength = std::max(stemLength, 23);
    }
    const qreal chordHeight = (chord->downLine() - chord->upLine()) * qPerLine / 4.0 * sp;
    qreal length = chordHeight + stemLength / 4.0 * sp * intrinsicMag;
    const Note* start = up ? chord->downNote() : chord->upNote();
    if (!start->fixed()) {
        const qreal stemEnd = start->pos().y() + (up ? -length : length);
        const qreal bottom = lineDistance * sp * (lines - 1);
        qreal targetY;
        if (lineDistance >= 1.0) {
            targetY = bottom < 2 * sp ? (up ? 0 : bottom)
                : qFuzzyCompare(lineDistance, 1.0) ? target / 4.0 * lineDistance * sp
                : (up ? bottom - 2 * sp : 2 * sp);
        }
        else {
            targetY = lines < 3 ? (up ? 0 : bottom)
                : (up ? bottom - 2 * lineDistance * sp : 2 * lineDistance * sp);
        }
        length += std::max(qreal(0), up ? stemEnd - targetY : targetY - stemEnd);
    }
    return length;
}

static double chordBeamAnchorX(const LayoutData* ldata, const ChordRest* cr, ChordBeamAnchorType type)
{
    qreal x = cr->stemPosX() + cr->pagePos().x() - ldata->beam->pagePos().x();
    if (!cr->isChord() || !toChord(cr)->stem())
        return ldata->up ? x : x - cr->stemPosX();
    const qreal width = toChord(cr)->stem()->lineWidthMag();
    if (type == ChordBeamAnchorType::Start && cr->up())
        x -= width;
    else if (type == ChordBeamAnchorType::End && !cr->up())
        x += width;
    else if (type == ChordBeamAnchorType::Middle)
        x += cr->up() ? -width / 2 : width / 2;
    return x;
}

static QPointF chordBeamAnchor(const LayoutData* ldata, const ChordRest* cr, ChordBeamAnchorType type)
{
    qreal y = cr->pagePos().y();
    if (cr->isChord()) {
        const Chord* chord = toChord(cr);
        const Note* note = cr->up() ? chord->downNote() : chord->upNote();
        const int direction = cr->up() ? -1 : 1;
        y = note->pagePos().y() - note->offset().y()
            + defaultStemLength4(chord) * direction - ldata->beamWidth / 2 * direction;
    }
    return QPointF(chordBeamAnchorX(ldata, cr, type), y);
}

static bool calculateAnchorsCross(const Beam* /*item*/, LayoutData* ldata,
                                              const Score* /*score*/)
{
    if (ldata->crossStaffBeamPos != CrossStaffBeamPosition::BETWEEN) {
        // The beam is either not cross or above/below and must be laid out like a non-cross beam
        return false;
    }

    double spatium = ldata->spatium;
    ChordRest* startCr = ldata->elements.front();
    ChordRest* endCr = ldata->elements.back();

    const double quarterSpace = spatium / 4;
    // imagine a line of beamed notes all in a row on the same staff. the first and last of those
    // are the 'outside' notes, and the slant of the beam is going to be affected by the 'middle' notes
    // between them.
    // we have to keep track of this for both staves.
    Chord* topFirst = nullptr;
    Chord* topLast = nullptr;
    Chord* bottomFirst = nullptr;
    Chord* bottomLast = nullptr;
    int maxMiddleTopLine = std::numeric_limits<int>::min(); // lowest note in the top staff
    int minMiddleBottomLine = std::numeric_limits<int>::max(); // highest note in the bottom staff
    int prevTopLine = maxMiddleTopLine; // previous note's line position (top)
    int prevBottomLine = minMiddleBottomLine; // previous note's line position (bottom)
    // if the immediate neighbor of one of the two 'outside' notes on either the top or bottom
    // are the same as that outside note, we need to record it so that we can add a 1/4 space slant.
    bool secondTopIsSame = false;
    bool secondBottomIsSame = false;
    bool penultimateTopIsSame = false;
    bool penultimateBottomIsSame = false;
    double maxY = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::lowest();
    // Get switches between above & below the beam
    // The direction is neutral if there is more than one switch
    bool prevBelowBeam = ldata->elements.front()->up();
    bool hasBeamSideSwitch = false;
    int beamSideSwitchDirection = 0;

    // Get switches between staves on the same side of a beam
    // The direction is neutral is there is more than one switch
    int prevStaff = ldata->elements.front()->vStaffIdx();
    bool hasStaffSwitch = false;
    int staffSwitchDirection = 0;

    // Find the notes on the top and bottom of staves
    //
    bool checkNextTop = false;
    bool checkNextBottom = false;
    for (ChordRest* cr : ldata->elements) {
        if (!cr->isChord()) {
            continue;
        }
        Chord* c = toChord(cr);

        if (c->up() == prevBelowBeam) {
            c->vStaffIdx();
            if (prevStaff != c->vStaffIdx()) {
                int diff = static_cast<int>(prevStaff - c->vStaffIdx());
                staffSwitchDirection = hasStaffSwitch ? 0 : (diff < 0 ? -1 : 1);
                hasStaffSwitch = true;
            }
        }
        prevStaff = c->vStaffIdx();

        if (c->up()) {
            // this chord is on the bottom staff
            if (penultimateBottomIsSame) {
                // the chord we took as the penultimate bottom note wasn't.
                // so treat it properly as a middle note
                minMiddleBottomLine = std::min(minMiddleBottomLine, prevBottomLine);
                penultimateBottomIsSame = false;
            }
            checkNextTop = false; // we are no longer looking for the second note in the top
                                  // staff being the same as the first--this note is on the bottom.
            if (!bottomFirst) {
                bottomFirst = c;
                checkNextBottom = true; // this was the first bottom note, so check for second next time
            } else {
                penultimateBottomIsSame = prevBottomLine == c->line();
                if (!penultimateBottomIsSame) {
                    minMiddleBottomLine = std::min(minMiddleBottomLine, prevBottomLine);
                }
                if (checkNextBottom) {
                    // this is the second bottom note, so we should see if this one is same line as first
                    secondBottomIsSame = c->line() == bottomFirst->line();
                    checkNextBottom = false;
                } else {
                    prevBottomLine = c->line();
                }
                bottomLast = c;
            }
            if (!prevBelowBeam) {
                beamSideSwitchDirection = hasBeamSideSwitch ? 0 : -1;
                hasBeamSideSwitch = true;
            }
            prevBelowBeam = true;
            maxY = std::min(maxY, chordBeamAnchor(ldata, c, ChordBeamAnchorType::Middle).y());
        } else {
            // this chord is on the top staff
            if (penultimateTopIsSame) {
                // the chord we took as the penultimate top note wasn't.
                // so treat it properly as a middle note
                maxMiddleTopLine = std::max(maxMiddleTopLine, prevTopLine);
                penultimateTopIsSame = false;
            }
            checkNextBottom = false; // no longer looking for a bottom second note since this is on top
            if (!topFirst) {
                topFirst = c;
                checkNextTop = true;
            } else {
                penultimateTopIsSame = prevTopLine == c->line();
                if (!penultimateTopIsSame) {
                    maxMiddleTopLine = std::max(maxMiddleTopLine, prevTopLine);
                }
                if (checkNextTop) {
                    secondTopIsSame = c->line() == topFirst->line();
                    checkNextTop = false;
                } else {
                    prevTopLine = c->line();
                }
                topLast = c;
            }
            if (prevBelowBeam) {
                beamSideSwitchDirection = hasBeamSideSwitch ? 0 : 1;
                hasBeamSideSwitch = true;
            }
            prevBelowBeam = false;
            minY = std::max(minY, chordBeamAnchor(ldata, c, ChordBeamAnchorType::Middle).y());
        }
    }

    double yMidPoint = (maxY + minY) / 2;
    // The 3.x adapter applies segment-stack centering after segment creation.

    ldata->startAnchor.setY(yMidPoint);
    ldata->endAnchor.setY(yMidPoint);
    ldata->startAnchor.setX(chordBeamAnchorX(ldata, startCr, ChordBeamAnchorType::Start));
    ldata->endAnchor.setX(chordBeamAnchorX(ldata, endCr, ChordBeamAnchorType::End));


    // Calculates & sets slope
    if (!ldata->beam->noSlope()) {
        int topFirstLine = topFirst ? topFirst->downNote()->line() : 0;
        int topLastLine = topLast ? topLast->downNote()->line() : 0;
        int bottomFirstLine = bottomFirst ? bottomFirst->upNote()->line() : 0;
        int bottomLastLine = bottomLast ? bottomLast->upNote()->line() : 0;
        bool constrainTopToQuarter = false;
        bool constrainBottomToQuarter = false;
        if ((topFirstLine > topLastLine && secondTopIsSame)
            || (topFirstLine < topLastLine && penultimateTopIsSame)) {
            constrainTopToQuarter = true;
        }
        if ((bottomFirstLine < bottomLastLine && secondBottomIsSame)
            || (bottomFirstLine > bottomLastLine && penultimateBottomIsSame)) {
            constrainBottomToQuarter = true;
        }
        if (!topLast && !bottomLast && topFirst && bottomFirst) {
            // if there are only two notes, one on each staff, special case
            // take max slope into account
            double yFirst, yLast;
            // we can't rely on comparing topFirst and bottomFirst ->tick() because beamed
            // graces have the same tick
            if (ldata->elements[0] == topFirst) {
                yFirst = topFirst->stemPos().y();
                yLast = bottomFirst->stemPos().y();
            } else {
                yFirst = bottomFirst->stemPos().y();
                yLast = topFirst->stemPos().y();
            }
            int desiredSlant = round((yFirst - yLast) / spatium);
            int slant = std::min(std::abs(desiredSlant), getMaxSlope(ldata));
            slant *= (desiredSlant < 0) ? -quarterSpace : quarterSpace;
            ldata->startAnchor.ry() += (slant / 2);
            ldata->endAnchor.ry() -= (slant / 2);
        } else {
            // Get the direction of groups of notes on top and bottom staves and the direction of switches between sides of the beam and switches between staves
            // Directions are up, down or neutral
            bool forceHoriz = false;
            bool singleNoteTop = false;
            bool singleNoteBottom = false;

            int topSlant = topFirstLine - topLastLine;
            if (constrainTopToQuarter && topSlant != 0) {
                topSlant = topFirstLine < topLastLine ? -1 : 1;
            }
            if (!topLast) {
                // If there's only one note, set direction to neutral
                topSlant = 0;
                singleNoteTop = true;
            }
            int bottomSlant = bottomFirstLine - bottomLastLine;
            if (constrainBottomToQuarter && bottomSlant != 0) {
                bottomSlant = bottomFirstLine < bottomLastLine ? -1 : 1;
            }
            if (!bottomLast) {
                // If there's only one note, set direction to neutral
                bottomSlant = 0;
                singleNoteBottom = true;
            }
            if ((maxMiddleTopLine >= std::max(topFirstLine, topLastLine)
                 || (minMiddleBottomLine <= std::min(bottomFirstLine, bottomLastLine)))) {
                forceHoriz = true;
            }

            const int topSlantDir = topSlant <= 0 ? (topSlant == 0 ? 0 : -1) : 1;
            const int bottomSlantDir = bottomSlant <= 0 ? (bottomSlant == 0 ? 0 : -1) : 1;
            // Check all directions which aren't neutral (0) match
            // If there is any contradiction between top staff direction, bottom direction, beam side switch direction and staff switch direction, force horizontal
            // If all are neutral, force horizontal
            int overallDirection = 0;
            for (int direction : { topSlantDir, bottomSlantDir, beamSideSwitchDirection, staffSwitchDirection }) {
                if (direction != 0) {
                    overallDirection = direction;
                    break;
                }
            }

            const bool overallSlantFlat = overallDirection == 0;
            const bool topSlantOverallDiff = (topSlantDir != 0 && topSlantDir != overallDirection);
            const bool bottomSlantOverallDiff = (bottomSlantDir != 0 && bottomSlantDir != overallDirection);
            const bool beamSideSwitchOverallDiff = (beamSideSwitchDirection != 0 && beamSideSwitchDirection != overallDirection);
            const bool staffSwitchOverallDiff = (staffSwitchDirection != 0 && staffSwitchDirection != overallDirection);
            const bool flattenNeutralBeamSideSwitch = beamSideSwitchDirection == 0 && (topSlantDir != bottomSlantDir)
                                                      && !(singleNoteTop || singleNoteBottom);

            forceHoriz = forceHoriz || overallSlantFlat || flattenNeutralBeamSideSwitch
                         || (topSlantOverallDiff || bottomSlantOverallDiff || beamSideSwitchOverallDiff || staffSwitchOverallDiff);

            if (!forceHoriz) {
                int slant = 0;
                if (topSlant == 0) {
                    slant = bottomSlant;
                } else if (bottomSlant == 0) {
                    slant = topSlant;
                } else {
                    slant = (std::abs(topSlant) < std::abs(bottomSlant)) ? topSlant : bottomSlant;
                }

                const int absSlant = std::min(std::abs(slant), getMaxSlope(ldata));
                const double slope = absSlant * ((slant < 0) ? -quarterSpace : quarterSpace);
                ldata->startAnchor.ry() += (slope / 2);
                ldata->endAnchor.ry() -= (slope / 2);
            }
        }
    }
    return true;
}

static int computeDesiredSlant(const LayoutData* ldata, const NotePosition& start,
                              const NotePosition& end, int target, int dictator, int pointer,
                              const std::vector<Chord*>& closest)
{
    if (ldata->beam->noSlope() || start == end)
        return 0;
    int dExtension = target - dictator;
    int pExtension = target - pointer;
    dExtension = ldata->up ? std::min(dExtension, 0) : std::max(dExtension, 0);
    pExtension = ldata->up ? std::min(pExtension, 0) : std::max(pExtension, 0);
    if (dictator + dExtension == target && pointer + pExtension == target)
        return 0;
    const auto constraint = getSlopeConstraint(ldata, start, end);
    if (constraint == SlopeConstraint::FLAT)
        return 0;
    if (constraint == SlopeConstraint::SMALL_SLOPE)
        return dictator > pointer ? -1 : 1;
    if (ldata->crossStaffBeamPos == CrossStaffBeamPosition::ABOVE
        || ldata->crossStaffBeamPos == CrossStaffBeamPosition::BELOW) {
        const int beamDir = start > end ? 1 : -1;
        const NotePosition nearStart(closest.empty() ? 0 : closest.front()->line(),
                                     closest.empty() ? 0 : closest.front()->vStaffIdx());
        const NotePosition nearEnd(closest.empty() ? 0 : closest.back()->line(),
                                   closest.empty() ? 0 : closest.back()->vStaffIdx());
        const int nearDir = nearStart == nearEnd ? 0 : (nearStart > nearEnd ? 1 : -1);
        if (beamDir != nearDir && nearDir != 0)
            return 0;
        if (nearDir == 0 && start.staff != end.staff)
            return ldata->up ? 1 : -1;
    }
    const int interval = std::min(std::abs(end.line - start.line), 7);
    return std::min(getMaxSlope(ldata), _maxSlopes[interval]) * (ldata->up ? 1 : -1);
}

} // namespace

bool layoutBeam4(Beam* beam, const std::vector<ChordRest*>& elements,
                 QPointF& start, QPointF& end, qreal& beamDistance)
{
    if (elements.empty() || beam->staff()->isTabStaff(beam->tick()))
        return false;
    Chord* first = nullptr;
    Chord* last = nullptr;
    LayoutData data;
    data.beam = beam;
    data.up = beam->up();
    data.isGrace = false;
    data.spatium = beam->spatium();
    data.beamWidth = beam->score()->styleS(Sid::beamWidth).val() * data.spatium * beam->mag();
    data.beamDist = beamDistance;
    data.beamSpacing = beamDistance / data.spatium / beam->mag() >= 0.875 ? 4 : 3;
    data.magnitude = beam->mag();
    data.elements = elements;
    for (ChordRest* cr : elements) {
        if (!cr->isChord())
            continue;
        Chord* chord = toChord(cr);
        if (chord->tremolo())
            return false; // tremolo anchor adaptation is a separate stage
        if (!first)
            first = chord;
        last = chord;
        for (const Note* note : chord->notes())
            data.notePositions.emplace_back(note->line(), chord->vStaffIdx());
    }
    if (!first || !last)
        return false;
    data.isGrace = first->isGrace();
    if (beam->cross()) {
        bool above = false, below = false;
        for (const ChordRest* cr : elements) {
            if (!cr->isChord())
                continue;
            below |= cr->up();
            above |= !cr->up();
        }
        data.crossStaffBeamPos = above && below ? CrossStaffBeamPosition::BETWEEN
            : below ? CrossStaffBeamPosition::ABOVE : CrossStaffBeamPosition::BELOW;
    }
    std::sort(data.notePositions.begin(), data.notePositions.end());
    if (calculateAnchorsCross(beam, &data, beam->score())) {
        start = data.startAnchor;
        end = data.endAnchor;
        return true;
    }
    int closestStaff = first->vStaffIdx();
    if (beam->cross()) {
        for (const ChordRest* cr : elements) {
            if (cr->isChord())
                closestStaff = data.up ? std::min(closestStaff, cr->vStaffIdx()) : std::max(closestStaff, cr->vStaffIdx());
        }
    }
    std::vector<Chord*> closest;
    for (ChordRest* cr : elements) {
        if (cr->isChord() && cr->vStaffIdx() == closestStaff)
            closest.push_back(toChord(cr));
    }
    const LayoutData* ldata = &data;
    const qreal quarter = data.spatium / 4;
    start = chordBeamAnchor(ldata, first, ChordBeamAnchorType::Start);
    end = chordBeamAnchor(ldata, last, ChordBeamAnchorType::End);
    const NotePosition firstPos(first->line(), first->vStaffIdx());
    const NotePosition lastPos(last->line(), last->vStaffIdx());
    const bool isStartDictator = data.up ? firstPos < lastPos : firstPos > lastPos;
    const QPointF origin = beam->pagePos();
    int dictator = std::lround(((isStartDictator ? start : end).y() - origin.y()) / quarter);
    int pointer = std::lround(((isStartDictator ? end : start).y() - origin.y()) / quarter);
    const int lines = beam->score()->staff(closestStaff)->lines(first->tick());
    const int firstTarget = minStaffOverlap(data.up, lines, first->beams(), data.beamSpacing,
                                           qFuzzyCompare(data.mag(), 1.0) && !data.isGrace);
    const int lastTarget = minStaffOverlap(data.up, lines, last->beams(), data.beamSpacing, !data.isGrace);
    int staffOffset = 0;
    if (beam->cross() && first->measure()->system()) {
        const System* system = first->measure()->system();
        staffOffset = std::lround(std::abs(system->staffYpage(closestStaff) - system->staffYpage(beam->staffIdx()))
                                 / data.spatium * 4 * (data.up ? -1 : 1));
    }
    const int target = staffOffset + (data.up ? std::min(firstTarget, lastTarget) + 1 : std::max(firstTarget, lastTarget) - 1);
    const int slant = computeDesiredSlant(ldata, firstPos, lastPos, target, dictator, pointer, closest);
    const bool flat = slant == 0;
    const auto special = flat ? getSlopeConstraint(ldata, firstPos, lastPos) : SlopeConstraint::NO_CONSTRAINT;
    if (flat) {
        dictator = data.up ? std::min(pointer, dictator) : std::max(pointer, dictator);
        pointer = dictator;
    }
    else if ((dictator > pointer) != (isStartDictator ? firstPos > lastPos : lastPos > firstPos))
        dictator = pointer - slant;
    else
        pointer = dictator + slant;
    const bool small = data.mag() < 1.0 || data.isGrace;
    if (end.x() > start.x()) {
        setSmallInnerBeamPos(ldata, dictator, pointer, lines, flat, small, beam->score());
        if (!small) {
            const Chord* d = isStartDictator ? first : last;
            const int stemLength = std::lround(defaultStemLength4(d) / data.spatium * 4);
            offsetBeamWithAnchorShortening(ldata, elements, dictator, pointer, lines, isStartDictator, stemLength, target);
        }
        offsetBeamToRemoveCollisions(beam, ldata, elements, dictator, pointer, start.x(), end.x(), flat, isStartDictator);
    }
    const int beamsD = (isStartDictator ? first : last)->beams();
    const int beamsP = (isStartDictator ? last : first)->beams();
    if (!data.isGrace)
        setValidBeamPositions(ldata, dictator, pointer, beamsD, beamsP, lines, isStartDictator, flat, firstPos > lastPos);
    const int interval = std::abs(firstPos.line - lastPos.line);
    if (special != SlopeConstraint::FLAT)
        addMiddleLineSlant(ldata, dictator, pointer, std::max(beamsD, beamsP), target, interval,
                          special == SlopeConstraint::SMALL_SLOPE ? 1 : slant);
    start.setY(quarter * (isStartDictator ? dictator : pointer) + origin.y());
    end.setY(quarter * (isStartDictator ? pointer : dictator) + origin.y());
    add8thSpaceSlant(&data, isStartDictator ? start : end, dictator, pointer,
                    std::max(beamsD, beamsP), interval, target, flat);
    start.setX(chordBeamAnchorX(ldata, elements.front(), ChordBeamAnchorType::Start));
    end.setX(chordBeamAnchorX(ldata, elements.back(), ChordBeamAnchorType::End));
    beamDistance = data.beamDist;
    return true;
}
} // namespace Ms

namespace Ms {
namespace {
// Studio SystemLayout::minVertSpaceForCrossStaffBeams uses the most
// restrictive note on each side, independently of the beam's slope.
bool crossStaffLimits4(Beam* beam, const std::vector<ChordRest*>& elements,
                      int upperStaff, int lowerStaff,
                      const Chord*& upper, const Chord*& lower,
                      qreal& upperLimit, qreal& lowerLimit)
{
    upper = lower = nullptr;
    upperLimit = std::numeric_limits<qreal>::lowest();
    lowerLimit = std::numeric_limits<qreal>::max();
    static const int lengths[] = {11, 13, 15, 18, 21, 24, 27, 30};
    if (!beam->cross() || beam->userModified() || beam->staff()->isTabStaff(beam->tick()))
        return false;
    for (const ChordRest* cr : elements) {
        if (!cr->isChord())
            continue;
        const Chord* chord = toChord(cr);
        if (chord->tremolo())
            return false;
        const qreal minLength = lengths[qBound(0, chord->beams() - 1, 7)] * chord->spatium() / 4;
        if (cr->up() && cr->vStaffIdx() == lowerStaff) {
            const qreal limit = chord->upNote()->y() - minLength;
            if (limit < lowerLimit) {
                lowerLimit = limit;
                lower = chord;
            }
        }
        else if (!cr->up() && cr->vStaffIdx() == upperStaff) {
            const qreal limit = chord->downNote()->y() + minLength;
            if (limit > upperLimit) {
                upperLimit = limit;
                upper = chord;
            }
        }
    }
    return upper && lower;
}
}

qreal beam4MinStaffDistance(Beam* beam, int upperStaff, int lowerStaff)
{
    std::vector<ChordRest*> elements;
    for (ChordRest* cr : beam->elements())
        elements.push_back(cr);
    const Chord* upper;
    const Chord* lower;
    qreal upperLimit, lowerLimit;
    if (!crossStaffLimits4(beam, elements, upperStaff, lowerStaff, upper, lower, upperLimit, lowerLimit))
        return 0.0;
    qreal distance = upperLimit - lowerLimit;
    bool upperAbove, lowerAbove;
    const QLineF* upperSegment = beam->topLevelSegment4(upper, &upperAbove);
    const QLineF* lowerSegment = beam->topLevelSegment4(lower, &lowerAbove);
    if (upperSegment && lowerSegment && upperAbove == lowerAbove) {
        const int beams = std::min(upper->beams(), lower->beams());
        const qreal height = beam->score()->styleP(Sid::beamWidth) + beam->beamDist() * (beams - 1);
        distance -= height;
    }
    return distance;
}

qreal beam4CenterOffset(Beam* beam, const std::vector<ChordRest*>& elements)
{
    if (elements.empty())
        return 0.0;
    int upperStaff = std::numeric_limits<int>::max();
    int lowerStaff = -1;
    for (const ChordRest* cr : elements) {
        if (!cr->isChord())
            continue;
        upperStaff = std::min(upperStaff, cr->vStaffIdx());
        lowerStaff = std::max(lowerStaff, cr->vStaffIdx());
    }
    const Chord* upper;
    const Chord* lower;
    qreal upperLimit, lowerLimit;
    if (!crossStaffLimits4(beam, elements, upperStaff, lowerStaff, upper, lower, upperLimit, lowerLimit))
        return 0.0;
    const QLineF* upperSegment = beam->topLevelSegment4(upper);
    const QLineF* lowerSegment = beam->topLevelSegment4(lower);
    if (!upperSegment || !lowerSegment)
        return 0.0;
    // Level-zero beam spans the endpoints of the whole group; its centre
    // follows the current linear beam equation regardless of inner levels.
    const QLineF* primary = beam->primarySegment4();
    if (!primary)
        return 0.0;
    const qreal centre = (primary->y1() + primary->y2()) / 2;
    const qreal upperDistance = (upperSegment->y1() + upperSegment->y2()) / 2 - centre;
    const qreal lowerDistance = (lowerSegment->y1() + lowerSegment->y2()) / 2 - centre;
    return -0.25 * (upperDistance + lowerDistance);
}
}

namespace Ms {
// Studio v4.7.5 BeamLayout::layout1 deduplicates note positions, then
// ChordLayout::computeAutoStemDirection compares the outer pair. Balanced
// pairs cancel; the next pair decides. A completely balanced group goes down.
bool beam4AutoStemUp(std::vector<int> noteLines, int middleLine)
{
    std::sort(noteLines.begin(), noteLines.end());
    noteLines.erase(std::unique(noteLines.begin(), noteLines.end()), noteLines.end());
    int left = 0;
    int right = static_cast<int>(noteLines.size()) - 1;
    while (left <= right) {
        const int net = (noteLines[left] - middleLine) + (noteLines[right] - middleLine);
        if (net != 0)
            return net > 0;
        ++left;
        --right;
    }
    return false;
}
}

namespace Ms {
namespace {
// Studio v4.7.5 Groups::baseBeamMode, adapted to Evolution's duration enums.
// Keep it local to the optional beam engine; the legacy Groups is unchanged.
Beam::Mode baseBeamMode4(ChordRest* cr, ChordRest* prev)
{
    using Mode = Beam::Mode;
    using Duration = TDuration::DurationType;
    if (cr->isGrace() || cr->beamMode() != Mode::AUTO)
        return cr->beamMode();
    const Fraction maxTickLen = qMax(cr->ticks(), prev ? prev->ticks() : Fraction(0, 1));
    Fraction smallestTickLen(1, 8);
    const Fraction tickLenLimit(1, 32);
    while (smallestTickLen > maxTickLen || cr->rtick().ticks() % smallestTickLen.ticks() != 0) {
        smallestTickLen /= 2;
        if (smallestTickLen < tickLenLimit) {
            smallestTickLen = cr->ticks();
            break;
        }
    }
    const Duration bigBeatDuration = TDuration(smallestTickLen).type();
    const TDuration duration = cr->durationType();
    const Groups& group = cr->staff()->group(cr->tick());
    const Fraction stretch = cr->staff()->timeStretch(cr->tick());
    const Fraction tick = cr->rtick() * stretch + cr->measure()->anacrusisOffset();
    const Mode byType = group.beamMode(tick.ticks(), duration.type());
    const Mode byPosition = group.beamMode(tick.ticks(), bigBeatDuration);
    Mode mode = byType == Mode::AUTO ? byPosition : byType;
    if (mode == Mode::AUTO && tick.isNotZero()) {
        if (prev && cr->tuplet() != prev->tuplet() && duration == prev->durationType()) {
            if (duration >= Duration::V_EIGHTH)
                mode = group.beamMode(tick.ticks(), Duration::V_16TH);
            else if (duration == Duration::V_16TH)
                mode = group.beamMode(tick.ticks(), Duration::V_32ND);
            else
                mode = group.beamMode(tick.ticks(), Duration::V_64TH);
        }
        if (cr->voice() && prev && !prev->tuplet() && prev->tick() + prev->actualTicks() < cr->tick())
            mode = Mode::BEGIN;
    }
    return mode;
}

// Studio BEGIN16/BEGIN32 mean secondary/tertiary breaks. Evolution calls
// the equivalent modes BEGIN32/BEGIN64 respectively; do not map by spelling.
void calcBeamBreaks4(ChordRest* cr, ChordRest* prev, int level,
                    bool& secondary, bool& tertiary)
{
    using Mode = Beam::Mode;
    secondary = tertiary = false;
    const Mode manual = cr->beamMode();
    if (cr->isRest() && (manual == Mode::MID || manual == Mode::BEGIN32 || manual == Mode::BEGIN64)) {
        secondary = manual == Mode::BEGIN32 && level > 0;
        tertiary = manual == Mode::BEGIN64 && level > 1;
        return;
    }
    const Mode defaults = baseBeamMode4(cr, prev);
    secondary = level >= 1 && (manual == Mode::BEGIN32 || (manual == Mode::AUTO && defaults == Mode::BEGIN32));
    tertiary = level >= 2 && (manual == Mode::BEGIN64 || (manual == Mode::AUTO && defaults == Mode::BEGIN64));
    if (level > 0 && prev && manual == Mode::AUTO) {
        const Tuplet* boundaryTuplet = nullptr;
        if (cr->tuplet() && cr->tuplet() != prev->tuplet())
            boundaryTuplet = cr->tuplet();
        else if (prev->tuplet() && prev->tuplet() != cr->tuplet())
            boundaryTuplet = prev->tuplet();
        if (boundaryTuplet) {
            const int beams = qMax(TDuration(boundaryTuplet->ticks()).hooks(), 1);
            if (beams <= level) {
                secondary = level == 1;
                tertiary = level >= 2;
            }
        }
    }
}
}

// Shared by segment construction and the beamlet-direction policy.
void beam4CalcBreaks(ChordRest* cr, ChordRest* prev, int level,
                     bool& secondary, bool& tertiary)
{
    calcBeamBreaks4(cr, prev, level, secondary, tertiary);
}

// Studio BeamLayout::calcIsBeamletBefore. True means a left-facing beamlet.
bool beam4BeamletBefore(const std::vector<ChordRest*>& elements, int index, int level)
{
    if (index < 0 || index >= static_cast<int>(elements.size()) || !elements[index]->isChord())
        return false;
    if (index == 0)
        return false;
    if (index == static_cast<int>(elements.size()) - 1)
        return true;
    ChordRest* chord = elements[index];
    Tuplet* tuplet = chord->tuplet();
    if (tuplet && chord == tuplet->elements().front())
        return false;
    if (tuplet && chord == tuplet->elements().back())
        return true;
    ChordRest* next = elements[index + 1];
    ChordRest* prev = elements[index - 1];
    if (next->isChord()) {
        bool nextSecondary, nextTertiary, currentSecondary, currentTertiary;
        calcBeamBreaks4(chord, prev, prev->beams(), currentSecondary, currentTertiary);
        calcBeamBreaks4(next, chord, level, nextSecondary, nextTertiary);
        if ((nextSecondary && level >= 1) || (!currentSecondary && nextTertiary && level >= 2))
            return true;
    }
    bool afterSecondary, afterTertiary;
    calcBeamBreaks4(chord, prev, level, afterSecondary, afterTertiary);
    int previousLevel = -1;
    int nextLevel = -1;
    for (int i = index - 1; i >= 0; --i) {
        if (elements[i]->isChord()) {
            previousLevel = elements[i]->beams();
            if (afterSecondary)
                previousLevel = qMin(previousLevel, 1);
            else if (afterTertiary)
                previousLevel = qMin(previousLevel, 2);
            break;
        }
    }
    for (int i = index + 1; i < static_cast<int>(elements.size()); ++i) {
        if (elements[i]->isChord()) {
            nextLevel = elements[i]->beams();
            break;
        }
    }
    if (nextLevel != previousLevel)
        return nextLevel < previousLevel;
    const Fraction baseTick = tuplet ? tuplet->tick() : chord->measure()->tick();
    const Fraction stretch = chord->staff()->timeStretch(baseTick);
    Fraction tickNext = (next->tick() - baseTick) * stretch;
    if (tuplet && (tuplet->ratio().numerator() & 1))
        tickNext = tickNext * tuplet->ratio();
    const int tickLargeSize = chord->ticks().ticks() * 2;
    if (tickLargeSize <= 0)
        return false;
    const int remainder = tickNext.ticks() % tickLargeSize;
    const int tolerance = 6;
    return remainder <= tolerance || tickLargeSize - remainder <= tolerance;
}
}
