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
#include "libmscore/tuplet.h"
#include "libmscore/text.h"
#include "libmscore/tremolo.h"
#include "libmscore/stem.h"
#include "libmscore/hook.h"
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
      void studioNestedTupletStacking();
      void studioSingleTremoloSwitch();
      void studioTwoNoteTremoloSpan();
      void studioInsetTremoloAnchors();
      void studioStandaloneTremoloAnchors();
      void studioFlaggedTremoloAnchors();
      void studioTremoloDirectionPolicy();
      void studioTremoloDirectionSwitch();
      void studioCrossStaffTremoloSwitch();
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

void TestBeam::studioNestedTupletStacking()
      {
      MasterScore* score = readScore(DIR + "studio-corners-tuplets.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      std::vector<Tuplet*> outers, inners;
      std::vector<QPointF> legacyPositions;
      int index = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++index) {
            if (index % 6 != 2 && index % 6 != 3)
                  continue;
            Tuplet* outer = nullptr;
            for (Segment* seg = m->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                  Element* e = seg->element(0);
                  if (e && e->isChordRest() && toChordRest(e)->tuplet()) {
                        outer = toChordRest(e)->tuplet();
                        while (outer->tuplet())
                              outer = outer->tuplet();
                        break;
                        }
                  }
            QVERIFY(outer);
            Tuplet* inner = nullptr;
            for (DurationElement* e : outer->elements()) {
                  if (e->isTuplet()) {
                        inner = toTuplet(e);
                        break;
                        }
                  }
            QVERIFY(inner);
            outers.push_back(outer);
            inners.push_back(inner);
            legacyPositions.push_back(outer->pos());
            }
      QCOMPARE(static_cast<int>(outers.size()), 4);
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      std::vector<QPointF> modernPositions;
      for (size_t i = 0; i < outers.size(); ++i) {
            Tuplet* outer = outers[i];
            Tuplet* inner = inners[i];
            QVERIFY(outer->number() && inner->number());
            const QRectF a = outer->number()->bbox().translated(outer->number()->pagePos());
            const QRectF b = inner->number()->bbox().translated(inner->number()->pagePos());
            const QByteArray detail = QString(
                  "nested pair %1, up=%2: outer=(%3,%4,%5,%6), inner=(%7,%8,%9,%10)")
                  .arg(static_cast<int>(i)).arg(outer->isUp())
                  .arg(a.x()).arg(a.y()).arg(a.width()).arg(a.height())
                  .arg(b.x()).arg(b.y()).arg(b.width()).arg(b.height()).toUtf8();
            QVERIFY2(qIsFinite(a.top()) && qIsFinite(b.top()), detail.constData());
            // Skyline placement allows diagonally separated numbers to have
            // overlapping Y ranges. Require outward ordering and no actual
            // 2D collision, not complete separation of their Y projections.
            QVERIFY2(outer->isUp() ? a.center().y() < b.center().y()
                                  : a.center().y() > b.center().y(), detail.constData());
            QVERIFY2(!a.intersects(b), detail.constData());
            modernPositions.push_back(outer->pos());
            }
      score->doLayout();
      for (size_t i = 0; i < outers.size(); ++i)
            QVERIFY(QLineF(outers[i]->pos(), modernPositions[i]).length() < 1e-7);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      for (size_t i = 0; i < outers.size(); ++i)
            QVERIFY(QLineF(outers[i]->pos(), legacyPositions[i]).length() < 1e-7);
      delete score;
      }

void TestBeam::studioSingleTremoloSwitch()
      {
      MasterScore* score = readScore(DIR + "studio-tremolo-single.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      std::vector<Tremolo*> tremolos;
      std::vector<QPointF> oldTrem, oldBeam;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (Segment* seg = m->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                  Element* e = seg->element(0);
                  if (e && e->isChord() && toChord(e)->tremolo()) {
                        Tremolo* t = toChord(e)->tremolo();
                        QVERIFY(!t->twoNotes());
                        QVERIFY(t->chord()->beam());
                        tremolos.push_back(t);
                        oldTrem.push_back(t->pos());
                        oldBeam.push_back(t->chord()->beam()->beamPos());
                        }
                  }
            }
      QCOMPARE(static_cast<int>(tremolos.size()), 20);
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      std::vector<QPointF> modernTrem, modernBeam;
      for (size_t i = 0; i < tremolos.size(); ++i) {
            Tremolo* t = tremolos[i];
            Beam* beam = t->chord()->beam();
            const bool supported = i % 10 < 8;
            QCOMPARE(beam4SupportsTremolos(beam), supported);
            QVERIFY(qIsFinite(t->pos().x()) && qIsFinite(t->pos().y()));
            if (!supported) {
                  QVERIFY(QLineF(t->pos(), oldTrem[i]).length() < 1e-7);
                  QVERIFY(QLineF(beam->beamPos(), oldBeam[i]).length() < 1e-7);
                  }
            // The first four cases have flat single-level beams. The full
            // tremolo bounding box must stay on the note side of the beam.
            if (i % 10 < 4) {
                  const QLineF* line = beam->primarySegment4();
                  QVERIFY(line);
                  const qreal beamY = line->y1() + beam->pagePos().y();
                  const qreal halfWidth = score->styleP(Sid::beamWidth) * beam->mag() / 2;
                  const QRectF box = t->bbox().translated(t->pagePos());
                  QVERIFY(t->chord()->up() ? box.top() >= beamY + halfWidth - 1e-7
                                          : box.bottom() <= beamY - halfWidth + 1e-7);
                  }
            modernTrem.push_back(t->pos());
            modernBeam.push_back(beam->beamPos());
            }
      score->doLayout();
      for (size_t i = 0; i < tremolos.size(); ++i) {
            QVERIFY(QLineF(tremolos[i]->pos(), modernTrem[i]).length() < 1e-7);
            QVERIFY(QLineF(tremolos[i]->chord()->beam()->beamPos(), modernBeam[i]).length() < 1e-7);
            }
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      for (size_t i = 0; i < tremolos.size(); ++i) {
            QVERIFY(QLineF(tremolos[i]->pos(), oldTrem[i]).length() < 1e-7);
            QVERIFY(QLineF(tremolos[i]->chord()->beam()->beamPos(), oldBeam[i]).length() < 1e-7);
            }
      delete score;
      }

void TestBeam::studioTwoNoteTremoloSpan()
      {
      MasterScore* score = readScore(DIR + "studio-tremolo-two-note.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      std::vector<Tremolo*> tremolos;
      std::vector<QPointF> positions;
      std::vector<qreal> legacyWidths;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (Segment* seg = m->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                  Element* e = seg->element(0);
                  if (!e || !e->isChord())
                        continue;
                  Chord* c = toChord(e);
                  Tremolo* t = c->tremolo();
                  if (!t || !t->twoNotes() || t->chord1() != c)
                        continue;
                  QVERIFY(t->chord2());
                  if (c->beam())
                        QVERIFY(beam4SupportsTremolos(c->beam()));
                  tremolos.push_back(t);
                  positions.push_back(t->pos());
                  legacyWidths.push_back(t->bbox().width());
                  }
            }
      QCOMPARE(static_cast<int>(tremolos.size()), 10);
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      std::vector<qreal> modernWidths;
      for (size_t i = 0; i < tremolos.size(); ++i) {
            Tremolo* t = tremolos[i];
            const qreal width = t->bbox().width();
            QVERIFY(qIsFinite(width) && width > 0);
            const Stem* s1 = t->chord1()->stem();
            const Stem* s2 = t->chord2()->stem();
            QVERIFY(s1 && s2);
            const bool inset = t->chord1()->beam();
            const qreal left = s1->pagePos().x() - (inset ? 0 : s1->lineWidthMag() / 2);
            const qreal right = s2->pagePos().x() + (inset ? 0 : s2->lineWidthMag() / 2);
            const QRectF rect = t->bbox().translated(t->pagePos());
            const qreal gap = t->spatium();
            // Inset anchors are stem centers; standalone anchors use
            // outside edges. Both have 1sp gaps in these wide fixtures.
            QVERIFY(qAbs(rect.left() - (left + gap)) < 1e-7);
            QVERIFY(qAbs(rect.right() - (right - gap)) < 1e-7);
            modernWidths.push_back(width);
            }
      score->doLayout();
      for (size_t i = 0; i < tremolos.size(); ++i)
            QVERIFY(qAbs(tremolos[i]->bbox().width() - modernWidths[i]) < 1e-7);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      for (size_t i = 0; i < tremolos.size(); ++i) {
            QVERIFY(qAbs(tremolos[i]->bbox().width() - legacyWidths[i]) < 1e-7);
            QVERIFY(QLineF(tremolos[i]->pos(), positions[i]).length() < 1e-7);
            }
      delete score;
      }

void TestBeam::studioInsetTremoloAnchors()
      {
      MasterScore* score = readScore(DIR + "studio-tremolo-anchors.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      std::vector<Tremolo*> tremolos;
      std::vector<QPointF> oldPositions, oldBeams;
      std::vector<QRectF> oldBoxes;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (Segment* seg = m->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                  Element* e = seg->element(0);
                  if (!e || !e->isChord())
                        continue;
                  Chord* c = toChord(e);
                  Tremolo* t = c->tremolo();
                  if (!t || !t->twoNotes() || t->chord1() != c || !c->beam())
                        continue;
                  tremolos.push_back(t);
                  oldPositions.push_back(t->pos());
                  oldBeams.push_back(c->beam()->beamPos());
                  oldBoxes.push_back(t->bbox());
                  }
            }
      QCOMPARE(static_cast<int>(tremolos.size()), 6);
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      std::vector<QRectF> modernBoxes;
      std::vector<QPointF> modernBeams;
      for (Tremolo* t : tremolos) {
            Beam* beam = t->chord1()->beam();
            QVERIFY(beam4SupportsTremolos(beam));
            const QLineF* primary = beam->primarySegment4();
            QVERIFY(primary && primary->dx() > 0);
            const qreal sp = t->spatium();
            const qreal left = t->chord1()->stem()->pagePos().x() + sp;
            const qreal right = t->chord2()->stem()->pagePos().x() - sp;
            QVERIFY(right > left);
            const qreal slope = primary->dy() / primary->dx();
            const auto beamY = [&](qreal x) {
                  return beam->pagePos().y() + primary->y1()
                        + (x - beam->pagePos().x() - primary->x1()) * slope;
                  };
            const int direction = t->chord1()->up() ? 1 : -1;
            const qreal spacing = 0.75 * sp; // default narrow grid in fixture
            const qreal offset = direction * t->chord1()->beams() * spacing;
            const qreal stack = direction * (t->lines() - 1) * spacing;
            const qreal halfWidth = score->styleP(Sid::beamWidth) * beam->mag() / 2;
            const qreal expectedTop = qMin(beamY(left), beamY(right)) + offset + qMin(qreal(0), stack) - halfWidth;
            const qreal expectedBottom = qMax(beamY(left), beamY(right)) + offset + qMax(qreal(0), stack) + halfWidth;
            const QRectF box = t->bbox().translated(t->pagePos());
            QVERIFY(qIsFinite(box.top()) && qIsFinite(box.bottom()));
            QVERIFY(qAbs(box.left() - left) < 1e-6);
            QVERIFY(qAbs(box.right() - right) < 1e-6);
            QVERIFY(qAbs(box.top() - expectedTop) < 1e-6);
            QVERIFY(qAbs(box.bottom() - expectedBottom) < 1e-6);
            // Bounds follow the actual primary slope; do not force V6 flat.
            modernBoxes.push_back(box);
            modernBeams.push_back(beam->beamPos());
            }
      score->doLayout();
      for (size_t i = 0; i < tremolos.size(); ++i) {
            const QRectF box = tremolos[i]->bbox().translated(tremolos[i]->pagePos());
            QVERIFY(QLineF(box.topLeft(), modernBoxes[i].topLeft()).length() < 1e-6);
            QVERIFY(QLineF(box.bottomRight(), modernBoxes[i].bottomRight()).length() < 1e-6);
            QVERIFY(QLineF(tremolos[i]->chord1()->beam()->beamPos(), modernBeams[i]).length() < 1e-6);
            }
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      for (size_t i = 0; i < tremolos.size(); ++i) {
            QVERIFY(QLineF(tremolos[i]->pos(), oldPositions[i]).length() < 1e-6);
            QVERIFY(QLineF(tremolos[i]->chord1()->beam()->beamPos(), oldBeams[i]).length() < 1e-6);
            QVERIFY(QLineF(tremolos[i]->bbox().topLeft(), oldBoxes[i].topLeft()).length() < 1e-6);
            QVERIFY(QLineF(tremolos[i]->bbox().bottomRight(), oldBoxes[i].bottomRight()).length() < 1e-6);
            }
      delete score;
      }


void TestBeam::studioStandaloneTremoloAnchors()
      {
      MasterScore* score = readScore(DIR + "studio-tremolo-anchors.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      std::vector<Tremolo*> tremolos;
      std::vector<QPointF> oldPositions;
      std::vector<QRectF> oldBoxes;
      std::vector<qreal> oldLengths;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (Segment* seg = m->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                  Element* e = seg->element(0);
                  if (!e || !e->isChord())
                        continue;
                  Chord* c = toChord(e);
                  Tremolo* t = c->tremolo();
                  if (!t || !t->twoNotes() || t->chord1() != c || c->beam())
                        continue;
                  QVERIFY(t->chord2() && c->stem() && t->chord2()->stem());
                  tremolos.push_back(t);
                  oldPositions.push_back(t->pos());
                  oldBoxes.push_back(t->bbox());
                  oldLengths.push_back(c->stem()->len());
                  oldLengths.push_back(t->chord2()->stem()->len());
                  }
            }
      QCOMPARE(static_cast<int>(tremolos.size()), 6); // V3/V4/V5, both directions
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      std::vector<QRectF> modernBoxes;
      std::vector<qreal> modernLengths;
      for (Tremolo* t : tremolos) {
            Chord* first = t->chord1();
            Chord* last = t->chord2();
            const qreal sp = t->spatium();
            const qreal left = first->stem()->pagePos().x() - first->stem()->lineWidthMag() / 2;
            const qreal right = last->stem()->pagePos().x() + last->stem()->lineWidthMag() / 2;
            const QRectF box = t->bbox().translated(t->pagePos());
            QVERIFY(qAbs(box.left() - left - sp) < 1e-6);
            QVERIFY(qAbs(box.right() - right + sp) < 1e-6);
            QVERIFY(qIsFinite(box.top()) && qIsFinite(box.bottom()));
            const qreal width = score->styleP(Sid::beamWidth) * t->mag();
            const qreal stack = (t->lines() - 1) * .75 * sp * t->mag();
            // Stem tips follow the outer stroke edge extrapolated through
            // the 1sp gaps; this checks their relationship independently of
            // the anchor solver's particular grid choice.
            const qreal x1 = first->stem()->pagePos().x();
            const qreal x2 = last->stem()->pagePos().x();
            const qreal y1 = first->stem()->pagePos().y() + first->stem()->p2().y();
            const qreal y2 = last->stem()->pagePos().y() + last->stem()->p2().y();
            const qreal slope = (y2 - y1) / (x2 - x1);
            const qreal edgeLeft = y1 + (box.left() - x1) * slope;
            const qreal edgeRight = y1 + (box.right() - x1) * slope;
            if (first->up()) {
                  QVERIFY(qAbs(box.top() - qMin(edgeLeft, edgeRight)) < 1e-6);
                  QVERIFY(qAbs(box.bottom() - qMax(edgeLeft, edgeRight) - stack - width) < 1e-6);
                  }
            else {
                  QVERIFY(qAbs(box.bottom() - qMax(edgeLeft, edgeRight)) < 1e-6);
                  QVERIFY(qAbs(box.top() - qMin(edgeLeft, edgeRight) + stack + width) < 1e-6);
                  }
            const int contour = last->line() - first->line();
            if (contour == 0)
                  QVERIFY(qAbs(slope) < 1e-7);
            else
                  QVERIFY(slope * contour > 0);
            modernBoxes.push_back(box);
            modernLengths.push_back(first->stem()->len());
            modernLengths.push_back(last->stem()->len());
            }
      score->doLayout();
      for (size_t i = 0; i < tremolos.size(); ++i) {
            Tremolo* t = tremolos[i];
            const QRectF box = t->bbox().translated(t->pagePos());
            QVERIFY(QLineF(box.topLeft(), modernBoxes[i].topLeft()).length() < 1e-6);
            QVERIFY(QLineF(box.bottomRight(), modernBoxes[i].bottomRight()).length() < 1e-6);
            QVERIFY(qAbs(t->chord1()->stem()->len() - modernLengths[2 * i]) < 1e-6);
            QVERIFY(qAbs(t->chord2()->stem()->len() - modernLengths[2 * i + 1]) < 1e-6);
            }
      // Unsupported manual stem lengths must bypass this adapter without
      // changing either stem or the supplied path.
      Tremolo* manual = tremolos.front();
      manual->chord1()->stem()->setUserLen(manual->spatium());
      QPainterPath sentinel;
      sentinel.addRect(1, 2, 3, 4);
      const QPainterPath original = sentinel;
      const qreal untouched = manual->chord2()->stem()->len();
      QVERIFY(!beam4LayoutStandaloneTremolo(manual, sentinel));
      QVERIFY(sentinel == original);
      QCOMPARE(manual->chord2()->stem()->len(), untouched);
      manual->chord1()->stem()->setUserLen(0);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      for (size_t i = 0; i < tremolos.size(); ++i) {
            Tremolo* t = tremolos[i];
            QVERIFY(QLineF(t->pos(), oldPositions[i]).length() < 1e-6);
            QVERIFY(QLineF(t->bbox().topLeft(), oldBoxes[i].topLeft()).length() < 1e-6);
            QVERIFY(QLineF(t->bbox().bottomRight(), oldBoxes[i].bottomRight()).length() < 1e-6);
            QVERIFY(qAbs(t->chord1()->stem()->len() - oldLengths[2 * i]) < 1e-6);
            QVERIFY(qAbs(t->chord2()->stem()->len() - oldLengths[2 * i + 1]) < 1e-6);
            }
      delete score;
      }


void TestBeam::studioFlaggedTremoloAnchors()
      {
      MasterScore* score = readScore(DIR + "studio-tremolo-flags.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      struct Snapshot {
            Tremolo* tremolo;
            QPointF position;
            QRectF box;
            qreal lengths[2];
            QPointF flags[2];
            };
      std::vector<Snapshot> old;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (Segment* seg = m->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                  Element* e = seg->element(0);
                  if (!e || !e->isChord())
                        continue;
                  Chord* c = toChord(e);
                  Tremolo* t = c->tremolo();
                  if (!t || !t->twoNotes() || t->chord1() != c)
                        continue;
                  QVERIFY(t->chord2());
                  Snapshot saved;
                  saved.tremolo = t;
                  saved.position = t->pos();
                  saved.box = t->bbox();
                  int i = 0;
                  for (Chord* chord : { t->chord1(), t->chord2() }) {
                        QVERIFY(!chord->beam() && chord->stem() && chord->hook());
                        saved.lengths[i] = chord->stem()->len();
                        saved.flags[i++] = chord->hook()->pos();
                        }
                  old.push_back(saved);
                  }
            }
      QCOMPARE(static_cast<int>(old.size()), 16);
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      std::vector<QRectF> modernBoxes;
      std::vector<qreal> modernLengths;
      for (const Snapshot& saved : old) {
            Tremolo* t = saved.tremolo;
            const QRectF box = t->bbox().translated(t->pagePos());
            const Stem* first = t->chord1()->stem();
            const Stem* last = t->chord2()->stem();
            QVERIFY(qIsFinite(box.top()) && qIsFinite(box.bottom()) && box.width() > 0);
            const qreal anchorDistance = last->pagePos().x() + last->lineWidthMag() / 2
                  - first->pagePos().x() + first->lineWidthMag() / 2;
            const qreal span = beam4InsetTremoloSpan(anchorDistance, t->spatium());
            QVERIFY(qAbs(box.width() - span) < 1e-6);
            for (Chord* chord : { t->chord1(), t->chord2() }) {
                  Stem* stem = chord->stem();
                  // Studio suppresses flags on two-note tremolos even when
                  // their written duration would ordinarily require a flag.
                  QVERIFY(!chord->hook());
                  QVERIFY(qIsFinite(stem->len()) && stem->len() > 0);
                  modernLengths.push_back(stem->len());
                  }
            // Calling the adapter again must preserve both path and stem lengths.
            QPainterPath repeated;
            QVERIFY(beam4LayoutStandaloneTremolo(t, repeated));
            QVERIFY(QLineF(repeated.boundingRect().topLeft(), t->bbox().topLeft()).length() < 1e-6);
            QVERIFY(QLineF(repeated.boundingRect().bottomRight(), t->bbox().bottomRight()).length() < 1e-6);
            modernBoxes.push_back(box);
            }
      score->doLayout();
      for (size_t i = 0; i < old.size(); ++i) {
            Tremolo* t = old[i].tremolo;
            const QRectF box = t->bbox().translated(t->pagePos());
            QVERIFY(QLineF(box.topLeft(), modernBoxes[i].topLeft()).length() < 1e-6);
            QVERIFY(QLineF(box.bottomRight(), modernBoxes[i].bottomRight()).length() < 1e-6);
            QVERIFY(qAbs(t->chord1()->stem()->len() - modernLengths[2 * i]) < 1e-6);
            QVERIFY(qAbs(t->chord2()->stem()->len() - modernLengths[2 * i + 1]) < 1e-6);
            QVERIFY(!t->chord1()->hook() && !t->chord2()->hook());
            }
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      for (const Snapshot& saved : old) {
            Tremolo* t = saved.tremolo;
            QVERIFY(QLineF(t->pos(), saved.position).length() < 1e-6);
            QVERIFY(QLineF(t->bbox().topLeft(), saved.box.topLeft()).length() < 1e-6);
            QVERIFY(QLineF(t->bbox().bottomRight(), saved.box.bottomRight()).length() < 1e-6);
            int i = 0;
            for (Chord* chord : { t->chord1(), t->chord2() }) {
                  QVERIFY(qAbs(chord->stem()->len() - saved.lengths[i]) < 1e-6);
                  QVERIFY(chord->hook());
                  QVERIFY(QLineF(chord->hook()->pos(), saved.flags[i]).length() < 1e-6);
                  ++i;
                  }
            }
      delete score;
      }


void TestBeam::studioTremoloDirectionPolicy()
      {
      QVERIFY(beam4TremoloAutoStemUp({ 10, 3 }, 4));
      QVERIFY(beam4TremoloAutoStemUp({ 3, 10 }, 4));
      QVERIFY(!beam4TremoloAutoStemUp({ 10, -2 }, 4));
      QVERIFY(beam4TremoloAutoStemUp({ 10, 10, -2 }, 4));
      QVERIFY(!beam4TremoloAutoStemUp({ 10, -2, -2 }, 4));
      QVERIFY(!beam4AutoStemUp({ 10, 10, -2 }, 4)); // beam policy uses distinct positions
      QVERIFY(beam4TremoloAutoStemUp({ 17, 13, 10, 10, 8, 6 }, 4));
      }

void TestBeam::studioTremoloDirectionSwitch()
      {
      MasterScore* score = readScore(DIR + "studio-tremolo-direction.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      struct Saved {
            Tremolo* tremolo;
            Direction directions[2];
            bool up[2];
            qreal lengths[2];
            QRectF box;
            QPointF position;
            };
      std::vector<Saved> saved;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (Segment* seg = m->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                  Element* e = seg->element(0);
                  if (!e || !e->isChord())
                        continue;
                  Tremolo* t = toChord(e)->tremolo();
                  if (!t || !t->twoNotes() || t->chord1() != e)
                        continue;
                  QVERIFY(t->chord2());
                  Saved entry;
                  entry.tremolo = t;
                  entry.box = t->bbox();
                  entry.position = t->pos();
                  int i = 0;
                  for (Chord* c : { t->chord1(), t->chord2() }) {
                        QVERIFY(c->stem() && !c->beam());
                        entry.directions[i] = c->stemDirection();
                        entry.up[i] = c->up();
                        entry.lengths[i++] = c->stem()->len();
                        }
                  bool result;
                  QVERIFY(!beam4TremoloStemUp(t, result));
                  saved.push_back(entry);
                  }
            }
      QCOMPARE(static_cast<int>(saved.size()), 12);
      const bool expected[] = { true, false, true, false, true, true, true, false, true, false, true, false };
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      std::vector<QRectF> boxes;
      std::vector<qreal> lengths;
      for (size_t i = 0; i < saved.size(); ++i) {
            Tremolo* t = saved[i].tremolo;
            bool result;
            QVERIFY(beam4TremoloStemUp(t, result));
            QCOMPARE(result, expected[i]);
            int j = 0;
            for (Chord* c : { t->chord1(), t->chord2() }) {
                  QCOMPARE(c->up(), expected[i]);
                  QCOMPARE(static_cast<int>(c->stemDirection()), static_cast<int>(saved[i].directions[j++]));
                  lengths.push_back(c->stem()->len());
                  }
            const QRectF box = t->bbox().translated(t->pagePos());
            QVERIFY(qIsFinite(box.top()) && qIsFinite(box.bottom()) && box.width() > 0);
            const Stem* a = t->chord1()->stem();
            const Stem* b = t->chord2()->stem();
            const qreal distance = b->pagePos().x() + b->lineWidthMag() / 2
                  - a->pagePos().x() + a->lineWidthMag() / 2;
            QVERIFY(qAbs(box.width() - beam4InsetTremoloSpan(distance, t->spatium())) < 1e-6);
            boxes.push_back(box);
            }
      score->doLayout();
      for (size_t i = 0; i < saved.size(); ++i) {
            Tremolo* t = saved[i].tremolo;
            const QRectF box = t->bbox().translated(t->pagePos());
            QVERIFY(QLineF(box.topLeft(), boxes[i].topLeft()).length() < 1e-6);
            QVERIFY(QLineF(box.bottomRight(), boxes[i].bottomRight()).length() < 1e-6);
            QVERIFY(qAbs(t->chord1()->stem()->len() - lengths[2 * i]) < 1e-6);
            QVERIFY(qAbs(t->chord2()->stem()->len() - lengths[2 * i + 1]) < 1e-6);
            QCOMPARE(t->chord1()->up(), expected[i]);
            QCOMPARE(t->chord2()->up(), expected[i]);
            }
      Tremolo* manual = saved.front().tremolo;
      manual->chord1()->stem()->setUserLen(manual->spatium());
      bool result;
      QVERIFY(!beam4TremoloStemUp(manual, result));
      manual->chord1()->stem()->setUserLen(0);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      for (const Saved& entry : saved) {
            Tremolo* t = entry.tremolo;
            QVERIFY(QLineF(t->pos(), entry.position).length() < 1e-6);
            QVERIFY(QLineF(t->bbox().topLeft(), entry.box.topLeft()).length() < 1e-6);
            QVERIFY(QLineF(t->bbox().bottomRight(), entry.box.bottomRight()).length() < 1e-6);
            int i = 0;
            for (Chord* c : { t->chord1(), t->chord2() }) {
                  QCOMPARE(c->up(), entry.up[i]);
                  QCOMPARE(static_cast<int>(c->stemDirection()), static_cast<int>(entry.directions[i]));
                  QVERIFY(qAbs(c->stem()->len() - entry.lengths[i]) < 1e-6);
                  ++i;
                  }
            }
      delete score;
      }


// X1-X6 exercise both source staffs, reversed order and multi-note chords.
// X7/X8 cover pairs moved together to the other staff.
void TestBeam::studioCrossStaffTremoloSwitch()
      {
      MasterScore* score = readScore(DIR + "studio-tremolo-cross-standalone.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      struct Saved {
            Tremolo* tremolo;
            QRectF box;
            QPointF pos;
            bool up[2];
            qreal lengths[2];
            };
      std::vector<Saved> saved;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (Segment* seg = m->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                  for (Element* e : seg->elist()) {
                        if (!e || !e->isChord())
                              continue;
                        Tremolo* t = toChord(e)->tremolo();
                        if (!t || !t->twoNotes() || t->chord1() != e)
                              continue;
                        QVERIFY(t->chord2() && t->chord1()->stem() && t->chord2()->stem());
                        Saved entry;
                        entry.tremolo = t;
                        entry.box = t->bbox();
                        entry.pos = t->pos();
                        int j = 0;
                        for (Chord* c : { t->chord1(), t->chord2() }) {
                              entry.up[j] = c->up();
                              entry.lengths[j++] = c->stem()->len();
                              }
                        saved.push_back(entry);
                        }
                  }
            }
      QCOMPARE(static_cast<int>(saved.size()), 8);
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      std::vector<QRectF> modernBoxes;
      std::vector<qreal> modernLengths;
      for (size_t i = 0; i < saved.size(); ++i) {
            Tremolo* t = saved[i].tremolo;
            Chord* a = t->chord1();
            Chord* b = t->chord2();
            bool ownerUp = false;
            {
                  QVERIFY(beam4TremoloStemUp(t, ownerUp));
                  QCOMPARE(ownerUp, i == 2 || i == 3 || i == 5);
                  if (i < 6)
                        QVERIFY(a->up() != b->up());
                  else {
                        QVERIFY(a->up() && b->up()); // X7/X8 combined Auto, then invert
                        QVERIFY(a->staffMove() == b->staffMove());
                        }
                  for (Chord* c : { a, b }) {
                        if (i < 6) { // Stems point into the space between staves.
                              QCOMPARE(c->up(), c->vStaffIdx() > qMin(a->vStaffIdx(), b->vStaffIdx()));
                              }
                        QCOMPARE(static_cast<int>(c->stemDirection()), static_cast<int>(Direction::AUTO));
                        QVERIFY(qIsFinite(c->stem()->len()) && c->stem()->len() > 0);
                        }
                  const qreal sp = t->spatium();
                  const qreal width = score->styleP(Sid::beamWidth) * t->mag();
                  const qreal stack = (t->lines() - 1) * .75 * sp * t->mag();
                  const qreal x1 = a->stem()->pagePos().x();
                  const qreal x2 = b->stem()->pagePos().x();
                  // Remove the outer edge/stack extension from each tip to
                  // independently reconstruct the common primary stroke line.
                  const auto anchorY = [=](const Chord* c) {
                        const qreal extension = width / 2 + (c->up() == ownerUp ? 0 : stack);
                        return c->stem()->pagePos().y() + c->stem()->p2().y()
                              - (c->up() ? -1 : 1) * extension;
                        };
                  const qreal y1 = anchorY(a);
                  const qreal y2 = anchorY(b);
                  const qreal slope = (y2 - y1) / (x2 - x1);
                  QVERIFY(qAbs(y2 - y1) <= 2 * sp); // Studio's maximum slant
                  if (i >= 6)
                        QVERIFY(qAbs(y2 - y1) < 1e-6); // X7/X8 flatten under Studio's target rule
                  const QRectF box = t->bbox().translated(t->pagePos());
                  QVERIFY(qIsFinite(box.top()) && qIsFinite(box.bottom()));
                  QVERIFY(qAbs(box.left() - (x1 - a->stem()->lineWidthMag() / 2) - sp) < 1e-6);
                  QVERIFY(qAbs(box.right() - (x2 + b->stem()->lineWidthMag() / 2) + sp) < 1e-6);
                  const qreal leftY = y1 + (box.left() - x1) * slope;
                  const qreal rightY = y1 + (box.right() - x1) * slope;
                  QVERIFY(qAbs(box.top() - qMin(leftY, rightY) + width / 2 + (ownerUp ? 0 : stack)) < 1e-6);
                  QVERIFY(qAbs(box.bottom() - qMax(leftY, rightY) - width / 2 - (ownerUp ? stack : 0)) < 1e-6);
                  // Explicit direction and manual stems remain outside this stage.
                  a->setStemDirection(Direction::UP);
                  QVERIFY(!beam4TremoloStemUp(t, ownerUp));
                  a->setStemDirection(Direction::AUTO);
                  a->stem()->setUserLen(sp);
                  QPainterPath path;
                  QVERIFY(!beam4LayoutStandaloneTremolo(t, path));
                  a->stem()->setUserLen(0);
                  if (i >= 6) {
                        // An invalid provisional stem origin must bypass
                        // atomically, rather than reflect a negative length.
                        const QPointF stemPos = a->stem()->pos();
                        const qreal lengthA = a->stem()->len();
                        const qreal lengthB = b->stem()->len();
                        a->stem()->setPos(stemPos + QPointF(0, -100 * sp));
                        QPainterPath sentinel;
                        sentinel.addRect(1, 2, 3, 4);
                        const QPainterPath original = sentinel;
                        QVERIFY(!beam4LayoutStandaloneTremolo(t, sentinel));
                        QVERIFY(sentinel == original);
                        QCOMPARE(a->stem()->len(), lengthA);
                        QCOMPARE(b->stem()->len(), lengthB);
                        a->stem()->setPos(stemPos);
                        }
                  }
            modernBoxes.push_back(t->bbox());
            modernLengths.push_back(a->stem()->len());
            modernLengths.push_back(b->stem()->len());
            }
      score->doLayout();
      for (size_t i = 0; i < saved.size(); ++i) {
            Tremolo* t = saved[i].tremolo;
            QVERIFY(QLineF(t->bbox().topLeft(), modernBoxes[i].topLeft()).length() < 1e-6);
            QVERIFY(QLineF(t->bbox().bottomRight(), modernBoxes[i].bottomRight()).length() < 1e-6);
            QVERIFY(qAbs(t->chord1()->stem()->len() - modernLengths[2 * i]) < 1e-6);
            QVERIFY(qAbs(t->chord2()->stem()->len() - modernLengths[2 * i + 1]) < 1e-6);
            }
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      for (const Saved& entry : saved) {
            Tremolo* t = entry.tremolo;
            QVERIFY(QLineF(t->pos(), entry.pos).length() < 1e-6);
            QVERIFY(QLineF(t->bbox().topLeft(), entry.box.topLeft()).length() < 1e-6);
            QVERIFY(QLineF(t->bbox().bottomRight(), entry.box.bottomRight()).length() < 1e-6);
            int j = 0;
            for (Chord* c : { t->chord1(), t->chord2() }) {
                  QCOMPARE(c->up(), entry.up[j]);
                  QVERIFY(qAbs(c->stem()->len() - entry.lengths[j++]) < 1e-6);
                  }
            }
      delete score;
      }

QTEST_MAIN(TestBeam)
#include "tst_beam.moc"

