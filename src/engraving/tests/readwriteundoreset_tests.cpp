/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

#include <memory>

#include "engraving/dom/masterscore.h"
#include "engraving/dom/arpeggio.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/segment.h"
#include "engraving/editing/undo.h"

#include "utils/scorerw.h"
#include "utils/scorecomp.h"

using namespace mu::engraving;

static const String RWUNDORESET_DATA_DIR("readwriteundoreset_data/");

class Engraving_ReadWriteUndoResetTests : public ::testing::Test
{
};

TEST_F(Engraving_ReadWriteUndoResetTests, testReadWriteResetPositions)
{
    std::vector<const char16_t*> files = {
        u"barlines",
        u"slurs",
        u"mmrestBarlineTextLinks" // see issue #296426
    };

    for (const char16_t* file : files) {
        String readFile(RWUNDORESET_DATA_DIR + file + u".mscx");
        String writeFile(String(file) + u"-undoreset-test.mscx");

        MasterScore* score = ScoreRW::readScore(readFile);
        EXPECT_TRUE(score);
        score->cmdResetAllPositions();
        score->undoRedo(/* undo */ true, nullptr);
        EXPECT_TRUE(ScoreComp::saveCompareScore(score, writeFile, readFile));

        delete score;
    }
}

//---------------------------------------------------------
//   testMMRestLinksRecreateMMRest
///   For barlines links with MM rests a separate test is
///   needed: in this test score, if creating MM rests from
///   scratch, <linked> tags in BarLines may have appeared
///   before <linkedMain> tags, so they were not able to
///   link and prevented text elements from linking as well.
///
///   See issue #296426
//---------------------------------------------------------

TEST_F(Engraving_ReadWriteUndoResetTests, testMMRestLinksRecreateMMRest)
{
    const String file("mmrestBarlineTextLinks");

    String readFile(RWUNDORESET_DATA_DIR + file + u".mscx");
    String writeFile(file + u"-recreate-mmrest-test.mscx");
    String disableMMRestRefFile(RWUNDORESET_DATA_DIR + file + u"-disable-mmrest-ref.mscx");
    String recreateMMRestRefFile(RWUNDORESET_DATA_DIR + file + u"-recreate-mmrest-ref.mscx");

    MasterScore* score = ScoreRW::readScore(readFile);
    EXPECT_TRUE(score);

    // Regenerate MM rests from scratch:
    // 1) turn MM rests off
    score->startCmd(TranslatableString::untranslatable("Read/write/undo/reset tests"));
    score->undoChangeStyleVal(Sid::createMultiMeasureRests, false);
    score->endCmd();

    // 2) save/close/reopen the score
    EXPECT_TRUE(ScoreComp::saveCompareScore(score, writeFile, disableMMRestRefFile));
    delete score;
    score = ScoreRW::readScore(writeFile, true);

    // 3) turn MM rests back on
    score->startCmd(TranslatableString::untranslatable("Read/write/undo/reset tests"));
    score->undoChangeStyleVal(Sid::createMultiMeasureRests, true);
    score->endCmd();

    EXPECT_TRUE(ScoreComp::saveCompareScore(score, writeFile, recreateMMRestRefFile));

    delete score;
}

TEST_F(Engraving_ReadWriteUndoResetTests, legacyArpeggioVoiceSpans)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(RWUNDORESET_DATA_DIR + u"arpeggio-voices-37e.mscx"));
    ASSERT_TRUE(score);

    const auto check = [](MasterScore* s) {
        // Same staff, cross staff, reversed tag order, legacy staff count,
        // default span, and an invalid extension clamped to one track.
        struct Case { track_idx_t track; int span; };
        const Case cases[] = { { 1, 3 }, { 1, 6 }, { 1, 6 }, { 0, 5 }, { 2, 1 }, { 0, 1 } };
        Segment* segment = s->firstSegment(SegmentType::ChordRest);
        for (const Case& c : cases) {
            ASSERT_TRUE(segment);
            EngravingItem* element = segment->element(c.track);
            ASSERT_TRUE(element && element->isChord());
            Arpeggio* arpeggio = toChord(element)->arpeggio();
            ASSERT_TRUE(arpeggio);
            EXPECT_EQ(arpeggio->track(), c.track);
            EXPECT_EQ(arpeggio->span(), c.span);
            EXPECT_EQ(arpeggio->endTrack(), c.track + c.span - 1);
            EXPECT_EQ(arpeggio->arpeggioType(), ArpeggioType::DOWN);
            EXPECT_FALSE(arpeggio->playArpeggio());
            EXPECT_DOUBLE_EQ(arpeggio->Stretch(), 1.5);
            EXPECT_NEAR(arpeggio->userLen1() / arpeggio->spatium(), 0.25, 0.000001);
            EXPECT_NEAR(arpeggio->userLen2() / arpeggio->spatium(), 0.5, 0.000001);
            segment = segment->next1(SegmentType::ChordRest);
        }
    };

    check(score.get());
    ASSERT_FALSE(HasFatalFailure());
    const String output(u"arpeggio-voices-37e-roundtrip.mscx");
    ASSERT_TRUE(ScoreRW::saveScore(score.get(), output));
    score.reset(ScoreRW::readScore(output, true));
    ASSERT_TRUE(score);
    check(score.get());
}
