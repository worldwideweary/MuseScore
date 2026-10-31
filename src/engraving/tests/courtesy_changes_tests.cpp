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
#include <gmock/gmock.h>

#include <array>
#include <memory>
#include <string>

#include "engraving/dom/barline.h"
#include "engraving/dom/clef.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "global/io/buffer.h"

#include "utils/scorerw.h"
#include "utils/scorecomp.h"

using namespace mu::engraving;
static const String COURTESY_CHANGES_DATA(u"courtesy_changes_data/");

class Engraving_CourtesyChangesTests : public ::testing::Test
{
protected:
    void setStyle(MasterScore* score)
    {
        score->startCmd(TranslatableString::untranslatable("Courtesy changes"));
        score->style().set(Sid::showCourtesiesRepeats, false);
        score->style().set(Sid::showCourtesiesOtherJumps, false);
        score->style().set(Sid::showCourtesiesAfterCancellingRepeats, false);
        score->style().set(Sid::showCourtesiesAfterCancellingOtherJumps, false);
        score->endCmd();
    }
};

TEST_F(Engraving_CourtesyChangesTests, toggleCourtesies) {
    // Open file with courtesies switched off
    String scoreName(u"courtesy-changes");
    MasterScore* score = ScoreRW::readScore(COURTESY_CHANGES_DATA + scoreName + u".mscx");

    EXPECT_TRUE(score);
    setStyle(score);
    // Toggle them and check correct segments exist and are enabled in the bar

    // Turn on "Show at repeats"
    score->startCmd(TranslatableString::untranslatable("Courtesy changes"));
    score->style().set(Sid::showCourtesiesRepeats, true);
    score->endCmd();
    Measure* m2 = toMeasure(score->measure(2));
    EXPECT_TRUE(m2);
    Segment* m2ClefRepeat = m2->findSegmentR(SegmentType::ClefRepeatAnnounce, m2->ticks());
    Segment* m2KsRepeat = m2->findSegmentR(SegmentType::KeySigRepeatAnnounce, m2->ticks());
    Segment* m2TsRepeat = m2->findSegmentR(SegmentType::TimeSigRepeatAnnounce, m2->ticks());
    EXPECT_TRUE(m2ClefRepeat && m2ClefRepeat->enabled());
    EXPECT_TRUE(m2KsRepeat && m2KsRepeat->enabled());
    EXPECT_TRUE(m2TsRepeat && m2TsRepeat->enabled());

    // Turn on "Show at all other jumps"
    score->startCmd(TranslatableString::untranslatable("Courtesy changes"));
    score->style().set(Sid::showCourtesiesOtherJumps, true);
    score->endCmd();
    Measure* m4 = toMeasure(score->measure(4));
    EXPECT_TRUE(m4);
    Segment* m4KsRepeat = m4->findSegmentR(SegmentType::KeySigRepeatAnnounce, m4->ticks());
    Segment* m4TsRepeat = m4->findSegmentR(SegmentType::TimeSigRepeatAnnounce, m4->ticks());
    EXPECT_TRUE(m4KsRepeat && m4KsRepeat->enabled());
    EXPECT_TRUE(m4TsRepeat && m4TsRepeat->enabled());

    Measure* m5 = toMeasure(score->measure(5));
    EXPECT_TRUE(m5);
    Segment* m5ClefRepeat = m5->findSegmentR(SegmentType::ClefRepeatAnnounce, m5->ticks());
    Segment* m5KsRepeat = m5->findSegmentR(SegmentType::KeySigRepeatAnnounce, m5->ticks());
    Segment* m5TsRepeat = m5->findSegmentR(SegmentType::TimeSigRepeatAnnounce, m5->ticks());
    EXPECT_TRUE(m5ClefRepeat && m5ClefRepeat->enabled());
    EXPECT_TRUE(m5KsRepeat && m5KsRepeat->enabled());
    EXPECT_TRUE(m5TsRepeat && m5TsRepeat->enabled());

    // Turn on "Show when cancelling a change before repeats"
    score->startCmd(TranslatableString::untranslatable("Courtesy changes"));
    score->style().set(Sid::showCourtesiesAfterCancellingRepeats, true);
    score->endCmd();
    Measure* m3 = toMeasure(score->measure(3));
    EXPECT_TRUE(m3);
    Segment* m3ClefRepeat = m3->findSegmentR(SegmentType::ClefStartRepeatAnnounce, Fraction(0, 1));
    Segment* m3KsRepeat = m3->findSegmentR(SegmentType::KeySigStartRepeatAnnounce, Fraction(0, 1));
    Segment* m3TsRepeat = m3->findSegmentR(SegmentType::TimeSigStartRepeatAnnounce, Fraction(0, 1));
    EXPECT_TRUE(m3ClefRepeat && m3ClefRepeat->enabled());
    EXPECT_TRUE(m3KsRepeat && m3KsRepeat->enabled());
    EXPECT_TRUE(m3TsRepeat && m3TsRepeat->enabled());

    // Turn on "Show when cancelling a change before all other jumps"
    score->startCmd(TranslatableString::untranslatable("Courtesy changes"));
    score->style().set(Sid::showCourtesiesAfterCancellingOtherJumps, true);
    score->endCmd();
    Segment* m5TsRepeatStart = m5->findSegmentR(SegmentType::TimeSigStartRepeatAnnounce, Fraction(0, 1));
    EXPECT_TRUE(m5TsRepeatStart && m5TsRepeatStart->enabled());

    Measure* m6 = toMeasure(score->measure(6));
    EXPECT_TRUE(m6);
    Segment* m6ClefRepeat = m6->findSegmentR(SegmentType::ClefStartRepeatAnnounce, Fraction(0, 1));
    Segment* m6KsRepeat = m6->findSegmentR(SegmentType::KeySigStartRepeatAnnounce, Fraction(0, 1));
    Segment* m6TsRepeat = m6->findSegmentR(SegmentType::TimeSigStartRepeatAnnounce, Fraction(0, 1));
    EXPECT_TRUE(m6ClefRepeat && m6ClefRepeat->enabled());
    EXPECT_TRUE(m6KsRepeat && m6KsRepeat->enabled());
    EXPECT_TRUE(m6TsRepeat && m6TsRepeat->enabled());
}

TEST_F(Engraving_CourtesyChangesTests, placementBetweenRepeats) {
    String scoreName(u"changes-inside-repeats");
    MasterScore* score = ScoreRW::readScore(COURTESY_CHANGES_DATA + scoreName + u".mscx");

    EXPECT_TRUE(score);

    Measure* m0 = toMeasure(score->measure(0));
    EXPECT_TRUE(m0);
    Measure* m1 = toMeasure(score->measure(1));
    EXPECT_TRUE(m1);

    EXPECT_FALSE(m0->findSegmentR(SegmentType::Clef, m0->ticks()));
    EXPECT_FALSE(m0->findSegmentR(SegmentType::KeySig, m0->ticks()));
    EXPECT_FALSE(m0->findSegmentR(SegmentType::TimeSig, m0->ticks()));

    auto checkSegOrder = [](Measure* m, std::array<SegmentType, 4>& segTypes) -> void {
        size_t idx = 0;
        Segment* seg = m->first();
        while (idx < segTypes.size()) {
            LOGI() << seg->subTypeName();
            EXPECT_TRUE(seg);
            EXPECT_TRUE(seg->segmentType() == segTypes.at(idx));
            idx++;
            seg = seg->next();
        }
    };

    std::array<SegmentType, 4> segTypes = { SegmentType::Clef, SegmentType::KeySig, SegmentType::TimeSig, SegmentType::StartRepeatBarLine };
    checkSegOrder(m1, segTypes);

    score->startCmd(TranslatableString::untranslatable("Courtesy changes"));
    score->style().set(Sid::changesBetweenEndStartRepeat, false);
    score->endCmd();
    score->setLayoutAll();
    score->doLayout();

    segTypes = { SegmentType::StartRepeatBarLine, SegmentType::Clef, SegmentType::KeySig, SegmentType::TimeSig };
    checkSegOrder(m1, segTypes);

    score->startCmd(TranslatableString::untranslatable("Courtesy changes"));
    score->style().set(Sid::placeClefsBeforeRepeats, true);
    score->endCmd();
    score->setLayoutAll();
    score->doLayout();

    Segment* clefSeg = m0->findSegmentR(SegmentType::Clef, m0->ticks());
    EXPECT_TRUE(clefSeg);
    EXPECT_TRUE(clefSeg->next() && clefSeg->next()->segmentType() == SegmentType::EndBarLine);
    EXPECT_FALSE(m1->findSegmentR(SegmentType::Clef, Fraction(0, 1)));
}

namespace {
constexpr std::array<Sid, 6> evolutionCourtesyOptions {
    Sid::changesBeforeBarlineRepeats, Sid::changesBeforeBarlineOtherJumps,
    Sid::showCourtesiesRepeats, Sid::showCourtesiesOtherJumps,
    Sid::showCourtesiesAfterCancellingRepeats, Sid::showCourtesiesAfterCancellingOtherJumps
};
}

TEST_F(Engraving_CourtesyChangesTests, evolutionLegacyCourtesyStyleOverrides)
{
    // Missing tags retain legacy false defaults; explicit true and false
    // values survive the same compatibility pass. Modern files are unchanged.
    for (int version : { 302, 450, 470 }) {
        for (int explicitValue : { -1, 0, 1 }) {
            MStyle style;
            for (Sid sid : evolutionCourtesyOptions) {
                style.set(sid, true);
            }
            std::string versionString = std::to_string(version);
            versionString.insert(versionString.size() - 2, ".");
            std::string xml = "<museScore version=\"" + versionString + "\"><Style>";
            if (explicitValue >= 0) {
                for (Sid sid : evolutionCourtesyOptions) {
                    const char* tag = MStyle::valueName(sid);
                    xml += "<"; xml += tag; xml += ">";
                    xml += explicitValue ? "1" : "0";
                    xml += "</"; xml += tag; xml += ">";
                }
            }
            xml += "</Style></museScore>";
            muse::ByteArray data(xml.c_str(), xml.size());
            muse::io::Buffer buffer(&data);
            ASSERT_TRUE(buffer.open(muse::io::IODevice::ReadOnly));
            ASSERT_TRUE(style.read(&buffer));
            const bool expected = explicitValue >= 0 ? explicitValue != 0 : version >= 450;
            for (Sid sid : evolutionCourtesyOptions) {
                EXPECT_EQ(style.styleB(sid), expected) << MStyle::valueName(sid);
            }
        }
    }
}

TEST_F(Engraving_CourtesyChangesTests, evolutionSavedCourtesyScore)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(COURTESY_CHANGES_DATA + u"evolution-courtesy-inline-style.mscx"));
    ASSERT_TRUE(score);
    for (Sid sid : evolutionCourtesyOptions) {
        EXPECT_TRUE(score->style().styleB(sid)) << MStyle::valueName(sid);
    }
    score->setLayoutAll();
    score->doLayout();
    Measure* repeatEnd = toMeasure(score->measure(2));
    ASSERT_TRUE(repeatEnd);
    for (SegmentType kind : { SegmentType::ClefRepeatAnnounce, SegmentType::KeySigRepeatAnnounce,
                              SegmentType::TimeSigRepeatAnnounce }) {
        const Segment* segment = repeatEnd->findSegmentR(kind, repeatEnd->ticks());
        ASSERT_TRUE(segment);
        EXPECT_TRUE(segment->enabled());
        EXPECT_TRUE(segment->element(0));
    }
    Measure* continuation = toMeasure(score->measure(3));
    ASSERT_TRUE(continuation);
    for (SegmentType kind : { SegmentType::ClefStartRepeatAnnounce, SegmentType::KeySigStartRepeatAnnounce,
                              SegmentType::TimeSigStartRepeatAnnounce }) {
        const Segment* segment = continuation->findSegmentR(kind, Fraction(0, 1));
        ASSERT_TRUE(segment);
        EXPECT_TRUE(segment->enabled());
        EXPECT_TRUE(segment->element(0));
    }
}

TEST_F(Engraving_CourtesyChangesTests, evolutionExplicitRepeatClefHeaders)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(COURTESY_CHANGES_DATA + u"evolution-repeat-clef-headers.mscx"));
    ASSERT_TRUE(score);
    // A legacy clef without a flag still uses context. The next two start
    // before their key/meter but explicitly distinguish small and full size.
    const SegmentType types[] = { SegmentType::HeaderClef, SegmentType::Clef, SegmentType::HeaderClef };
    for (int pass = 0; pass < 3; ++pass) {
        score->setLayoutAll();
        score->doLayout();
        Measure* measure = score->firstMeasure();
        for (int index = 0; index < 3; ++index) {
            ASSERT_TRUE(measure);
            Segment* segment = measure->findSegmentR(types[index], Fraction(0, 1));
            ASSERT_TRUE(segment);
            EngravingItem* item = segment->element(0);
            ASSERT_TRUE(item && item->isClef());
            EXPECT_EQ(toClef(item)->isSmall(), index == 1);
            EXPECT_EQ(toClef(item)->isHeader(), index != 1);
            measure = measure->nextMeasure();
        }
    }
}
