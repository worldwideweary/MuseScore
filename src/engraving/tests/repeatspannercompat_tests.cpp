/* SPDX-License-Identifier: GPL-3.0-only */
#include <gtest/gtest.h>
#include <QTemporaryDir>
#include "engraving/dom/chord.h"
#include "engraving/dom/lyrics.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/partialtie.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/slur.h"
#include "utils/scorerw.h"

using namespace mu::engraving;

static void expectPartialTieOffsets(const PartialTie* tie)
{
    ASSERT_TRUE(tie);
    ASSERT_EQ(tie->spannerSegments().size(), 1u);
    const SlurTieSegment* segment = tie->segmentAt(0);
    const Grip grips[] = { Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END };
    const PointF offsets[] = { PointF(-2.5, 0.0), PointF(-0.5, 0.75), PointF(0.25, -0.5), PointF(1.0, 0.25) };
    const double spatium = segment->style().spatium();
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_NEAR(segment->ups(grips[i]).off.x() / spatium, offsets[i].x(), 0.0001);
        EXPECT_NEAR(segment->ups(grips[i]).off.y() / spatium, offsets[i].y(), 0.0001);
    }
}

static void expectRepeatSpanners(MasterScore* score)
{
    std::vector<Note*> notes;
    for (Measure* measure = score->firstMeasure(); measure; measure = measure->nextMeasure()) {
        notes.push_back(toChord(measure->first(SegmentType::ChordRest)->element(0))->upNote());
    }
    ASSERT_EQ(notes.size(), 6u);
    ASSERT_TRUE(notes[1]->tieFor());
    EXPECT_EQ(notes[1]->tieFor()->endNote(), notes[2]);
    EXPECT_TRUE(notes[1]->incomingPartialTie());
    EXPECT_TRUE(notes[2]->outgoingPartialTie());
    EXPECT_TRUE(notes[3]->incomingPartialTie());
    expectPartialTieOffsets(notes[1]->incomingPartialTie());
    expectPartialTieOffsets(notes[2]->outgoingPartialTie());
    // The normal legacy tie deliberately retains upstream's offset reset.
    ASSERT_FALSE(notes[1]->tieFor()->spannerSegments().empty());
    EXPECT_TRUE(notes[1]->tieFor()->segmentAt(0)->ups(Grip::START).off.isNull());
    size_t incomingSlurs = 0;
    size_t partialLyrics = 0;
    for (const auto& entry : score->spanner()) {
        const Spanner* spanner = entry.second;
        if (spanner->isSlur() && toSlur(spanner)->partialSpannerDirection() == PartialSpannerDirection::INCOMING) {
            ++incomingSlurs;
        }
        if (spanner->isPartialLyricsLine()) {
            ++partialLyrics;
            EXPECT_TRUE(toPartialLyricsLine(spanner)->isEndMelisma());
        }
    }
    EXPECT_EQ(incomingSlurs, 1u);
    EXPECT_EQ(partialLyrics, 1u);
    EXPECT_TRUE(notes[3]->chord()->lyrics().empty()); // Empty 3.x owner migrated.
    ASSERT_FALSE(notes[1]->chord()->lyrics().empty());
    EXPECT_EQ(notes[1]->chord()->lyrics().front()->plainText(), String(u"jump"));
}


// Conversion must survive a native 4.7 save/reopen, not only the legacy read.
TEST(Engraving_RepeatSpannerCompatTests, evolution302)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(String(u"repeatspannercompat_data/evolution-repeat-spanners.mscx")));
    ASSERT_TRUE(score);
    expectRepeatSpanners(score.get());

    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    const String path = String::fromQString(temporary.filePath("repeat-spanner-compat.mscx"));
    ASSERT_TRUE(ScoreRW::saveScore(score.get(), path));
    std::unique_ptr<MasterScore> reloaded(ScoreRW::readScore(path, true));
    ASSERT_TRUE(reloaded);
    expectRepeatSpanners(reloaded.get());
}
