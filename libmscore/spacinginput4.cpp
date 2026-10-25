// Read-only musical inputs for the optional Studio spacing pipeline.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#include "spacinginput4.h"
#include "score.h"
#include "measure.h"
#include "segment.h"
#include "chord.h"
#include "rest.h"
#include "note.h"
#include "slur.h"
#include "spannermap.h"
#include <utility>
#include <map>
#include <algorithm>

namespace Ms {
const SpacingAnchorInput4* SpacingInput4::anchor(const ChordRest* item) const
{
    const auto found = _anchorIndex.find(item);
    return found == _anchorIndex.end() ? nullptr : &_anchors[found->second];
}

const SpacingSlurInput4* SpacingInput4::slur(const Slur* item) const
{
    const auto found = _slurIndex.find(item);
    return found == _slurIndex.end() ? nullptr : &_slurs[found->second];
}

const SpacingNoteInput4* SpacingInput4::note(const Note* item) const
{
    const auto found = _noteIndex.find(item);
    return found == _noteIndex.end() ? nullptr : &_notes[found->second];
}

bool buildSpacingInput4(Score* score, SpacingInput4& result, QString* reason)
{
    const auto fail = [reason](const char* message) {
        if (reason) *reason = QString::fromLatin1(message);
        return false;
    };
    if (!score) return fail("Missing score for spacing inputs");
    SpacingInput4 pending;
    pending._score = score;
    std::unordered_map<int, size_t> nextOrder;
    const auto append = [&](const ChordRest* item, const Chord* main, const Measure* measure,
                            const Segment* segment, SpacingMemberKind4 kind, size_t member) {
        if (!item || item->score() != score || item->track() < 0
            || item->track() >= score->ntracks() || pending._anchorIndex.count(item)) return false;
        SpacingAnchorInput4 input;
        input.item = item;
        input.main = main;
        input.measure = measure;
        input.segment = segment;
        input.tick = item->tick();
        input.track = item->track();
        input.orderInTrack = nextOrder[input.track]++;
        input.kind = kind;
        input.memberIndex = member;
        input.up = item->up();
        input.visible = item->visible();
        input.segmentEnabled = segment->enabled();
        input.segmentVisible = segment->visible();
        input.staffMove = item->staffMove();
        const size_t index = pending._anchors.size();
        pending._anchorIndex.emplace(item, index);
        pending._anchors.push_back(std::move(input));
        if (item->isChord()) {
            for (const Note* note : toChord(item)->notes()) {
                if (!note || note->score() != score || note->chord() != item
                    || pending._noteIndex.count(note)) return false;
                SpacingNoteInput4 n;
                n.note = note;
                n.anchorIndex = index;
                n.tieFor = note->tieFor();
                n.tieBack = note->tieBack();
                for (const Spanner* spanner : note->spannerFor()) n.spannersFor.push_back(spanner);
                for (const Spanner* spanner : note->spannerBack()) n.spannersBack.push_back(spanner);
                pending._noteIndex.emplace(note, pending._notes.size());
                pending._notes.push_back(std::move(n));
            }
        }
        return true;
    };
    for (const Measure* measure = score->firstMeasure(); measure; measure = measure->nextMeasure()) {
        for (const Segment& segment : measure->segments()) {
            if (!segment.isChordRestType()) continue;
            for (const Element* element : segment.elist()) {
                if (!element || !element->isChordRest()) continue;
                const ChordRest* item = toChordRest(element);
                const Chord* main = item->isChord() ? toChord(item) : nullptr;
                if (item->segment() != &segment || item->measure() != measure || (main && main->isGrace()))
                    return fail("Invalid main spacing anchor ownership");
                if (main) {
                    size_t index = 0;
                    for (const Chord* grace : main->graceNotesBefore()) {
                        if (!grace || grace->parent() != main || !grace->isGraceBefore()
                            || grace->track() != main->track()
                            || !append(grace, main, measure, &segment, SpacingMemberKind4::BEFORE, index++))
                            return fail("Invalid before-grace spacing ownership");
                    }
                }
                if (!append(item, main, measure, &segment, SpacingMemberKind4::MAIN, 0))
                    return fail("Invalid or duplicate main spacing anchor");
                if (main) {
                    size_t index = 0;
                    // Evolution stores after-graces in reverse; the accessor
                    // supplies their actual navigation/musical order.
                    for (const Chord* grace : main->graceNotesAfter()) {
                        if (!grace || grace->parent() != main || !grace->isGraceAfter()
                            || grace->track() != main->track()
                            || !append(grace, main, measure, &segment, SpacingMemberKind4::AFTER, index++))
                            return fail("Invalid after-grace spacing ownership");
                    }
                }
            }
        }
    }
    std::map<std::pair<int, Fraction>, std::vector<size_t>> anchorsAt;
    for (size_t i = 0; i < pending._anchors.size(); ++i) {
        const auto& anchor = pending._anchors[i];
        anchorsAt[{anchor.track, anchor.tick}].push_back(i);
    }
    const auto endpoint = [&pending](const Element* element) {
        SpacingEndpointInput4 end;
        end.element = element;
        const ChordRest* anchor = nullptr;
        if (!element) return end;
        if (element->isChordRest()) {
            anchor = toChordRest(element);
            end.kind = element->isChord() ? SpacingEndpointKind4::CHORD : SpacingEndpointKind4::REST;
        }
        else if (element->isNote()) {
            anchor = toNote(element)->chord();
            end.kind = SpacingEndpointKind4::NOTE;
        }
        else end.kind = SpacingEndpointKind4::OTHER;
        const auto found = pending._anchorIndex.find(anchor);
        if (found != pending._anchorIndex.end()) end.anchorIndex = found->second;
        return end;
    };
    for (const auto& entry : score->spannerMap().map()) {
        if (!entry.second || !entry.second->isSlur()) continue;
        const Slur* slur = toSlur(entry.second);
        if (slur->score() != score || pending._slurIndex.count(slur))
            return fail("Invalid or duplicate root slur input");
        SpacingSlurInput4 input;
        input.slur = slur;
        input.start = endpoint(slur->startElement());
        input.end = endpoint(slur->endElement());
        input.tick = slur->tick();
        input.tick2 = slur->tick2();
        input.track = slur->track();
        input.track2 = slur->track2();
        input.requestedDirection = slur->slurDirection();
        input.cachedUp = slur->up();
        input.visible = slur->visible();
        input.lineType = slur->lineType();
        const size_t index = pending._slurs.size();
        pending._slurIndex.emplace(slur, index);
        pending._slurs.push_back(std::move(input));
        const auto& stored = pending._slurs.back();
        if (stored.start.anchorIndex != SPACING_INPUT_NONE4)
            pending._anchors[stored.start.anchorIndex].startingSlurs.push_back(index);
        if (stored.end.anchorIndex != SPACING_INPUT_NONE4)
            pending._anchors[stored.end.anchorIndex].endingSlurs.push_back(index);
        std::vector<size_t> related;
        const auto addAt = [&anchorsAt, &related](int track, const Fraction& tick) {
            const auto found = anchorsAt.find({track, tick});
            if (found != anchorsAt.end()) related.insert(related.end(), found->second.begin(), found->second.end());
        };
        addAt(stored.track, stored.tick);
        addAt(stored.track2, stored.tick2);
        if (stored.start.kind != SpacingEndpointKind4::NOTE && stored.start.anchorIndex != SPACING_INPUT_NONE4)
            related.push_back(stored.start.anchorIndex);
        if (stored.end.kind != SpacingEndpointKind4::NOTE && stored.end.anchorIndex != SPACING_INPUT_NONE4)
            related.push_back(stored.end.anchorIndex);
        std::sort(related.begin(), related.end());
        related.erase(std::unique(related.begin(), related.end()), related.end());
        // Root iteration order is preserved once per recipient, even when
        // start/end/tick conditions all match the same grace member.
        for (size_t anchor : related) pending._anchors[anchor].relatedSlurs.push_back(index);
    }
    result = std::move(pending);
    if (reason) reason->clear();
    return true;
}
}
