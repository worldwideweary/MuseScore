/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
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

#include <gtest/gtest.h>
#include <algorithm>
#include <memory>

#include "engraving/compat/scoreaccess.h"
#include "engraving/dom/box.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/guitarbend.h"
#include "engraving/dom/hammeronpulloff.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/parenthesis.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/tapping.h"
#include "engraving/rw/read400/readcontext.h"
#include "engraving/rw/read400/tread.h"
#include "engraving/rw/xmlreader.h"
#include "utils/scorerw.h"

using namespace mu::engraving;

static const String LEGACY37E_GUITAR_FILE(u"legacy37eguitar_data/guitar-and-legend-302.mscx");

class Engraving_Legacy37EGuitarTests : public ::testing::Test
{
};

static void checkLegacyGuitarScore(MasterScore* score)
{
    ASSERT_TRUE(score->firstMeasure());
    Segment* first = score->firstMeasure()->first(SegmentType::ChordRest);
    ASSERT_TRUE(first);
    Segment* second = first->next(SegmentType::ChordRest);
    ASSERT_TRUE(second);
    ASSERT_TRUE(first->element(0) && first->element(0)->isChord());
    ASSERT_TRUE(second->element(0) && second->element(0)->isChord());
    Chord* from = toChord(first->element(0));
    Chord* to = toChord(second->element(0));
    Note* start = from->upNote();
    Note* end = to->upNote();
    ASSERT_TRUE(start && end);
    GuitarBend* bend = nullptr;
    int bends = 0;
    for (Spanner* spanner : start->spannerFor()) {
        if (spanner->isGuitarBend()) {
            bend = toGuitarBend(spanner);
            ++bends;
        }
    }
    ASSERT_EQ(bends, 1);
    ASSERT_TRUE(bend);
    EXPECT_EQ(bend->startElement(), start);
    EXPECT_EQ(bend->endElement(), end);
    EXPECT_EQ(bend->tick(), Fraction(0, 1));
    EXPECT_EQ(bend->tick2(), Fraction(1, 2));
    EXPECT_EQ(bend->anchor(), Spanner::Anchor::NOTE);
    EXPECT_EQ(bend->bendType(), GuitarBendType::BEND);
    EXPECT_NEAR(bend->startTimeFactor(), 0.25, 1e-6);
    EXPECT_NEAR(bend->endTimeFactor(), 0.75, 1e-6);
    EXPECT_FALSE(bend->playSpanner());
    EXPECT_NE(std::find(end->spannerBack().begin(), end->spannerBack().end(), bend), end->spannerBack().end());
    EXPECT_TRUE(to->endsNoteAnchoredLine());
    ASSERT_FALSE(bend->spannerSegments().empty());
    const auto* segment = toGuitarBendSegment(bend->frontSegment());
    EXPECT_NEAR(segment->vertexPointOff().x() / score->style().spatium(), 0.3, 1e-6);
    EXPECT_NEAR(segment->vertexPointOff().y() / score->style().spatium(), -0.4, 1e-6);
    EXPECT_NEAR(segment->userOff2().x() / score->style().spatium(), 0.1, 1e-6);
    EXPECT_NEAR(segment->userOff2().y() / score->style().spatium(), 0.2, 1e-6);

    HammerOnPullOff* hopo = nullptr;
    int hopos = 0;
    for (const auto& entry : score->spanner()) {
        if (entry.second->isHammerOnPullOff()) {
            hopo = toHammerOnPullOff(entry.second);
            ++hopos;
        }
    }
    ASSERT_EQ(hopos, 1);
    ASSERT_TRUE(hopo);
    EXPECT_EQ(hopo->startElement(), from);
    EXPECT_EQ(hopo->endElement(), to);
    EXPECT_EQ(hopo->tick(), Fraction(0, 1));
    EXPECT_EQ(hopo->tick2(), Fraction(1, 2));
    EXPECT_EQ(hopo->styleType(), SlurStyleType::Dotted);

    ASSERT_TRUE(score->first() && score->first()->isFBox());
    FBox* frame = toFBox(score->first());
    EXPECT_DOUBLE_EQ(frame->getProperty(Pid::FRET_FRAME_TEXT_SCALE).toDouble(), 1.2);
    EXPECT_DOUBLE_EQ(frame->getProperty(Pid::FRET_FRAME_DIAGRAM_SCALE).toDouble(), 0.9);
    EXPECT_DOUBLE_EQ(frame->getProperty(Pid::FRET_FRAME_COLUMN_GAP).value<Spatium>().val(), 3.5);
    EXPECT_DOUBLE_EQ(frame->getProperty(Pid::FRET_FRAME_ROW_GAP).value<Spatium>().val(), 2.25);
    EXPECT_EQ(frame->getProperty(Pid::FRET_FRAME_CHORDS_PER_ROW).toInt(), 5);
    EXPECT_EQ(frame->getProperty(Pid::FRET_FRAME_H_ALIGN).toInt(), 0);
    EXPECT_DOUBLE_EQ(frame->getProperty(Pid::PADDING_TO_NOTATION_ABOVE).value<Spatium>().val(), 1.25);
    EXPECT_DOUBLE_EQ(frame->getProperty(Pid::PADDING_TO_NOTATION_BELOW).value<Spatium>().val(), 2.75);
}

TEST_F(Engraving_Legacy37EGuitarTests, importAndNativeRoundTrip)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(LEGACY37E_GUITAR_FILE));
    ASSERT_TRUE(score);
    ASSERT_NO_FATAL_FAILURE(checkLegacyGuitarScore(score.get()));
    const String output(u"legacy37e-guitar-roundtrip.mscx");
    ASSERT_TRUE(ScoreRW::saveScore(score.get(), output));
    std::unique_ptr<MasterScore> reopened(ScoreRW::readScore(output, true));
    ASSERT_TRUE(reopened);
    ASSERT_NO_FATAL_FAILURE(checkLegacyGuitarScore(reopened.get()));
}

TEST_F(Engraving_Legacy37EGuitarTests, allBendTypesAndHoldProperties)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(LEGACY37E_GUITAR_FILE));
    ASSERT_TRUE(score);
    Note* start = toChord(score->firstMeasure()->first(SegmentType::ChordRest)->element(0))->upNote();
    for (int kind = 0; kind <= 7; ++kind) {
        SCOPED_TRACE(kind);
        std::unique_ptr<GuitarBend> bend(Factory::createGuitarBend(start));
        bend->setTrack(0);
        const String data = String(u"<GuitarBend><guitarBendType>%1</guitarBendType>"
                                  "<bendStartTimeFactor>0.2</bendStartTimeFactor>"
                                  "<bendEndTimeFactor>0.8</bendEndTimeFactor>"
                                  "<bendTargetTimeFactor>0.4</bendTargetTimeFactor>"
                                  "<guitarBendAmount>-6</guitarBendAmount>"
                                  "<bendShowHoldLine>1</bendShowHoldLine><play>0</play>"
                                  "<GuitarBendHold><lineStyle>dashed</lineStyle></GuitarBendHold>"
                                  "</GuitarBend>").arg(kind);
        XmlReader xml(data.toUtf8());
        ASSERT_TRUE(xml.readNextStartElement());
        read400::ReadContext context(score.get());
        read400::TRead::readItem(bend.get(), xml, context);
        ASSERT_EQ(xml.error(), muse::XmlStreamReader::NoError);
        EXPECT_EQ(int(bend->bendType()), kind);
        EXPECT_NEAR(bend->startTimeFactor(), 0.2, 1e-6);
        EXPECT_NEAR(bend->endTimeFactor(), 0.8, 1e-6);
        EXPECT_NEAR(bend->getProperty(Pid::BEND_TARGET_TIME_FACTOR).toDouble(), 0.4, 1e-6);
        EXPECT_EQ(bend->bendAmountInQuarterTones(), -6);
        EXPECT_EQ(bend->showHoldLine(), GuitarBendShowHoldLine::SHOW);
        EXPECT_FALSE(bend->playSpanner());
        ASSERT_TRUE(bend->holdLine());
        EXPECT_EQ(bend->holdLine()->explicitParent(), bend.get());
    }
}

static void checkLegacyParentheses(MasterScore* score)
{
    Measure* first = score->firstMeasure();
    ASSERT_TRUE(first);
    bool checkedTabGhost = false;
    for (const Segment& segment : first->segments()) {
        const EngravingItem* standard = segment.element(0);
        if (standard && standard->isChord()) {
            // TAB-only parentheses must not leak onto linked normal notes.
            EXPECT_TRUE(toChord(standard)->noteParentheses().empty());
        }
        const EngravingItem* tab = segment.element(4);
        if (tab && tab->isChord()) {
            for (const Note* note : toChord(tab)->notes()) {
                if (note->ghost()) {
                    const NoteParenthesisInfo* group = note->parenthesisInfo();
                    ASSERT_TRUE(group);
                    ASSERT_TRUE(group->leftParen());
                    ASSERT_TRUE(group->rightParen());
                    EXPECT_EQ(group->leftParen()->parentItem(), toChord(tab));
                    EXPECT_EQ(group->rightParen()->parentItem(), toChord(tab));
                    checkedTabGhost = true;
                }
            }
        }
    }
    EXPECT_TRUE(checkedTabGhost);

    Measure* ninth = first;
    for (int i = 1; i < 9 && ninth; ++i) {
        ninth = ninth->nextMeasure();
    }
    ASSERT_TRUE(ninth);
    // Both staves contain an explicit three-note group at the end of m. 9.
    // The old reader ignored the group and parenthesized just one TAB note.
    for (track_idx_t track : { track_idx_t(0), track_idx_t(4) }) {
        bool found = false;
        for (const Segment& segment : ninth->segments()) {
            const EngravingItem* item = segment.element(track);
            if (!item || !item->isChord()) {
                continue;
            }
            const Chord* chord = toChord(item);
            for (const NoteParenthesisInfo* group : chord->noteParentheses()) {
                if (group->notes().size() == 3) {
                    ASSERT_TRUE(group->leftParen());
                    ASSERT_TRUE(group->rightParen());
                    EXPECT_EQ(group->leftParen()->parentItem(), chord);
                    EXPECT_EQ(group->rightParen()->parentItem(), chord);
                    found = true;
                }
            }
        }
        EXPECT_TRUE(found) << "Missing three-note parenthesis group on track " << track;
    }
}

TEST_F(Engraving_Legacy37EGuitarTests, staffLocalParenthesesAndChordGroups)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(
        String(u"legacy37eguitar_data/parentheses-302.mscx")));
    ASSERT_TRUE(score);
    ASSERT_NO_FATAL_FAILURE(checkLegacyParentheses(score.get()));
    const String output(u"legacy37e-parentheses-roundtrip.mscx");
    ASSERT_TRUE(ScoreRW::saveScore(score.get(), output));
    std::unique_ptr<MasterScore> reopened(ScoreRW::readScore(output, true));
    ASSERT_TRUE(reopened);
    ASSERT_NO_FATAL_FAILURE(checkLegacyParentheses(reopened.get()));
}

TEST_F(Engraving_Legacy37EGuitarTests, legacyParenthesesPropertyValue)
{
    std::unique_ptr<MasterScore> score(compat::ScoreAccess::createMasterScore(nullptr));
    read400::ReadContext ctx(score.get());
    for (bool enabled : { false, true }) {
        const String data = enabled ? String(u"<parentheses>both</parentheses>")
                                    : String(u"<parentheses>none</parentheses>");
        XmlReader xml(data.toUtf8());
        ASSERT_TRUE(xml.readNextStartElement());
        const PropertyValue value = read400::TRead::readPropertyValue(Pid::HAS_PARENTHESES, xml, ctx);
        EXPECT_EQ(value.value<ParenthesesMode>(), enabled ? ParenthesesMode::BOTH : ParenthesesMode::NONE);
        EXPECT_EQ(xml.error(), muse::XmlStreamReader::NoError);
    }
}

static void checkLegacyTapping(MasterScore* score)
{
    int counts[2][2] = {};
    for (Measure* measure = score->firstMeasure(); measure; measure = measure->nextMeasure()) {
        for (const Segment& segment : measure->segments()) {
            for (int staff = 0; staff < 2; ++staff) {
                const EngravingItem* item = segment.element(staff * 4);
                if (!item || !item->isChord()) {
                    continue;
                }
                const Chord* chord = toChord(item);
                for (const Articulation* articulation : chord->articulations()) {
                    if (!articulation->isTapping()) {
                        continue;
                    }
                    const Tapping* tapping = toTapping(articulation);
                    EXPECT_EQ(tapping->explicitParent(), chord);
                    EXPECT_EQ(tapping->track(), chord->track());
                    ASSERT_TRUE(tapping->hand() == TappingHand::LEFT || tapping->hand() == TappingHand::RIGHT);
                    ++counts[staff][tapping->hand() == TappingHand::RIGHT ? 1 : 0];
                }
            }
        }
    }
    // The reported 37E score contains left- and right-hand tapping on both
    // linked staves. Losing either element removes its staff-specific symbol.
    for (int staff = 0; staff < 2; ++staff) {
        EXPECT_EQ(counts[staff][0], 1);
        EXPECT_EQ(counts[staff][1], 1);
    }
}

TEST_F(Engraving_Legacy37EGuitarTests, tappingLegacyImportAndNativeRoundTrip)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(
        String(u"legacy37eguitar_data/parentheses-302.mscx")));
    ASSERT_TRUE(score);
    ASSERT_NO_FATAL_FAILURE(checkLegacyTapping(score.get()));
    const String output(u"legacy37e-tapping-roundtrip.mscx");
    ASSERT_TRUE(ScoreRW::saveScore(score.get(), output));
    std::unique_ptr<MasterScore> reopened(ScoreRW::readScore(output, true));
    ASSERT_TRUE(reopened);
    ASSERT_NO_FATAL_FAILURE(checkLegacyTapping(reopened.get()));
}

TEST_F(Engraving_Legacy37EGuitarTests, tappingHalfSlurLegacyDispatch)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(LEGACY37E_GUITAR_FILE));
    ASSERT_TRUE(score);
    Chord* chord = toChord(score->firstMeasure()->first(SegmentType::ChordRest)->element(0));
    std::unique_ptr<Tapping> tapping(Factory::createTapping(chord));
    tapping->setTrack(chord->track());
    const String data(u"<Tapping><hand>left</hand>"
                      "<TappingHalfSlur><isHalfSlurAbove>1</isHalfSlurAbove>"
                      "<TappingHalfSlurSegment no=\"0\"/></TappingHalfSlur>"
                      "<TappingHalfSlur><isHalfSlurAbove>0</isHalfSlurAbove>"
                      "<TappingHalfSlurSegment no=\"0\"/></TappingHalfSlur></Tapping>");
    XmlReader xml(data.toUtf8());
    ASSERT_TRUE(xml.readNextStartElement());
    read400::ReadContext ctx(score.get());
    read400::TRead::readItem(tapping.get(), xml, ctx);
    EXPECT_EQ(xml.error(), muse::XmlStreamReader::NoError);
    EXPECT_EQ(tapping->hand(), TappingHand::LEFT);
    ASSERT_TRUE(tapping->halfSlurAbove());
    ASSERT_TRUE(tapping->halfSlurBelow());
    EXPECT_TRUE(tapping->halfSlurAbove()->isHalfSlurAbove());
    EXPECT_FALSE(tapping->halfSlurBelow()->isHalfSlurAbove());
    EXPECT_EQ(tapping->halfSlurAbove()->explicitParent(), tapping.get());
    EXPECT_EQ(tapping->halfSlurBelow()->explicitParent(), tapping.get());
    EXPECT_FALSE(tapping->halfSlurAbove()->segmentsEmpty());
    EXPECT_FALSE(tapping->halfSlurBelow()->segmentsEmpty());
}

static void checkLegacyFrameClearances(MasterScore* score)
{
    const double expectedAbove[] = { 2.0, 2.5, 0.0, 0.0, 2.5 };
    const double expectedBelow[] = { 2.0, 3.5, 0.0, 0.0, 0.0 };
    MeasureBase* frame = score->first();
    for (int kind = 0; kind < 3; ++kind) {
        for (int value = 0; value < 5; ++value) {
            ASSERT_TRUE(frame);
            ASSERT_TRUE(frame->isVBoxBase());
            const ElementType expectedType = kind == 0 ? ElementType::VBOX
                                            : kind == 1 ? ElementType::TBOX : ElementType::FBOX;
            EXPECT_EQ(frame->type(), expectedType);
            EXPECT_DOUBLE_EQ(frame->getProperty(Pid::PADDING_TO_NOTATION_ABOVE).value<Spatium>().val(), expectedAbove[value]);
            EXPECT_DOUBLE_EQ(frame->getProperty(Pid::PADDING_TO_NOTATION_BELOW).value<Spatium>().val(), expectedBelow[value]);
            frame = frame->next();
        }
    }
    ASSERT_TRUE(frame);
    EXPECT_TRUE(frame->isMeasure());
}

TEST_F(Engraving_Legacy37EGuitarTests, frameClearanceLegacyImportAndNativeRoundTrip)
{
    // Explicit defaults, custom values, explicit zero, absent sides, and a
    // one-sided override must survive independently for VBox, TBox and FBox.
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(
        String(u"legacy37eguitar_data/frame-clearance-302.mscx")));
    ASSERT_TRUE(score);
    ASSERT_NO_FATAL_FAILURE(checkLegacyFrameClearances(score.get()));
    const String output(u"legacy37e-frame-clearance-roundtrip.mscx");
    ASSERT_TRUE(ScoreRW::saveScore(score.get(), output));
    std::unique_ptr<MasterScore> reopened(ScoreRW::readScore(output, true));
    ASSERT_TRUE(reopened);
    ASSERT_NO_FATAL_FAILURE(checkLegacyFrameClearances(reopened.get()));
}
