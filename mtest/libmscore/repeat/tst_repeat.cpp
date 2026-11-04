//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2012 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <QtTest/QtTest>
#include <QBuffer>
#include <QDomDocument>
#include <cmath>
#include <map>
#include <tuple>
#include "mtest/testutils.h"
#include "libmscore/score.h"
#include "libmscore/measure.h"
#include "libmscore/repeatlist.h"
#include "libmscore/utils.h"
#include "libmscore/chord.h"
#include "libmscore/note.h"
#include "libmscore/navigate.h"
#include "libmscore/tie.h"
#include "libmscore/lyrics.h"
#include "libmscore/slur.h"
#include "libmscore/stem.h"
#include "libmscore/staff.h"
#include "libmscore/stafftype.h"
#include "libmscore/accidental.h"
#include "libmscore/undo.h"
#include "libmscore/barline.h"
#include "libmscore/ledgerline.h"
#include "libmscore/arpeggio.h"
#include "libmscore/segment.h"
#include "libmscore/system.h"
#include "libmscore/mscoreview.h"
#include "audio/midi/event.h"

#define DIR QString("libmscore/repeat/")

using namespace Ms;

//---------------------------------------------------------
//   TestRepeat
//---------------------------------------------------------

// Exercise the editor's segment handoff across system breaks without a GUI.
class SlurKeyboardView : public MuseScoreView {
   public:
      int handoffs = 0;
      void dataChanged(const QRectF&) override {}
      void updateAll() override {}
      void drawBackground(QPainter*, const QRectF&) const override {}
      const QRect geometry() const override { return QRect(); }
      void startEdit(Element*, Grip) override { ++handoffs; }
      };

class TestRepeat : public QObject, public MTest
      {
      Q_OBJECT
      void repeat(const char* f1, const QString & ref);

   private slots:
      void initTestCase();
      void notationRoutes_data();
      void notationRoutes();
      void slurKeyboardNavigation_data();
      void slurKeyboardNavigation();
      void slurKeyboardNormal();
      void slurKeyboardVolta();
      void partialTieRoundTrip();
      void partialTieClearance_data();
      void partialTieClearance();
      void partialTieOffsets();
      void partialTieChordPlacement_data();
      void partialTieChordPlacement();
      void repeatTieNotes();
      void automaticRepeatTies();
      void repeatTiePlayback();
      void partialTieAttackPlayback_data();
      void partialTieAttackPlayback();
      void partialLyricsLayout();
      void partialLyricsRendering_data();
      void partialLyricsRendering();
      void nativeLyricsWithoutRepeat();
      void partialLyricsDashCount_data();
      void partialLyricsDashCount();
      void lyricDashDimensions_data();
      void lyricDashDimensions();
      void partialLyricsMelismaEnd_data();
      void partialLyricsMelismaEnd();
      void partialSlurLayout();
      void partialSlurClearance_data();
      void partialSlurClearance();
      void partialSlurSystemHeader();
      void partialSlurHeight_data();
      void partialSlurHeight();
      void oneNotePartialSlurStaffClearance();
      void partialSlurRepeatDotGap_data();
      void partialSlurRepeatDotGap();
      void startRepeatNoteSpacing_data();
      void startRepeatNoteSpacing();
      void startRepeatHeaderSpacing_data();
      void startRepeatHeaderSpacing();
      void explicitSignatureHeaderSpacing_data();
      void explicitSignatureHeaderSpacing();
      void barlineChordRestSpacing_data();
      void barlineChordRestSpacing();
      void incomingTieMinimumSpace_data();
      void incomingTieMinimumSpace();
      void partialSlurBoundaryValidity_data();
      void partialSlurBoundaryValidity();
      void invalidOneNotePartialSlur_data();
      void invalidOneNotePartialSlur();
      void repeatTieNoteEntry_data();
      void repeatTieNoteEntry();
      void repeatTieToggle();
      void repeat01() { repeat("repeat01.mscx", "1;2;3; 2;3;4;5;6"); }        // repeat barline 2 measures ||: | :||
      void repeat02() { repeat("repeat02.mscx", "1;2; 2;3;4;5;6"); }          // repeat barline 1 measure ||: :||
      void repeat03() { repeat("repeat03.mscx", "1;2; 1;2;3;4;5;6"); }        // repeat barline end to start :||
      void repeat04() { repeat("repeat04.mscx", "1;2;3; 2;3;4; 2;3;4;5;6"); }  // repeat barline ||: | :|| :||
      void repeat05() { repeat("repeat05.mscx", "1;2;3; 2;3; 2;3;4; 2;3;4;5;6"); } // repeat barline ||: | x2 :|| :||
      void repeat06() { repeat("repeat06.mscx", "1;2;3; 2; 4;5;6"); }             // simple volta
      void repeat07() { repeat("repeat07.mscx", "1;2;3;4;5;6; 1;2;3"); }         // DC al fine
      void repeat08() { repeat("repeat08.mscx", "1;2;3;4;5;6; 2;3;4; 7;8;9;10;11"); } // DS al coda

      void repeat09() { repeat("repeat09.mscx", "1;2;3; 2; 5;6"); }                        // 3 voltas, but twice a volta1
      void repeat10() { repeat("repeat10.mscx", "1;2;3;4; 1;2; 5;6;7;8; 1;2; 9;10; 1;2; 11;12"); } // 3 voltas
      void repeat11() { repeat("repeat11.mscx", "1;2;3;4; 2;3; 5;6;7;8; 2; 9;10"); }  // volta after to coda
      void repeat12() { repeat("repeat12.mscx", "1;2;3;4; 3; 5;6; 2;3; 5;6;7"); }     // volta between segno & DS
      void repeat13() { repeat("repeat13.mscx", "1;2;3;4;5; 1;2;3;4;5"); }                   // incomplete jump -> carry on as good as you can
      void repeat14() { repeat("repeat14.mscx", "1;2;3;4;5;6;7;8;9;10; 2;3;4;5;6;7;8; 11;12; 2;3;4;5;6;7;8; 13;14;15;16;17;18; 16;17;18; 19;20;21;22;23; 5;6;7; 24;25;26"); } // complex roadmap DS al coda, volta, repeat
      void repeat15() { repeat("repeat15.mscx", "1;2; 2; 2; 2; 2; 2; 2; 2;3"); } // repeat barline ||: x8 :||

      void repeat16() { repeat("repeat16.mscx", "1;2;3;4; 4; 1;2"); } // jump in simple repeat
      void repeat17() { repeat("repeat17.mscx", "1;2; 1; 3;4;5; 4; 6;7;8; 7; 9"); } // volta after coda

      void repeat18() { repeat("repeat18.mscx", "1;2; 1; 3;4;5;6; 5; 7;8"); } // twice volta
      void repeat19() { repeat("repeat19.mscx", "1;2;3;4; 1;2; 4"); } // DS al coda after the coda

      void repeat20() { repeat("repeat20.mscx", "1;2;3; 1; 4;5;6;7;8; 5;6"); } // 1) DS al Coda, 2) DS1 al Fine
      void repeat21() { repeat("repeat21.mscx", "1;2;3; 1;2;3;4;5;6;7; 5; 8"); } // 1) DS, 2) DS1 al Coda

      void repeat22() { repeat("repeat22.mscx", "1;2;3; 2;3;4;5; 5; 5;6"); } // DS and ||: 3x :||
      // complex roadmap
      void repeat23() { repeat("repeat23.mscx", "1;2; 1;2;3; 2;3;4;5;6;7; 6;7;8;9;10;11; 9;10; 12; 12;13;14; 13;14;15;16; 13;14"); }

      void repeat24() { repeat("repeat24.mscx", "1;2;3;4;5; 3;4; 2;3;4;5;6"); } // S ||: DS :||
      void repeat25() { repeat("repeat25.mscx", "1;2; 1;2;3;4;5; 4; 2;3;4;5"); } // ||: S :||  ||: DS :||

      void repeat26() { repeat("repeat26.mscx", "1; 1;2; 2;3"); } // empty and garbage jump

      void repeat27() { repeat("repeat27.mscx", "1;2; 2; 1"); }        // #73486 single-measure repeat at end of section
      void repeat28() { repeat("repeat28.mscx", "1;2; 2; 1;2; 1"); }   // #73486 single-measure repeat at end of section w/DC
      void repeat29() { repeat("repeat29.mscx", "1;2;3; 3; 2;3; 1"); } // #73486 single-measure repeat at end of section w/DS

      void repeat30() { repeat("repeat30.mscx", "1; 1;2; 1;2"); }     // #73496 single measure section at beginning of score followed by a section with end repeat (without beginning repeat)

      void repeat31() { repeat("repeat31.mscx", "1;2; 2; 1;2"); }               // #73531 ending measure has jump and repeat m1 |: m2 DC :|
      void repeat32() { repeat("repeat32.mscx", "1;2;3; 3; 2;3"); }             // #73531 ending measure has jump and repeat m1 |S m2 |: m3 DS :|
      void repeat33() { repeat("repeat33.mscx", "1;2;3; 2;3; 1;2;3"); }         // #73531 ending measure has jump and repeat m1 |: m2 | m3 DC :|
      void repeat34() { repeat("repeat34.mscx", "1;2;3; 2; 4;5; 5; 1;2; 4;5"); }// #73531 ending measure has jump and repeat m1 |: m2 |1e m3 :| 2e m4 |: m5 | DC :|

      void repeat35() { repeat("repeat35.mscx", "1;2; 1;2; 1;2;3;4; 2;3; 1;2;3; 1;2; 1;2;3;4; 2;3; 5"); } // #65161 multiple sections, each with possible DC, DS, al Fine
      void repeat36() { repeat("repeat36.mscx", "1;2;3;4;5; 1;2;3"); } // incomplete jump, missing Coda from D.S. al Coda

      void repeat37() { repeat("repeat37.mscx", "1;2;1; 1;2;1; 1;2;1; 1;2;1; 1;2;1"); } // #65161, with section breaks occurring on non-measure frames
      void repeat38() { repeat("repeat38.mscx", "1;2; 2;3;4; 2; 2;3; 5"); } // D.S. with repeats at start repeat

      void repeat39() { repeat("repeat39.mscx", "1;2;3; 2; 4;5; 2;3; 2; 4;5; 2; 6"); } // volta 1.3.,2.4.

      void repeat40() { repeat("repeat40.mscx", "1;2; 1; 3;4; 1; 5"); } // #148276 Open volta lines must extend over entire ending: next endRepeat
      void repeat41() { repeat("repeat41.mscx", "1;2; 1; 3; 1; 4;5"); } // #148276 Open volta lines must extend over entire ending: next endRepeat at volta (SLine) end
      void repeat42() { repeat("repeat42.mscx", "1;2; 1; 3; 5; 1; 4;5"); } // #148276 Open volta lines must extend over entire ending: next volta
      void repeat43() { repeat("repeat43.mscx", "1;2; 1;2; 1; 3; 1;2; 1; 4; 1; 5"); } // #8604 Complex alternate endings 1.2.4. / 3./ 5.
      void repeat44() { repeat("repeat44.mscx", "1;2;3;4;5; 3; 6; 3;4; 1; 7"); } // Jump from within a volta

      void repeat45() { repeat("repeat45.mscx", "1;2;3;4; 3; 5;6; 2;3;4; 3; 5;6;7"); } // repeat12 but with 'play repeats' enabled
      void repeat46() { repeat("repeat46.mscx", "1;2;3;4;5; 3;4; 2;3;4;5; 3;4;5;6"); } // repeat24 but with 'play repeats' enabled

      void repeat47() { repeat("repeat47.mscx", "1;2;3; 2; 4;5;6;7; 6; 8;9;10;11; 2; 4; 13;14;15; 9;10; 16;17;18"); } // #269378 Double Coda messed up repeat rewind logic

      void repeat48() { repeat("repeat48.mscx", "1;2;3;4; 1; 5;6;7; 3;4; 1; 5;8"); } // jump into first volta, without playRepeats
      void repeat49() { repeat("repeat49.mscx", "1;2;3; 1;2;3;4;5;6; 3; 1;2;3;4; 7"); } // D.S. with playRepeats
      void repeat50() { repeat("repeat50.mscx", "1;2;3;4; 1;2;3;4;5;6; 1;2;3;4; 1;2;3; 7"); } // D.S. with playRepeats with ToCoda inside the repeat
      void repeat51() { repeat("repeat51.mscx", "1;2;3;4;5;6; 3;4; 7;8;9; 3;4; 10;11"); } // #270332 twice D.S. with playRepeats to same target with different Coda

      void repeat52() { repeat("repeat52.mscx", "1;2;3; 1; 4;5; 1;2;3; 1; 4;5; 1; 6; 3; 1; 4;5; 1; 6"); } // Jump into volta "final" playthrough
      void repeat53() { repeat("repeat53.mscx", "1;2;3; 1; 4;5; 1;2;3; 1; 4;5; 1; 6; 5; 1;2;3; 1; 4;5; 1; 6"); } // Jump into volta with repeats

      void repeat54() { repeat("repeat54.mscx", "1;2;3; 1; 4;5;6;7;8; 5;6"); } // repeat20 without label changes, pick correct segno
      void repeat55() { repeat("repeat55.mscx", "1;2;3; 1;2;3;4;5;6;7; 5; 8"); } // repeat21 without label changes, pick correct segno

      void repeat56() { repeat("repeat56.mscx", "1;2;3;4; 2; 5;6;7; 5;6;7;8"); } // start of volta and start repeat on same measure
      void repeat57() { repeat("repeat57.mscx", "1;2;3"); } // no repeat, skip volta until section end, relates to #274690

      void repeat58() { repeat("repeat58.mscx", "1;2; 1; 3;4;5; 4; 6;7"); } // duplicate voltas #311986 - single instrument
      void repeat59() { repeat("repeat59.mscx", "1;2; 1; 3;4;5; 4; 6;7"); } // duplicate voltas #311986 - multiple instruments
      void repeat60() { repeat("repeat60.mscx", "1;2;6;7; 1;2;3;6;7; 1;2;3;4;5;6;7; 1;2;3;6;7; 1;2;6;7; 1;7"); } // overlapping voltas
      void repeat61() { repeat("repeat61.mscx", "1;2;3;6;7; 1;2;3;4;5;6;7; 1;2;3;6;7; 1;7"); } // overlapping voltas - nested
      void repeat62() { repeat("repeat62.mscx", "1;5;6;7; 1;2;3;4;5;6;7; 1;5;6;7; 1;7"); } // overlapping voltas - same start
      void repeat63() { repeat("repeat63.mscx", "1;2;3;7; 1;2;3;4;5;6;7; 1;2;3;7; 1;7"); } // overlapping voltas - same end
      void repeat64() { repeat("repeat64.mscx", "1;2;3;7; 1;2;3;4;5;6;7; 1;2;3;7; 1;7"); } // overlapping voltas

      void repeat65() { repeat("repeat65.mscx", "1;2;3;4; 2;3;4; 1;2; 5"); } // Jump at volta end with end repeat
      void repeat66() { repeat("repeat66.mscx", "1;2;3;4; 2;3;4; 1;2; 5; 1;2;3;4; 2;3;4; 2; 5"); } // final repeat and new section

      void repeat67() { repeat("repeat67.mscx", "1;2; 1"); } // Jump at skipped open volta end with end repeat at end of score: #327681
      };

//---------------------------------------------------------
//   initTestCase
//---------------------------------------------------------

void TestRepeat::initTestCase()
      {
      initMTest();
      }

//---------------------------------------------------------
//   repeat
//---------------------------------------------------------

void TestRepeat::repeat(const char* f1, const QString & ref)
      {
      MasterScore* score = readScore(DIR + f1);
      QVERIFY(score);
      score->setExpandRepeats(true);
      QStringList sl;
      for (const RepeatSegment* rs : score->repeatList()) {
            int startTick  = rs->tick;
            int endTick    = startTick + rs->len();
            for (Measure* m = score->tick2measure(Fraction::fromTicks(startTick)); m; m = m->nextMeasure()) {
                  sl.append(QString::number(m->no()+1));
                  if (m->endTick().ticks() >= endTick)
                        break;
                  }
            }
      QString s = sl.join(";");
      QString ref1 = ref;
      ref1.replace(" ","");
      qDebug("File <%s> sequence %s", f1, qPrintable(s));
      QCOMPARE(s, ref1);
      delete score;
      }


// Keep notation independent of the Play Repeats preference. Expected edges are
// non-linear transitions only, in first-encounter order, without duplicates.
void TestRepeat::notationRoutes_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<QString>("edges");
      QTest::newRow("ordinary-repeat") << "repeat01.mscx" << "3>2";
      QTest::newRow("second-ending") << "repeat06.mscx" << "2>4;3>2";
      QTest::newRow("dc-al-fine") << "repeat07.mscx" << "6>1";
      QTest::newRow("ds-al-coda") << "repeat08.mscx" << "4>7;6>2";
      QTest::newRow("deduplicate") << "repeat15.mscx" << "2>2";
      QTest::newRow("no-transition") << "repeat57.mscx" << "";
      }

void TestRepeat::notationRoutes()
      {
      QFETCH(QString, file);
      QFETCH(QString, edges);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      score->setExpandRepeats(false);
      QStringList actual;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (Measure* target : followingRepeatMeasures(m)) {
                  actual << QString("%1>%2").arg(m->no() + 1).arg(target->no() + 1);
                  const auto incoming = precedingRepeatMeasures(target);
                  QVERIFY(std::find(incoming.begin(), incoming.end(), m) != incoming.end());
                  }
            }
      QCOMPARE(actual.join(";"), edges);
      QVERIFY(!score->expandRepeats());
      // Reading the ordinary playback list must still produce a flat score.
      QCOMPARE(score->repeatList().size(), 1);
      delete score;
      }

void TestRepeat::slurKeyboardNavigation_data()
      {
      QTest::addColumn<QString>("file");
      QTest::newRow("repeat") << QString("partial-slur-S01.mscx");
      QTest::newRow("system-breaks") << QString("partial-slur-S04.mscx");
      }

void TestRepeat::slurKeyboardNavigation()
      {
      QFETCH(QString, file);
      std::unique_ptr<MasterScore> score(readScore(DIR + file));
      QVERIFY(score);
      Slur* slur = nullptr;
      for (const auto& entry : score->spanner()) {
            if (entry.second->isSlur() && toSlur(entry.second)->isIncoming()) {
                  slur = toSlur(entry.second);
                  break;
                  }
            }
      QVERIFY(slur);
      ChordRest* first = slur->startCR();
      QCOMPARE(Slur::boundaryAnchor(first->measure(), first->track(), true), first);
      slur->setEndElement(first);
      slur->setTick2(first->tick());
      score->setLayoutAll();
      score->doLayout();
      SlurKeyboardView view;
      view.setScore(score.get());
      auto key = [&](Grip grip, int keyCode) {
            EditData ed(&view);
            ed.key = keyCode;
            ed.modifiers = Qt::ShiftModifier;
            ed.curGrip = grip;
            SlurSegment* segment = grip == Grip::START ? slur->frontSegment() : slur->backSegment();
            score->startCmd();
            const bool handled = segment->edit(ed);
            score->endCmd();
            return handled;
            };
      QVERIFY(key(Grip::END, Qt::Key_Right));
      QCOMPARE(slur->endCR(), nextChordRest(first));
      QCOMPARE(slur->partialDirection(), 0);
      QVERIFY(key(Grip::END, Qt::Key_Left));
      QCOMPARE(slur->endCR(), first); // contraction to one note
      QCOMPARE(slur->partialDirection(), 0);
      score->undoRedo(true, nullptr);
      QCOMPARE(slur->endCR(), nextChordRest(first));
      score->undoRedo(false, nullptr);
      QCOMPARE(slur->endCR(), first);
      QVERIFY(key(Grip::END, Qt::Key_Left));
      QCOMPARE(slur->endCR(), first); // cannot cross the opposite endpoint

      int steps = 0;
      while (!slur->isOutgoing() && steps++ < 40)
            QVERIFY(key(Grip::END, Qt::Key_Right));
      QVERIFY(slur->isOutgoing());
      QCOMPARE(slur->partialDirection(), 2);
      ChordRest* last = slur->endCR();
      QCOMPARE(Slur::boundaryAnchor(last->measure(), last->track(), false), last);
      ChordRest* next = nextChordRest(last);
      QVERIFY(next);
      QVERIFY(key(Grip::END, Qt::Key_Right));
      QCOMPARE(slur->endCR(), next); // boundary -> next written note
      QCOMPARE(slur->partialDirection(), 0);
      score->undoRedo(true, nullptr);
      QCOMPARE(slur->endCR(), last);
      QCOMPARE(slur->partialDirection(), 2);
      score->undoRedo(false, nullptr);
      QCOMPARE(slur->endCR(), next);
      QCOMPARE(slur->partialDirection(), 0);
      QVERIFY(key(Grip::END, Qt::Key_Left));
      QCOMPARE(slur->endCR(), last); // previous boundary is a stop again
      QCOMPARE(slur->partialDirection(), 2);
      QVERIFY(key(Grip::END, Qt::Key_Left));
      QCOMPARE(slur->endCR(), last); // boundary -> its owning note
      QCOMPARE(slur->partialDirection(), 0);
      QVERIFY(key(Grip::END, Qt::Key_Right));
      QCOMPARE(slur->partialDirection(), 2);

      ChordRest* previous = prevChordRest(first);
      QVERIFY(previous);
      QVERIFY(key(Grip::START, Qt::Key_Left));
      QCOMPARE(slur->startCR(), previous);
      QCOMPARE(slur->partialDirection(), 1);
      QVERIFY(key(Grip::START, Qt::Key_Right));
      QCOMPARE(slur->startCR(), first);
      QCOMPARE(slur->partialDirection(), 2);
      QVERIFY(key(Grip::START, Qt::Key_Right));
      QCOMPARE(slur->startCR(), first);
      QCOMPARE(slur->partialDirection(), 1);
      QVERIFY(key(Grip::START, Qt::Key_Left));
      QCOMPARE(slur->partialDirection(), 2);
      if (file == "partial-slur-S04.mscx")
            QVERIFY(view.handoffs > 0);

      QTemporaryDir directory;
      const QString saved = directory.path() + "/keyboard-slur.mscx";
      QVERIFY(saveScore(score.get(), saved));
      std::unique_ptr<MasterScore> copy(readCreatedScore(saved));
      QVERIFY(copy);
      bool found = false;
      for (const auto& entry : copy->spanner()) {
            if (entry.second->isSlur() && toSlur(entry.second)->partialDirection() == 2
                && entry.second->tick() == slur->tick() && entry.second->tick2() == slur->tick2())
                  found = true;
            }
      QVERIFY(found);
      }

void TestRepeat::slurKeyboardVolta()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "partial-slur-S02.mscx"));
      QVERIFY(score);
      Slur* slur = nullptr;
      for (const auto& entry : score->spanner()) {
            if (entry.second->isSlur() && toSlur(entry.second)->isOutgoing()) {
                  slur = toSlur(entry.second);
                  break;
                  }
            }
      QVERIFY(slur);
      ChordRest* last = slur->endCR();
      QVERIFY(!last->measure()->repeatEnd()); // skipped-volta boundary, not repeat dots
      QCOMPARE(Slur::boundaryAnchor(last->measure(), last->track(), false), last);
      ChordRest* next = nextChordRest(last);
      QVERIFY(next && next->measure() != last->measure());
      SlurKeyboardView view;
      view.setScore(score.get());
      EditData ed(&view);
      ed.curGrip = Grip::END;
      ed.modifiers = Qt::ShiftModifier;
      for (int key : { int(Qt::Key_Right), int(Qt::Key_Left) }) {
            ed.key = key;
            score->startCmd();
            QVERIFY(slur->backSegment()->edit(ed));
            score->endCmd();
            QCOMPARE(slur->endCR(), key == Qt::Key_Right ? next : last);
            QCOMPARE(slur->partialDirection(), key == Qt::Key_Right ? -1 : 1);
            }
      }

void TestRepeat::slurKeyboardNormal()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "partial-slur-S01.mscx"));
      QVERIFY(score);
      ChordRest* first = toChordRest(score->firstMeasure()->first(SegmentType::ChordRest)->element(0));
      ChordRest* second = nextChordRest(first);
      QVERIFY(!Slur::boundaryAnchor(first->measure(), 0, true));
      QVERIFY(!Slur::boundaryAnchor(first->measure(), 0, false));
      Slur* slur = new Slur(score.get());
      slur->setTrack(0);
      slur->setTrack2(0);
      slur->setStartElement(first);
      slur->setEndElement(second);
      slur->setTick(first->tick());
      slur->setTick2(second->tick());
      score->addElement(slur);
      score->doLayout();
      SlurKeyboardView view;
      view.setScore(score.get());
      EditData ed(&view);
      ed.curGrip = Grip::END;
      ed.key = Qt::Key_Left;
      ed.modifiers = Qt::ShiftModifier;
      score->startCmd();
      QVERIFY(slur->backSegment()->edit(ed));
      score->endCmd();
      QCOMPARE(slur->endCR(), second); // ordinary slur cannot collapse to zero
      QCOMPARE(slur->partialDirection(), -1);
      }

void TestRepeat::partialTieRoundTrip()
      {
      MasterScore* score = readScore(DIR + "repeat06.mscx");
      QVERIFY(score);
      Note* note = toChord(score->firstMeasure()->first(SegmentType::ChordRest)->element(0))->upNote();
      score->startCmd();
      Tie* outgoing = score->undoAddPartialTie(note, false);
      score->undoAddPartialTie(note, true);
      score->endCmd();
      QVERIFY(note->partialTie(true));
      QVERIFY(note->partialTie(false));
      QVERIFY(!note->tieFor());
      QVERIFY(!note->tieBack());
      QCOMPARE(int(outgoing->spannerSegments().size()), 1);
      MasterScore* copy = score->clone();
      QVERIFY(copy);
      Note* copiedNote = toChord(copy->firstMeasure()->first(SegmentType::ChordRest)->element(0))->upNote();
      QVERIFY(copiedNote->partialTie(true));
      QVERIFY(copiedNote->partialTie(false));
      QVERIFY(!copiedNote->tieFor());
      QVERIFY(!copiedNote->tieBack());
      QVERIFY(copiedNote->partialTie(true)->startNote() == copiedNote);
      score->undoRedo(true, nullptr);
      QVERIFY(!note->partialTie(true));
      QVERIFY(!note->partialTie(false));
      score->undoRedo(false, nullptr);
      QVERIFY(note->partialTie(true));
      QVERIFY(note->partialTie(false));
      delete copy;
      delete score;
      }

// Reference cases from the paired 37E/4.7.5 comparison suite.
void TestRepeat::partialTieClearance_data()
      {
      QTest::addColumn<QString>("file");
      for (int i = 1; i <= 5; ++i) {
            const QString name = QString("partial-tie-R%1.mscx").arg(i, 2, 10, QChar('0'));
            QTest::newRow(qPrintable(name)) << name;
            }
      QTest::newRow("repeat at system start") << QString("partial-tie-R01-system.mscx");
      }

void TestRepeat::partialTieClearance()
      {
      QFETCH(QString, file);
      std::unique_ptr<MasterScore> score(readScore(DIR + file));
      QVERIFY(score);
      int checked = 0;
      for (Measure* measure = score->firstMeasure(); measure; measure = measure->nextMeasure()) {
            for (Segment* seg = measure->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                  for (int track = 0; track < score->ntracks(); ++track) {
                        const Element* el = seg->element(track);
                        if (!el || !el->isChord())
                              continue;
                        for (Note* note : toChord(el)->notes()) {
                              for (bool incoming : { false, true }) {
                                    Tie* tie = note->partialTie(incoming);
                                    if (!tie)
                                          continue;
                                    QCOMPARE(int(tie->spannerSegments().size()), 1);
                                    const TieSegment* ts = tie->segmentAt(0);
                                    const qreal freeX = ts->ups(incoming ? Grip::START : Grip::END).p.x();
                                    System* system = measure->system();
                                    const Segment* header = seg->prevActive();
                                    if (incoming && measure == system->firstMeasure()
                                        && !(header && header->isStartRepeatBarLineType())) {
                                          QVERIFY(qAbs(freeX - system->firstNoteRestSegmentX(true)) < 0.001);
                                          }
                                    else {
                                          // The free end must clear the whole barline, dots included.
                                          const Segment* barSeg = incoming ? seg->prev1() : seg->next1();
                                          while (barSeg && !barSeg->isType(SegmentType::BarLineType))
                                                barSeg = incoming ? barSeg->prev1() : barSeg->next1();
                                          QVERIFY(barSeg);
                                          const Element* bar = barSeg->element(note->chord()->vStaffIdx() * VOICES);
                                          QVERIFY(bar && bar->isBarLine());
                                          const qreal x = barSeg->measure()->x() + barSeg->x() + bar->x();
                                          const qreal gap = incoming ? freeX - x - bar->shape().right()
                                                                    : x + bar->shape().left() - freeX;
                                          const qreal expected = incoming && toBarLine(bar)->barLineType() != BarLineType::START_REPEAT
                                                ? 0.65 : 0.25;
                                          QVERIFY2(qAbs(gap / score->spatium() - expected) < 0.001,
                                                qPrintable(QString("free-end clearance: %1 sp, expected %2 sp").arg(gap / score->spatium()).arg(expected)));
                                          }
                                    ++checked;
                                    }
                              }
                        }
                  }
            }
      QVERIFY(checked > 0);
      }

void TestRepeat::partialTieOffsets()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "repeat06.mscx"));
      QVERIFY(score);
      Note* note = toChord(score->firstMeasure()->first(SegmentType::ChordRest)->element(0))->upNote();
      score->startCmd();
      score->undoAddPartialTie(note, true);
      score->undoAddPartialTie(note, false);
      score->endCmd();
      const Grip grips[] = { Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END };
      const QPointF offsets[] = { QPointF(-2.5, 0), QPointF(-0.5, 0.75), QPointF(0.25, -0.5), QPointF(1, 0.25) };
      for (bool incoming : { false, true }) {
            Tie* tie = note->partialTie(incoming);
            for (int i = 0; i < 4; ++i)
                  tie->segmentAt(0)->ups(grips[i]).off = offsets[i] * score->spatium();
            tie->layoutPartial(note->chord()->measure()->system());
            for (int i = 0; i < 4; ++i)
                  QCOMPARE(tie->segmentAt(0)->ups(grips[i]).off, offsets[i] * score->spatium());
            }
      QTemporaryDir temporary;
      QVERIFY(temporary.isValid());
      const QString path = temporary.filePath("partial-tie-offsets.mscx");
      QVERIFY(saveScore(score.get(), path));
      std::unique_ptr<MasterScore> reloaded(readCreatedScore(path));
      QVERIFY(reloaded);
      Note* restored = toChord(reloaded->firstMeasure()->first(SegmentType::ChordRest)->element(0))->upNote();
      for (bool incoming : { false, true }) {
            Tie* tie = restored->partialTie(incoming);
            QVERIFY(tie);
            QCOMPARE(int(tie->spannerSegments().size()), 1);
            for (int i = 0; i < 4; ++i)
                  QVERIFY((tie->segmentAt(0)->ups(grips[i]).off / reloaded->spatium() - offsets[i]).manhattanLength() < 0.001);
            }
      }

// R05's ordinary ties identify the repeat sources and control placement. Partial ties use
// Studio-style attachments rather than the legacy ordinary-tie geometry.
// Check physical head clearance and save/reopen stability with extra space.
void TestRepeat::partialTieChordPlacement_data()
      {
      QTest::addColumn<double>("leadingSpace");
      QTest::addColumn<int>("direction");
      for (double space : { 0.0, 12.0 }) {
            for (Direction dir : { Direction::AUTO, Direction::UP, Direction::DOWN }) {
                  const QByteArray name = QString("space-%1-direction-%2").arg(space).arg(int(dir)).toLatin1();
                  QTest::newRow(name.constData()) << space << int(dir);
                  }
            }
      }

void TestRepeat::partialTieChordPlacement()
      {
      QFETCH(double, leadingSpace);
      QFETCH(int, direction);
      std::unique_ptr<MasterScore> score(readScore(DIR + "partial-tie-R05.mscx"));
      QVERIFY(score);
      Measure* landing = score->firstMeasure()->nextMeasure();
      Measure* linear = landing->nextMeasure()->nextMeasure()->nextMeasure();
      score->startCmd();
      landing->first(SegmentType::ChordRest)->setExtraLeadingSpace(Spatium(leadingSpace));
      for (int track = 0; track < score->ntracks(); ++track) {
            Element* el = landing->first(SegmentType::ChordRest)->element(track);
            if (!el || !el->isChord())
                  continue;
            Chord* reference = toChord(linear->first(SegmentType::ChordRest)->element(track));
            QVERIFY(reference);
            for (Note* note : toChord(el)->notes()) {
                  QVERIFY(note->partialTie(true));
                  score->undoAddPartialTie(note, false);
                  for (bool incoming : { false, true })
                        note->partialTie(incoming)->setSlurDirection(Direction(direction));
                  for (Note* ref : reference->notes()) {
                        if (ref->pitch() == note->pitch()) {
                              QVERIFY(ref->tieBack());
                              ref->tieBack()->setSlurDirection(Direction(direction));
                              }
                        }
                  }
            }
      score->setLayoutAll();
      score->endCmd();

      QTemporaryDir temporary;
      QVERIFY(temporary.isValid());
      std::map<std::tuple<int, int, bool>, QPointF> originalAnchors;
      for (int pass = 0; pass < 2; ++pass) {
            landing = score->firstMeasure()->nextMeasure();
            linear = landing->nextMeasure()->nextMeasure()->nextMeasure();
            QCOMPARE(landing->first(SegmentType::ChordRest)->extraLeadingSpace().val(), leadingSpace);
            int checked = 0;
            for (int track = 0; track < score->ntracks(); ++track) {
                  Element* el = landing->first(SegmentType::ChordRest)->element(track);
                  if (!el || !el->isChord())
                        continue;
                  Chord* reference = toChord(linear->first(SegmentType::ChordRest)->element(track));
                  QVERIFY(reference);
                  for (Note* note : toChord(el)->notes()) {
                        Note* refNote = nullptr;
                        for (Note* ref : reference->notes()) {
                              if (ref->pitch() == note->pitch())
                                    refNote = ref;
                              }
                        QVERIFY(refNote && refNote->tieBack());
                        Tie* normal = refNote->tieBack();
                        for (bool incoming : { false, true }) {
                              Tie* partial = note->partialTie(incoming);
                              QVERIFY(partial);
                              QCOMPARE(int(partial->slurDirection()), direction);
                              if (direction == int(Direction::AUTO)) {
                                    const Note* directionNote = incoming ? normal->startNote() : note;
                                    QVERIFY(directionNote);
                                    if (incoming) {
                                          const auto destinations = repeatTieDestinations(const_cast<Note*>(directionNote));
                                          QVERIFY(std::find(destinations.begin(), destinations.end(), note) != destinations.end());
                                          }
                                    const Chord* directionChord = directionNote->chord();
                                    // Every R05 staff has visible concurrent voices.
                                    // Auto follows the owner's stem for outgoing ties,
                                    // and the enabled repeat-source stem for incoming.
                                    QVERIFY(directionChord->measure()->hasVoices(directionChord->staffIdx(),
                                          directionChord->tick(), directionChord->actualTicks()));
                                    QVERIFY2(partial->up() == directionChord->up(),
                                          qPrintable(QString("Auto direction: track %1 pitch %2 incoming %3 source tick %4 tie up %5 stem up %6")
                                                .arg(track).arg(note->pitch()).arg(incoming).arg(directionNote->tick().ticks())
                                                .arg(partial->up()).arg(directionChord->up())));
                                    }
                              else
                                    QCOMPARE(partial->up(), direction == int(Direction::UP));
                              QCOMPARE(partial->isInside(), normal->isInside());
                              const Grip grip = incoming ? Grip::END : Grip::START;
                              const TieSegment* partialSegment = partial->segmentAt(0);
                              QVERIFY(partialSegment);
                              const auto origin = [](const Note* n) {
                                    const Chord* c = n->chord();
                                    return n->pos() + c->pos() + c->segment()->pos() + c->measure()->pos();
                                    };
                              const QPointF partialAnchor = partialSegment->ups(grip).p - origin(note);
                              QVERIFY(partialSegment->ups(Grip::END).p.x() > partialSegment->ups(Grip::START).p.x());
                              QVERIFY(std::isfinite(partialAnchor.x()) && std::isfinite(partialAnchor.y()));
                              const qreal sp = score->spatium();
                              if (partial->isInside()) {
                                    // Inside attaches beyond the appropriate head edge,
                                    // including when a second voice shifts the head.
                                    if (incoming)
                                          QVERIFY(partialAnchor.x() <= -.19 * sp);
                                    else
                                          QVERIFY(partialAnchor.x() >= note->headWidth() + .19 * sp);
                                    }
                              const Segment* segment = note->chord()->segment();
                              const int firstTrack = note->chord()->staffIdx() * VOICES;
                              QRectF headCluster = note->bbox();
                              for (int voiceTrack = firstTrack; voiceTrack < firstTrack + VOICES; ++voiceTrack) {
                                    const Element* otherElement = segment->element(voiceTrack);
                                    if (!otherElement || !otherElement->isChord())
                                          continue;
                                    for (const Note* other : toChord(otherElement)->notes()) {
                                          if (!other->visible())
                                                continue;
                                          // Use actual ink bounds, independently of the
                                          // attachment helper being tested. Trim only the
                                          // rectangle edge to avoid boundary roundoff.
                                          const QPointF offset = origin(other) - origin(note);
                                          const QRectF bounds = other->bbox().translated(offset);
                                          headCluster |= bounds;
                                          if (other->accidental() && other->accidental()->visible())
                                                headCluster |= other->accidental()->bbox().translated(offset + other->accidental()->pos());
                                          const QRectF ink = bounds.adjusted(.001 * sp, .001 * sp, -.001 * sp, -.001 * sp);
                                          QVERIFY2(!ink.contains(partialAnchor),
                                                qPrintable(QString("track %1 pitch %2 incoming %3 overlaps head %4: anchor (%5,%6), bounds (%7,%8,%9,%10)")
                                                      .arg(track).arg(note->pitch()).arg(incoming).arg(other->pitch())
                                                      .arg(partialAnchor.x()).arg(partialAnchor.y())
                                                      .arg(ink.x()).arg(ink.y()).arg(ink.width()).arg(ink.height())));
                                          }
                                    }
                              // Leading space moves the chord, not its local attachment.
                              // Retain an upper bound so a wrong coordinate origin or a
                              // needlessly displaced anchor cannot satisfy clearance alone.
                              QVERIFY(partialAnchor.x() >= headCluster.left() - .5 * sp);
                              QVERIFY(partialAnchor.x() <= headCluster.right() + .5 * sp);
                              // Inside/outside vertical placement must stay on the correct
                              // side of its note, including the down-stem second voice.
                              QVERIFY(partial->up() ? partialAnchor.y() < 0 : partialAnchor.y() > 0);
                              const auto key = std::make_tuple(track, note->pitch(), incoming);
                              if (pass == 0)
                                    originalAnchors.emplace(key, partialAnchor);
                              else {
                                    const auto found = originalAnchors.find(key);
                                    QVERIFY(found != originalAnchors.end());
                                    QVERIFY2((partialAnchor - found->second).manhattanLength() < .001 * sp,
                                          qPrintable(QString("round trip: track %1 pitch %2 incoming %3 before (%4,%5) after (%6,%7)")
                                                .arg(track).arg(note->pitch()).arg(incoming)
                                                .arg(found->second.x()).arg(found->second.y()).arg(partialAnchor.x()).arg(partialAnchor.y())));
                                    }
                              ++checked;
                              }
                        }
                  }
            QCOMPARE(checked, 14); // seven notes, incoming and outgoing
            if (pass == 0) {
                  const QString path = temporary.filePath("chord-partial-ties.mscx");
                  QVERIFY(saveScore(score.get(), path));
                  score.reset(readCreatedScore(path));
                  QVERIFY(score);
                  }
            }
      }

void TestRepeat::repeatTieNotes()
      {
      MasterScore* score = readScore(DIR + "repeat06.mscx");
      QVERIFY(score);
      std::vector<Note*> notes;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            Note* n = toChord(m->first(SegmentType::ChordRest)->element(0))->upNote();
            n->setPitch(60);
            notes.push_back(n);
            }
      auto toEnding = repeatTieDestinations(notes[1]);
      QCOMPARE(int(toEnding.size()), 1);
      QCOMPARE(toEnding.front(), notes[3]);
      auto toRepeat = repeatTieDestinations(notes[2]);
      QCOMPARE(int(toRepeat.size()), 1);
      QCOMPARE(toRepeat.front(), notes[1]);
      notes[3]->setPitch(61);
      QVERIFY(repeatTieDestinations(notes[1]).empty());
      delete score;
      }

void TestRepeat::automaticRepeatTies()
      {
      MasterScore* score = readScore(DIR + "repeat06.mscx");
      QVERIFY(score);
      std::vector<Note*> notes;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            Note* n = toChord(m->first(SegmentType::ChordRest)->element(0))->upNote();
            n->setPitch(60);
            notes.push_back(n);
            }
      score->setExpandRepeats(false);
      score->select(notes[1]);
      score->cmdAddTie();
      QVERIFY(notes[1]->tieFor());
      QCOMPARE(notes[1]->tieFor()->endNote(), notes[2]);
      QVERIFY(notes[3]->partialTie(true));
      QVERIFY(!notes[1]->partialTie(false));
      QVERIFY(!score->expandRepeats());
      // Deleting the source tie also removes its second-ending endpoint.
      score->startCmd();
      score->undoRemoveElement(notes[1]->tieFor());
      score->endCmd();
      QVERIFY(!notes[3]->partialTie(true));
      score->undoRedo(true, nullptr);
      QVERIFY(notes[1]->tieFor());
      QVERIFY(notes[3]->partialTie(true));
      score->undoRedo(true, nullptr);
      QVERIFY(!notes[1]->tieFor());
      QVERIFY(!notes[3]->partialTie(true));
      score->undoRedo(false, nullptr);
      QVERIFY(notes[1]->tieFor());
      QVERIFY(notes[3]->partialTie(true));
      score->select(notes[2]);
      score->cmdAddTie();
      // First ending is never played into second ending: use a hanging tie.
      QVERIFY(!notes[2]->tieFor());
      QVERIFY(notes[2]->partialTie(false));
      QVERIFY(notes[1]->partialTie(true));
      score->startCmd();
      score->undoRemoveElement(notes[2]->chord());
      score->endCmd();
      QVERIFY(!notes[1]->partialTie(true));
      QVERIFY(!notes[2]->partialTie(false));
      QVERIFY(!notes[3]->partialTie(true));
      score->undoRedo(true, nullptr);
      QVERIFY(notes[1]->partialTie(true));
      QVERIFY(notes[2]->partialTie(false));
      QVERIFY(notes[3]->partialTie(true));
      MasterScore* copy = score->clone();
      QVERIFY(copy);
      Note* copiedFirstEnding = toChord(copy->firstMeasure()->nextMeasure()->nextMeasure()->first(SegmentType::ChordRest)->element(0))->upNote();
      QVERIFY(copiedFirstEnding->partialTie(false));
      QVERIFY(!copiedFirstEnding->tieFor());
      delete copy;
      delete score;
      }

void TestRepeat::repeatTiePlayback()
      {
      MasterScore* score = readScore(DIR + "repeat06.mscx");
      QVERIFY(score);
      std::vector<Note*> notes;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            Note* n = toChord(m->first(SegmentType::ChordRest)->element(0))->upNote();
            n->setPitch(60);
            notes.push_back(n);
            }
      score->setExpandRepeats(true);
      score->select(notes[1]);
      score->cmdAddTie();
      score->select(notes[2]);
      score->cmdAddTie();
      const int bar = notes[0]->chord()->actualTicks().ticks();
      QCOMPARE(nextTiedNoteInPlayback(notes[1], bar), static_cast<const Note*>(notes[2]));
      QCOMPARE(nextTiedNoteInPlayback(notes[2], bar * 2), static_cast<const Note*>(notes[1]));
      QCOMPARE(nextTiedNoteInPlayback(notes[1], bar * 3), static_cast<const Note*>(notes[3]));
      QVERIFY(!hasPreviousTiedNoteInPlayback(notes[1], bar));
      QVERIFY(hasPreviousTiedNoteInPlayback(notes[1], bar * 3));
      QVERIFY(hasPreviousTiedNoteInPlayback(notes[3], bar * 4));
      score->startCmd();
      score->undoRemoveElement(notes[3]->partialTie(true));
      score->endCmd();
      QVERIFY(!nextTiedNoteInPlayback(notes[1], bar * 3));
      QVERIFY(!hasPreviousTiedNoteInPlayback(notes[3], bar * 4));
      score->setExpandRepeats(false);
      QVERIFY(!nextTiedNoteInPlayback(notes[2], bar * 2));
      delete score;
      }

void TestRepeat::partialTieAttackPlayback_data()
      {
      QTest::addColumn<QString>("format");
      QTest::addColumn<bool>("expandRepeats");
      QTest::addColumn<bool>("incomingTies");
      QTest::addColumn<bool>("outgoingTies");
      for (const QString& format : { QString("37E"), QString("475") }) {
            for (bool expandRepeats : { false, true }) {
                  for (bool incomingTies : { false, true }) {
                        for (bool outgoingTies : { false, true })
                              QTest::newRow(qPrintable(QString("%1-%2-%3-%4").arg(format)
                                    .arg(expandRepeats ? "expanded" : "linear").arg(incomingTies ? "incoming" : "no-incoming")
                                    .arg(outgoingTies ? "outgoing" : "no-outgoing")))
                                    << format << expandRepeats << incomingTies << outgoingTies;
                        }
                  }
            }
      }

void TestRepeat::partialTieAttackPlayback()
      {
      QFETCH(QString, format);
      QFETCH(bool, expandRepeats);
      QFETCH(bool, incomingTies);
      QFETCH(bool, outgoingTies);
      const QString path = root + "/libmscore/spanners/manual-tie-properties/v0.15-C-hanging-ties-"
            + format + ".mscx";
      QFile input(path);
      QVERIFY(input.open(QIODevice::ReadOnly));
      QByteArray xml = input.readAll();
      if (!outgoingTies) {
            QDomDocument document;
            QVERIFY(document.setContent(xml));
            const QDomNodeList ties = document.elementsByTagName("PartialTie");
            for (int i = ties.count() - 1; i >= 0; --i) {
                  const QDomElement tie = ties.at(i).toElement();
                  if (tie.firstChildElement("partialSpannerDirection").text() == "outgoing")
                        tie.parentNode().removeChild(tie);
                  }
            xml = document.toByteArray();
            }
      QBuffer buffer(&xml);
      QVERIFY(buffer.open(QIODevice::ReadOnly));
      std::unique_ptr<MasterScore> score(new MasterScore(MScore::defaultStyle()));
      // The ordinary MTest loader deliberately rejects newer versions.
      // Opt into the backport reader for the native 4.70 comparison fixture.
      QCOMPARE(int(score->loadMsc(path, &buffer, format == "475")), int(Score::FileError::FILE_NO_ERROR));
      score->doLayout();
      int incomingCount = 0;
      if (!incomingTies)
            score->startCmd();
      for (Measure* measure = score->firstMeasure(); measure; measure = measure->nextMeasure()) {
            for (Segment* segment = measure->first(SegmentType::ChordRest); segment; segment = segment->next(SegmentType::ChordRest)) {
                  Element* element = segment->element(0);
                  if (!element || !element->isChord())
                        continue;
                  for (Note* note : toChord(element)->notes()) {
                        if (Tie* tie = note->partialTie(true)) {
                              ++incomingCount;
                              if (!incomingTies)
                                    score->undoRemoveElement(tie);
                              }
                        }
                  }
            }
      QCOMPARE(incomingCount, 9);
      if (!incomingTies)
            score->endCmd();
      EventMap events;
      SynthesizerState synthState;
      score->renderMidi(&events, false, expandRepeats, synthState);
      using EventCounts = std::map<std::pair<int, int>, int>;
      EventCounts attacks, releases, expectedAttacks, expectedReleases;
      for (const auto& timedEvent : events) {
            const NPlayEvent& event = timedEvent.second;
            if (event.discard() || (event.type() != ME_NOTEON && event.type() != ME_NOTEOFF))
                  continue;
            EventCounts& counts = event.type() == ME_NOTEON && event.velo() > 0 ? attacks : releases;
            ++counts[{ timedEvent.first, event.pitch() }];
            }
      auto expectChord = [&](int quarter, int duration) {
            for (int pitch : { 60, 64, 67 }) {
                  ++expectedAttacks[{ quarter * DIVISION, pitch }];
                  ++expectedReleases[{ (quarter + duration) * DIVISION - 1, pitch }];
                  }
            };
      const int passes = expandRepeats ? 2 : 1;
      const bool connected = incomingTies && outgoingTies;
      for (int group = 0; group < 3; ++group) {
            for (int pass = 0; pass < passes; ++pass) {
                  const int quarter = (group * passes + pass) * 8;
                  // Only the score's opening C2 has no preceding source.
                  // C4/C6's first arrivals continue forward from C1/C3;
                  // later repeat arrivals continue across the backward jump.
                  if (!connected || (group == 0 && pass == 0))
                        expectChord(quarter, 2);
                  const bool continues = connected && (pass + 1 < passes || group + 1 < 3);
                  expectChord(quarter + 6, continues ? 4 : 2);
                  }
            }
      auto describe = [](const EventCounts& counts) {
            QStringList result;
            for (const auto& entry : counts)
                  result << QString("%1:%2x%3").arg(entry.first.first).arg(entry.first.second).arg(entry.second);
            return result.join(", ");
            };
      QVERIFY2(attacks == expectedAttacks,
               qPrintable(QString("attacks [%1], expected [%2]").arg(describe(attacks), describe(expectedAttacks))));
      QVERIFY2(releases == expectedReleases,
               qPrintable(QString("releases [%1], expected [%2]").arg(describe(releases), describe(expectedReleases))));
      }

void TestRepeat::partialLyricsLayout()
      {
      for (int direction : { 0, 1, 2 }) {
            for (bool melisma : { false, true }) {
                  std::unique_ptr<MasterScore> score(readScore(DIR + "repeat06.mscx"));
                  QVERIFY(score);
                  Measure* measure = score->firstMeasure()->nextMeasure()->nextMeasure()->nextMeasure();
                  Chord* chord = toChord(measure->first(SegmentType::ChordRest)->element(0));
                  Lyrics* lyric = new Lyrics(score.get());
                  lyric->setParent(chord);
                  lyric->setTrack(0);
                  lyric->setProperty(Pid::PARTIAL_SPANNER_DIRECTION, direction);
                  lyric->setProperty(Pid::PARTIAL_LYRIC_MELISMA, melisma);
                  if (direction == 1)
                        lyric->setXmlText("jump");
                  score->startCmd();
                  score->undoAddElement(lyric);
                  score->endCmd();
                  LyricsLine* line = nullptr;
                  for (Spanner* spanner : score->unmanagedSpanners()) {
                        if (spanner->isLyricsLine() && spanner->parent() == lyric)
                              line = toLyricsLine(spanner);
                        }
                  QVERIFY(line);
                  QVERIFY(!line->spannerSegments().empty());
                  SpannerSegment* segment = line->frontSegment();
                  QVERIFY(segment->bbox().width() > 0);
                  QVERIFY(segment->selectable());
                  QVERIFY(!segment->generated());
                  std::unique_ptr<MasterScore> copy(score->clone());
                  QVERIFY(copy);
                  score->startCmd();
                  score->deleteItem(segment);
                  score->endCmd();
                  if (direction == 1) {
                        QVERIFY(!lyric->isPartial());
                        QCOMPARE(lyric->plainText(), QString("jump"));
                        }
                  else
                        QVERIFY(chord->lyrics().empty());
                  score->undoRedo(true, nullptr);
                  QVERIFY(!chord->lyrics().empty());
                  QVERIFY(chord->lyrics().front()->isPartial());
                  }
            }
      }

void TestRepeat::partialLyricsRendering_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<int>("expectedLines");
      QTest::addColumn<bool>("native");
      QTest::newRow("L01-37E") << QString("partial-lyrics-L01.mscx") << 2 << false;
      QTest::newRow("L01-475") << QString("partial-lyrics-L01-475.mscx") << 2 << true;
      QTest::newRow("L02-37E") << QString("partial-lyrics-L02.mscx") << 2 << false;
      QTest::newRow("L02-475") << QString("partial-lyrics-L02-475.mscx") << 2 << true;
      QTest::newRow("L03-37E") << QString("partial-lyrics-L03.mscx") << 6 << false;
      QTest::newRow("L03-475") << QString("partial-lyrics-L03-475.mscx") << 6 << true;
      QTest::newRow("L04-37E") << QString("partial-lyrics-L04.mscx") << 3 << false;
      QTest::newRow("L04-475") << QString("partial-lyrics-L04-475.mscx") << 3 << true;
      QTest::newRow("X01-37E") << QString("partial-lyrics-X01.mscx") << 3 << false;
      QTest::newRow("X01-475") << QString("partial-lyrics-X01-475.mscx") << 3 << true;
      QTest::newRow("L04-475-compressed") << QString("partial-lyrics-L04-475.mscz") << 3 << true;
      QTest::newRow("X01-475-compressed") << QString("partial-lyrics-X01-475.mscz") << 3 << true;
      }

void TestRepeat::partialLyricsRendering()
      {
      QFETCH(QString, file);
      QFETCH(int, expectedLines);
      QFETCH(bool, native);
      std::unique_ptr<MasterScore> score;
      if (native) {
            score.reset(new MasterScore(MScore::defaultStyle()));
            QCOMPARE(int(score->loadMsc(root + "/" + DIR + file, true)), int(Score::FileError::FILE_NO_ERROR));
            score->doLayout();
            }
      else
            score.reset(readScore(DIR + file));
      QVERIFY(score);
      const auto content = [](MasterScore* value) {
            QStringList result;
            for (Measure* m = value->firstMeasure(); m; m = m->nextMeasure()) {
                  result << QString("measure:%1:%2").arg(m->tick().toString()).arg(m->ticks().toString());
                  for (Segment* seg = m->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                        for (int track = 0; track < value->ntracks(); ++track) {
                              Element* item = seg->element(track);
                              if (!item || !item->isChordRest())
                                    continue;
                              ChordRest* cr = toChordRest(item);
                              const QString start = QString("%1:%2:%3:").arg(track).arg(cr->tick().toString()).arg(cr->actualTicks().toString());
                              if (item->isChord()) {
                                    for (Note* note : toChord(item)->notes())
                                          result << start + QString::number(note->pitch());
                                    }
                              else
                                    result << start + "rest";
                              }
                        }
                  }
            return result;
            };
      const auto lyricProperties = [](MasterScore* value) {
            QStringList result;
            for (Measure* m = value->firstMeasure(); m; m = m->nextMeasure()) {
                  for (Segment* seg = m->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                        for (int track = 0; track < value->ntracks(); ++track) {
                              Element* item = seg->element(track);
                              if (item && item->isChordRest()) {
                                    for (Lyrics* lyric : toChordRest(item)->lyrics()) {
                                          if (lyric->isPartial())
                                                result << QString("%1:%2:%3:%4:%5:%6:%7:%8").arg(track).arg(lyric->tick().reduced().toString())
                                                      .arg(lyric->no()).arg(int(lyric->placement())).arg(lyric->partialDirection())
                                                      .arg(lyric->partialMelisma()).arg(lyric->ticks().reduced().toString()).arg(lyric->plainText());
                                          }
                                    }
                              }
                        }
                  }
            result.sort();
            return result;
            };
      QStringList expectedProperties = lyricProperties(score.get());
      if (native) {
            std::unique_ptr<MasterScore> reference(readScore(DIR + file.left(file.indexOf("-475")) + ".mscx"));
            QVERIFY(reference);
            QCOMPARE(content(score.get()), content(reference.get()));
            expectedProperties = lyricProperties(reference.get());
            }
      QTemporaryDir temporary;
      QVERIFY(temporary.isValid());
      for (int pass = 0; pass < 2; ++pass) {
            QCOMPARE(lyricProperties(score.get()), expectedProperties);
            int checked = 0;
            for (Spanner* sp : score->unmanagedSpanners()) {
                  if (!sp->isLyricsLine())
                        continue;
                  LyricsLine* line = toLyricsLine(sp);
                  if (!line->lyrics()->isPartial())
                        continue;
                  QVERIFY(!line->spannerSegments().empty());
                  for (SpannerSegment* ss : line->spannerSegments()) {
                        LyricsLineSegment* seg = toLyricsLineSegment(ss);
                        QVERIFY(seg->system());
                        QVERIFY2(seg->pos2().x() > 0, "Partial lyric line has no drawable horizontal span");
                        QVERIFY(line->lineWidth() > 0);
                        std::vector<Element*> rendered;
                        seg->system()->scanElements(&rendered, [](void* data, Element* item) {
                              static_cast<std::vector<Element*>*>(data)->push_back(item);
                              }, false);
                        QVERIFY(std::find(rendered.begin(), rendered.end(), seg) != rendered.end());
                        QImage image(512, 64, QImage::Format_ARGB32_Premultiplied);
                        image.fill(Qt::transparent);
                        QPainter painter(&image);
                        painter.translate(8, 32);
                        painter.scale(496.0 / seg->pos2().x(), 1.0);
                        seg->draw(&painter);
                        painter.end();
                        bool ink = false;
                        for (int y = 0; y < image.height() && !ink; ++y) {
                              for (int x = 0; x < image.width(); ++x) {
                                    if (qAlpha(image.pixel(x, y))) {
                                          ink = true;
                                          break;
                                          }
                                    }
                              }
                        QVERIFY2(ink, "Partial lyric segment exists but paints no extender or dashes");
                        }
                  ++checked;
                  }
            QCOMPARE(checked, expectedLines);
            if (pass == 0) {
                  const QString path = temporary.filePath("partial-lyrics.mscx");
                  QVERIFY(saveScore(score.get(), path));
                  score.reset(readCreatedScore(path));
                  QVERIFY(score);
                  }
            }
      }

void TestRepeat::nativeLyricsWithoutRepeat()
      {
      QFile input(root + "/" + DIR + "partial-lyrics-X01-475.mscx");
      QVERIFY(input.open(QIODevice::ReadOnly));
      QDomDocument document;
      QVERIFY(document.setContent(input.readAll()));
      for (const QString& tag : { QString("Spanner"), QString("startRepeat"), QString("endRepeat") }) {
            QDomNodeList nodes = document.elementsByTagName(tag);
            while (nodes.count() > 0) {
                  QDomNode node = nodes.at(0);
                  node.parentNode().removeChild(node);
                  }
            }
      QTemporaryDir temporary;
      QVERIFY(temporary.isValid());
      const QString path = temporary.filePath("ordinary-native-lyrics.mscx");
      QFile output(path);
      QVERIFY(output.open(QIODevice::WriteOnly));
      QVERIFY(output.write(document.toByteArray()) > 0);
      output.close();
      std::unique_ptr<MasterScore> score(new MasterScore(MScore::defaultStyle()));
      QCOMPARE(int(score->loadMsc(path, true)), int(Score::FileError::FILE_NO_ERROR));
      int checked = 0;
      bool sawHold = false;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (Segment* seg = m->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                  for (int track = 0; track < score->ntracks(); ++track) {
                        Element* item = seg->element(track);
                        if (item && item->isChordRest()) {
                              for (Lyrics* lyric : toChordRest(item)->lyrics()) {
                                    QVERIFY(!lyric->isPartial());
                                    if (lyric->plainText() == "hold") {
                                          QCOMPARE(lyric->ticks(), Fraction::fromTicks(Lyrics::TEMP_MELISMA_TICKS));
                                          sawHold = true;
                                          }
                                    ++checked;
                                    }
                              }
                        }
                  }
            }
      QVERIFY(checked > 0);
      QVERIFY(sawHold);
      }

void TestRepeat::partialLyricsMelismaEnd_data()
      {
      QTest::addColumn<QString>("file");
      QTest::newRow("L04-above-over-breaks") << QString("partial-lyrics-L04.mscx");
      QTest::newRow("X01-combined") << QString("partial-lyrics-X01.mscx");
      }

void TestRepeat::partialLyricsMelismaEnd()
      {
      QFETCH(QString, file);
      std::unique_ptr<MasterScore> score(readScore(DIR + file));
      QVERIFY(score);
      int checked = 0;
      for (Spanner* sp : score->unmanagedSpanners()) {
            if (!sp->isLyricsLine())
                  continue;
            LyricsLine* line = toLyricsLine(sp);
            if (!line->lyrics()->isPartial() || !line->isEndMelisma() || line->lyrics()->isOutgoing())
                  continue;
            QVERIFY(!line->spannerSegments().empty());
            ChordRest* cr = toChordRest(line->endElement());
            QVERIFY(cr && cr->isChord());
            Chord* chord = toChord(cr);
            const Note* note = chord->up() ? chord->downNote() : chord->upNote();
            const SpannerSegment* segment = line->backSegment();
            const qreal expected = note->pagePos().x() + note->headWidth();
            const qreal actual = segment->pagePos().x() + toLyricsLineSegment(segment)->pos2().x();
            QVERIFY(qAbs(actual - expected) < 0.001);
            ++checked;
            }
      QVERIFY(checked > 0);
      }

static int paintedDashCount(const LyricsLineSegment* segment)
      {
      QImage image(1024, 64, QImage::Format_ARGB32_Premultiplied);
      image.fill(Qt::transparent);
      QPainter painter(&image);
      painter.translate(8, 32);
      painter.scale(1000.0 / qMax(qreal(1), segment->pos2().x()), 1.0);
      segment->draw(&painter);
      painter.end();
      int count = 0;
      bool previous = false;
      for (int x = 0; x < image.width(); ++x) {
            bool ink = false;
            for (int y = 0; y < image.height(); ++y)
                  ink = ink || qAlpha(image.pixel(x, y));
            if (ink && !previous)
                  ++count;
            previous = ink;
            }
      return count;
      }

void TestRepeat::partialLyricsDashCount_data()
      {
      QTest::addColumn<double>("distance");
      QTest::addColumn<int>("expected");
      QTest::newRow("L03-first-ending") << -1.0 << 2;
      QTest::newRow("short") << 0.5 << 1;
      QTest::newRow("over-one-spacing") << 1.1 << 2;
      QTest::newRow("over-two-spacings") << 2.1 << 3;
      QTest::newRow("empty") << 0.0 << 0;
      }

void TestRepeat::partialLyricsDashCount()
      {
      QFETCH(double, distance);
      QFETCH(int, expected);
      std::unique_ptr<MasterScore> score(readScore(DIR + "partial-lyrics-L03.mscx"));
      QVERIFY(score);
      LyricsLineSegment* segment = nullptr;
      for (Spanner* sp : score->unmanagedSpanners()) {
            if (!sp->isLyricsLine())
                  continue;
            LyricsLine* line = toLyricsLine(sp);
            Lyrics* lyric = line->lyrics();
            if (lyric->measure()->no() == 3 && lyric->isPartial() && !line->isEndMelisma())
                  segment = toLyricsLineSegment(line->frontSegment());
            }
      QVERIFY(segment);
      if (distance >= 0) {
            segment->setPos2(QPointF(distance * score->styleP(Sid::lyricsDashMaxDistance), 0));
            segment->layout();
            }
      QCOMPARE(paintedDashCount(segment), expected);
      }

void TestRepeat::lyricDashDimensions_data()
      {
      QTest::addColumn<bool>("native");
      QTest::newRow("37E") << false;
      QTest::newRow("475") << true;
      }

void TestRepeat::lyricDashDimensions()
      {
      QFETCH(bool, native);
      std::unique_ptr<MasterScore> score(new MasterScore(MScore::defaultStyle()));
      const QString file = native ? "partial-lyrics-L02-475.mscx" : "partial-lyrics-L02.mscx";
      QCOMPARE(int(score->loadMsc(root + "/" + DIR + file, native)), int(Score::FileError::FILE_NO_ERROR));
      score->doLayout();
      QCOMPARE(score->styleS(Sid::lyricsDashMaxLength).val(), 0.6);
      QCOMPARE(score->styleS(Sid::lyricsDashLineThickness).val(), 0.1);
      LyricsLine* line = nullptr;
      for (Spanner* sp : score->unmanagedSpanners()) {
            if (sp->isLyricsLine() && !toLyricsLine(sp)->isEndMelisma())
                  line = toLyricsLine(sp);
            }
      QVERIFY(line && !line->spannerSegments().empty());
      for (int pass = 0; pass < 2; ++pass) {
            const qreal length = pass ? 0.9 : 0.6;
            const qreal thickness = pass ? 0.22 : 0.1;
            score->setStyleValue(Sid::lyricsDashMaxLength, Spatium(length));
            score->setStyleValue(Sid::lyricsDashLineThickness, Spatium(thickness));
            line->styleChanged();
            LyricsLineSegment* seg = toLyricsLineSegment(line->frontSegment());
            seg->setPos2(QPointF(3 * score->spatium(), 0));
            seg->layout();
            QImage image(1024, 256, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            QPainter painter(&image);
            painter.translate(64, 128);
            painter.scale(8, 8);
            seg->draw(&painter);
            painter.end();
            int left = image.width(), right = -1, top = image.height(), bottom = -1;
            for (int y = 0; y < image.height(); ++y) {
                  for (int x = 0; x < image.width(); ++x) {
                        if (!qAlpha(image.pixel(x, y)))
                              continue;
                        left = qMin(left, x); right = qMax(right, x);
                        top = qMin(top, y); bottom = qMax(bottom, y);
                        }
                  }
            QVERIFY(right >= left && bottom >= top);
            QVERIFY(qAbs((right - left + 1) / 8.0 / score->spatium() - length) < 0.02);
            QVERIFY(qAbs((bottom - top + 1) / 8.0 / score->spatium() - thickness) < 0.02);
            }
      QTemporaryDir directory;
      const QString saved = directory.path() + "/dash-custom.mscx";
      QVERIFY(saveScore(score.get(), saved));
      std::unique_ptr<MasterScore> copy(readCreatedScore(saved));
      QVERIFY(copy);
      QCOMPARE(copy->styleS(Sid::lyricsDashMaxLength).val(), 0.9);
      QCOMPARE(copy->styleS(Sid::lyricsDashLineThickness).val(), 0.22);
      }

void TestRepeat::partialSlurLayout()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "repeat06.mscx"));
      QVERIFY(score);
      Measure* measure = score->firstMeasure()->nextMeasure()->nextMeasure()->nextMeasure();
      Chord* chord = toChord(measure->first(SegmentType::ChordRest)->element(0));
      Slur* slur = new Slur(score.get());
      slur->setTrack(0);
      slur->setTrack2(0);
      slur->setTick(chord->tick());
      slur->setTick2(chord->tick());
      slur->setStartElement(chord);
      slur->setEndElement(chord);
      slur->setPartialDirection(0);
      score->startCmd();
      score->undoAddElement(slur);
      score->endCmd();
      QVERIFY(!slur->spannerSegments().empty());
      QVERIFY(slur->frontSegment()->bbox().width() > 0);
      std::unique_ptr<MasterScore> copy(score->clone());
      QVERIFY(copy);
      }

static qreal paintedHeaderRight(const Element*);

void TestRepeat::partialSlurClearance_data()
      {
      QTest::addColumn<QString>("file");
      for (int i = 1; i <= 4; ++i) {
            const QString file = QString("partial-slur-S%1.mscx").arg(i, 2, 10, QChar('0'));
            QTest::newRow(qPrintable(file)) << file;
            }
      }

void TestRepeat::partialSlurClearance()
      {
      QFETCH(QString, file);
      std::unique_ptr<MasterScore> score(readScore(DIR + file));
      QVERIFY(score);
      int checked = 0;
      for (const auto& entry : score->spanner()) {
            if (!entry.second->isSlur())
                  continue;
            Slur* slur = toSlur(entry.second);
            if (slur->partialDirection() < 0)
                  continue;
            QVERIFY(!slur->spannerSegments().empty());
            if (slur->isIncoming()) {
                  const SlurSegment* seg = slur->frontSegment();
                  const Measure* measure = slur->startCR()->measure();
                  const Segment* firstCR = measure->first(SegmentType::ChordRest);
                  const Segment* header = firstCR->prevActive();
                  while (header && header->allElementsInvisible())
                        header = header->prevActive();
                  if (header) {
                        const Element* item = header->element(slur->staffIdx() * VOICES);
                        QVERIFY(item);
                        const qreal right = measure->x() + header->x() + item->x()
                              + (header->isStartRepeatBarLineType() ? paintedHeaderRight(item) : item->bbox().right());
                        const qreal margin = (header->isStartRepeatBarLineType() ? 0.25 : 1.0) * score->spatium();
                        const qreal expected = header->isStartRepeatBarLineType() ? right + margin
                              : qMin(right + margin, measure->x() + firstCR->x());
                        const qreal tolerance = header->isStartRepeatBarLineType() ? 0.04 * score->spatium() : 0.001;
                        QVERIFY(qAbs(seg->ups(Grip::START).p.x() - expected) < tolerance);
                        QVERIFY(seg->ups(Grip::START).p.x() >= right);
                        }
                  else {
                        QVERIFY(qAbs(seg->ups(Grip::START).p.x() - measure->firstNoteRestSegmentX(true)) < 0.001);
                        }
                  ++checked;
                  }
            if (slur->isOutgoing()) {
                  const SlurSegment* seg = slur->backSegment();
                  const Measure* measure = slur->endCR()->measure();
                  const BarLine* bar = measure->endBarLine();
                  QVERIFY(bar);
                  const qreal left = measure->x() + bar->segment()->x() + bar->x() + bar->bbox().left();
                  QVERIFY(qAbs(seg->ups(Grip::END).p.x() - (left - 0.25 * score->spatium())) < 0.001);
                  ++checked;
                  }
            if (slur->startCR() == slur->endCR() && slur->isIncoming() != slur->isOutgoing()) {
                  SlurPos raw;
                  slur->slurPos(&raw);
                  const qreal delta = slur->isIncoming() ? raw.p1.y() - raw.p2.y() : raw.p2.y() - raw.p1.y();
                  QVERIFY(qAbs(delta / score->spatium() - (slur->up() ? -0.25 : 0.25)) < 0.001);
                  }
            }
      QVERIFY(checked > 0);
      }

void TestRepeat::partialSlurSystemHeader()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "partial-slur-S02.mscx"));
      QVERIFY(score);
      Slur* incoming = nullptr;
      for (const auto& entry : score->spanner()) {
            if (entry.second->isSlur()) {
                  Slur* slur = toSlur(entry.second);
                  if (slur->startCR()->measure() == slur->startCR()->measure()->system()->firstMeasure())
                        incoming = slur;
                  }
            }
      QVERIFY(incoming);
      // There is no jump arrival at the first ending. Like Studio 4.7.5,
      // loading the score should have reset its incoming flag.
      QCOMPARE(incoming->partialDirection(), -1);
      // Still exercise header clearance if the user explicitly enables it.
      incoming->setPartialDirection(0);
      score->setLayoutAll();
      score->doLayout();
      const SlurSegment* seg = incoming->frontSegment();
      const qreal partialTop = seg->y() + seg->bbox().top();
      const qreal partialEndY = seg->y() + seg->ups(Grip::END).pos().y();
      // Vanilla 37E ignores the partial property and draws an ordinary slur.
      // Its curve is the control for the reported inflated first-ending slur.
      incoming->setProperty(Pid::PARTIAL_SPANNER_DIRECTION, -1);
      score->setLayoutAll();
      score->doLayout();
      seg = incoming->frontSegment();
      const qreal ordinaryTop = seg->y() + seg->bbox().top();
      const qreal ordinaryEndY = seg->y() + seg->ups(Grip::END).pos().y();
      QVERIFY(qAbs(partialEndY - ordinaryEndY) < score->spatium());
      QVERIFY(qAbs(partialTop - ordinaryTop) < score->spatium());
      }

void TestRepeat::partialSlurHeight_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<double>("scale");
      QTest::addColumn<bool>("forceDown");
      for (int i = 1; i <= 4; ++i) {
            const QString file = QString("partial-slur-S%1.mscx").arg(i, 2, 10, QChar('0'));
            for (double scale : { 0.75, 1.0, 1.5 }) {
                  for (bool forceDown : { false, true })
                        QTest::newRow(qPrintable(file + QString("-%1-%2").arg(scale).arg(forceDown ? "down" : "original")))
                              << file << scale << forceDown;
                  }
            }
      }

void TestRepeat::partialSlurHeight()
      {
      QFETCH(QString, file);
      QFETCH(double, scale);
      QFETCH(bool, forceDown);
      std::unique_ptr<MasterScore> score(readScore(DIR + file));
      QVERIFY(score);
      if (forceDown) {
            for (const auto& entry : score->spanner()) {
                  if (entry.second->isSlur() && toSlur(entry.second)->partialDirection() >= 0)
                        toSlur(entry.second)->setSlurDirection(Direction::DOWN);
                  }
            }
      const qreal oldSpatium = score->spatium();
      score->setSpatium(oldSpatium * scale);
      score->spatiumChanged(oldSpatium, score->spatium());
      score->setLayoutAll();
      score->doLayout();
      int checked = 0;
      for (const auto& entry : score->spanner()) {
            if (!entry.second->isSlur())
                  continue;
            Slur* slur = toSlur(entry.second);
            if (slur->partialDirection() < 0)
                  continue;
            // These fixtures have C4 quarter notes with up stems. Studio's
            // floating end meets the stem tip; the note-owned end sits 0.25sp
            // into the stem. This is independent of score size and line length.
            if (slur->startCR() == slur->endCR() && slur->isIncoming() != slur->isOutgoing()) {
                  SlurPos raw;
                  slur->slurPos(&raw);
                  const qreal delta = slur->isIncoming() ? raw.p1.y() - raw.p2.y() : raw.p2.y() - raw.p1.y();
                  QVERIFY(qAbs(delta / score->spatium() - (slur->up() ? -0.25 : 0.25)) < 0.001);
                  ++checked;
                  }
            else if (slur->up()) {
                  for (bool incoming : { true, false }) {
                        if (incoming ? !slur->isIncoming() : !slur->isOutgoing())
                              continue;
                        const ChordRest* cr = incoming ? slur->startCR() : slur->endCR();
                        const Chord* chord = toChord(cr);
                        QVERIFY(chord->stem());
                        const qreal stemTip = chord->stem()->y() + chord->stem()->bbox().top();
                        const SlurSegment* segment = incoming ? slur->frontSegment() : slur->backSegment();
                        const Grip grip = incoming ? Grip::START : Grip::END;
                        QVERIFY(qAbs(segment->ups(grip).p.y() - stemTip) < 0.001);
                        ++checked;
                        }
                  }
            else {
                  for (bool incoming : { true, false }) {
                        if (incoming ? !slur->isIncoming() : !slur->isOutgoing())
                              continue;
                        const Chord* chord = toChord(incoming ? slur->startCR() : slur->endCR());
                        const Note* note = chord->downNote();
                        const qreal noteBottom = note->y() + note->bbox().bottom();
                        const SlurSegment* segment = incoming ? slur->frontSegment() : slur->backSegment();
                        const Grip grip = incoming ? Grip::START : Grip::END;
                        QVERIFY(qAbs(segment->ups(grip).p.y() - noteBottom - 0.4 * score->spatium()) < 0.001);
                        ++checked;
                        }
                  }
            }
      QVERIFY(checked > 0);
      }

// Measure painted ink rather than trusting BarLine::bbox(): 3.x calculates
// repeat-dot layout width at font magnification 1, unlike the actual drawing.
static qreal paintedHeaderRight(const Element* element)
      {
      constexpr int scale = 16;
      constexpr int origin = 64;
      QImage image(2048, 2048, QImage::Format_ARGB32_Premultiplied);
      image.fill(Qt::transparent);
      QPainter painter(&image);
      painter.translate(origin, origin);
      painter.scale(scale, scale);
      element->draw(&painter);
      painter.end();
      int right = -1;
      for (int y = 0; y < image.height(); ++y) {
            const QRgb* row = reinterpret_cast<const QRgb*>(image.constScanLine(y));
            for (int x = right + 1; x < image.width(); ++x) {
                  if (qAlpha(row[x]))
                        right = x;
                  }
            }
      if (right == image.width() - 1)
            return std::numeric_limits<qreal>::quiet_NaN(); // clipped test image
      return qreal(right + 1 - origin) / scale;
      }

void TestRepeat::oneNotePartialSlurStaffClearance()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "partial-slur-S03.mscx"));
      QVERIFY(score);
      Slur* slur = nullptr;
      for (const auto& entry : score->spanner()) {
            if (entry.second->isSlur() && toSlur(entry.second)->isIncoming()
                && entry.second->startElement() == entry.second->endElement())
                  slur = toSlur(entry.second);
            }
      QVERIFY(slur);
      SlurSegment* segment = slur->frontSegment();
      // For the C4 stem tip at 1.5sp, the half-spatium inset puts the
      // attached end on line 2. Staff clearance moves it to 1.77sp, while
      // the constrained free end is at 1.75sp: a near-horizontal 0.02sp tilt.
      const qreal startY = segment->ups(Grip::START).p.y() / score->spatium();
      const qreal endY = segment->ups(Grip::END).p.y() / score->spatium();
      QVERIFY(qAbs(startY - 1.75) < 0.02);
      QVERIFY(qAbs(endY - 1.77) < 0.02);
      QVERIFY(qAbs(endY - startY) < 0.05);
      const QPointF edit(0.2 * score->spatium(), -0.3 * score->spatium());
      segment->ups(Grip::END).off = edit;
      score->setLayoutAll();
      score->doLayout();
      QCOMPARE(slur->frontSegment()->ups(Grip::END).off, edit);
      }

void TestRepeat::partialSlurRepeatDotGap_data()
      {
      QTest::addColumn<double>("scale");
      QTest::addColumn<double>("staffMag");
      for (double scale : { 0.4, 0.75, 1.0, 1.5 }) {
            for (double staffMag : { 0.7, 1.0 })
                  QTest::newRow(qPrintable(QString("size-%1-staff-%2").arg(scale).arg(staffMag))) << scale << staffMag;
            }
      }

void TestRepeat::partialSlurRepeatDotGap()
      {
      QFETCH(double, scale);
      QFETCH(double, staffMag);
      std::unique_ptr<MasterScore> score(readScore(DIR + "partial-slur-S03.mscx"));
      QVERIFY(score);
      const qreal oldSpatium = score->spatium();
      score->setSpatium(oldSpatium * scale);
      score->spatiumChanged(oldSpatium, score->spatium());
      score->staff(0)->staffType(Fraction(0, 1))->setUserMag(staffMag);
      score->setLayoutAll();
      score->doLayout();
      int checked = 0;
      for (const auto& entry : score->spanner()) {
            if (!entry.second->isSlur() || !toSlur(entry.second)->isIncoming())
                  continue;
            Slur* slur = toSlur(entry.second);
            const Measure* measure = slur->startCR()->measure();
            const Segment* first = measure->first(SegmentType::ChordRest);
            const Segment* header = first->prevActive();
            QVERIFY(header && header->isStartRepeatBarLineType());
            const Element* bar = header->element(0);
            QVERIFY(bar && bar->isBarLine());
            const qreal inkRight = measure->x() + header->x() + bar->x() + paintedHeaderRight(bar);
            const qreal expected = inkRight + 0.25 * score->spatium();
            const qreal actual = slur->frontSegment()->ups(Grip::START).p.x();
            QVERIFY2(qAbs(actual - expected) < 0.04 * score->spatium(), qPrintable(QString("Repeat-dot clearance: start %1, expected %2, painted edge %3, spatium %4")
                        .arg(actual).arg(expected).arg(inkRight).arg(score->spatium())));
            QVERIFY(actual < slur->frontSegment()->ups(Grip::END).p.x());
            ++checked;
            }
      QCOMPARE(checked, 2);
      }

void TestRepeat::startRepeatNoteSpacing_data()
      {
      QTest::addColumn<int>("pitch");
      QTest::addColumn<int>("tpc");
      QTest::newRow("C3-below") << 48 << 14;
      QTest::newRow("C4-ledger") << 60 << 14;
      QTest::newRow("C5-on-staff") << 72 << 14;
      QTest::newRow("C6-above") << 84 << 14;
      QTest::newRow("Csharp4-accidental") << 61 << 21;
      }

void TestRepeat::startRepeatNoteSpacing()
      {
      QFETCH(int, pitch);
      QFETCH(int, tpc);
      QFile input(root + "/" + DIR + "partial-slur-S01.mscx");
      QVERIFY(input.open(QIODevice::ReadOnly));
      QDomDocument document;
      QVERIFY(document.setContent(input.readAll()));
      const QDomNodeList notes = document.elementsByTagName("Note");
      for (int i = 0; i < notes.count(); ++i) {
            QDomElement note = notes.at(i).toElement();
            note.firstChildElement("pitch").firstChild().setNodeValue(QString::number(pitch));
            note.firstChildElement("tpc").firstChild().setNodeValue(QString::number(tpc));
            }
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString saved = directory.path() + "/repeat-note-spacing.mscx";
      QFile output(saved);
      QVERIFY(output.open(QIODevice::WriteOnly));
      output.write(document.toByteArray());
      output.close();
      std::unique_ptr<MasterScore> score(readCreatedScore(saved));
      QVERIFY(score);
      Measure* repeat = score->firstMeasure()->nextMeasure();
      const Segment* first = repeat->first(SegmentType::ChordRest);
      const Segment* header = first->prevActive();
      QVERIFY(header && header->isStartRepeatBarLineType());
      const Element* bar = header->element(0);
      QVERIFY(bar && bar->isBarLine());
      const Chord* chord = toChord(first->element(0));
      const Note* note = chord->upNote();
      const qreal right = bar->pagePos().x() + paintedHeaderRight(bar);
      const qreal gap = note->pagePos().x() + note->bbox().left() - right;
      QVERIFY2(gap + 0.04 * score->spatium() >= score->styleP(Sid::barNoteDistance),
               qPrintable(QString("Repeat-dot to note gap %1sp, required %2sp")
                          .arg(gap / score->spatium()).arg(score->styleP(Sid::barNoteDistance) / score->spatium())));
      if (pitch == 61) {
            const Accidental* accidental = note->accidental();
            QVERIFY(accidental);
            const qreal accidentalGap = accidental->pagePos().x() + accidental->bbox().left() - right;
            QVERIFY(accidentalGap + 0.04 * score->spatium() >= score->styleP(Sid::barAccidentalDistance));
            }
      }

void TestRepeat::startRepeatHeaderSpacing_data()
      {
      QTest::addColumn<int>("pitch");
      QTest::addColumn<int>("tpc");
      QTest::addColumn<QString>("duration");
      QTest::addColumn<bool>("rest");
      QTest::newRow("quarter-ledger") << 60 << 14 << QString("quarter") << false;
      QTest::newRow("half-ledger") << 60 << 14 << QString("half") << false;
      QTest::newRow("whole-ledger") << 60 << 14 << QString("whole") << false;
      QTest::newRow("whole-above") << 84 << 14 << QString("whole") << false;
      QTest::newRow("half-on-staff") << 72 << 14 << QString("half") << false;
      QTest::newRow("half-sharp") << 61 << 21 << QString("half") << false;
      QTest::newRow("quarter-rest") << 60 << 14 << QString("quarter") << true;
      }

void TestRepeat::startRepeatHeaderSpacing()
      {
      QFETCH(int, pitch);
      QFETCH(int, tpc);
      QFETCH(QString, duration);
      QFETCH(bool, rest);
      QFile input(root + "/" + DIR + "start-repeat-header-spacing.mscx");
      QVERIFY(input.open(QIODevice::ReadOnly));
      QDomDocument document;
      QVERIFY(document.setContent(input.readAll()));
      const QDomNodeList voices = document.elementsByTagName("voice");
      for (int i = 0; i < voices.count(); ++i) {
            QDomElement voice = voices.at(i).toElement();
            QDomElement chord = voice.firstChildElement("Chord");
            QVERIFY(!chord.isNull());
            while (!voice.firstChildElement("Rest").isNull())
                  voice.removeChild(voice.firstChildElement("Rest"));
            chord.firstChildElement("durationType").firstChild().setNodeValue(duration);
            QDomElement note = chord.firstChildElement("Note");
            note.firstChildElement("pitch").firstChild().setNodeValue(QString::number(pitch));
            note.firstChildElement("tpc").firstChild().setNodeValue(QString::number(tpc));
            auto appendRest = [&](const QString& type) {
                  QDomElement r = document.createElement("Rest");
                  QDomElement d = document.createElement("durationType");
                  d.appendChild(document.createTextNode(type));
                  r.appendChild(d);
                  voice.appendChild(r);
                  return r;
                  };
            if (rest) {
                  QDomElement replacement = appendRest(duration);
                  voice.replaceChild(replacement, chord);
                  }
            if (duration == "quarter")
                  appendRest("quarter");
            if (duration != "whole")
                  appendRest("half");
            }
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString path = directory.filePath("repeat-header-spacing.mscx");
      QFile output(path);
      QVERIFY(output.open(QIODevice::WriteOnly));
      output.write(document.toByteArray());
      output.close();
      std::unique_ptr<MasterScore> score(readCreatedScore(path));
      QVERIFY(score);
      qreal originalGap = 0.0;
      qreal originalHeaderDistance = 0.0;
      for (int pass = 0; pass < 2; ++pass) {
            score->style().set(Sid::systemHeaderDistance, Spatium(pass ? 6.0 : 2.5));
            score->doLayout();
            Measure* start = score->firstMeasure()->nextMeasure();
            Measure* middle = start->nextMeasure()->nextMeasure();
            QVERIFY(start->isFirstInSystem());
            QVERIFY(!middle->isFirstInSystem());
            QCOMPARE(start->system(), middle->system());
            qreal gaps[2];
            int index = 0;
            for (Measure* measure : { start, middle }) {
                  Segment* first = measure->first(SegmentType::ChordRest);
                  Segment* repeat = measure->findSegmentR(SegmentType::StartRepeatBarLine, Fraction(0,1));
                  QVERIFY(repeat && repeat->enabled());
                  const BarLine* bar = toBarLine(repeat->element(0));
                  QVERIFY(bar);
                  const qreal systemDistance = repeat->minHorizontalDistance(first, true);
                  const qreal ordinaryDistance = repeat->minHorizontalDistance(first, false);
                  QVERIFY(qAbs(systemDistance - ordinaryDistance) < .000001 * score->spatium());
                  const Element* item = first->element(0);
                  QVERIFY(item);
                  Sid margin = Sid::barNoteDistance;
                  if (item->isChord()) {
                        const Note* n = toChord(item)->upNote();
                        if (n->accidental() && n->accidental()->visible() && n->accidental()->addToSkyline()) {
                              item = n->accidental();
                              margin = Sid::barAccidentalDistance;
                              }
                        else
                              item = n;
                        }
                  gaps[index++] = item->pagePos().x() + item->bbox().left()
                        - bar->pagePos().x() - paintedHeaderRight(bar);
                  QVERIFY(gaps[index - 1] + .04 * score->spatium() >= score->styleP(margin));
                  }
            QVERIFY2(qAbs(gaps[0] - gaps[1]) < .001 * score->spatium(),
                  qPrintable(QString("pass %1 system-start gap %2sp, mid-system gap %3sp")
                        .arg(pass).arg(gaps[0] / score->spatium()).arg(gaps[1] / score->spatium())));
            if (pass == 0)
                  originalGap = gaps[0];
            else
                  QVERIFY(qAbs(gaps[0] - originalGap) < .001 * score->spatium());
            // An ordinary clef header still responds to the header style,
            // even when the caller did not identify it as a generated header.
            Segment* header = start->findSegmentR(SegmentType::HeaderClef, Fraction(0,1));
            QVERIFY(header);
            Segment* first = start->first(SegmentType::ChordRest);
            const qreal headerDistance = header->minHorizontalDistance(first, false);
            QVERIFY(qAbs(headerDistance - header->minHorizontalDistance(first, true)) < .000001 * score->spatium());
            if (pass == 0)
                  originalHeaderDistance = headerDistance;
            else
                  QVERIFY(headerDistance > originalHeaderDistance + 3.0 * score->spatium());
            }
      }

void TestRepeat::explicitSignatureHeaderSpacing_data()
      {
      QTest::addColumn<bool>("explicitClef");
      QTest::addColumn<QString>("signature");
      QTest::addColumn<QString>("item");
      for (bool explicitClef : { false, true }) {
            for (const QString& signature : { QString("time"), QString("sharp-key"), QString("flat-key"),
                                             QString("key-time"), QString("clef") }) {
                  for (const QString& item : { QString("half-ledger"), QString("whole-ledger"), QString("whole-above"), QString("rest") })
                        QTest::newRow(qPrintable(QString("%1-%2-%3").arg(explicitClef ? "file-clef" : "generated-clef")
                                                .arg(signature).arg(item))) << explicitClef << signature << item;
                  }
            }
      }

void TestRepeat::explicitSignatureHeaderSpacing()
      {
      QFETCH(bool, explicitClef);
      QFETCH(QString, signature);
      QFETCH(QString, item);
      QFile input(root + "/" + DIR + "start-repeat-header-spacing.mscx");
      QVERIFY(input.open(QIODevice::ReadOnly));
      QDomDocument document;
      QVERIFY(document.setContent(input.readAll()));
      QDomElement voice = document.elementsByTagName("voice").at(0).toElement();
      QVERIFY(!voice.isNull());
      auto appendText = [&](QDomElement parent, const QString& tag, const QString& value) {
            QDomElement e = document.createElement(tag);
            e.appendChild(document.createTextNode(value));
            parent.appendChild(e);
            };
      const bool hasTime = signature == "time" || signature == "key-time";
      if (!hasTime)
            voice.removeChild(voice.firstChildElement("TimeSig"));
      if (signature.contains("key")) {
            QDomElement key = document.createElement("KeySig");
            appendText(key, "accidental", signature == "flat-key" ? "-2" : "2");
            voice.insertBefore(key, voice.firstChild());
            }
      if (explicitClef) {
            QDomElement clef = document.createElement("Clef");
            appendText(clef, "concertClefType", "G");
            appendText(clef, "transposingClefType", "G");
            voice.insertBefore(clef, voice.firstChild());
            }
      QDomElement chord = voice.firstChildElement("Chord");
      while (!voice.firstChildElement("Rest").isNull())
            voice.removeChild(voice.firstChildElement("Rest"));
      const bool whole = item.startsWith("whole");
      chord.firstChildElement("durationType").firstChild().setNodeValue(whole ? "whole" : "half");
      chord.firstChildElement("Note").firstChildElement("pitch").firstChild().setNodeValue(item == "whole-above" ? "84" : "60");
      if (item == "rest") {
            QDomElement rest = document.createElement("Rest");
            appendText(rest, "durationType", "half");
            voice.replaceChild(rest, chord);
            }
      if (!whole) {
            QDomElement rest = document.createElement("Rest");
            appendText(rest, "durationType", "half");
            voice.appendChild(rest);
            }
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString path = directory.filePath("explicit-header-spacing.mscx");
      QFile output(path);
      QVERIFY(output.open(QIODevice::WriteOnly));
      output.write(document.toByteArray());
      output.close();
      std::unique_ptr<MasterScore> score(readCreatedScore(path));
      QVERIFY(score);
      const Sid margin = hasTime ? Sid::systemHeaderTimeSigDistance : Sid::systemHeaderDistance;
      qreal originalDistance = 0.0;
      for (int pass = 0; pass < 2; ++pass) {
            score->style().set(margin, Spatium(pass ? 6.0 : hasTime ? 2.0 : 2.5));
            score->doLayout();
            Measure* measure = score->firstMeasure();
            QVERIFY(measure->isFirstInSystem());
            Segment* first = measure->first(SegmentType::ChordRest);
            QVERIFY(first);
            Segment* header = first->prevActive();
            while (header && header->allElementsInvisible())
                  header = header->prevActive();
            QVERIFY(header);
            if (hasTime)
                  QVERIFY(header->isTimeSigType());
            else if (signature.contains("key"))
                  QVERIFY(header->isKeySigType());
            else
                  QVERIFY(header->isHeaderClefType());
            // Reproduce the v0.14 opening-clef case: its header flag is false.
            Segment* clef = measure->findSegmentR(SegmentType::HeaderClef, Fraction(0,1));
            QVERIFY(clef);
            QCOMPARE(clef->header(), !explicitClef);
            const qreal distance = header->minHorizontalDistance(first, false);
            QVERIFY(qAbs(distance - header->minHorizontalDistance(first, true)) < .000001 * score->spatium());
            const qreal minimum = header->minRight() + score->styleP(margin);
            QVERIFY2(distance + .001 * score->spatium() >= minimum,
                     qPrintable(QString("%1: signature-to-note distance %2sp, minimum %3sp")
                           .arg(signature).arg(distance / score->spatium()).arg(minimum / score->spatium())));
            QVERIFY(first->x() - header->x() + .001 * score->spatium() >= minimum);
            if (pass == 0)
                  originalDistance = distance;
            else
                  QVERIFY(distance > originalDistance + 3.0 * score->spatium());
            }
      }

void TestRepeat::barlineChordRestSpacing_data()
      {
      QTest::addColumn<bool>("endRepeat");
      QTest::addColumn<int>("pitch");
      QTest::addColumn<QString>("duration");
      QTest::addColumn<QString>("attachment");
      for (bool endRepeat : { false, true }) {
            const QString boundary = endRepeat ? "end-repeat" : "single";
            for (int pitch : { 60, 72, 84, 61 }) {
                  for (const QString& duration : { QString("half"), QString("whole") })
                        QTest::newRow(qPrintable(QString("%1-%2-%3").arg(boundary).arg(pitch).arg(duration)))
                              << endRepeat << pitch << duration << QString();
                  }
            for (const QString& attachment : { QString("rest"), QString("arpeggio"), QString("grace") })
                  QTest::newRow(qPrintable(boundary + "-" + attachment))
                        << endRepeat << 60 << QString("half") << attachment;
            }
      }

void TestRepeat::barlineChordRestSpacing()
      {
      QFETCH(bool, endRepeat);
      QFETCH(int, pitch);
      QFETCH(QString, duration);
      QFETCH(QString, attachment);
      QFile input(root + "/" + DIR + "start-repeat-header-spacing.mscx");
      QVERIFY(input.open(QIODevice::ReadOnly));
      QDomDocument document;
      QVERIFY(document.setContent(input.readAll()));
      auto appendText = [&](QDomElement parent, const QString& tag, const QString& value) {
            QDomElement e = document.createElement(tag);
            e.appendChild(document.createTextNode(value));
            parent.appendChild(e);
            };
      const QDomNodeList measures = document.elementsByTagName("Measure");
      for (int i = 0; i < measures.count(); ++i) {
            QDomElement measure = measures.at(i).toElement();
            for (const QString& tag : { QString("startRepeat"), QString("endRepeat"), QString("LayoutBreak") }) {
                  while (!measure.firstChildElement(tag).isNull())
                        measure.removeChild(measure.firstChildElement(tag));
                  }
            if (i == 0 && endRepeat)
                  appendText(measure, "endRepeat", "2");
            QDomElement voice = measure.firstChildElement("voice");
            QDomElement chord = voice.firstChildElement("Chord");
            chord.firstChildElement("durationType").firstChild().setNodeValue(duration);
            QDomElement note = chord.firstChildElement("Note");
            note.firstChildElement("pitch").firstChild().setNodeValue(QString::number(pitch));
            note.firstChildElement("tpc").firstChild().setNodeValue(pitch == 61 ? "21" : "14");
            while (!voice.firstChildElement("Rest").isNull())
                  voice.removeChild(voice.firstChildElement("Rest"));
            if (attachment == "rest") {
                  QDomElement rest = document.createElement("Rest");
                  appendText(rest, "durationType", duration);
                  voice.replaceChild(rest, chord);
                  }
            else if (attachment == "arpeggio") {
                  QDomElement arpeggio = document.createElement("Arpeggio");
                  appendText(arpeggio, "subtype", "0");
                  chord.appendChild(arpeggio);
                  }
            else if (attachment == "grace") {
                  QDomElement grace = document.createElement("Chord");
                  grace.appendChild(document.createElement("acciaccatura"));
                  appendText(grace, "durationType", "eighth");
                  grace.appendChild(note.cloneNode(true));
                  voice.insertBefore(grace, chord);
                  }
            if (duration == "half") {
                  QDomElement rest = document.createElement("Rest");
                  appendText(rest, "durationType", "half");
                  voice.appendChild(rest);
                  }
            }
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString path = directory.filePath("barline-chordrest-spacing.mscx");
      QFile output(path);
      QVERIFY(output.open(QIODevice::WriteOnly));
      output.write(document.toByteArray());
      output.close();
      std::unique_ptr<MasterScore> score(readCreatedScore(path));
      QVERIFY(score);
      for (qreal ledgerLength : { .35, .9, 1.5 }) {
            score->style().set(Sid::ledgerLineLength, Spatium(ledgerLength));
            score->doLayout();
            Measure* measure = score->firstMeasure()->nextMeasure();
            QVERIFY(!measure->isFirstInSystem());
            QCOMPARE(measure->system(), measure->prevMeasure()->system());
            QCOMPARE(measure->prevMeasure()->repeatEnd(), endRepeat);
            Segment* first = measure->first(SegmentType::ChordRest);
            QVERIFY(first);
            const Shape barrier(QRectF(0.0, 0.0, 0.0, 4.0 * score->spatium()));
            const Shape before = first->staffShape(0);
            const Chord* chord = first->element(0)->isChord() ? toChord(first->element(0)) : nullptr;
            const Note* note = chord ? chord->upNote() : nullptr;
            const Accidental* accidental = note ? note->accidental() : nullptr;
            const bool hasAccidental = accidental && accidental->addToSkyline();
            const qreal padding = score->styleP(hasAccidental ? Sid::barAccidentalDistance : Sid::barNoteDistance);
            const qreal expected = first->minLeftAfterBarline(barrier, padding);
            QVERIFY2(qAbs(first->x() - expected) < .001 * score->spatium(),
                     qPrintable(QString("barline-to-first-segment %1sp, expected %2sp")
                           .arg(first->x() / score->spatium()).arg(expected / score->spatium())));
            if (attachment.isEmpty() && !hasAccidental) {
                  QVERIFY(note);
                  const qreal headLeft = chord->x() + note->x() + note->bbox().left();
                  qreal minimum = qMax(0.0, -headLeft) + padding;
                  const qreal ledgerPadding = qMax(score->styleP(Sid::barNoteDistance)
                        - score->styleP(Sid::ledgerLineLength), .25 * score->spatium());
                  for (const LedgerLine* line = chord->ledgerLines(); line; line = line->next())
                        minimum = qMax(minimum, ledgerPadding - chord->x() - line->x() - line->bbox().left());
                  // Chord::shape also reserves general horizontal space for
                  // whole-note centering. That non-ink constraint can exceed
                  // the head/ledger minimum, and the padding fix preserves it.
                  for (const QRectF& rect : before) {
                        if (rect.height() == 0.0 && rect.y() == 0.0)
                              minimum = qMax(minimum, padding - rect.left());
                        }
                  QVERIFY2(qAbs(first->x() - minimum) < .001 * score->spatium(),
                           qPrintable(QString("pitch %1/%2, ledger %3sp: segment %4sp, minimum %5sp, chord x %6sp, head left %7sp")
                                 .arg(pitch).arg(duration).arg(ledgerLength).arg(first->x() / score->spatium())
                                 .arg(minimum / score->spatium()).arg(chord->x() / score->spatium())
                                 .arg(headLeft / score->spatium())));
                  if (chord->ledgerLines() && ledgerLength == .35) {
                        const qreal original = first->minLeft(barrier) + padding;
                        QVERIFY(first->x() <= original + .001 * score->spatium());
                        // D2/D8's half notes must lose the excess ledger gap.
                        // A whole note can instead be limited by its centering.
                        if (duration == "half")
                              QVERIFY(first->x() < original - .2 * score->spatium());
                        }
                  }
            else if (attachment == "rest")
                  QVERIFY(qAbs(expected - first->minLeft(barrier) - padding) < .001 * score->spatium());
            else if (attachment == "arpeggio") {
                  QVERIFY(chord && chord->arpeggio());
                  const Arpeggio* arpeggio = chord->arpeggio();
                  QVERIFY(first->x() + chord->x() + arpeggio->x() + arpeggio->bbox().left()
                        + .001 * score->spatium() >= padding);
                  }
            else if (attachment == "grace") {
                  QVERIFY(chord && !chord->graceNotes().empty());
                  const Chord* grace = chord->graceNotes().front();
                  const Note* graceNote = grace->upNote();
                  QVERIFY(first->x() + chord->x() + grace->x() + graceNote->x() + graceNote->bbox().left()
                        + .001 * score->spatium() >= padding);
                  }
            if (hasAccidental) {
                  const qreal left = first->x() + chord->x() + note->x() + accidental->x() + accidental->bbox().left();
                  QVERIFY(left + .001 * score->spatium() >= padding);
                  }
            QCOMPARE(int(first->staffShape(0).size()), int(before.size()));
            for (size_t i = 0; i < before.size(); ++i)
                  QCOMPARE(QRectF(first->staffShape(0)[i]), QRectF(before[i]));
            // User leading space is added after the computed boundary margin.
            first->setExtraLeadingSpace(Spatium(2.0));
            measure->computeMinWidth();
            QVERIFY(qAbs(first->x() - expected - 2.0 * score->spatium()) < .001 * score->spatium());
            first->setExtraLeadingSpace(Spatium(0.0));
            }
      }

void TestRepeat::incomingTieMinimumSpace_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<double>("minimum");
      QTest::addColumn<QString>("boundary");
      for (const QString& file : { QString("partial-lyrics-X01.mscx"), QString("partial-tie-R05.mscx") }) {
            for (double minimum : { 1.5, 2.25 }) {
                  for (const QString& boundary : { QString("repeat"), QString("barline"), QString("header") })
                        QTest::newRow(qPrintable(file + QString("-%1-").arg(minimum) + boundary)) << file << minimum << boundary;
                  }
            }
      }

void TestRepeat::incomingTieMinimumSpace()
      {
      QFETCH(QString, file);
      QFETCH(double, minimum);
      QFETCH(QString, boundary);
      std::unique_ptr<MasterScore> score(readScore(DIR + file));
      QVERIFY(score);
      if (boundary != "repeat")
            score->firstMeasure()->nextMeasure()->setRepeatStart(false);
      if (boundary == "header")
            score->firstMeasure()->undoSetLineBreak(true);
      score->setStyleValue(Sid::minHangingTieLength, Spatium(minimum));
      score->setLayoutAll();
      score->doLayout();
      Measure* landing = score->firstMeasure()->nextMeasure();
      if (boundary == "header")
            QCOMPARE(landing, landing->system()->firstMeasure());
      if (boundary != "repeat")
            QVERIFY(!landing->repeatStart());
      int checked = 0;
      int secondEndingTies = 0;
      for (Measure* measure = score->firstMeasure(); measure; measure = measure->nextMeasure()) {
            for (Segment* first = measure->first(SegmentType::ChordRest); first; first = first->next(SegmentType::ChordRest)) {
                  std::vector<qreal> lengths;
                  for (int track = 0; track < score->ntracks(); ++track) {
                        Element* item = first->element(track);
                        if (!item || !item->isChord())
                              continue;
                        for (Note* note : toChord(item)->notes()) {
                              Tie* tie = note->partialTie(true);
                              if (!tie)
                                    continue;
                              QVERIFY(!tie->spannerSegments().empty());
                              const TieSegment* segment = tie->frontSegment();
                              const qreal length = (segment->ups(Grip::END).pos().x() - segment->ups(Grip::START).pos().x()) / score->spatium();
                              lengths.push_back(length);
                              QVERIFY2(length + 0.01 >= score->styleS(Sid::minTieLength).val(),
                                    qPrintable(QString("Measure %1: short chord tie %2sp; segment x %3sp, start offset %4sp, end offset %5sp, parent x %6sp").arg(measure->no()+1).arg(length).arg(first->x()/score->spatium()).arg(tie->incomingStartOffset()/score->spatium()).arg(tie->incomingEndOffset()/score->spatium()).arg(measure->x()/score->spatium())));
                              ++checked;
                              if (measure->prevMeasure() && measure->prevMeasure()->repeatEnd() && !measure->repeatStart())
                                    ++secondEndingTies;
                              }
                        }
                  if (!lengths.empty()) {
                        const qreal longest = *std::max_element(lengths.begin(), lengths.end());
                        QVERIFY2(longest + 0.01 >= minimum,
                              qPrintable(QString("Measure %1: longest incoming tie %2sp; requested %3sp").arg(measure->no()+1).arg(longest).arg(minimum)));
                        }
                  }
            }
      QVERIFY(checked >= 3);
      if (file == "partial-lyrics-X01.mscx")
            QVERIFY(secondEndingTies >= 3);
      QTemporaryDir directory;
      const QString saved = directory.path() + "/hanging-tie-length.mscx";
      QVERIFY(saveScore(score.get(), saved));
      std::unique_ptr<MasterScore> copy(readCreatedScore(saved));
      QVERIFY(copy);
      QCOMPARE(copy->styleS(Sid::minHangingTieLength).val(), minimum);
      }

void TestRepeat::partialSlurBoundaryValidity_data()
      {
      QTest::addColumn<QString>("file");
      QTest::newRow("37E") << QString("partial-slur-S02.mscx");
      QTest::newRow("475") << QString("partial-slur-S02-475.mscx");
      }

void TestRepeat::partialSlurBoundaryValidity()
      {
      QFETCH(QString, file);
      std::unique_ptr<MasterScore> score(new MasterScore(MScore::defaultStyle()));
      QCOMPARE(int(score->loadMsc(root + "/" + DIR + file, true)), int(Score::FileError::FILE_NO_ERROR));
      score->doLayout();
      const QMap<int, int> expected { { 2, 1 }, { 3, -1 }, { 4, 0 } };
      int checked = 0;
      for (const auto& entry : score->spanner()) {
            if (!entry.second->isSlur())
                  continue;
            Slur* slur = toSlur(entry.second);
            const int measure = slur->startCR()->measure()->no();
            QVERIFY(expected.contains(measure));
            QCOMPARE(slur->partialDirection(), expected.value(measure));
            ++checked;
            // Round-trip BOTH flags at all three anchors: A may only go out,
            // D may do neither, and E may only come in. Keep manual offsets.
            slur->setPartialDirection(2);
            for (Grip grip : { Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END })
                  slur->frontSegment()->ups(grip).off = QPointF(0.15, -0.2) * score->spatium();
            }
      QCOMPARE(checked, 3);
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString saved = directory.path() + "/slur-boundaries.mscx";
      QVERIFY(saveScore(score.get(), saved));
      std::unique_ptr<MasterScore> copy(readCreatedScore(saved));
      QVERIFY(copy);
      checked = 0;
      for (const auto& entry : copy->spanner()) {
            if (!entry.second->isSlur())
                  continue;
            Slur* slur = toSlur(entry.second);
            const int measure = slur->startCR()->measure()->no();
            QVERIFY(expected.contains(measure));
            QCOMPARE(slur->partialDirection(), expected.value(measure));
            for (Grip grip : { Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END })
                  QVERIFY(QLineF(slur->frontSegment()->ups(grip).off / copy->spatium(), QPointF(0.15, -0.2)).length() < 0.001);
            ++checked;
            }
      QCOMPARE(checked, 3);
      }

void TestRepeat::invalidOneNotePartialSlur_data()
      {
      QTest::addColumn<bool>("incoming");
      QTest::addColumn<int>("direction");
      QTest::addColumn<bool>("removed");
      QTest::newRow("first-note-outgoing") << true << 1 << true;
      QTest::newRow("first-note-both") << true << 2 << true;
      QTest::newRow("last-note-incoming") << false << 0 << true;
      QTest::newRow("last-note-both") << false << 2 << true;
      QTest::newRow("valid-incoming") << true << 0 << false;
      QTest::newRow("valid-outgoing") << false << 1 << false;
      }

void TestRepeat::invalidOneNotePartialSlur()
      {
      QFETCH(bool, incoming);
      QFETCH(int, direction);
      QFETCH(bool, removed);
      std::unique_ptr<MasterScore> score(readScore(DIR + "partial-slur-S03.mscx"));
      QVERIFY(score);
      Slur* target = nullptr;
      for (const auto& entry : score->spanner()) {
            if (entry.second->isSlur() && toSlur(entry.second)->partialDirection() == (incoming ? 0 : 1))
                  target = toSlur(entry.second);
            }
      QVERIFY(target);
      QCOMPARE(target->startCR(), target->endCR());
      const Fraction tick = target->tick();
      const int lineType = target->lineType();
      target->setPartialDirection(direction);
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString saved = directory.path() + "/invalid-one-note-slur.mscx";
      QVERIFY(saveScore(score.get(), saved));
      std::unique_ptr<MasterScore> copy(readCreatedScore(saved));
      QVERIFY(copy);
      int checked = 0;
      bool found = false;
      for (const auto& entry : copy->spanner()) {
            if (entry.second->isSlur()) {
                  if (entry.second->tick() == tick) {
                        QVERIFY(!removed);
                        Slur* slur = toSlur(entry.second);
                        QCOMPARE(slur->partialDirection(), direction);
                        QCOMPARE(slur->lineType(), lineType);
                        QCOMPARE(slur->startCR(), slur->endCR());
                        found = true;
                        }
                  ++checked;
                  }
            }
      QCOMPARE(found, !removed);
      QCOMPARE(checked, removed ? 2 : 3);
      }

void TestRepeat::repeatTieNoteEntry_data()
      {
      QTest::addColumn<bool>("justEntered");
      QTest::addColumn<bool>("chord");
      QTest::newRow("selected-note") << false << false;
      QTest::newRow("just-entered-note") << true << false;
      QTest::newRow("selected-chord") << false << true;
      QTest::newRow("just-entered-chord") << true << true;
      }

void TestRepeat::repeatTieNoteEntry()
      {
      QFETCH(bool, justEntered);
      QFETCH(bool, chord);
      std::unique_ptr<MasterScore> score(readScore(DIR + "repeat06.mscx"));
      QVERIFY(score);
      Measure* landing = score->firstMeasure()->nextMeasure();
      Measure* ending = landing->nextMeasure();
      Note* source = toChord(ending->first(SegmentType::ChordRest)->element(0))->upNote();
      Note* destination = toChord(landing->first(SegmentType::ChordRest)->element(0))->upNote();
      source->setPitch(destination->pitch());
      Note* secondEnding = toChord(ending->nextMeasure()->first(SegmentType::ChordRest)->element(0))->upNote();
      const int untouchedPitch = secondEnding->pitch();
      const size_t untouchedNotes = secondEnding->chord()->notes().size();
      Note* upperSource = nullptr;
      Note* upperDestination = nullptr;
      if (chord) {
            upperSource = source->clone();
            upperSource->setPitch(source->pitch() + 4);
            source->chord()->add(upperSource);
            upperDestination = destination->clone();
            upperDestination->setPitch(upperSource->pitch());
            destination->chord()->add(upperDestination);
            }
      score->select(source);
      score->inputState().setTrack(source->track());
      score->inputState().setSegment(source->chord()->segment());
      score->setNoteEntryMode(true);
      if (justEntered)
            score->inputState().moveToNextInputPos();
      Segment* previousInput = score->inputState().segment();
      ChordRest* following = nextChordRest(score->inputState().cr());
      QVERIFY(following);
      score->cmdAddTie();
      QCOMPARE(score->inputState().segment(), following->segment());
      QCOMPARE(score->inputState().lastSegment(), previousInput);
      QCOMPARE(score->playPos(), following->tick());
      QCOMPARE(score->inputState().track(), source->track());
      QVERIFY(source->partialTie(false));
      QVERIFY(destination->partialTie(true));
      QVERIFY(!source->tieFor());
      QCOMPARE(secondEnding->pitch(), untouchedPitch);
      QCOMPARE(secondEnding->chord()->notes().size(), untouchedNotes);
      if (chord) {
            QVERIFY(upperSource->partialTie(false));
            QVERIFY(upperDestination->partialTie(true));
            QVERIFY(!upperSource->tieFor());
            }
      score->setNoteEntryMode(false);
      }

// Exercise the command dispatched by T outside note entry, not cmdAddTie.
void TestRepeat::repeatTieToggle()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "repeat06.mscx"));
      QVERIFY(score);
      std::vector<Note*> notes;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            Note* n = toChord(m->first(SegmentType::ChordRest)->element(0))->upNote();
            n->setPitch(60);
            notes.push_back(n);
            }
      score->setExpandRepeats(false);
      score->select(notes[1]);
      score->cmdToggleTie();
      QVERIFY(notes[1]->tieFor());
      QCOMPARE(notes[1]->tieFor()->endNote(), notes[2]);
      QVERIFY(notes[3]->partialTie(true));
      score->cmdToggleTie();
      QVERIFY(!notes[1]->tieFor());
      QVERIFY(!notes[3]->partialTie(true));
      score->undoRedo(true, nullptr);
      QVERIFY(notes[1]->tieFor());
      QVERIFY(notes[3]->partialTie(true));

      // The end of volta 1 jumps back, never into volta 2.
      // A different pitch in volta 2 must remain untouched.
      notes[3]->setPitch(62);
      score->select(notes[2]);
      score->cmdToggleTie();
      QVERIFY(!notes[2]->tieFor());
      QVERIFY(notes[2]->partialTie(false));
      QVERIFY(notes[1]->partialTie(true));
      QCOMPARE(notes[3]->pitch(), 62);
      score->cmdToggleTie();
      QVERIFY(!notes[2]->partialTie(false));
      QVERIFY(!notes[1]->partialTie(true));
      score->undoRedo(true, nullptr);
      QVERIFY(notes[2]->partialTie(false));
      QVERIFY(notes[1]->partialTie(true));
      score->undoRedo(false, nullptr);
      QVERIFY(!notes[2]->partialTie(false));
      QVERIFY(!notes[1]->partialTie(true));
      QVERIFY(!score->expandRepeats());
      }

QTEST_MAIN(TestRepeat)
#include "tst_repeat.moc"
