//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2015 Werner Schweer & others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <QtTest/QtTest>
#include <QFileInfo>
#include <cmath>
#include "mtest/testutils.h"
#include "libmscore/beam.h"
#include "libmscore/chord.h"
#include "libmscore/note.h"
#include "libmscore/notedot.h"
#include "libmscore/tie.h"
#include "libmscore/slur.h"
#include "libmscore/stem.h"
#include "libmscore/laissezvib.h"
#include "libmscore/segment.h"
#include "libmscore/excerpt.h"
#include "libmscore/glissando.h"
#include "libmscore/layoutbreak.h"
#include "libmscore/lyrics.h"
#include "libmscore/measure.h"
#include "libmscore/part.h"
#include "libmscore/staff.h"
#include "libmscore/stafftype.h"
#include "libmscore/stringdata.h"
#include "libmscore/instrument.h"
#include "libmscore/score.h"
#include "libmscore/system.h"
#include "libmscore/undo.h"
#include "libmscore/line.h"
#include "libmscore/textline.h"

#define DIR QString("libmscore/spanners/")

using namespace Ms;

//---------------------------------------------------------
//   TestSpanners
//---------------------------------------------------------

class TestSpanners : public QObject, public MTest
      {
      Q_OBJECT

   private slots:
      void initTestCase();
      void tieLayoutStylesRoundTrip();
      void partialSlurHangingAngles();
      void nativeOrdinarySlurFit();
      void nativeSlantedSlurFit();
      void nativeAfterGraceSlurFit();
      void nativePreGraceSlurFit();
      void nativeNestedBeamedSlurFit();
      void partialTieAttachments();
      void laissezVibChordGeometry();
      void laissezVibRangeSelection();
      void laissezVibLinkedTab();
      void spanners01();            // adding glissandos in several contexts
      void spanners02();            // loading back existing cross-staff glissando from lower to higher staff
      void spanners03();            // adding glissandos from/to grace notes
      void spanners04();            // linking a staff to a staff containing a glissando
      void spanners05();            // creating part from an existing staff containing a glissando
      void spanners06();            // add a glissando to a staff with a linked staff
      void spanners07();            // add a glissando to a staff with an excerpt attached
//      void spanners08();            // delete a lyrics with separator and undo
      void spanners09();            // remove a measure containing the end point of a LyricsLine and undo
      void spanners10();            // remove a measure containing the start point of a LyricsLine and undo
      void spanners11();            // remove a measure entirely containing a LyricsLine and undo
      void spanners12();            // remove a measure containing the middle portion of a LyricsLine and undo
//      void spanners13();            // drop a line break at the middle of a LyricsLine and check LyricsLineSegments
      void spanners14();            // creating part from an existing grand staff containing a cross staff glissando
      void spanners15();            // change the color & min distance of a line and save it
      void arrowLinesRoundTrip();
      void arrowLinesStudioImport();
      void arrowLinesGeometry();
      void spanners16();            // read lines with manual adjustments on a small staff and save
      };

//---------------------------------------------------------
//   initTestCase
//---------------------------------------------------------

void TestSpanners::initTestCase()
      {
      initMTest();
      }

//---------------------------------------------------------
///  spanners01
///   Adds glissandi in several contexts.
//---------------------------------------------------------

void TestSpanners::spanners01()
      {
      EditData    dropData(0);
      Glissando*  gliss;

      MasterScore* score = readScore(DIR + "glissando01.mscx");
      QVERIFY(score);

      // SIMPLE CASE: GLISSANDO FROM A NOTE TO THE FOLLOWING
      // go to top note of first chord
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      Segment*    seg   = msr->findSegment(SegmentType::ChordRest, Fraction(0,1));
      QVERIFY(seg);
      Ms::Chord*      chord = static_cast<Ms::Chord*>(seg->element(0));
      QVERIFY(chord && chord->type() == ElementType::CHORD);
      Note*       note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score); // create a new element each time, as drop() will eventually delete it
      dropData.pos      = note->pagePos();
      dropData.dropElement  = gliss;
      note->drop(dropData);

      // GLISSANDO FROM TOP STAFF TO BOTTOM STAFF
      // go to top note of first chord of next measure
      msr   = msr->nextMeasure();
      QVERIFY(msr);
      seg   = msr->first();
      QVERIFY(seg);
      chord = static_cast<Ms::Chord*>(seg->element(0));   // voice 0 of staff 0
      QVERIFY(chord && chord->type() == ElementType::CHORD);
      note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.dropElement  = gliss;
      note->drop(dropData);

      // GLISSANDO FROM BOTTOM STAFF TO TOP STAFF
      // go to bottom note of first chord of next measure
      msr   = msr->nextMeasure();
      QVERIFY(msr);
      seg   = msr->first();
      QVERIFY(seg);
      chord = static_cast<Ms::Chord*>(seg->element(4));   // voice 0 of staff 1
      QVERIFY(chord && chord->type() == ElementType::CHORD);
      note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.dropElement  = gliss;
      note->drop(dropData);

      // GLISSANDO OVER INTERVENING NOTES IN ANOTHER VOICE
      // go to top note of first chord of next measure
      msr   = msr->nextMeasure();
      QVERIFY(msr);
      seg   = msr->first();
      QVERIFY(seg);
      chord = static_cast<Ms::Chord*>(seg->element(0));   // voice 0 of staff 0
      QVERIFY(chord && chord->type() == ElementType::CHORD);
      note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.dropElement  = gliss;
      note->drop(dropData);

      // GLISSANDO OVER INTERVENING NOTES IN ANOTHER STAFF
      // go to top note of first chord of next measure
      msr   = msr->nextMeasure()->nextMeasure();
      QVERIFY(msr);
      seg   = msr->first();
      QVERIFY(seg);
      chord = static_cast<Ms::Chord*>(seg->element(0));   // voice 0 of staff 0
      QVERIFY(chord && chord->type() == ElementType::CHORD);
      note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.dropElement  = gliss;
      note->drop(dropData);

      QVERIFY(saveCompareScore(score, "glissando01.mscx", DIR + "glissando01-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners02
///   Check loading of score with a glissando from a lower to a higher staff:
//    A score with:
//          grand staff,
//          glissando from a bass staff note to a treble staff note
//    is loaded and laid out and saved: should be round-trip safe.
//---------------------------------------------------------

void TestSpanners::spanners02()
      {
      MasterScore* score = readScore(DIR + "glissando-crossstaff01.mscx");
      QVERIFY(score);

      QVERIFY(saveCompareScore(score, "glissando-crossstaff01.mscx", DIR + "glissando-crossstaff01-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners03
///   Loads a score with before- and after-grace notes and adds several glissandi from/to them.
//---------------------------------------------------------

void TestSpanners::spanners03()
      {
      EditData    dropData(0);
      Glissando*  gliss;

      MasterScore* score = readScore(DIR + "glissando-graces01.mscx");
      QVERIFY(score);

      // GLISSANDO FROM MAIN NOTE TO AFTER-GRACE
      // go to top note of first chord
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      Segment*    seg   = msr->findSegment(SegmentType::ChordRest, Fraction(0,1));
      QVERIFY(seg);
      Ms::Chord*      chord = static_cast<Ms::Chord*>(seg->element(0));
      QVERIFY(chord && chord->type() == ElementType::CHORD);
      Note*       note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score); // create a new element each time, as drop() will eventually delete it
      dropData.pos      = note->pagePos();
      dropData.dropElement  = gliss;
      note->drop(dropData);

      // GLISSANDO FROM AFTER-GRACE TO BEFORE-GRACE OF NEXT CHORD
      // go to last after-grace of chord and drop a glissando on it
      Ms::Chord*      grace = chord->graceNotesAfter().last();
      QVERIFY(grace && grace->type() == ElementType::CHORD);
      note              = grace->upNote();
      QVERIFY(note);
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.dropElement  = gliss;
      note->drop(dropData);

      // GLISSANDO FROM MAIN NOTE TO BEFORE-GRACE OF NEXT CHORD
      // go to next chord
      seg               = seg->nextCR(0);
      QVERIFY(seg);
      chord             = static_cast<Ms::Chord*>(seg->element(0));
      QVERIFY(chord && chord->type() == ElementType::CHORD);
      note              = chord->upNote();
      QVERIFY(note);
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.dropElement  = gliss;
      note->drop(dropData);

      // GLISSANDO FROM BEFORE-GRACE TO MAIN NOTE
      // go to next chord
      seg               = seg->nextCR(0);
      QVERIFY(seg);
      chord             = static_cast<Ms::Chord*>(seg->element(0));
      QVERIFY(chord && chord->type() == ElementType::CHORD);
      // go to its last before-grace note
      grace             = chord->graceNotesBefore().last();
      QVERIFY(grace && grace->type() == ElementType::CHORD);
      note              = grace->upNote();
      QVERIFY(note);
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.dropElement  = gliss;
      note->drop(dropData);

      QVERIFY(saveCompareScore(score, "glissando-graces01.mscx", DIR + "glissando-graces01-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners04
///   Linking a staff to an existing staff containing a glissando
//---------------------------------------------------------

void TestSpanners::spanners04()
      {
      MasterScore* score = readScore(DIR + "glissando-cloning01.mscx");
      QVERIFY(score);

      // add a linked staff to the existing staff
      // (copied and adapted from void MuseScore::editInstrList() in mscore/instrdialog.cpp)
      Staff* oldStaff   = score->staff(0);
      Staff* newStaff   = new Staff(score);
      newStaff->setPart(oldStaff->part());
      newStaff->initFromStaffType(oldStaff->staffType(Fraction(0,1)));
      newStaff->setDefaultClefType(ClefTypeList(ClefType::G));

      KeySigEvent ke;
      ke.setKey(Key::C);
      newStaff->setKey(Fraction(0,1), ke);

      score->undoInsertStaff(newStaff, 1, false);
      Excerpt::cloneStaff(oldStaff, newStaff);

      QVERIFY(saveCompareScore(score, "glissando-cloning01.mscx", DIR + "glissando-cloning01-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners05
///   Creating part from an existing staff containing a glissando
//---------------------------------------------------------

void TestSpanners::spanners05()
      {
      MasterScore* score = readScore(DIR + "glissando-cloning02.mscx");
      QVERIFY(score);

      // create parts
      // (copied and adapted from void TestParts::createParts() in mtest/libmscore/parts/tst_parts.cpp)
      QList<Part*> parts;
      parts.append(score->parts().at(0));
      Score* nscore = new Score(score);

      Excerpt* ex = new Excerpt(score);
      ex->setPartScore(nscore);
      ex->setTitle(parts.front()->longName());
      ex->setParts(parts);
      Excerpt::createExcerpt(ex);
      QVERIFY(nscore);

//      nscore->setName(parts.front()->partName());

//      QMultiMap<int, int> tracks;
      score->Score::undo(new AddExcerpt(ex));

      QVERIFY(saveCompareScore(score, "glissando-cloning02.mscx", DIR + "glissando-cloning02-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners06
///   Drop a glissando on a staff with a linked staff
//---------------------------------------------------------

void TestSpanners::spanners06()
      {
      EditData    dropData(0);
      Glissando*  gliss;

      MasterScore* score = readScore(DIR + "glissando-cloning03.mscx");
      QVERIFY(score);

      // DROP A GLISSANDO ON FIRST NOTE
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      Segment*    seg   = msr->findSegment(SegmentType::ChordRest, Fraction(0,1));
      QVERIFY(seg);
      Ms::Chord*      chord = static_cast<Ms::Chord*>(seg->element(0));
      QVERIFY(chord && chord->type() == ElementType::CHORD);
      Note*       note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.dropElement  = gliss;
      note->drop(dropData);

      QVERIFY(saveCompareScore(score, "glissando-cloning03.mscx", DIR + "glissando-cloning03-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners07
///   Drop a glissando on a staff with an excerpt
//---------------------------------------------------------

void TestSpanners::spanners07()
      {
      EditData    dropData(0);
      Glissando*  gliss;

      MasterScore* score = readScore(DIR + "glissando-cloning04.mscx");
      QVERIFY(score);

      // DROP A GLISSANDO ON FIRST NOTE
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      Segment*    seg   = msr->findSegment(SegmentType::ChordRest, Fraction(0,1));
      QVERIFY(seg);
      Ms::Chord*      chord = static_cast<Ms::Chord*>(seg->element(0));
      QVERIFY(chord && chord->type() == ElementType::CHORD);
      Note*       note  = chord->upNote();
      QVERIFY(note);
      // drop a glissando on note
      gliss             = new Glissando(score);
      dropData.pos      = note->pagePos();
      dropData.dropElement  = gliss;
      note->drop(dropData);

      QVERIFY(saveCompareScore(score, "glissando-cloning04.mscx", DIR + "glissando-cloning04-ref.mscx"));
      delete score;
      }
#if 0
//---------------------------------------------------------
///  spanners08
///   Delete a lyrics with separator and undo
//---------------------------------------------------------

void TestSpanners::spanners08()
      {
      MasterScore* score = readScore(DIR + "lyricsline01.mscx");
      QVERIFY(score);

      // verify initial LyricsLine setup
      System* sys = score->systems().at(0);
      QVERIFY(sys->spannerSegments().size() == 1);
      QVERIFY(score->unmanagedSpanners().size() == 1);

      // DELETE LYRICS
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      Segment*    seg   = msr->findSegment(SegmentType::ChordRest, 0);
      QVERIFY(seg);
      Ms::Chord*      chord = static_cast<Ms::Chord*>(seg->element(0));
      QVERIFY(chord && chord->type() == ElementType::CHORD);
      QVERIFY(chord->lyrics().size() > 0);
      Lyrics*     lyr   = chord->lyrics(0, Element::Placement::BELOW);
      score->startCmd();
      score->undoRemoveElement(lyr);
      score->endCmd();

      // verify setup after deletion
      QVERIFY(sys->spannerSegments().size() == 0);
      QVERIFY(score->unmanagedSpanners().size() == 0);

      // save and verify score after deletion
      QVERIFY(saveCompareScore(score, "lyricsline01.mscx", DIR + "lyricsline01-ref.mscx"));

      // UNDO
      score->undoStack()->undo();
      score->doLayout();

      // verify setup after undo
      QVERIFY(sys->spannerSegments().size() == 1);
      QVERIFY(score->unmanagedSpanners().size() == 1);

      // save and verify score after undo
      QVERIFY(saveCompareScore(score, "lyricsline01.mscx", DIR + "lyricsline01.mscx"));
      delete score;
      }
#endif
//---------------------------------------------------------
///  spanners09
///   Remove a measure containing the end point of a LyricsLine and undo
//
//  +---spanner---+
//         +---remove----+
//
//---------------------------------------------------------

void TestSpanners::spanners09()
      {
      MasterScore* score = readScore(DIR + "lyricsline02.mscx");
      QVERIFY(score);

      // DELETE SECOND MEASURE AND VERIFY
      Measure* msr   = score->firstMeasure();
      QVERIFY(msr);
      msr = msr->nextMeasure();
      QVERIFY(msr);
      score->startCmd();
      score->select(msr);
      score->cmdTimeDelete();
      score->endCmd();
      QVERIFY(saveCompareScore(score, "lyricsline02.mscx", DIR + "lyricsline02-ref.mscx"));

      // UNDO AND VERIFY
      score->undoStack()->undo(&ed);
      score->doLayout(); // measure needs to be renumbered
      QVERIFY(saveCompareScore(score, "lyricsline02.mscx", DIR + "lyricsline02.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners10
///   Remove a measure containing the start point of a LyricsLine and undo
//
//         +---spanner---+
//  +---remove----+
//
//---------------------------------------------------------

void TestSpanners::spanners10()
      {
      MasterScore* score = readScore(DIR + "lyricsline03.mscx");
      QVERIFY(score);

      // DELETE SECOND MEASURE AND VERIFY
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      msr = msr->nextMeasure();
      QVERIFY(msr);
      score->startCmd();
      score->select(msr);
      score->cmdTimeDelete();
      score->endCmd();
      QVERIFY(saveCompareScore(score, "lyricsline03.mscx", DIR + "lyricsline03-ref.mscx"));

      // UNDO AND VERIFY
      score->undoStack()->undo(&ed);
      score->doLayout(); // measure needs to be renumbered
      QVERIFY(saveCompareScore(score, "lyricsline03.mscx", DIR + "lyricsline03.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners11
///   Remove a measure entirely containing a LyricsLine and undo
//
//         +---spanner---+
//  +-----------remove------------+
//
//---------------------------------------------------------

void TestSpanners::spanners11()
      {
      MasterScore* score = readScore(DIR + "lyricsline04.mscx");
      QVERIFY(score);

      // DELETE SECOND MEASURE AND VERIFY
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      msr = msr->nextMeasure();
      QVERIFY(msr);
      score->startCmd();
      score->select(msr);
      score->cmdTimeDelete();
      score->endCmd();
      QVERIFY(saveCompareScore(score, "lyricsline04.mscx", DIR + "lyricsline04-ref.mscx"));

      // UNDO AND VERIFY
      score->undoStack()->undo(&ed);
      score->doLayout(); // measure needs to be renumbered
      QVERIFY(saveCompareScore(score, "lyricsline04.mscx", DIR + "lyricsline04.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners12
///   Remove a measure containing the middle portion of a LyricsLine and undo
//
//  +-----------spanner-----------+
//          +---remove----+
//
//---------------------------------------------------------

void TestSpanners::spanners12()
      {
      MasterScore* score = readScore(DIR + "lyricsline05.mscx");
      QVERIFY(score);

      // DELETE SECOND MEASURE AND VERIFY
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      msr = msr->nextMeasure();
      QVERIFY(msr);
      score->startCmd();
      score->select(msr);
      score->cmdTimeDelete();
      score->endCmd();
      QVERIFY(saveCompareScore(score, "lyricsline05.mscx", DIR + "lyricsline05-ref.mscx"));

      // UNDO AND VERIFY
      score->undoStack()->undo(&ed);
      score->doLayout(); // measure needs to be renumbered
      QVERIFY(saveCompareScore(score, "lyricsline05.mscx", DIR + "lyricsline05.mscx"));
      delete score;
      }
#if 0
//---------------------------------------------------------
///  spanners13
///   Drop a line break at a bar line in the middle of a LyricsLine and check LyricsLineSegments are correct
//
//---------------------------------------------------------

void TestSpanners::spanners13()
      {
      EditData          dropData(0);
      LayoutBreak*      brk;

      MasterScore* score = readScore(DIR + "lyricsline06.mscx");
      QVERIFY(score);

      // DROP A BREAK AT FIRST MEASURE AND VERIFY
      Measure*    msr   = score->firstMeasure();
      QVERIFY(msr);
      brk               = new LayoutBreak(score);
      brk->setLayoutBreakType(LayoutBreak::Type::LINE);
      dropData.pos      = msr->pagePos();
      dropData.dropElement  = brk;
      score->startCmd();
      msr->drop(dropData);
      score->endCmd();
      // VERIFY SEGMENTS IN SYSTEMS AND THEN SCORE
      for (System* sys : score->systems())
            QVERIFY(sys->spannerSegments().size() == 1);
      QVERIFY(saveCompareScore(score, "lyricsline06.mscx", DIR + "lyricsline06-ref.mscx"));

      // UNDO AND VERIFY
      score->undoStack()->undo();
      score->doLayout();      // systems need to be re-computed
      QVERIFY(saveCompareScore(score, "lyricsline06.mscx", DIR + "lyricsline06.mscx"));
      delete score;
      }
#endif

//---------------------------------------------------------
///  spanners14
///   creating part from an existing grand staff containing a cross staff glissando
//---------------------------------------------------------

void TestSpanners::spanners14()
      {
      MasterScore* score = readScore(DIR + "glissando-cloning05.mscx");
      QVERIFY(score);

      // create parts
      // (copied and adapted from void TestParts::createParts() in mtest/libmscore/parts/tst_parts.cpp)
      QList<Part*> parts;
      parts.append(score->parts().at(0));
      Score* nscore = new Score(score);

      Excerpt* ex = new Excerpt(score);
      ex->setPartScore(nscore);
      ex->setTitle(parts.front()->longName());
      ex->setParts(parts);
      Excerpt::createExcerpt(ex);
      QVERIFY(nscore);

//      nscore->setName(parts.front()->partName());

//      QMultiMap<int, int> tracks;
      score->Score::undo(new AddExcerpt(ex));

      QVERIFY(saveCompareScore(score, "glissando-cloning05.mscx", DIR + "glissando-cloning05-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners15
///   set the color of a spanner and save
//---------------------------------------------------------

void TestSpanners::spanners15()
      {
      MasterScore* score = readScore(DIR + "linecolor01.mscx");
      QVERIFY(score);

      for (auto it = score->spanner().cbegin(); it != score->spanner().cend(); ++it) {
            Spanner* spanner = (*it).second;
            SLine* sl = static_cast<SLine*>(spanner);
            sl->setProperty(Pid::COLOR, QVariant::fromValue(QColor(255, 0, 0, 255)));
            for (auto ss : sl->spannerSegments()) {
                  ss->setProperty(Pid::MIN_DISTANCE, 0.0);
                  ss->setPropertyFlags(Pid::MIN_DISTANCE, PropertyFlags::UNSTYLED);
                  }
            }

      QVERIFY(saveCompareScore(score, "linecolor01.mscx", DIR + "linecolor01-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
///  spanners16
///   read manually adjusted lines on a small staff and save
//---------------------------------------------------------

void TestSpanners::spanners16()
      {
      MasterScore* score = readScore(DIR + "smallstaff01.mscx");
      QVERIFY(score);

      QVERIFY(saveCompareScore(score, "smallstaff01.mscx", DIR + "smallstaff01-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
//    arrowLines
//    MuseScore Studio property names must survive a 3.x save/reopen,
//    including inactive arrow dimensions or score-style dimensions
//---------------------------------------------------------

static QList<TextLine*> arrowLines(Score* score)
      {
      QList<TextLine*> lines;
      for (const auto& entry : score->spanner()) {
            if (entry.second->isTextLine())
                  lines.append(toTextLine(entry.second));
            }
      return lines;
      }

//---------------------------------------------------------
//    checkArrowLines
//---------------------------------------------------------

static void checkArrowLines(Score* score)
      {
      const auto lines = arrowLines(score);
      QCOMPARE(lines.size(), 3);
      const std::array<Pid, 8> dimensions {{
            Pid::BEGIN_LINE_ARROW_HEIGHT,
            Pid::BEGIN_LINE_ARROW_WIDTH,
            Pid::BEGIN_FILLED_ARROW_HEIGHT,
            Pid::BEGIN_FILLED_ARROW_WIDTH,
            Pid::END_LINE_ARROW_HEIGHT,
            Pid::END_LINE_ARROW_WIDTH,
            Pid::END_FILLED_ARROW_HEIGHT,
            Pid::END_FILLED_ARROW_WIDTH,
            }};
      for (int i = 0; i < 2; ++i) {
            QCOMPARE(int(lines[i]->beginHookType()), i == 0 ? 4 : 5);
            QCOMPARE(int(lines[i]->endHookType()), i == 0 ? 4 : 5);
            for (size_t j = 0; j < dimensions.size(); ++j) {
                  QVERIFY(qAbs(lines[i]->getProperty(dimensions[j]).value<Spatium>().val() - (1.1 + .1 * j)) < 1e-9);
                  QVERIFY(!lines[i]->spannerSegments().empty());
                  QVERIFY(lines[i]->spannerSegments().front()->propertyDelegate(dimensions[j]) == lines[i]);
                  }
            std::unique_ptr<TextLine> copy(lines[i]->clone());
            for (Pid pid : dimensions)
                  QCOMPARE(copy->getProperty(pid), lines[i]->getProperty(pid));
            }
      QCOMPARE(lines[2]->beginFilledArrowWidth().val(), 1.25);
      QCOMPARE(lines[2]->endFilledArrowWidth().val(), .85);
      }

//---------------------------------------------------------
//    arrowLinesRoundTrip
//---------------------------------------------------------

void TestSpanners::arrowLinesRoundTrip()
      {
      const QString path = root + "/" + DIR + "arrowlines-302.mscx";
      QVERIFY2(QFileInfo(path).isFile(), qPrintable("Missing arrow-line fixture: " + path));
      QVERIFY2(QFileInfo(path).isReadable(), qPrintable("Unreadable arrow-line fixture: " + path));
      std::unique_ptr<MasterScore> score(new MasterScore(MScore::defaultStyle()));
      const auto error = score->loadMsc(path, false);
      QVERIFY2(error == Score::FileError::FILE_NO_ERROR,
               qPrintable(QString("Cannot load %1 (FileError %2): %3")
                          .arg(path).arg(int(error)).arg(MScore::lastError)));
      for (Score* s : score->scoreList())
            s->doLayout();
      checkArrowLines(score.get());
      QVERIFY(saveScore(score.get(), "arrowlines-roundtrip.mscx"));
      std::unique_ptr<MasterScore> reopened(readCreatedScore("arrowlines-roundtrip.mscx"));
      QVERIFY(reopened);
      checkArrowLines(reopened.get());
      auto line = arrowLines(reopened.get()).front();
      line->resetProperty(Pid::BEGIN_LINE_ARROW_HEIGHT);
      QCOMPARE(line->beginLineArrowHeight().val(), 1.0);
      line->score()->startCmd();
      line->undoChangeProperty(Pid::BEGIN_LINE_ARROW_HEIGHT, QVariant::fromValue(Spatium(2.5)), PropertyFlags::UNSTYLED);
      line->score()->endCmd();
      QCOMPARE(line->beginLineArrowHeight().val(), 2.5);
      line->score()->undoRedo(true, nullptr);
      QCOMPARE(line->beginLineArrowHeight().val(), 1.0);
      line->score()->undoRedo(false, nullptr);
      QCOMPARE(line->beginLineArrowHeight().val(), 2.5);
      }

//---------------------------------------------------------
//    arrowLinesStudioImport
//---------------------------------------------------------

void TestSpanners::arrowLinesStudioImport()
      {
      std::unique_ptr<MasterScore> score(new MasterScore(MScore::defaultStyle()));
      const QString path = root + "/" + DIR + "arrowlines-460.mscx";
      QVERIFY2(QFileInfo(path).isFile(), qPrintable("Missing arrow-line fixture: " + path));
      QVERIFY2(QFileInfo(path).isReadable(), qPrintable("Unreadable arrow-line fixture: " + path));
      const auto error = score->loadMsc(path, true);
      QVERIFY2(error == Score::FileError::FILE_NO_ERROR,
               qPrintable(QString("Cannot load %1 (FileError %2): %3")
                          .arg(path).arg(int(error)).arg(MScore::lastError)));
      score->doLayout();
      checkArrowLines(score.get());
      }

//---------------------------------------------------------
//    ArrowTestSegment
//---------------------------------------------------------

class ArrowTestSegment : public TextLineBaseSegment {
   public:
      ArrowTestSegment(TextLine* tl, Score* score) : TextLineBaseSegment(tl, score) {}
      ElementType type() const override { return ElementType::TEXTLINE_SEGMENT; }
      ArrowTestSegment* clone() const override { return new ArrowTestSegment(*this); }
      const QPolygonF& beginHead() const { return beginArrow; }
      const QPolygonF& endHead() const { return endArrow; }
      };

//---------------------------------------------------------
//    arrowLinesGeometry
//---------------------------------------------------------

void TestSpanners::arrowLinesGeometry()
      {
      std::unique_ptr<MasterScore> score(new MasterScore(MScore::defaultStyle()));
      TextLine line(score.get());
      line.setDiagonal(true);
      line.setBeginHookType(HookType::ARROW_FILLED);
      line.setEndHookType(HookType::ARROW);
      ArrowTestSegment segment(&line, score.get());
      for (const QPointF& end : { QPointF(100, 40), QPointF(100, -40), QPointF(0, 100),
                                  QPointF(-100, 30), QPointF(0, 0), QPointF(1, 0) }) {
            segment.setPos2(end);
            segment.setSpannerSegmentType(SpannerSegmentType::SINGLE);
            segment.layout();
            QCOMPARE(segment.beginHead().size(), 3);
            QCOMPARE(segment.endHead().size(), 3);
            QVERIFY(QLineF(segment.beginHead()[1], QPointF()).length() < 1e-9);
            QVERIFY(QLineF(segment.endHead()[1], end).length() < 1e-9);
            for (const auto& head : { segment.beginHead(), segment.endHead() }) {
                  for (const QPointF& p : head) {
                        QVERIFY(std::isfinite(p.x()) && std::isfinite(p.y()));
                        QVERIFY(segment.bbox().contains(p));
                        }
                  }
            }
      segment.setSpannerSegmentType(SpannerSegmentType::BEGIN);
      segment.layout();
      QVERIFY(!segment.beginHead().isEmpty());
      QVERIFY(segment.endHead().isEmpty());
      segment.setSpannerSegmentType(SpannerSegmentType::MIDDLE);
      segment.layout();
      QVERIFY(segment.beginHead().isEmpty() && segment.endHead().isEmpty());
      segment.setSpannerSegmentType(SpannerSegmentType::END);
      segment.layout();
      QVERIFY(segment.beginHead().isEmpty());
      QVERIFY(!segment.endHead().isEmpty());
      line.setBeginHookType(HookType::NONE);
      line.setEndHookType(HookType::NONE);
      segment.layout();
      QVERIFY(segment.beginHead().isEmpty() && segment.endHead().isEmpty());
      }

void TestSpanners::laissezVibLinkedTab()
      {
      MasterScore* score = readScore(DIR + "laissez-vibrer-range.mscx");
      QVERIFY(score);
      int tuning[] = { 40, 45, 50, 55, 59, 64 };
      score->staff(0)->part()->instrument()->setStringData(StringData(24, 6, tuning));
      Staff* tab = new Staff(score);
      tab->setPart(score->staff(0)->part());
      tab->initFromStaffType(StaffType::preset(StaffTypes::TAB_DEFAULT));
      tab->setDefaultClefType(ClefTypeList(ClefType::TAB));
      score->startCmd();
      score->undoInsertStaff(tab, 1, false);
      Excerpt::cloneStaff(score->staff(0), tab);
      score->endCmd();
      score->doLayout();
      Segment* first = score->firstSegment(SegmentType::ChordRest);
      QVERIFY(first && first->element(0) && first->element(VOICES));
      auto release = [](Note* note) -> LaissezVib* {
            for (Element* attached : note->el())
                  if (attached->isLaissezVib())
                        return toLaissezVib(attached);
            return nullptr;
            };
      auto check = [&](bool present) {
            for (Segment* segment = first; segment; segment = segment->next1(SegmentType::ChordRest)) {
                  for (int track : { 0, VOICES }) {
                        Element* e = segment->element(track);
                        if (!e || !e->isChord())
                              continue;
                        for (Note* note : toChord(e)->notes()) {
                              int count = 0;
                              for (Element* attached : note->el())
                                    count += attached->isLaissezVib() ? 1 : 0;
                              QCOMPARE(count, present && !note->tieFor() ? 1 : 0);
                              if (LaissezVib* lv = release(note))
                                    QCOMPARE(int(lv->linkList().size()), 2);
                              }
                        }
                  }
            };
      auto toggle = [&](int staffStart, int staffEnd) {
            score->selection().setRange(first, nullptr, staffStart, staffEnd);
            score->startCmd();
            score->cmdToggleLaissezVib();
            score->endCmd();
            };
      toggle(0, 1); // notation only: both linked staves receive marks
      check(true);
      score->undoStack()->undo(&ed);
      check(false);
      score->undoStack()->redo(&ed);
      check(true);
      toggle(0, 2); // both selected: remove once per linked-note group
      check(false);
      toggle(0, 2); // both selected: add once per linked-note group
      check(true);
      toggle(1, 2); // TAB only: remove from both linked staves
      check(false);
      toggle(1, 2);
      check(true);
      Note* fret = toChord(first->element(VOICES))->upNote();
      fret->setString(2); // G string, fret 12, sounding G4
      fret->setFret(12);
      fret->layout();
      QVERIFY(fret->fretString().contains("12"));
      LaissezVib* lv = release(fret);
      QVERIFY(lv);
      for (bool smufl : { false, true }) {
            score->style().set(Sid::laissezVibUseSmuflSym, smufl);
            for (int placement : { 0, 1, 2 }) {
                  lv->setProperty(Pid::TIE_PLACEMENT, placement);
                  lv->layout();
                  QVERIFY(lv->x() >= fret->bbox().right() + .2 * lv->spatium() - .000001);
                  QVERIFY(lv->x() + lv->bbox().left() > fret->bbox().right());
                  }
            }
      score->style().set(Sid::laissezVibUseSmuflSym, false);
      const qreal oldSize = tab->staffType(Fraction(0, 1))->fretFontSize();
      lv->layout();
      const qreal oldX = lv->x();
      tab->staffType(Fraction(0, 1))->setFretFontSize(oldSize * 1.5);
      fret->layout();
      lv->layout();
      QVERIFY(lv->x() > oldX);
      QVERIFY(lv->x() + lv->bbox().left() > fret->bbox().right());
      delete score;
      }

void TestSpanners::laissezVibRangeSelection()
      {
      MasterScore* score = readScore(DIR + "laissez-vibrer-range.mscx");
      QVERIFY(score);
      std::vector<Chord*> chords;
      for (Segment* segment = score->firstSegment(SegmentType::ChordRest); segment;
            segment = segment->next1(SegmentType::ChordRest)) {
            Element* e = segment->element(0);
            if (e && e->isChord())
                  chords.push_back(toChord(e));
            }
      QCOMPARE(int(chords.size()), 3);
      Note* tieStart = chords[1]->notes().front();
      Note* tieEnd = chords[2]->notes().front();
      QVERIFY(tieStart->tieFor());
      QCOMPARE(tieStart->tieFor()->endNote(), tieEnd);
      auto releases = [](const Note* note) {
            int count = 0;
            for (Element* attached : note->el())
                  count += attached->isLaissezVib() ? 1 : 0;
            return count;
            };
      score->selection().setRange(chords[0]->segment(), nullptr, 0, 1);
      score->startCmd();
      score->cmdToggleLaissezVib();
      score->endCmd();
      for (Note* note : chords[0]->notes())
            QCOMPARE(releases(note), 1);
      QCOMPARE(releases(tieStart), 0);
      QCOMPARE(releases(tieEnd), 1);
      QVERIFY(score->selection().isRange());
      // One undo/redo covers the complete range operation.
      score->undoStack()->undo(&ed);
      for (Chord* chord : chords)
            for (Note* note : chord->notes())
                  QCOMPARE(releases(note), 0);
      score->undoStack()->redo(&ed);
      for (Note* note : chords[0]->notes())
            QCOMPARE(releases(note), 1);
      QCOMPARE(releases(tieStart), 0);
      QCOMPARE(releases(tieEnd), 1);
      // Repeating the palette action removes marks instead of duplicating them.
      score->selection().setRange(chords[0]->segment(), nullptr, 0, 1);
      score->startCmd();
      score->cmdToggleLaissezVib();
      score->endCmd();
      for (Chord* chord : chords)
            for (Note* note : chord->notes())
                  QCOMPARE(releases(note), 0);
      // An outgoing tie is still skipped when its destination is outside the range.
      score->selection().setRange(chords[1]->segment(), chords[2]->segment(), 0, 1);
      score->startCmd();
      score->cmdToggleLaissezVib();
      score->endCmd();
      QCOMPARE(releases(tieStart), 0);
      QCOMPARE(releases(tieEnd), 0);
      // Repeat backports keep partial ties outside tieFor/tieBack. An outgoing
      // repeat tie also excludes a release; an incoming one is still eligible.
      Note* repeatSource = chords[0]->notes()[0];
      Note* repeatEnd = chords[0]->notes()[1];
      score->startCmd();
      for (int direction : { 0, 1 }) {
            Tie* partial = new Tie(score);
            partial->setPartialDirection(direction);
            partial->setStartNote(direction ? repeatSource : repeatEnd);
            score->undoAddElement(partial);
            }
      score->endCmd();
      QVERIFY(repeatSource->partialTie(false));
      QVERIFY(repeatEnd->partialTie(true));
      score->selection().setRange(chords[0]->segment(), chords[1]->segment(), 0, 1);
      score->startCmd();
      score->cmdToggleLaissezVib();
      score->endCmd();
      QCOMPARE(releases(repeatSource), 0);
      QCOMPARE(releases(repeatEnd), 1);
      QCOMPARE(releases(chords[0]->notes()[2]), 1);
      delete score;
      }

void TestSpanners::tieLayoutStylesRoundTrip()
      {
      MasterScore* original = readScore(DIR + "tie-layout-styles.mscx");
      QVERIFY(original);
      const QString path = "tie-layout-styles-roundtrip.mscx";
      QVERIFY(saveScore(original, path));
      MasterScore* reloaded = readCreatedScore(path);
      QVERIFY(reloaded);
      for (MasterScore* score : { original, reloaded }) {
            QCOMPARE(score->styleI(Sid::tiePlacementSingleNote), 2);
            QCOMPARE(score->styleI(Sid::tiePlacementChord), 2);
            QCOMPARE(score->styleI(Sid::tieDotsPlacement), 2);
            QVERIFY(score->styleB(Sid::angleHangingSlursAwayFromStaff));
            int ties = 0;
            int insideOverrides = 0;
            int releases = 0;
            for (Segment* segment = score->firstSegment(SegmentType::ChordRest); segment;
                  segment = segment->next1(SegmentType::ChordRest)) {
                  Element* element = segment->element(0);
                  if (!element || !element->isChord())
                        continue;
                  for (Note* note : toChord(element)->notes()) {
                        if (note->tieFor()) {
                              ++ties;
                              if (note->tieFor()->tiePlacement() == 1)
                                    ++insideOverrides;
                              QVERIFY(note->tieFor()->endNote());
                              }
                        for (Element* attached : note->el()) {
                              if (attached->isLaissezVib()) {
                                    ++releases;
                                    QVERIFY(!note->tieFor());
                                    QVERIFY(!attached->bbox().isEmpty());
                                    }
                              }
                        }
                  }
            QCOMPARE(ties, 14);
            QCOMPARE(insideOverrides, 7);
            QCOMPARE(releases, 14);
            int partialSlurs = 0;
            for (const auto& entry : score->spanner()) {
                  if (entry.second->isSlur()) {
                        const Slur* slur = toSlur(entry.second);
                        if (slur->isIncoming() || slur->isOutgoing())
                              ++partialSlurs;
                        }
                  }
            QCOMPARE(partialSlurs, 2);
            }
      delete reloaded;
      delete original;
      }

void TestSpanners::partialSlurHangingAngles()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "manual-tie-properties/v0.15-D-partial-slurs-37E.mscx"));
      QVERIFY(score);
      score->doLayout();
      int bothHanging = 0;
      int oneHanging = 0;
      for (const auto& entry : score->spanner()) {
            if (!entry.second->isSlur())
                  continue;
            Slur* slur = toSlur(entry.second);
            if (slur->partialDirection() < 0)
                  continue;
            SlurSegment* segment = slur->frontSegment();
            QVERIFY(segment);
            QVERIFY(segment->spannerSegmentType() == SpannerSegmentType::SINGLE);
            segment->setAutoplace(false);
            const qreal sp = segment->spatium();
            const QPointF p1(0, 10 * sp);
            const QPointF p2(8 * sp, 10 * sp);
            score->style().set(Sid::angleHangingSlursAwayFromStaff, false);
            segment->layoutSegment(p1, p2);
            const QPointF contourStart = segment->ups(Grip::START).p;
            const QPointF contourEnd = segment->ups(Grip::END).p;
            score->style().set(Sid::angleHangingSlursAwayFromStaff, true);
            segment->layoutSegment(p1, p2);
            if (slur->isIncoming() && slur->isOutgoing()) {
                  ++bothHanging;
                  QCOMPARE(segment->ups(Grip::START).p, contourStart);
                  QCOMPARE(segment->ups(Grip::END).p, contourEnd);
                  }
            else {
                  ++oneHanging;
                  // Keep the feature working for exactly one floating end.
                  QVERIFY(segment->ups(Grip::START).p != contourStart
                          || segment->ups(Grip::END).p != contourEnd);
                  }
            }
      QCOMPARE(bothHanging, 2); // D4 and the one-note D12
      QCOMPARE(oneHanging, 6);
      }

void TestSpanners::partialTieAttachments()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "manual-tie-properties/v0.15-C-hanging-ties-37E.mscx"));
      QVERIFY(score);
      score->doLayout();
      std::vector<Chord*> chords;
      for (Segment* segment = score->firstSegment(SegmentType::ChordRest); segment;
            segment = segment->next1(SegmentType::ChordRest)) {
            Element* element = segment->element(0);
            if (element && element->isChord())
                  chords.push_back(toChord(element));
            }
      QCOMPARE(int(chords.size()), 6);
      // Incoming direction follows the source, even when the destination stem
      // differs. In each three-note chord, the middle tie opposes the source stem.
      for (int pair = 0; pair < 3; ++pair) {
            Chord* destination = chords[2 * pair];
            Chord* source = chords[2 * pair + 1];
            Tie* incoming = destination->notes()[1]->partialTie(true);
            Tie* outgoing = source->notes()[1]->partialTie(false);
            QVERIFY(incoming && outgoing);
            outgoing->calculateDirection();
            incoming->calculateDirection();
            QCOMPARE(incoming->up(), outgoing->up());
            QCOMPARE(incoming->up(), !source->up());
            for (Direction stem : { Direction::UP, Direction::DOWN }) {
                  destination->setStemDirection(stem);
                  score->doLayout();
                  incoming->calculateDirection();
                  QCOMPARE(incoming->up(), outgoing->up());
                  }
            }
      // C2's middle incoming and C1's middle outgoing must clear the adjacent
      // lower notehead, rather than retaining an Outside midpoint inside it.
      Note* incomingNote = chords[0]->notes()[1];
      Note* outgoingNote = chords[1]->notes()[1];
      const qreal sp = score->spatium();
      const QPointF end = tieNoteAttachment(incomingNote, false, false, false);
      const QPointF start = tieNoteAttachment(outgoingNote, false, false, true);
      QVERIFY(end.x() <= -.19 * sp);
      QVERIFY(start.x() >= outgoingNote->headWidth() + .19 * sp);
      }

void TestSpanners::laissezVibChordGeometry()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "manual-tie-properties/v0.15-E-laissez-vibrer-37E.mscx"));
      QVERIFY(score);
      score->doLayout();
      std::vector<Chord*> chords;
      auto release = [](const Note* note) -> LaissezVib* {
            for (Element* attached : note->el())
                  if (attached->isLaissezVib())
                        return toLaissezVib(attached);
            return nullptr;
            };
      for (Segment* segment = score->firstSegment(SegmentType::ChordRest); segment;
            segment = segment->next1(SegmentType::ChordRest)) {
            Element* element = segment->element(0);
            if (element && element->isChord())
                  chords.push_back(toChord(element));
            }
      QCOMPARE(int(chords.size()), 8);
      for (bool smufl : { false, true }) {
            score->style().set(Sid::laissezVibUseSmuflSym, smufl);
            score->doLayout();
            int marks = 0;
            for (Chord* chord : chords) {
                  qreal end = 0;
                  bool first = true;
                  for (Note* note : chord->notes()) {
                        LaissezVib* lv = release(note);
                        QVERIFY(lv);
                        ++marks;
                        const qreal right = note->x() + lv->x() + lv->bbox().right();
                        if (!first)
                              QVERIFY(qAbs(right - end) < .000001);
                        first = false;
                        end = right;
                        if (!smufl) {
                              const qreal minLength = lv->getProperty(Pid::LAISSEZ_VIB_MIN_LENGTH).value<Spatium>().val() * lv->spatium();
                              QVERIFY(lv->bbox().width() - score->styleP(Sid::tieEndWidth) >= minLength - .000001);
                              }
                        }
                  }
            QCOMPARE(marks, 22);
            }
      score->style().set(Sid::laissezVibUseSmuflSym, false);
      score->doLayout();
      // E3's bottom Down/Outside LV is outer even with a down stem.
      QVERIFY(release(chords[2]->downNote())->y() > .5 * score->spatium());
      QVERIFY(release(chords[2]->downNote())->x() < chords[2]->downNote()->headWidth());
      // Outside inner ties still clear the neighboring head in E6/E7.
      for (int index : { 5, 6 }) {
            Note* middleNote = chords[index]->notes()[1];
            QVERIFY(release(middleNote)->x() >= middleNote->headWidth() + .19 * score->spatium());
            }
      Chord* dotted = chords[3];
      LaissezVib* middle = release(dotted->notes()[1]);
      LaissezVib* outer = release(dotted->downNote());
      const qreal outerX = outer->x();
      score->style().set(Sid::tieDotsPlacement, 2); // after all dots, including Outside inner ties
      score->doLayout();
      qreal dotsRight = 0;
      for (Note* note : dotted->notes())
            for (const NoteDot* dot : note->dots())
                  dotsRight = qMax(dotsRight, note->x() + dot->x() + dot->bbox().right());
      const qreal afterX = middle->x();
      QVERIFY(dotted->notes()[1]->x() + afterX >= dotsRight + .19 * score->spatium());
      QVERIFY(qAbs(outer->x() - outerX) < .000001);
      score->style().set(Sid::tieDotsPlacement, 1); // before dots
      score->doLayout();
      QVERIFY(middle->x() < afterX);
      }

void TestSpanners::nativeOrdinarySlurFit()
      {
      if (qEnvironmentVariableIntValue("MSCORE_NATIVE_SLUR_FIT") != 1)
            QSKIP("Run with MSCORE_NATIVE_SLUR_FIT=1 to exercise the preview.");
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-ordinary-slur-fit.mscx"));
      QVERIFY(score);

      int checked = 0;
      Slur* editedSlur = nullptr;
      QPointF editedTip;
      for (const auto& entry : score->spanner()) {
            if (!entry.second->isSlur())
                  continue;
            Slur* slur = toSlur(entry.second);
            SlurSegment* segment = slur->frontSegment();
            QVERIFY(segment && slur->startCR() && slur->endCR());
            const int measure = slur->startCR()->measure()->no() + 1;
            QVERIFY(measure == 2 || measure == 4 || measure == 6);
            const qreal expected = measure == 2 ? 0.77 : measure == 4 ? 0.29 : 3.23;
            const qreal sp = segment->spatium();
            for (Grip tip : { Grip::START, Grip::END }) {
                  const qreal y = (segment->y() + segment->ups(tip).pos().y()) / sp;
                  QVERIFY2(qAbs(y - expected) < 0.16, qPrintable(QString("M%1 tip=%2").arg(measure).arg(y)));
                  }
            if (measure == 2) {
                  // An offset should be relative to the snapped tip even after a fresh layout.
                  const QPointF initial = segment->ups(Grip::START).pos();
                  const QPointF delta(0.0, -0.25 * sp);
                  segment->setSlurOffset(Grip::START, delta);
                  segment->computeBezier();
                  QVERIFY(QLineF(segment->ups(Grip::START).pos(), initial + delta).length() < 1e-6 * sp);
                  editedSlur = slur;
                  editedTip = initial + delta;
                  }
            ++checked;
            }
      QCOMPARE(checked, 3);
      QVERIFY(editedSlur);
      score->doLayout();
      const SlurSegment* edited = editedSlur->frontSegment();
      QVERIFY(edited);
      QVERIFY(QLineF(edited->ups(Grip::START).pos(), editedTip).length() < 1e-6 * edited->spatium());
      }

void TestSpanners::nativeSlantedSlurFit()
      {
      if (qEnvironmentVariableIntValue("MSCORE_NATIVE_SLUR_FIT") != 1)
            QSKIP("Run with MSCORE_NATIVE_SLUR_FIT=1 to exercise the preview.");
      std::unique_ptr<MasterScore> score(readScore(DIR + "Native-Spacing-09-Slanted-Slurs.mscx"));
      QVERIFY(score);

      int checked = 0;
      for (const auto& entry : score->spanner()) {
            if (!entry.second->isSlur())
                  continue;
            Slur* slur = toSlur(entry.second);
            const SlurSegment* segment = slur->frontSegment();
            QVERIFY(segment && slur->startCR() && slur->endCR());
            const int measure = slur->startCR()->measure()->no() + 1;
            QVERIFY(measure == 2 || measure == 4 || measure == 6);
            QCOMPARE(slur->up(), measure != 6);
            const qreal sp = segment->spatium();
            const QPointF first = segment->ups(Grip::START).pos();
            const QPointF last = segment->ups(Grip::END).pos();
            QVERIFY(last.x() > first.x());
            QVERIFY((measure == 2 ? first.y() - last.y() : last.y() - first.y()) > 0.5 * sp);
            // Clear the staff lines without shifting both tips off their anchors.
            QVERIFY(qAbs(segment->y()) < 1e-6 * sp);
            for (const QPointF& tip : { first, last }) {
                  const qreal y = tip.y() / sp;
                  const int line = qRound(y);
                  QVERIFY2(line < 0 || line > 4 || qAbs(y - line) >= 0.22,
                           qPrintable(QString("M%1 tip=%2").arg(measure).arg(y)));
                  }

            const Segment* middle = slur->startCR()->segment()->next(SegmentType::ChordRest);
            QVERIFY(middle && middle->element(slur->track()) && middle->element(slur->track())->isChord());
            const Chord* chord = toChord(middle->element(slur->track()));
            const Note* note = chord->notes().front();
            const QRectF bounds = note->bbox().translated(middle->measure()->pos() + middle->pos()
                  + chord->pos() + note->pos());
            const CubicBezier curve(first, segment->ups(Grip::BEZIER1).pos(),
                                    segment->ups(Grip::BEZIER2).pos(), last);
            const qreal x = bounds.center().x();
            QVERIFY(first.x() < x && x < last.x());
            qreal begin = 0.0;
            qreal end = 1.0;
            for (int i = 0; i < 16; ++i) {
                  const qreal t = 0.5 * (begin + end);
                  if (curve.pointAtPercent(t).x() < x)
                        begin = t;
                  else
                        end = t;
                  }
            const qreal y = curve.pointAtPercent(0.5 * (begin + end)).y();
            QVERIFY(slur->up() ? y < bounds.top() - 0.2 * sp : y > bounds.bottom() + 0.2 * sp);
            ++checked;
            }
      QCOMPARE(checked, 3);
      }

void TestSpanners::nativeAfterGraceSlurFit()
      {
      if (qEnvironmentVariableIntValue("MSCORE_NATIVE_SLUR_FIT") != 1)
            QSKIP("Run with MSCORE_NATIVE_SLUR_FIT=1 to exercise the preview.");
      std::unique_ptr<MasterScore> score(readScore(DIR + "../layout/native-after-grace-rest-clearance.mscx"));
      QVERIFY(score);
      int lowEnds = 0;
      int levelGraceSlurs = 0;
      int fallingGraceSlurs = 0;
      for (const auto& entry : score->spanner()) {
            if (!entry.second->isSlur())
                  continue;
            Slur* slur = toSlur(entry.second);
            QVERIFY(slur->endCR() && slur->endCR()->isChord() && slur->endCR()->isGraceAfter());
            const Chord* grace = toChord(slur->endCR());
            const SlurSegment* segment = slur->frontSegment();
            QVERIFY(segment && grace->stem());
            const qreal sp = segment->spatium();
            QVERIFY(slur->startCR() && slur->startCR()->isChord());
            const Chord* owner = toChord(slur->startCR());
            SlurPos base;
            slur->slurPos(&base);
            const QPointF end = segment->ups(Grip::END).pos();
            QVERIFY(qAbs(end.x() - base.p2.x()) < 1e-6 * sp);
            const qreal endY = (end.y() + segment->y()) / sp;
            if (grace->upNote()->pitch() == 53) {
                  QVERIFY(qAbs(grace->stem()->len() / sp - 4.0) < 0.02);
                  // The low grace tip should stay below the staff's middle
                  // line. The main-note clearance put it above the line.
                  QVERIFY(qAbs(endY - 2.37) < 0.12);
                  ++lowEnds;
                  continue;
                  }
            const qreal graceMag = score->styleD(Sid::graceNoteMag);
            QVERIFY(qAbs(grace->stem()->len() / sp - 3.25 * graceMag) < 0.02);
            if (grace->upNote()->pitch() == owner->upNote()->pitch()) {
                  QVERIFY(qAbs(endY - 0.77) < 0.10);
                  const CubicBezier center(segment->ups(Grip::START).pos(), segment->ups(Grip::BEZIER1).pos(),
                                           segment->ups(Grip::BEZIER2).pos(), segment->ups(Grip::END).pos());
                  // A nearly level arc has its crown near the midpoint.
                  // Measure the ink over the centerline, excluding the
                  // separate pen stroke, to catch a full-width ribbon.
                  const qreal ribbon = (center.pointAtPercent(0.5).y() - segment->bbox().top()) / sp;
                  QVERIFY2(ribbon > 0.04 && ribbon < 0.085, "Local grace slur ribbon is not tapered");
                  ++levelGraceSlurs;
                  }
            else {
                  QVERIFY(qAbs(endY + 0.38) < 0.12);
                  ++fallingGraceSlurs;
                  }
            }
      QCOMPARE(lowEnds, 2);
      QCOMPARE(levelGraceSlurs, 3);
      QCOMPARE(fallingGraceSlurs, 1);
      }

void TestSpanners::nativePreGraceSlurFit()
      {
      if (qEnvironmentVariableIntValue("MSCORE_NATIVE_SLUR_FIT") != 1)
            QSKIP("Run with MSCORE_NATIVE_SLUR_FIT=1 to exercise the preview.");
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-pre-grace-slurs.mscx"));
      QVERIFY(score && score->styleB(Sid::beamLayout4));
      int beamedPairs = 0;
      int betweenBeats = 0;
      int flaggedStarts = 0;
      Slur* inner = nullptr;
      Slur* acrossBarline = nullptr;
      Slur* next = nullptr;
      Slur* levelPair = nullptr;
      for (const auto& entry : score->spanner()) {
            if (!entry.second->isSlur())
                  continue;
            Slur* slur = toSlur(entry.second);
            ChordRest* start = slur->startCR();
            ChordRest* end = slur->endCR();
            QVERIFY(start && start->isChord() && end && end->isChord());
            const SlurSegment* segment = slur->frontSegment();
            QVERIFY(segment && segment->system());
            const qreal sp = segment->spatium();
            SlurPos anchors;
            slur->slurPos(&anchors);
            if (slur->up() && start->isGraceBefore() && start->up() && !start->beam()
                && start->segment() == end->segment()) {
                  const Chord* grace = toChord(start);
                  QVERIFY(grace->stem() && grace->hook());
                  const qreal stemEndY = start->measure()->y() + start->segment()->y() + start->parent()->y()
                        + start->y() + (grace->stemPos() - grace->pagePos() + grace->stem()->p2()).y();
                  const qreal graceMag = score->styleD(Sid::graceNoteMag);
                  const qreal inset = (anchors.p1.y() - stemEndY) / sp;
                  QVERIFY2(qAbs(inset - 0.5 * graceMag) < 0.04,
                           qPrintable(QString("Flagged grace tip inset: %1sp").arg(inset)));
                  ++flaggedStarts;
                  }
            if (slur->up() && start->isGrace() && start->tick().ticks() == 1440) {
                  if (start->segment() == end->segment())
                        inner = slur;
                  else if (end->tick().ticks() == 1920)
                        acrossBarline = slur;
                  }
            if (slur->up() && start->tick().ticks() == 1920 && end->isGrace())
                  next = slur;
            if (slur->up() && start->isGrace() && end->isGrace() && start->beam()
                && start->beam() == end->beam() && start->up() && end->up()) {
                  const Beam* beam = start->beam();
                  const QLineF* line = beam->primarySegment4();
                  QVERIFY(line && line->dx() > 0.0);
                  const QPointF beamOrigin = beam->pagePos()
                        - QPointF(segment->system()->pagePos().x(), segment->system()->staffYpage(segment->vStaffIdx()));
                  const qreal halfWidth = 0.5 * score->styleS(Sid::beamWidth).val() * beam->spatium() * beam->mag()
                        * std::hypot(qreal(1.0), line->dy() / line->dx());
                  const auto point = [&](Grip grip) { return segment->ups(grip).pos() + segment->pos(); };
                  const auto gap = [&](const QPointF& p) {
                        const qreal t = qBound(0.0, (p.x() - beamOrigin.x() - line->x1()) / line->dx(), 1.0);
                        return (beamOrigin.y() + line->pointAt(t).y() - halfWidth - p.y()) / sp;
                        };
                  QVERIFY(qAbs(point(Grip::START).x() - anchors.p1.x()) < 1e-6 * sp);
                  QVERIFY(qAbs(point(Grip::END).x() - anchors.p2.x()) < 1e-6 * sp);
                  for (Grip grip : { Grip::START, Grip::END }) {
                        const Chord* grace = toChord(grip == Grip::START ? start : end);
                        QVERIFY(grace->stem());
                        const qreal stemX = grace->pageX() + grace->stem()->x() - segment->system()->pagePos().x();
                        const qreal stemInset = (point(grip).x() - stemX) / sp;
                        QVERIFY2(qAbs(stemInset) < 0.02,
                                 qPrintable(QString("Beamed grace tip inset: %1sp").arg(stemInset)));
                        }
                  const CubicBezier curve(point(Grip::START), point(Grip::BEZIER1), point(Grip::BEZIER2), point(Grip::END));
                  for (qreal t : { 0.25, 0.5, 0.75 })
                        QVERIFY(gap(curve.pointAtPercent(t)) > 0.12);
                  // The level pair's rendered slur should stay near the beam.
                  // With provisional stem lengths its tips were too high.
                  if (start->tick().ticks() == 480) {
                        levelPair = slur;
                        for (Grip grip : { Grip::START, Grip::END }) {
                              const qreal clearance = gap(point(grip));
                              QVERIFY2(clearance > 0.15 && clearance < 0.55,
                                       qPrintable(QString("Level grace beam to slur tip: %1sp").arg(clearance)));
                              }
                        }
                  ++beamedPairs;
                  }
            if (!slur->up() && start->segment() != end->segment()) {
                  const ChordRest* cr = start->isGrace() ? start : end;
                  if (!cr->isGrace() || cr->isGraceAfter())
                        continue;
                  QVERIFY(cr->up());
                  const Chord* grace = toChord(cr);
                  const qreal noteY = cr->measure()->y() + cr->segment()->y() + cr->parent()->y()
                        + cr->y() + grace->downNote()->y();
                  const qreal anchorY = cr == start ? anchors.p1.y() : anchors.p2.y();
                  const qreal expected = 0.9 * score->styleD(Sid::graceNoteMag) * sp;
                  QVERIFY(qAbs(anchorY - noteY - expected) < 0.02 * sp);
                  ++betweenBeats;
                  }
            }
      QCOMPARE(beamedPairs, 2);
      QCOMPARE(betweenBeats, 2);
      QCOMPARE(flaggedStarts, 2);
      QVERIFY(inner && acrossBarline && next);
      QCOMPARE(inner->startCR(), acrossBarline->startCR());
      QCOMPARE(acrossBarline->endCR(), next->startCR());

      const Chord* owner = toChord(inner->endCR());
      const auto graces = owner->graceNotesBefore();
      QCOMPARE(graces.size(), 2);
      const Chord* last = graces.back();
      QVERIFY(last->beam() && last->beam() == graces.front()->beam());
      const Note* graceHead = last->upNote();
      const Note* ownerHead = owner->upNote();
      const qreal gap = ownerHead->pagePos().x() + ownerHead->bbox().left()
            - graceHead->pagePos().x() - graceHead->bbox().right();
      const qreal sp = inner->frontSegment()->spatium();
      QVERIFY2(gap >= 0.63 * sp, qPrintable(QString("Grace beam to parent: %1sp").arg(gap / sp)));

      const auto tip = [](const Slur* slur, Grip grip) {
            const SlurSegment* s = slur->frontSegment();
            return s->ups(grip).pos() + s->pos();
            };
      const QPointF insideStart = tip(inner, Grip::START);
      const QPointF outsideStart = tip(acrossBarline, Grip::START);
      QVERIFY(qAbs(insideStart.x() - outsideStart.x()) < 0.02 * sp);
      QVERIFY2(insideStart.y() - outsideStart.y() > 0.55 * sp, "Nested tips touch at the grace beam");

      const qreal tipGap = tip(next, Grip::START).x() - tip(acrossBarline, Grip::END).x();
      QVERIFY2(tipGap > 0.32 * sp, qPrintable(QString("Incoming to outgoing slur tips: %1sp").arg(tipGap / sp)));

      // Lift the end of the short beam slur and of the long slur into
      // a grace. Their fitted shoulders should follow during the edit.
      QVERIFY(levelPair);
      for (Slur* slur : { levelPair, next }) {
            SlurSegment* segment = slur->frontSegment();
            const QPointF first = tip(slur, Grip::START);
            const QPointF last = tip(slur, Grip::END);
            const QPointF rightControl = tip(slur, Grip::BEZIER2);
            const QPointF rightOffset = segment->ups(Grip::BEZIER2).off;
            const auto midpoint = [&]() {
                  return CubicBezier(tip(slur, Grip::START), tip(slur, Grip::BEZIER1),
                                     tip(slur, Grip::BEZIER2), tip(slur, Grip::END)).pointAtPercent(0.5);
                  };
            const QPointF before = midpoint();
            const QPointF lift(0.0, -2.0 * sp);
            const QPointF endOffset = segment->ups(Grip::END).off;
            segment->ups(Grip::END).off += lift;
            segment->computeBezier();
            QCOMPARE(tip(slur, Grip::START), first);
            QCOMPARE(tip(slur, Grip::END), last + lift);
            QVERIFY(midpoint().y() < before.y() - 0.55 * sp);
            QVERIFY(tip(slur, Grip::BEZIER2).y() < rightControl.y() - 0.55 * sp);
            QCOMPARE(segment->ups(Grip::BEZIER2).off, rightOffset);
            const QPointF lifted = midpoint();
            segment->computeBezier();
            QCOMPARE(midpoint(), lifted);
            segment->ups(Grip::END).off = endOffset;
            segment->computeBezier();
            QCOMPARE(midpoint(), before);
            }

      // The shoulder nudge must keep the separated tips after a full layout.
      EditData shoulderEdit;
      shoulderEdit.curGrip = Grip::SHOULDER;
      shoulderEdit.delta = QPointF(0.0, -0.1 * sp);
      acrossBarline->frontSegment()->editDrag(shoulderEdit);
      const QPointF editedShoulder = tip(acrossBarline, Grip::SHOULDER);
      score->doLayout();
      QVERIFY(QLineF(tip(acrossBarline, Grip::START), outsideStart).length() < 1e-6 * sp);
      QVERIFY(QLineF(tip(acrossBarline, Grip::SHOULDER), editedShoulder).length() < 1e-6 * sp);
      QVERIFY(qAbs(tip(next, Grip::START).x() - tip(acrossBarline, Grip::END).x() - tipGap) < 1e-6 * sp);
      }

void TestSpanners::nativeNestedBeamedSlurFit()
      {
      if (qEnvironmentVariableIntValue("MSCORE_NATIVE_SLUR_FIT") != 1)
            QSKIP("Run with MSCORE_NATIVE_SLUR_FIT=1 to exercise the preview.");
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-nested-beamed-slurs.mscx"));
      QVERIFY(score && score->styleB(Sid::beamLayout4));
      Slur* outer = nullptr;
      std::vector<Slur*> inner;
      for (const auto& entry : score->spanner()) {
            if (!entry.second->isSlur())
                  continue;
            Slur* slur = toSlur(entry.second);
            QVERIFY(slur->up() && slur->frontSegment());
            if (slur->ticks() == Fraction(7, 8))
                  outer = slur;
            else
                  inner.push_back(slur);
            }
      QVERIFY(outer);
      QCOMPARE(inner.size(), size_t(2));
      const auto curve = [](const Slur* slur) {
            const SlurSegment* s = slur->frontSegment();
            const auto pt = [s](Grip grip) { return s->ups(grip).pos() + s->pos(); };
            return CubicBezier(pt(Grip::START), pt(Grip::BEZIER1), pt(Grip::BEZIER2), pt(Grip::END));
            };
      const auto outside = curve(outer);
      const qreal sp = score->spatium();
      const SlurSegment* outSeg = outer->frontSegment();
      SlurPos anchors;
      outer->slurPos(&anchors);
      QVERIFY(qAbs(outSeg->ups(Grip::START).pos().x() - anchors.p1.x()) < 1e-6 * sp);
      QVERIFY(qAbs(outSeg->ups(Grip::END).pos().x() - anchors.p2.x()) < 1e-6 * sp);
      for (const Slur* inside : inner) {
            const auto innerCurve = curve(inside);
            for (const qreal t : { 0.3, 0.5, 0.7 }) {
                  const QPointF p = innerCurve.pointAtPercent(t);
                  qreal a = 0.0;
                  qreal b = 1.0;
                  for (int i = 0; i < 20; ++i) {
                        const qreal mid = (a + b) * 0.5;
                        if (outside.pointAtPercent(mid).x() < p.x())
                              a = mid;
                        else
                              b = mid;
                        }
                  QVERIFY2(p.y() - outside.pointAtPercent((a + b) * 0.5).y() > 0.1 * sp,
                           "The outer slur touches or crosses a nested slur");
                  }
            }

      const auto grips = [](const Slur* slur) {
            const SlurSegment* segment = slur->frontSegment();
            std::array<QPointF, int(Grip::GRIPS)> result;
            for (Grip grip : { Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END })
                  result[int(grip)] = segment->ups(grip).pos() + segment->pos();
            return result;
            };
      // Compare the curve immediately after the drag with the one after
      // its layout is rebuilt. Both must use the same separated baseline.
      const auto automatic = grips(outer);
      EditData edit;
      edit.delta = QPointF(0.0, -0.1 * sp);
      for (Grip grip : { Grip::SHOULDER, Grip::BEZIER1 }) {
            edit.curGrip = grip;
            outer->frontSegment()->editDrag(edit);
            const auto dragged = grips(outer);
            score->doLayout();
            const auto laidOut = grips(outer);
            for (Grip point : { Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END })
                  QVERIFY(QLineF(dragged[int(point)], laidOut[int(point)]).length() < 1e-6 * sp);
            for (Grip tip : { Grip::START, Grip::END })
                  QVERIFY(QLineF(automatic[int(tip)], laidOut[int(tip)]).length() < 1e-6 * sp);
            }
      for (Grip grip : { Grip::BEZIER1, Grip::BEZIER2 })
            outer->frontSegment()->setSlurOffset(grip, QPointF());
      score->doLayout();
      const auto restored = grips(outer);
      for (Grip grip : { Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END })
            QVERIFY(QLineF(automatic[int(grip)], restored[int(grip)]).length() < 1e-6 * sp);

      // Manual changes to an inner curve don't refit the outer slur.
      edit.curGrip = Grip::SHOULDER;
      edit.delta = QPointF(0.0, -0.4 * sp);
      inner.front()->frontSegment()->editDrag(edit);
      score->doLayout();
      const auto afterInnerEdit = grips(outer);
      for (Grip grip : { Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END })
            QVERIFY(QLineF(automatic[int(grip)], afterInnerEdit[int(grip)]).length() < 1e-6 * sp);
      }

QTEST_MAIN(TestSpanners)
#include "tst_spanners.moc"

