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
#include "libmscore/chordrest.h"
#include "libmscore/chord.h"
#include "libmscore/rest.h"
#include "libmscore/staff.h"
#include "libmscore/beam.h"
#include "libmscore/beamlayout4.h"

#define DIR QString("libmscore/beam/")

using namespace Ms;

//---------------------------------------------------------
//   TestBeam
//---------------------------------------------------------

class TestBeam : public QObject, public MTest
      {
      Q_OBJECT

      void beam(const char* path);

   private slots:
      void initTestCase();
      void beamA()   { beam("Beam-A.mscx"); }
      void beamB()   { beam("Beam-B.mscx"); }
      void beamC()   { beam("Beam-C.mscx"); }
      void beamD()   { beam("Beam-D.mscx"); }
      void beamE()   { beam("Beam-E.mscx"); }
      void beamF()   { beam("Beam-F.mscx"); }
      void beamG()   { beam("Beam-G.mscx"); }
      void beam2()   { beam("Beam-2.mscx"); }
      void beam23()  { beam("Beam-23.mscx"); }
      void beamS0()  { beam("Beam-S0.mscx"); }
      void beamDir() { beam("Beam-dir.mscx"); }
      void beamCrossMeasure1();
      void beamCrossMeasure2() { beam("Beam-CrossM2.mscx"); }
      void beamCrossMeasure3() { beam("Beam-CrossM3.mscx"); }
      void beamCrossMeasure4() { beam("Beam-CrossM4.mscx"); }
      void beamStemDir();
      void flipBeamStemDir();
      void studioBeamStyleRoundTrip();
      void studioBeamLayoutSwitch();
      void studioBeamManualPosition();
      void studioBeamCrossMeasure();
      void studioBeamCrossStaff();
      void studioBeamDirectionPolicy();
      void studioBeamDirectionSwitch();
      void studioBeamletControl();
      void studioBeamletRestNeighbor();
      void studioInnerBreaks();
      void studioBeamedRestClearance();
      void studioRestRunSegmentEnds();
      void studioBeamShortWidth();
      };

//---------------------------------------------------------
//   initTestCase
//---------------------------------------------------------

void TestBeam::initTestCase()
      {
      initMTest();
      }

//---------------------------------------------------------
//   beam
//---------------------------------------------------------

void TestBeam::beam(const char* path)
      {
      MasterScore* score = readScore(DIR + path);
      QVERIFY(score);
      QVERIFY(saveCompareScore(score, path, DIR + path));
      delete score;
      }

//---------------------------------------------------------
//   beamCrossMeasure1
//   This method simulates following operations:
//   - Update the score
//   - Check if the beam has been recreated. If yes, this is wrong behaviour
//---------------------------------------------------------
void TestBeam::beamCrossMeasure1()
      {
      MasterScore* score = readScore(DIR + "Beam-CrossM1.mscx");
      QVERIFY(score);
      Measure* first_measure = score->firstMeasure();
      // find the first segment that has a chord
      Segment* s = first_measure->first(SegmentType::ChordRest);
      while (s && !s->element(0)->isChord())
            s = s->next(SegmentType::ChordRest);
      // locate the first beam
      ChordRest* first_note = toChordRest(s->element(0));
      Beam* b = first_note->beam();
      score->update();
      // locate the beam again, and check if it is still b
      Beam* new_b = first_note->beam();
      QCOMPARE(new_b, b);
      delete score;
      }

//---------------------------------------------------------
//   beamStemDir
//   This method tests if a beam's stem direction can be
//   set with a note other than the first one.
//---------------------------------------------------------
void TestBeam::beamStemDir()
      {
      MasterScore* score = readScore(DIR + "beamStemDir.mscx");
      QVERIFY(score);
      Measure* m1 = score->firstMeasure();
      ChordRest* cr = toChordRest(m1->findSegment(SegmentType::ChordRest, m1->tick())->element(0));
      Chord* c2 = toChord(cr->beam()->elements()[1]);
      c2->setStemDirection(Direction::UP);
      score->update();
      score->doLayout();
      QVERIFY(saveCompareScore(score, "beamStemDir-01.mscx", DIR + "beamStemDir-01-ref.mscx"));
      delete score;
      }

//---------------------------------------------------------
//   flipBeamStemDir
//   This method tests if a beam's stem direction can be
//   set with a note after its direction has been set
//   with the beam's own setBeamDirection method.
//---------------------------------------------------------
void TestBeam::flipBeamStemDir()
      {
      MasterScore* score = readScore(DIR + "flipBeamStemDir.mscx");
      QVERIFY(score);
      Measure* m1 = score->firstMeasure();
      ChordRest* cr = toChordRest(m1->findSegment(SegmentType::ChordRest, m1->tick())->element(0));
      Chord* c2 = toChord(cr->beam()->elements()[1]);
      cr->beam()->setBeamDirection(Direction::UP);
      c2->setStemDirection(Direction::DOWN);
      score->update();
      score->doLayout();
      QVERIFY(saveCompareScore(score, "flipBeamStemDir-01.mscx", DIR + "flipBeamStemDir-01-ref.mscx"));
      delete score;
      }
void TestBeam::studioBeamStyleRoundTrip()
      {
      MasterScore* score = readScore(DIR + "studio-beams.mscx");
      QVERIFY(score);
      QVERIFY(!score->styleB(Sid::beamLayout4));
      score->setStyleValue(Sid::beamLayout4, true);
      QVERIFY(saveScore(score, "studio-beams-roundtrip.mscx"));
      MasterScore* reloaded = readCreatedScore("studio-beams-roundtrip.mscx");
      QVERIFY(reloaded);
      QVERIFY(reloaded->styleB(Sid::beamLayout4));
      delete reloaded;
      delete score;
      }

void TestBeam::studioBeamLayoutSwitch()
      {
      MasterScore* score = readScore(DIR + "studio-beams.mscx");
      QVERIFY(score);
      ChordRest* cr = toChordRest(score->firstMeasure()->first(SegmentType::ChordRest)->element(0));
      QVERIFY(cr && cr->beam());
      const QPointF legacy = cr->beam()->beamPos();
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      const QPointF modern = cr->beam()->beamPos();
      QVERIFY(qIsFinite(modern.x()) && qIsFinite(modern.y()));
      score->doLayout();
      QVERIFY(QLineF(modern, cr->beam()->beamPos()).length() < 1e-7);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      QVERIFY(QLineF(legacy, cr->beam()->beamPos()).length() < 1e-7);
      delete score;
      }

void TestBeam::studioBeamManualPosition()
      {
      MasterScore* score = readScore(DIR + "studio-beams.mscx");
      QVERIFY(score);
      ChordRest* cr = toChordRest(score->firstMeasure()->first(SegmentType::ChordRest)->element(0));
      QVERIFY(cr && cr->beam());
      Beam* beam = cr->beam();
      beam->setUserModified(true);
      beam->setBeamPos(QPointF(-4.0, -3.0));
      score->doLayout();
      const QPointF manual = beam->beamPos();
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      QVERIFY(QLineF(manual, cr->beam()->beamPos()).length() < 1e-7);
      delete score;
      }

void TestBeam::studioBeamShortWidth()
      {
      MasterScore* score = readScore(DIR + "studio-beams.mscx");
      QVERIFY(score);
      ChordRest* first = toChordRest(score->firstMeasure()->first(SegmentType::ChordRest)->element(0));
      QVERIFY(first && first->beam());
      Beam* beam = first->beam();
      QVERIFY(beam->elements().size() >= 2);
      ChordRest* second = beam->elements()[1];
      // Exercise the pre-spacing/final-spacing interface directly. Do not
      // rely on page justification to happen to produce a short beam.
      second->rxpos() += first->pagePos().x() + 2 * beam->spatium() - second->pagePos().x();
      std::vector<ChordRest*> elements {first, second};
      QPointF start, end;
      qreal distance = 0.75 * beam->spatium();
      QVERIFY(layoutBeam4(beam, elements, start, end, distance));
      QVERIFY(qAbs(end.y() - start.y()) <= 0.25 * beam->spatium() + 1e-7);
      delete score;
      }

void TestBeam::studioBeamCrossMeasure()
      {
      const char* fixtures[] = {"Beam-CrossM1.mscx", "Beam-CrossM2.mscx", "Beam-CrossM3.mscx", "Beam-CrossM4.mscx"};
      for (const char* fixture : fixtures) {
            MasterScore* score = readScore(DIR + fixture);
            QVERIFY(score);
            score->setStyleValue(Sid::beamLayout4, true);
            score->doLayout();
            std::vector<std::pair<ChordRest*, QPointF>> positions;
            for (Measure* measure = score->firstMeasure(); measure; measure = measure->nextMeasure()) {
                  for (Segment* segment = measure->first(SegmentType::ChordRest); segment;
                       segment = segment->next(SegmentType::ChordRest)) {
                        for (int track = 0; track < score->ntracks(); ++track) {
                              Element* e = segment->element(track);
                              if (!e || !e->isChordRest())
                                    continue;
                              ChordRest* cr = toChordRest(e);
                              Beam* beam = cr->beam();
                              if (!beam || beam->empty() || beam->elements().front() != cr)
                                    continue;
                              const QPointF p = beam->beamPos();
                              QVERIFY(qIsFinite(p.x()) && qIsFinite(p.y()));
                              positions.emplace_back(cr, p);
                              }
                        }
                  }
            QVERIFY(!positions.empty());
            score->doLayout();
            for (const auto& position : positions)
                  QVERIFY(QLineF(position.second, position.first->beam()->beamPos()).length() < 1e-7);
            delete score;
            }
      }

void TestBeam::studioBeamDirectionPolicy()
      {
      // Treble B4 is line 4. The photographed D5-B4-A4-G4 group is
      // lines 2,4,5,6: D5/G4 cancel, then B4/A4 choose up-stems.
      QVERIFY(beam4AutoStemUp({2, 4, 5, 6}, 4));
      QVERIFY(!beam4AutoStemUp({2, 3, 4, 6}, 4));
      QVERIFY(!beam4AutoStemUp({2, 6}, 4));
      QVERIFY(!beam4AutoStemUp({2, 3, 5, 6}, 4));
      QVERIFY(!beam4AutoStemUp({4}, 4));
      // Repeated pitches do not vote multiple times; order is irrelevant.
      QVERIFY(beam4AutoStemUp({6, 2, 4, 4, 4, 5}, 4));
      // All notes in each chord participate, not only outer chord notes.
      QVERIFY(beam4AutoStemUp({2, 4, 5, 6, 4, 5}, 4));
      // The same relationship works with a different staff centre.
      QVERIFY(beam4AutoStemUp({0, 2, 3, 4}, 2));
      }

void TestBeam::studioBeamDirectionSwitch()
      {
      MasterScore* score = readScore(DIR + "studio-direction.mscx");
      QVERIFY(score);
      ChordRest* first = toChordRest(score->firstMeasure()->first(SegmentType::ChordRest)->element(0));
      QVERIFY(first && first->beam());
      QVERIFY(!first->beam()->up());
      const QPointF legacyPosition = first->beam()->beamPos();
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      QVERIFY(first->beam()->up());
      for (ChordRest* cr : first->beam()->elements())
            QVERIFY(cr->up());
      score->doLayout();
      QVERIFY(first->beam()->up());
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      QVERIFY(!first->beam()->up());
      QVERIFY(QLineF(legacyPosition, first->beam()->beamPos()).length() < 1e-7);
      // A manually specified beam direction remains authoritative.
      score->setStyleValue(Sid::beamLayout4, true);
      first->beam()->setBeamDirection(Direction::DOWN);
      score->doLayout();
      QVERIFY(!first->beam()->up());
      delete score;
      }

void TestBeam::studioBeamCrossStaff()
      {
      MasterScore* score = readScore(DIR + "studio-cross-staff.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      ChordRest* first = toChordRest(score->firstMeasure()->first(SegmentType::ChordRest)->element(0));
      QVERIFY(first && first->beam());
      Beam* beam = first->beam();
      QVERIFY(beam->cross());
      bool hasUpper = false, hasLower = false;
      for (ChordRest* cr : beam->elements()) {
            hasUpper |= cr->vStaffIdx() == 0;
            hasLower |= cr->vStaffIdx() == 1;
            }
      QVERIFY(hasUpper && hasLower);
      const QPointF position = beam->beamPos();
      QVERIFY(qIsFinite(position.x()) && qIsFinite(position.y()));
      score->doLayout();
      QVERIFY(QLineF(position, first->beam()->beamPos()).length() < 1e-7);
      delete score;
      }

void TestBeam::studioBeamletControl()
      {
      MasterScore* score = readScore(DIR + "studio-mixed-levels.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      Measure* measure = score->firstMeasure()->nextMeasure()->nextMeasure();
      QVERIFY(measure);
      ChordRest* first = toChordRest(measure->first(SegmentType::ChordRest)->element(0));
      QVERIFY(first && first->beam());
      std::vector<ChordRest*> elements;
      for (ChordRest* cr : first->beam()->elements())
            elements.push_back(cr);
      QCOMPARE(static_cast<int>(elements.size()), 5);
      // The close-up supplied by the user: second beamlet right, fourth left.
      QVERIFY(!beam4BeamletBefore(elements, 1, 2));
      QVERIFY(beam4BeamletBefore(elements, 3, 2));
      const QLineF* second = first->beam()->topLevelSegment4(elements[1]);
      const QLineF* fourth = first->beam()->topLevelSegment4(elements[3]);
      QVERIFY(second && fourth);
      QVERIFY(second->dx() > 0);
      QVERIFY(fourth->dx() < 0);
      delete score;
      }

void TestBeam::studioBeamletRestNeighbor()
      {
      MasterScore* score = readScore(DIR + "studio-beamlet-rest.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      for (Measure* measure = score->firstMeasure(); measure; measure = measure->nextMeasure()) {
            ChordRest* first = toChordRest(measure->first(SegmentType::ChordRest)->element(0));
            QVERIFY(first && first->beam());
            std::vector<ChordRest*> elements;
            for (ChordRest* cr : first->beam()->elements())
                  elements.push_back(cr);
            QCOMPARE(static_cast<int>(elements.size()), 5);
            QVERIFY(elements[1]->isRest());
            QCOMPARE(elements[2]->beams(), 4);
            // Ignore the adjacent eighth rest: the nearest previous note has
            // three beams and the next note has two, so the beamlet goes left.
            QVERIFY(beam4BeamletBefore(elements, 2, 3));
            const QLineF* beamlet = first->beam()->topLevelSegment4(elements[2]);
            QVERIFY(beamlet);
            QVERIFY(beamlet->dx() < 0);
            }
      delete score;
      }

void TestBeam::studioInnerBreaks()
      {
      MasterScore* score = readScore(DIR + "studio-inner-breaks.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      int measureIndex = 0;
      for (Measure* measure = score->firstMeasure(); measure; measure = measure->nextMeasure(), ++measureIndex) {
            const int pattern = measureIndex % 5;
            ChordRest* first = toChordRest(measure->first(SegmentType::ChordRest)->element(0));
            QVERIFY(first && first->beam());
            Beam* beam = first->beam();
            std::vector<ChordRest*> elements;
            for (ChordRest* cr : beam->elements())
                  elements.push_back(cr);
            QCOMPARE(static_cast<int>(elements.size()), pattern == 4 ? 5 : (pattern % 2 == 0 ? 4 : 8));
            const int level = pattern == 0 || pattern == 2 ? 1 : 2;
            const int boundary = pattern == 0 ? 2 : (pattern == 1 ? 4 : (pattern == 2 ? 1 : (pattern == 3 ? 3 : 1)));
            bool secondary = false, tertiary = false;
            beam4CalcBreaks(elements[boundary], elements[boundary - 1], level, secondary, tertiary);
            QVERIFY(secondary || tertiary);
            beam4CalcBreaks(elements[boundary], elements[boundary - 1], 0, secondary, tertiary);
            QVERIFY(!secondary && !tertiary);
            const QLineF* before = beam->topLevelSegment4(elements[0]);
            const int afterIndex = pattern == 2 ? 2 : (pattern == 3 ? 4 : boundary);
            const QLineF* after = beam->topLevelSegment4(elements[afterIndex]);
            QVERIFY(before && after);
            QVERIFY(before != after);
            const int last = pattern == 4 ? 3 : static_cast<int>(elements.size()) - 1;
            QCOMPARE(beam->topLevelSegment4(elements[last]), after);
            if (pattern == 4) {
                  // Only the third beam breaks at entry/exit. The triplet's
                  // three notes stay joined and the final normal note separates.
                  beam4CalcBreaks(elements[1], elements[0], 1, secondary, tertiary);
                  QVERIFY(!secondary && !tertiary);
                  beam4CalcBreaks(elements[4], elements[3], 2, secondary, tertiary);
                  QVERIFY(tertiary);
                  QVERIFY(beam->topLevelSegment4(elements[4]) != after);
                  }
            }
      QCOMPARE(measureIndex, 10);
      delete score;
      }

void TestBeam::studioBeamedRestClearance()
      {
      MasterScore* score = readScore(DIR + "studio-inner-breaks.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      std::vector<Rest*> rests;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (Segment* seg = m->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                  Element* e = seg->element(0);
                  if (e && e->isRest() && toRest(e)->beam())
                        rests.push_back(toRest(e));
                  }
            }
      QCOMPARE(static_cast<int>(rests.size()), 4);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      std::vector<qreal> legacyY;
      for (Rest* rest : rests)
            legacyY.push_back(rest->ipos().y());
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      std::vector<qreal> modernY;
      bool movedUpStemRest = false;
      for (size_t i = 0; i < rests.size(); ++i) {
            Rest* rest = rests[i];
            const qreal y = rest->ipos().y();
            modernY.push_back(y);
            QVERIFY(qIsFinite(y));
            QCOMPARE(rest->offset(), QPointF());
            const qreal delta = y - legacyY[i];
            QVERIFY(rest->beam()->up() ? delta >= -1e-7 : delta <= 1e-7);
            const qreal steps = delta / (rest->staff()->lineDistance(rest->tick()) * rest->spatium());
            QVERIFY(qAbs(steps - qRound(steps)) < 1e-7);
            movedUpStemRest |= rest->beam()->up() && delta > 0;
            }
      QVERIFY(movedUpStemRest);
      // Exercise beam-only relayout as well as a complete score layout.
      for (Rest* rest : rests)
            rest->beam()->layout();
      for (size_t i = 0; i < rests.size(); ++i)
            QVERIFY(qAbs(rests[i]->ipos().y() - modernY[i]) < 1e-7);
      score->doLayout();
      for (size_t i = 0; i < rests.size(); ++i)
            QVERIFY(qAbs(rests[i]->ipos().y() - modernY[i]) < 1e-7);
      // Stored offsets survive the automatic clearance pass.
      const QPointF offset(0, rests[0]->spatium());
      rests[0]->setProperty(Pid::OFFSET, offset);
      score->doLayout();
      QCOMPARE(rests[0]->offset(), offset);
      rests[0]->setProperty(Pid::OFFSET, QPointF());
      rests[0]->setAutoplace(false);
      score->doLayout();
      QVERIFY(qAbs(rests[0]->ipos().y() - legacyY[0]) < 1e-7);
      rests[0]->setAutoplace(true);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      for (size_t i = 0; i < rests.size(); ++i)
            QVERIFY(qAbs(rests[i]->ipos().y() - legacyY[i]) < 1e-7);
      delete score;
      }

void TestBeam::studioRestRunSegmentEnds()
      {
      MasterScore* score = readScore(DIR + "studio-corners-rests.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      int index = 0, checked = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++index) {
            if (index % 10 != 6 && index % 10 != 9)
                  continue;
            ChordRest* first = toChordRest(m->first(SegmentType::ChordRest)->element(0));
            QVERIFY(first && first->beam());
            Beam* beam = first->beam();
            const QLineF* line = beam->topLevelSegment4(first);
            QVERIFY(line);
            // R7 and R10 start with a right-facing beamlet, not a segment
            // extending to either of the following rests.
            const qreal expected = score->styleP(Sid::beamMinLen) * beam->mag() * first->staff()->mag(first);
            QVERIFY(qAbs(line->dx() - expected) < 1e-7);
            auto it = beam->elements().begin();
            ++it;
            QVERIFY((*it)->isRest());
            QVERIFY(beam->topLevelSegment4(*it) != line);
            ++checked;
            }
      QCOMPARE(checked, 4);
      delete score;
      }

QTEST_MAIN(TestBeam)
#include "tst_beam.moc"

