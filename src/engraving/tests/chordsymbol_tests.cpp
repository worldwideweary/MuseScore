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

#include "draw/bufferedpaintprovider.h"
#include "draw/utils/drawdatacomp.h"

#include "engraving/compat/scoreaccess.h"
#include "engraving/dom/chordrest.h"
#include "engraving/dom/chordlist.h"
#include "engraving/dom/durationtype.h"
#include "engraving/dom/excerpt.h"
#include "engraving/dom/fret.h"
#include "engraving/dom/harmony.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/part.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/editing/editdata.h"
#include "engraving/editing/transpose.h"
#include "engraving/rendering/single/singlerenderer.h"
#include "engraving/rendering/iscorerenderer.h"

#include "utils/scorerw.h"
#include "utils/scorecomp.h"

using namespace mu::engraving;

static constexpr double POSITION_ERROR = 1e-6;

static const String CHORDSYMBOL_DATA_DIR("chordsymbol_data/");

class Engraving_ChordSymbolTests : public ::testing::Test
{
public:
    MasterScore* test_pre(const char16_t* p);
    void test_post(MasterScore* score, const char16_t* p);

    void selectAllChordSymbols(MasterScore* score);
    void realizeSelectionVoiced(MasterScore* score, Voicing voicing);
};

MasterScore* Engraving_ChordSymbolTests::test_pre(const char16_t* p)
{
    String p1 = CHORDSYMBOL_DATA_DIR + p + ".mscx";
    MasterScore* score = ScoreRW::readScore(p1);
    EXPECT_TRUE(score);
    score->doLayout();
    return score;
}

void Engraving_ChordSymbolTests::test_post(MasterScore* score, const char16_t* p)
{
    String p1 = p;
    p1 += u"-test.mscx";
    String p2 = CHORDSYMBOL_DATA_DIR + p + u"-ref.mscx";
    EXPECT_TRUE(ScoreComp::saveCompareScore(score, p1, p2));
    delete score;
}

//---------------------------------------------------------
//   select all chord symbols within the specified score
//---------------------------------------------------------
void Engraving_ChordSymbolTests::selectAllChordSymbols(MasterScore* score)
{
    //find a chord symbol
    Segment* seg = score->firstSegment(SegmentType::ChordRest);
    EngravingItem* e = 0;
    while (seg) {
        e = seg->findAnnotation(ElementType::HARMONY,
                                0, score->ntracks());
        if (e) {
            break;
        }
        seg = seg->next1();
    }
    score->selectSimilar(e, false);
}

//---------------------------------------------------------
//   realize the current selection of the score using
//   the specified voicing
//---------------------------------------------------------
void Engraving_ChordSymbolTests::realizeSelectionVoiced(MasterScore* score, Voicing voicing)
{
    for (EngravingItem* e : score->selection().elements()) {
        if (e->isHarmony()) {
            e->setProperty(Pid::HARMONY_VOICING, int(voicing));
        }
    }
    score->startCmd(TranslatableString::untranslatable("Realize selection voiced"));
    score->cmdRealizeChordSymbols();
    score->endCmd();
}

TEST_F(Engraving_ChordSymbolTests, testExtend)
{
    MasterScore* score = test_pre(u"extend");
    Measure* m = score->firstMeasure();
    Segment* s = m->first(SegmentType::ChordRest);
    ChordRest* cr = s->cr(0);
    score->changeCRlen(cr, DurationType::V_WHOLE);
    score->doLayout();
    test_post(score, u"extend");
}

TEST_F(Engraving_ChordSymbolTests, testClear)
{
    MasterScore* score = test_pre(u"clear");
    Measure* m = score->firstMeasure();
    score->select(m, SelectType::SINGLE, 0);
    score->cmdDeleteSelection();
    score->doLayout();
    test_post(score, u"clear");
}

TEST_F(Engraving_ChordSymbolTests, testAddLink)
{
    MasterScore* score = test_pre(u"add-link");
    Segment* seg = score->firstSegment(SegmentType::ChordRest);
    ChordRest* cr = seg->cr(0);
    Harmony* harmony = new Harmony(cr->segment());
    harmony->setHarmony(u"C7");
    harmony->setTrack(cr->track());
    harmony->setParent(cr->segment());
    score->undoAddElement(harmony);
    score->doLayout();
    test_post(score, u"add-link");
}

TEST_F(Engraving_ChordSymbolTests, harmonyEditKeepsDisplayPosition)
{
    MasterScore* score = test_pre(u"add-link");
    ASSERT_TRUE(score);

    Segment* seg = score->firstSegment(SegmentType::ChordRest);
    ASSERT_TRUE(seg);
    ChordRest* cr = seg->cr(0);
    ASSERT_TRUE(cr);

    Harmony* harmony = new Harmony(cr->segment());
    harmony->setHarmony(u"C");
    harmony->setTrack(cr->track());
    harmony->setParent(cr->segment());
    score->undoAddElement(harmony);
    score->doLayout();

    const double x = harmony->canvasPos().x();
    const double y = harmony->canvasPos().y();

    EditData ed;
    harmony->startEdit(ed);

    EXPECT_NEAR(harmony->canvasPos().x(), x, POSITION_ERROR);
    EXPECT_NEAR(harmony->canvasPos().y(), y, POSITION_ERROR);

    ed.s = String(u"7");
    harmony->cursor()->moveCursorToEnd();
    harmony->edit(ed);

    EXPECT_NEAR(harmony->canvasPos().x(), x, POSITION_ERROR);
    EXPECT_NEAR(harmony->canvasPos().y(), y, POSITION_ERROR);

    harmony->endEdit(ed);
    delete score;
}

TEST_F(Engraving_ChordSymbolTests, testAddPart)
{
    MasterScore* score = test_pre(u"add-part");
    Segment* seg = score->firstSegment(SegmentType::ChordRest);
    ChordRest* cr = seg->cr(0);
    Harmony* harmony = new Harmony(cr->segment());
    harmony->setHarmony(u"C7");
    harmony->setTrack(cr->track());
    harmony->setParent(cr->segment());
    score->undoAddElement(harmony);
    score->doLayout();
    test_post(score, u"add-part");
}

TEST_F(Engraving_ChordSymbolTests, testNoSystem)
{
    MasterScore* score = test_pre(u"no-system");

    //
    // create first part
    //
    std::vector<Part*> parts;
    parts.push_back(score->parts().at(0));
    Score* nscore = score->createScore();

    Excerpt* ex = new Excerpt(score);
    ex->setExcerptScore(nscore);
    nscore->setExcerpt(ex);
    score->excerpts().push_back(ex);
    ex->setName(parts.front()->longName());
    ex->setParts(parts);
    Excerpt::createExcerpt(ex);

//      nscore->setTitle(parts.front()->partName());
    nscore->style().set(Sid::createMultiMeasureRests, true);

    //
    // create second part
    //
    parts.clear();
    parts.push_back(score->parts().at(1));
    nscore = score->createScore();

    ex = new Excerpt(score);
    ex->setExcerptScore(nscore);
    nscore->setExcerpt(ex);
    score->excerpts().push_back(ex);
    ex->setName(parts.front()->longName());
    ex->setParts(parts);
    Excerpt::createExcerpt(ex);

//      nscore->setTitle(parts.front()->partName());
    nscore->style().set(Sid::createMultiMeasureRests, true);

    score->setExcerptsChanged(true);
    score->doLayout();
    test_post(score, u"no-system");
}

TEST_F(Engraving_ChordSymbolTests, testTranspose)
{
    MasterScore* score = test_pre(u"transpose");
    score->startCmd(TranslatableString::untranslatable("Engraving chord symbol tests"));
    score->cmdSelectAll();
    Transpose::transpose(score, TransposeMode::BY_INTERVAL, TransposeDirection::UP, Key::C, 4, false, true, true);
    score->endCmd();
    test_post(score, u"transpose");
}

TEST_F(Engraving_ChordSymbolTests, testTransposePart)
{
    MasterScore* score = test_pre(u"transpose-part");
    score->startCmd(TranslatableString::untranslatable("Engraving chord symbol tests"));
    score->cmdSelectAll();
    Transpose::transpose(score, TransposeMode::BY_INTERVAL, TransposeDirection::UP, Key::C, 4, false, true, true);
    score->endCmd(false, /*layoutAllParts = */ true);
    test_post(score, u"transpose-part");
}

//---------------------------------------------------------
//   check close voicing algorithm
//---------------------------------------------------------
TEST_F(Engraving_ChordSymbolTests, testRealizeClose)
{
    MasterScore* score = test_pre(u"realize");
    selectAllChordSymbols(score);
    realizeSelectionVoiced(score, Voicing::CLOSE);
    test_post(score, u"realize-close");
}

//---------------------------------------------------------
//   check Drop 2 voicing algorithm
//---------------------------------------------------------
TEST_F(Engraving_ChordSymbolTests, testRealizeDrop2)
{
    MasterScore* score = test_pre(u"realize");
    selectAllChordSymbols(score);
    realizeSelectionVoiced(score, Voicing::DROP_2);
    test_post(score, u"realize-drop2");
}

//---------------------------------------------------------
//   check 3 note voicing algorithm
//---------------------------------------------------------
TEST_F(Engraving_ChordSymbolTests, testRealize3Note)
{
    MasterScore* score = test_pre(u"realize");
    selectAllChordSymbols(score);
    realizeSelectionVoiced(score, Voicing::THREE_NOTE);
    test_post(score, u"realize-3note");
}

//---------------------------------------------------------
//   check 4 note voicing algorithm
//---------------------------------------------------------
TEST_F(Engraving_ChordSymbolTests, testRealize4Note)
{
    MasterScore* score = test_pre(u"realize");
    selectAllChordSymbols(score);
    realizeSelectionVoiced(score, Voicing::FOUR_NOTE);
    test_post(score, u"realize-4note");
}

//---------------------------------------------------------
//   check 6 note voicing algorithm
//---------------------------------------------------------
TEST_F(Engraving_ChordSymbolTests, testRealize6Note)
{
    MasterScore* score = test_pre(u"realize");
    selectAllChordSymbols(score);
    realizeSelectionVoiced(score, Voicing::SIX_NOTE);
    test_post(score, u"realize-6note");
}

//---------------------------------------------------------
//   Check if the note pitches and tpcs are correct after realizing
//   chord symbols on transposed instruments.
//---------------------------------------------------------
TEST_F(Engraving_ChordSymbolTests, testRealizeConcertPitch)
{
    MasterScore* score = test_pre(u"realize-concert-pitch");
    //concert pitch off
    score->startCmd(TranslatableString::untranslatable("Engraving chord symbol tests"));
    score->cmdConcertPitchChanged(false);
    score->endCmd();

    //realize all chord symbols
    selectAllChordSymbols(score);
    score->startCmd(TranslatableString::untranslatable("Engraving chord symbol tests"));
    score->cmdRealizeChordSymbols();
    score->endCmd();
    test_post(score, u"realize-concert-pitch");
}

//---------------------------------------------------------
//   Check if the note pitches and tpcs are correct after
//   transposing the score
//---------------------------------------------------------
TEST_F(Engraving_ChordSymbolTests, testRealizeTransposed)
{
    MasterScore* score = test_pre(u"transpose");
    //transpose
    score->cmdSelectAll();
    Transpose::transpose(score, TransposeMode::BY_INTERVAL, TransposeDirection::UP, Key::C, 4, false, true, true);

    //realize all chord symbols
    selectAllChordSymbols(score);
    score->startCmd(TranslatableString::untranslatable("Engraving chord symbol tests"));
    score->cmdRealizeChordSymbols();
    score->endCmd();
    test_post(score, u"realize-transpose");
}

//---------------------------------------------------------
//   Check for correctness when using the override
//   feature for realizing chord symbols
//---------------------------------------------------------
TEST_F(Engraving_ChordSymbolTests, testRealizeOverrides)
{
    MasterScore* score = test_pre(u"realize-override");
    //realize all chord symbols
    selectAllChordSymbols(score);
    score->startCmd(TranslatableString::untranslatable("Engraving chord symbol tests"));
    score->cmdRealizeChordSymbols(true, Voicing::ROOT_ONLY, HDuration::SEGMENT_DURATION);
    score->endCmd();
    test_post(score, u"realize-override");
}

//---------------------------------------------------------
//   Check for correctness when realizing chord symbols on triplets
//---------------------------------------------------------
TEST_F(Engraving_ChordSymbolTests, testRealizeTriplet)
{
    MasterScore* score = test_pre(u"realize-triplet");
    //realize all chord symbols
    selectAllChordSymbols(score);
    score->startCmd(TranslatableString::untranslatable("Engraving chord symbol tests"));
    score->cmdRealizeChordSymbols();
    score->endCmd();
    test_post(score, u"realize-triplet");
}

//---------------------------------------------------------
//   Check for correctness when realizing chord symbols
//   with different durations
//---------------------------------------------------------
TEST_F(Engraving_ChordSymbolTests, testRealizeDuration)
{
    MasterScore* score = test_pre(u"realize-duration");
    //realize all chord symbols
    selectAllChordSymbols(score);
    score->startCmd(TranslatableString::untranslatable("Engraving chord symbol tests"));
    score->cmdRealizeChordSymbols();
    score->endCmd();
    test_post(score, u"realize-duration");
}

//---------------------------------------------------------
//   Check for correctness when realizing chord symbols
//   with jazz mode
//---------------------------------------------------------
TEST_F(Engraving_ChordSymbolTests, testRealizeJazz)
{
    MasterScore* score = test_pre(u"realize-jazz");
    //realize all chord symbols
    selectAllChordSymbols(score);
    score->startCmd(TranslatableString::untranslatable("Engraving chord symbol tests"));
    score->cmdRealizeChordSymbols();
    score->endCmd();
    test_post(score, u"realize-jazz");
}

TEST_F(Engraving_ChordSymbolTests, testNashvilleNumbers) {
    MasterScore* score = test_pre(u"nashville-numbers");
    selectAllChordSymbols(score);

    static const std::array<std::pair<int, String>, 23> tpcAndExtension { {
        { 14, u"" },
        { 16, u"" },
        { 18, u"" },
        { 13, u"" },
        { 16, u"" },
        { 18, u"" },
        { 20, u"" },
        { 15, u"" },
        { 10, u"" },
        { 12, u"" },
        { 14, u"" },
        { 9, u"" },
        { 5, u"" },
        { 7, u"" },
        { 9, u"" },
        { 4, u"" },
        { 12, u"m" },
        { 14, u"o" },
        { 16, u"o7#11" },
        { 14, u"6" },
        { 14, u"69" },
        { 18, u"sus" },
        { 13, u"6" }
    } };

    size_t idx = 0;

    std::vector<EngravingItem*> els = score->selection().elements(ElementType::HARMONY);

    ASSERT_EQ(els.size(), tpcAndExtension.size());

    for (EngravingItem* e : els) {
        Harmony* h = toHarmony(e);
        EXPECT_FALSE(h->chords().empty());
        HarmonyInfo* info = h->chords().front();

        int tpc = tpcAndExtension.at(idx).first;
        String ext = tpcAndExtension.at(idx).second;

        EXPECT_EQ(tpc, info->rootTpc());
        EXPECT_EQ(ext, info->textName());

        idx++;
    }
}

TEST_F(Engraving_ChordSymbolTests, testParserSuffix)
{
    MasterScore* score = compat::ScoreAccess::createMasterScore(nullptr);
    StringList symbols = { u"Cno3rd", u"Domit1st", u"Cadd9th", u"Ebsus2nd" };

    // Expect these symbols to parse successfully
    ParsedChord* pc = new ParsedChord();
    for (const String& chord : symbols) {
        EXPECT_TRUE(pc->parse(chord, score->chordList()));
    }

    delete pc;
    delete score;
}

TEST_F(Engraving_ChordSymbolTests, testAddHarmonyToFretDiagram)
{
    MasterScore* score = ScoreRW::readScore(CHORDSYMBOL_DATA_DIR + u"add-to-fret" + ".mscz");
    EXPECT_TRUE(score);
    score->doLayout();

    Measure* firstMeasure = score->firstMeasure();
    Segment* firstSeg = firstMeasure->findFirstR(SegmentType::ChordRest, Fraction(0, 1));
    FretDiagram* fretDiag = toFretDiagram(firstSeg->findAnnotation(ElementType::FRET_DIAGRAM, 0, 0));
    EXPECT_TRUE(fretDiag);

    score->addText(TextStyleType::HARMONY_A, fretDiag);

    EXPECT_TRUE(fretDiag->harmony());

    delete score;
}

TEST_F(Engraving_ChordSymbolTests, testEvolutionPresentationImport)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(CHORDSYMBOL_DATA_DIR + u"evolution-presentation.mscx"));
    ASSERT_TRUE(score);
    const MStyle& style = score->style();
    EXPECT_EQ(style.styleV(Sid::chordStyle).value<ChordStylePreset>(), ChordStylePreset::STANDARD);
    EXPECT_DOUBLE_EQ(style.styleD(Sid::chordExtensionAdjust), -0.3);
    EXPECT_DOUBLE_EQ(style.styleD(Sid::chordModifierAdjust), 0.25);
    EXPECT_DOUBLE_EQ(style.styleD(Sid::chordBassNoteGap), 0.16);
    EXPECT_DOUBLE_EQ(style.styleS(Sid::harmonyHarmonyDistance).val(), 0.7);
    // Unrelated 4.7 compatibility migrations must still run.
    EXPECT_FALSE(style.styleB(Sid::maskBarlinesForText));
    EXPECT_TRUE(style.styleB(Sid::verticallyStackModifiers));
    EXPECT_FALSE(style.styleB(Sid::verticallyAlignChordSymbols));
    EXPECT_EQ(style.styleV(Sid::chordSymPosition).value<AlignH>(), AlignH::LEFT);
    EXPECT_EQ(style.styleV(Sid::chordAlignmentToFretboard).value<AlignH>(), AlignH::RIGHT);
    EXPECT_DOUBLE_EQ(style.styleS(Sid::polychordDividerSpacing).val(), 0.6);
    Segment* segment = score->firstSegment(SegmentType::ChordRest);
    ASSERT_TRUE(segment);
    EngravingItem* element = segment->findAnnotation(ElementType::HARMONY, 0, score->ntracks());
    ASSERT_TRUE(element);
    Harmony* harmony = toHarmony(element);
    ASSERT_EQ(harmony->chords().size(), size_t(2));
    EXPECT_EQ(harmony->chords()[0]->rootTpc(), 9);
    EXPECT_EQ(harmony->chords()[0]->bassTpc(), 13);
    EXPECT_EQ(harmony->chords()[1]->rootTpc(), 15);
    EXPECT_DOUBLE_EQ(harmony->getProperty(Pid::HARMONY_BASS_SCALE).toDouble(), 0.65);
    ASSERT_TRUE(ScoreRW::saveScore(score.get(), u"evolution-slash-gap-roundtrip-test.mscx"));
    std::unique_ptr<MasterScore> reloaded(ScoreRW::readScore(u"evolution-slash-gap-roundtrip-test.mscx", true));
    ASSERT_TRUE(reloaded);
    EXPECT_DOUBLE_EQ(reloaded->style().styleD(Sid::chordBassNoteGap), 0.16);
}

TEST_F(Engraving_ChordSymbolTests, testEvolutionLegacyStillMigrates)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(CHORDSYMBOL_DATA_DIR + u"evolution-legacy.mscx"));
    ASSERT_TRUE(score);
    EXPECT_DOUBLE_EQ(score->style().styleD(Sid::chordBassNoteGap), -1.0);
    EXPECT_EQ(score->style().styleV(Sid::chordStyle).value<ChordStylePreset>(), ChordStylePreset::LEGACY);
    EXPECT_EQ(score->style().styleSt(Sid::chordDescriptionFile), String(u"chords_legacy.xml"));
}

TEST_F(Engraving_ChordSymbolTests, testEvolutionPolychordCentre)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(CHORDSYMBOL_DATA_DIR + u"evolution-polychord-centre.mscx"));
    ASSERT_TRUE(score);
    score->doLayout();
    Segment* segment = score->firstSegment(SegmentType::ChordRest);
    ASSERT_TRUE(segment);
    EngravingItem* element = segment->findAnnotation(ElementType::HARMONY, 0, score->ntracks());
    ASSERT_TRUE(element);
    Harmony* harmony = toHarmony(element);
    ASSERT_TRUE(harmony->isPolychord());
    EXPECT_EQ(harmony->position(), AlignH::HCENTER);
    // An inherited left text style must not override chord positioning.
    EXPECT_TRUE(harmony->align() == AlignH::LEFT);
    const TextSegment* c = nullptr;
    const TextSegment* g = nullptr;
    for (const HarmonyRenderItem* item : harmony->ldata()->renderItemList()) {
        const TextSegment* text = dynamic_cast<const TextSegment*>(item);
        if (!text) {
            continue;
        }
        if (text->text() == u"C") {
            c = text;
        } else if (text->text() == u"G") {
            g = text;
        }
    }
    ASSERT_TRUE(c);
    ASSERT_TRUE(g);
    EXPECT_NEAR(c->x() + c->width() / 2, g->x() + g->width() / 2, 1e-6);
}

#ifndef NO_QT_SUPPORT
TEST_F(Engraving_ChordSymbolTests, testEvolutionSlashGapLayout)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(CHORDSYMBOL_DATA_DIR + u"evolution-slash-gap.mscx"));
    ASSERT_TRUE(score);
    EXPECT_DOUBLE_EQ(score->style().styleD(Sid::chordBassNoteGap), 0.16);
    Segment* segment = score->firstSegment(SegmentType::ChordRest);
    ASSERT_TRUE(segment);
    EngravingItem* element = segment->findAnnotation(ElementType::HARMONY, 0, score->ntracks());
    ASSERT_TRUE(element);
    Harmony* harmony = toHarmony(element);
    auto bassPosition = [&]() -> PointF {
        const TextSegment* root = nullptr;
        const TextSegment* bass = nullptr;
        for (const HarmonyRenderItem* renderItem : harmony->ldata()->renderItemList()) {
            const TextSegment* text = dynamic_cast<const TextSegment*>(renderItem);
            if (!text) {
                continue;
            }
            if (text->text() == u"C") {
                root = text;
            } else if (text->text() == u"A") {
                bass = text;
            }
        }
        EXPECT_TRUE(root && bass);
        return root && bass ? bass->pos() - root->pos() : PointF();
    };
    score->style().set(Sid::chordBassNoteGap, 0.08);
    score->doLayout();
    const PointF close = bassPosition();
    score->style().set(Sid::chordBassNoteGap, 0.24);
    score->doLayout();
    const PointF wide = bassPosition();
    EXPECT_GT(wide.x(), close.x());
    EXPECT_GT(wide.y(), close.y());
    score->doLayout();
    const PointF repeated = bassPosition();
    EXPECT_NEAR(repeated.x(), wide.x(), 1e-6);
    EXPECT_NEAR(repeated.y(), wide.y(), 1e-6);
    score->style().set(Sid::chordBassNoteStagger, false);
    score->doLayout();
    const PointF inlineWide = bassPosition();
    score->style().set(Sid::chordBassNoteGap, 0.08);
    score->doLayout();
    const PointF inlineClose = bassPosition();
    EXPECT_NEAR(inlineClose.x(), inlineWide.x(), 1e-6);
    EXPECT_NEAR(inlineClose.y(), inlineWide.y(), 1e-6);
}
#endif

TEST_F(Engraving_ChordSymbolTests, testEvolutionJazzKeepsExplicitParenthesisChoice)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(CHORDSYMBOL_DATA_DIR + u"evolution-jazz-presentation.mscx"));
    ASSERT_TRUE(score);
    const MStyle& style = score->style();
    EXPECT_EQ(style.styleV(Sid::chordStyle).value<ChordStylePreset>(), ChordStylePreset::JAZZ);
    // 4.7's pre-470 Jazz migration enables these by default. Respect the
    // explicitly saved Evolution choice after that migration has run.
    EXPECT_FALSE(style.styleB(Sid::harmonyParenUseSmuflSym));
    EXPECT_FALSE(style.styleB(Sid::verticallyAlignChordSymbols));
    EXPECT_DOUBLE_EQ(style.styleD(Sid::chordBassNoteGap), 0.16);
}

TEST_F(Engraving_ChordSymbolTests, testStackedModifierSeparators)
{
    // A plain chord list keeps render text literal, independent of glyph mappings.
    ChordList chordList;
    auto renderedText = [](const std::vector<RenderActionPtr>& actions) {
        String text;
        for (const RenderActionPtr& action : actions) {
            if (action->actionType() == RenderAction::RenderActionType::SET) {
                const auto set = std::static_pointer_cast<RenderActionSet>(action);
                if (set->renderText()) {
                    text += set->text();
                }
            }
        }
        return text;
    };
    const std::vector<std::pair<String, String> > cases {
        { u"7(b9,#11)", u"7(b9#11)" },
        { u"13(b9,#11,b13)", u"13(b9#11b13)" },
        { u"6/9(b9,#11)", u"6/9(b9#11)" },
        { u"6,9(b9,#11)", u"6,9(b9#11)" }
    };
    for (const auto& [name, stacked] : cases) {
        ParsedChord parsed;
        ASSERT_TRUE(parsed.parse(name, &chordList));
        const String originalName = parsed.name();
        EXPECT_EQ(renderedText(parsed.renderList(&chordList, false)), name);
        EXPECT_EQ(renderedText(parsed.renderList(&chordList, true)), stacked);
        EXPECT_EQ(parsed.name(), originalName);
    }
}

TEST_F(Engraving_ChordSymbolTests, testEvolutionDragCopyPreview)
{
    rendering::single::SingleRenderer renderer;
    for (const String& fixture : { String(u"evolution-slash-gap.mscx"),
                                  String(u"evolution-polychord-centre.mscx"),
                                  String(u"evolution-presentation.mscx") }) {
        std::unique_ptr<MasterScore> score(ScoreRW::readScore(CHORDSYMBOL_DATA_DIR + fixture));
        ASSERT_TRUE(score);
        score->doLayout();
        Segment* segment = score->firstSegment(SegmentType::ChordRest);
        ASSERT_TRUE(segment);
        EngravingItem* element = segment->findAnnotation(ElementType::HARMONY, 0, score->ntracks());
        ASSERT_TRUE(element);
        Harmony* source = toHarmony(element);

        // Like startDropSingle(): serialize/read a fresh item under the score's
        // dummy parent, then use single-item layout before inserting the copy.
        std::unique_ptr<EngravingItem> copy(ScoreRW::writeReadElement(source));
        ASSERT_TRUE(copy && copy->isHarmony());
        Harmony* preview = toHarmony(copy.get());
        ASSERT_EQ(preview->chords().size(), source->chords().size());
        for (size_t i = 0; i < source->chords().size(); ++i) {
            EXPECT_EQ(preview->chords()[i]->rootTpc(), source->chords()[i]->rootTpc());
            EXPECT_EQ(preview->chords()[i]->bassTpc(), source->chords()[i]->bassTpc());
        }
        EXPECT_DOUBLE_EQ(preview->getProperty(Pid::HARMONY_BASS_SCALE).toDouble(),
                         source->getProperty(Pid::HARMONY_BASS_SCALE).toDouble());
        renderer.layoutItem(preview);
        EXPECT_TRUE(preview->ldata()->bbox().isValid());
        EXPECT_FALSE(preview->ldata()->renderItemList().empty());
        EXPECT_EQ(preview->ldata()->polychordDividerLines().size(), source->ldata()->polychordDividerLines().size());

        ASSERT_TRUE(preview->renderer());
        for (bool smufl : { false, true }) {
            score->style().set(Sid::harmonyParenUseSmuflSym, smufl);
            renderer.layoutItem(preview);
            auto drawData = [&](bool single) {
                auto provider = std::make_shared<muse::draw::BufferedPaintProvider>();
                muse::draw::Painter painter(provider, "chord-preview");
                const rendering::PaintOptions options;
                if (single) {
                    renderer.drawItem(preview, &painter, options);
                } else {
                    preview->renderer()->drawItem(preview, &painter, options);
                }
                painter.endDraw();
                return provider->drawData();
            };
            const auto singleData = drawData(true);
            const auto scoreData = drawData(false);
            ASSERT_FALSE(singleData->empty());
            ASSERT_FALSE(scoreData->empty());
            EXPECT_TRUE(muse::draw::DrawDataComp::compare(singleData, scoreData).empty());
        }
    }
}

TEST_F(Engraving_ChordSymbolTests, testEvolutionCopiesKeepStackingOverride)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(CHORDSYMBOL_DATA_DIR + u"evolution-presentation.mscx"));
    ASSERT_TRUE(score);
    Segment* segment = score->firstSegment(SegmentType::ChordRest);
    ASSERT_TRUE(segment);
    EngravingItem* element = segment->findAnnotation(ElementType::HARMONY, 0, score->ntracks());
    ASSERT_TRUE(element);
    Harmony* source = toHarmony(element);
    source->setProperty(Pid::HARMONY_DO_NOT_STACK_MODIFIERS, true);
    ASSERT_TRUE(source->doNotStackModifiers());
    ASSERT_FALSE(source->chords().empty());
    const int originalRoot = source->chords().front()->rootTpc();

    std::unique_ptr<Harmony> clone(source->clone());
    ASSERT_TRUE(clone);
    EXPECT_TRUE(clone->doNotStackModifiers());
    EXPECT_DOUBLE_EQ(clone->getProperty(Pid::HARMONY_BASS_SCALE).toDouble(),
                     source->getProperty(Pid::HARMONY_BASS_SCALE).toDouble());
    ASSERT_EQ(clone->chords().size(), source->chords().size());
    for (size_t i = 0; i < source->chords().size(); ++i) {
        EXPECT_NE(clone->chords()[i], source->chords()[i]);
        EXPECT_EQ(clone->chords()[i]->rootTpc(), source->chords()[i]->rootTpc());
        EXPECT_EQ(clone->chords()[i]->bassTpc(), source->chords()[i]->bassTpc());
    }
    clone->setProperty(Pid::HARMONY_DO_NOT_STACK_MODIFIERS, false);
    clone->chords().front()->setRootTpc(originalRoot == 14 ? 15 : 14);
    EXPECT_TRUE(source->doNotStackModifiers());
    EXPECT_EQ(source->chords().front()->rootTpc(), originalRoot);

    std::unique_ptr<EngravingItem> xmlCopy(ScoreRW::writeReadElement(source));
    ASSERT_TRUE(xmlCopy && xmlCopy->isHarmony());
    EXPECT_TRUE(toHarmony(xmlCopy.get())->doNotStackModifiers());
}
