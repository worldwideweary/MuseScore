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
#include "libmscore/undo.h"

#define DIR QString("libmscore/repeat/")

using namespace Ms;

//---------------------------------------------------------
//   TestRepeat
//---------------------------------------------------------

class TestRepeat : public QObject, public MTest
      {
      Q_OBJECT
      void repeat(const char* f1, const QString & ref);

   private slots:
      void initTestCase();
      void notationRoutes_data();
      void notationRoutes();
      void partialTieRoundTrip();
      void repeatTieNotes();
      void automaticRepeatTies();
      void repeatTiePlayback();
      void partialLyricsLayout();
      void partialSlurLayout();
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
