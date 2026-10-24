// MuseScore — optional Studio spacing regression tests.
// Licensed under GNU GPL version 2, as published in LICENSE.GPL.
#include <QtTest/QtTest>
#include "mtest/testutils.h"
#include "libmscore/score.h"
#include "libmscore/measure.h"
#include "libmscore/segment.h"
#include "libmscore/spacinglayout4.h"
#include "libmscore/spacinggeometry4.h"
#include "libmscore/spacingcollision4.h"
#include "libmscore/spacingplan4.h"
#include "libmscore/gracespacing4.h"
#include "libmscore/system.h"
#include "libmscore/shape.h"
#include "libmscore/notedot.h"
#include "libmscore/clef.h"
#include "libmscore/rest.h"
#include "libmscore/tie.h"
#include <limits>
#include "libmscore/chordrest.h"
#include "libmscore/beam.h"
#include "libmscore/chord.h"
#include "libmscore/note.h"
#include "libmscore/accidental.h"
#include "libmscore/pitchspelling.h"
#include "libmscore/stem.h"
#include "libmscore/slur.h"
#include "libmscore/spannermap.h"

using namespace Ms;
#define DIR QString("libmscore/layout/")

class TestSpacing : public QObject, public MTest {
      Q_OBJECT
   private slots:
      void initTestCase() { initMTest(); }
      void styleRoundTrip();
      void durationPolicy();
      void springForce();
      void graceBeforePlan_data();
      void graceBeforePlan();
      void graceBeforeLayout_data();
      void graceBeforeLayout();
      void graceNeighborPlan_data();
      void graceNeighborPlan();
      void graceAfterSystem_data();
      void graceAfterSystem();
      void graceAfterGeometry_data();
      void graceAfterGeometry();
      void graceAfterLayout_data();
      void graceAfterLayout();
      void graceAfterPlan_data();
      void graceAfterPlan();
      void graceAfterBaseline_data();
      void graceAfterBaseline();
      void graceFitting_data();
      void graceFitting();
      void graceSqueeze_data();
      void graceSqueeze();
      void graceSlurSpacing_data();
      void graceSlurSpacing();
      void fixedOffsetPlan();
      void geometryResolution();
      void geometryInputs_data();
      void geometryInputs();
      void collisionPolicy();
      void placementPlan_data();
      void placementPlan();
      void finalPlacement_data();
      void finalPlacement();
      void fittingSwitch_data();
      void fittingSwitch();
      void squeezePlan_data();
      void squeezePlan();
      void courtesyPlan_data();
      void courtesyPlan();
      void polyrhythmInputs_data();
      void polyrhythmInputs();
      void crossStaffPlanBoundary_data();
      void crossStaffPlanBoundary();
      void crossStaffInterleaved_data();
      void crossStaffInterleaved();
      void crossStaffSystems_data();
      void crossStaffSystems();
      void crossStaffFitting_data();
      void crossStaffFitting();
      void rhythmicLayoutSwitch();
      void crossBeamSwitch_data();
      void crossBeamSwitch();
      void crossBeamCorners_data();
      void crossBeamCorners();
      void sustainedVoiceSwitch_data();
      void sustainedVoiceSwitch();
      void accidentalInputs_data();
      void accidentalInputs();
      void systemInputs_data();
      void systemInputs();
      void finalSystemSwitch_data();
      void finalSystemSwitch();
      void boundarySystems_data();
      void boundarySystems();
};

void TestSpacing::styleRoundTrip()
      {
      MasterScore* score = readScore(DIR + "studio-spacing-natural-h1.mscx");
      QVERIFY(score);
      QVERIFY(!score->styleB(Sid::spacingLayout4));
      QVERIFY(score->styleB(Sid::beamLayout4));
      score->setStyleValue(Sid::spacingLayout4, true);
      score->setStyleValue(Sid::beamLayout4, false);
      QVERIFY(saveScore(score, "studio-spacing-roundtrip.mscx"));
      MasterScore* loaded = readCreatedScore("studio-spacing-roundtrip.mscx");
      QVERIFY(loaded);
      QVERIFY(loaded->styleB(Sid::spacingLayout4));
      QVERIFY(!loaded->styleB(Sid::beamLayout4));
      score->setStyleValue(Sid::spacingLayout4, false);
      QVERIFY(saveScore(score, "studio-spacing-off.mscx"));
      MasterScore* off = readCreatedScore("studio-spacing-off.mscx");
      QVERIFY(off);
      QVERIFY(!off->styleB(Sid::spacingLayout4));
      delete off;
      delete loaded;
      delete score;
      }

void TestSpacing::finalSystemSwitch_data()
      {
      QTest::addColumn<QString>("file");
      QTest::newRow("equal") << QString("studio-spacing-natural-h1.mscx");
      QTest::newRow("contour") << QString("studio-spacing-natural-h2.mscx");
      QTest::newRow("rhythm") << QString("studio-spacing-natural-h5.mscx");
      QTest::newRow("triplets") << QString("studio-spacing-natural-h8.mscx");
      QTest::newRow("trailing-frame") << QString("studio-spacing-final-frame.mscx");
      }

void TestSpacing::finalSystemSwitch()
      {
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      score->doLayout();
      const qreal legacy = score->firstMeasure()->width();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      const qreal modern = score->firstMeasure()->width();
      QVERIFY(qIsFinite(modern) && modern > 0);
      QVERIFY(modern < legacy - 1e-6);
      score->doLayout();
      QVERIFY(qAbs(score->firstMeasure()->width() - modern) < 1e-6);
      // The spacing policy remains enabled when the beam choice changes.
      // Different beam geometry may independently affect collision shapes.
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      QVERIFY(score->styleB(Sid::spacingLayout4));
      QVERIFY(qIsFinite(score->firstMeasure()->width()) && score->firstMeasure()->width() > 0);
      score->setStyleValue(Sid::beamLayout4, true);
      score->setStyleValue(Sid::lastSystemFillLimit, 0.0);
      score->doLayout();
      QVERIFY(score->firstMeasure()->width() > modern + 1e-6);
      score->setStyleValue(Sid::lastSystemFillLimit, 1.0);
      score->doLayout();
      QVERIFY(qAbs(score->firstMeasure()->width() - modern) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      QVERIFY(qAbs(score->firstMeasure()->width() - legacy) < 1e-6);
      delete score;
      }

void TestSpacing::boundarySystems_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<bool>("section");
      QTest::newRow("line-break") << QString("studio-spacing-final-line.mscx") << false;
      QTest::newRow("section-break") << QString("studio-spacing-final-section.mscx") << true;
      }

void TestSpacing::boundarySystems()
      {
      QFETCH(QString, file);
      QFETCH(bool, section);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      score->doLayout();
      Measure* first = score->firstMeasure();
      Measure* last = first->nextMeasure();
      QVERIFY(last && first->system() != last->system());
      const qreal legacyFirst = first->width();
      const qreal legacyLast = last->width();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      if (section)
            QVERIFY(first->width() < legacyFirst - 1e-6);
      else
            QVERIFY(qAbs(first->width() - legacyFirst) < 1e-6);
      QVERIFY(last->width() < legacyLast - 1e-6);
      const qreal modernFirst = first->width();
      const qreal modernLast = last->width();
      score->doLayout();
      QVERIFY(qAbs(first->width() - modernFirst) < 1e-6);
      QVERIFY(qAbs(last->width() - modernLast) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      QVERIFY(qAbs(first->width() - legacyFirst) < 1e-6);
      QVERIFY(qAbs(last->width() - legacyLast) < 1e-6);
      delete score;
      }


void TestSpacing::durationPolicy()
      {
      QVERIFY(qAbs(spacing4DurationStretch(1.5, .25, .25) - 1) < 1e-12);
      QVERIFY(qAbs(spacing4DurationStretch(1.5, .125, .125) - 2.0 / 3) < 1e-12);
      QVERIFY(qAbs(spacing4DurationStretch(1.5, .0625, .0625) - 4.0 / 9) < 1e-12);
      const double triplet = std::pow(1.5, std::log2(1.0 / 3));
      QVERIFY(qAbs(spacing4DurationStretch(1.5, 1.0 / 12, 1.0 / 12) - triplet) < 1e-12);
      // Non-adjacent onsets divide the shortest sounding value proportionally.
      QVERIFY(qAbs(spacing4DurationStretch(1.5, .0625, .25) - .25) < 1e-12);
      // A preceding finer subdivision carries its pulse into the next onset.
      QVERIFY(qAbs(spacing4DurationStretch(1.5, .0625, .25, .03125, .125) - 1.0 / 3) < 1e-12);
      // A preceding adjacent onset does not carry that special subdivision.
      QVERIFY(qAbs(spacing4DurationStretch(1.5, .0625, .25, .125, .125) - .25) < 1e-12);
      QVERIFY(qAbs(spacing4DurationStretch(1.2, .125, .125) - 1.0 / 1.2) < 1e-12);
      }

void TestSpacing::rhythmicLayoutSwitch()
      {
      MasterScore* score = readScore(DIR + "studio-spacing-baseline.mscx");
      QVERIFY(score);
      score->doLayout();
      struct Saved {
            Measure* measure;
            qreal width;
            std::vector<qreal> x;
            std::vector<qreal> widths;
            };
      std::vector<Saved> saved;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            Saved entry;
            entry.measure = m;
            entry.width = m->width();
            for (Segment& seg : m->segments()) {
                  entry.x.push_back(seg.x());
                  entry.widths.push_back(seg.width());
                  }
            saved.push_back(entry);
            }
      QCOMPARE(static_cast<int>(saved.size()), 8);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      std::vector<std::vector<qreal>> modernX;
      for (size_t i = 0; i < saved.size(); ++i) {
            Measure* m = saved[i].measure;
            std::vector<qreal> positions;
            qreal springForce = 0;
            for (Segment& seg : m->segments()) {
                  QVERIFY(qIsFinite(seg.x()) && qIsFinite(seg.width()) && seg.width() >= 0);
                  positions.push_back(seg.x());
                  if (!seg.isChordRestType() || seg.ticks() <= Fraction(0, 1))
                        continue;
                  // H1-H8 all contain a chord/rest ending at every next onset,
                  // including H7's eighth-note voice and H8's actual triplet ticks.
                  const qreal expected = std::pow(1.5, std::log2(seg.ticks().toDouble() / .25));
                  QVERIFY(qAbs(seg.stretch() - expected) < 1e-9);
                  QVERIFY(seg.width() + 1e-6 >= 3.5 * m->spatium() * expected);
                  // On these generously filled H5/H8 systems all springs are
                  // free of their collision floor: widths follow rhythmic weights.
                  if (i == 4 || i == 7) {
                        const qreal force = seg.width() / expected;
                        if (springForce > 0)
                              QVERIFY(qAbs(force - springForce) < 1e-6);
                        springForce = force;
                        }
                  }
            modernX.push_back(positions);
            }
      score->doLayout();
      for (size_t i = 0; i < saved.size(); ++i) {
            size_t j = 0;
            for (Segment& seg : saved[i].measure->segments()) {
                  QVERIFY(j < modernX[i].size());
                  QVERIFY(qAbs(seg.x() - modernX[i][j++]) < 1e-6);
                  }
            QCOMPARE(static_cast<int>(j), static_cast<int>(modernX[i].size()));
            }
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      for (const Saved& entry : saved) {
            QVERIFY(qAbs(entry.measure->width() - entry.width) < 1e-6);
            size_t j = 0;
            for (Segment& seg : entry.measure->segments()) {
                  QVERIFY(j < entry.x.size());
                  QVERIFY(qAbs(seg.x() - entry.x[j]) < 1e-6);
                  QVERIFY(qAbs(seg.width() - entry.widths[j++]) < 1e-6);
                  }
            QCOMPARE(static_cast<int>(j), static_cast<int>(entry.x.size()));
            }
      delete score;
      }

void TestSpacing::springForce()
      {
      const double weights = 1 + 2.0 / 3 + 4.0 / 9;
      std::vector<SpacingSpring4> rhythm = { { 3.5, 1 }, { 3.5 * 2 / 3, 2.0 / 3 }, { 3.5 * 4 / 9, 4.0 / 9 } };
      QVERIFY(qAbs(spacing4SpringForce(rhythm, 2 * weights) - 5.5) < 1e-12);
      std::vector<SpacingSpring4> collision = { { 1, 1 }, { 10, 1 } };
      QCOMPARE(spacing4SpringForce(collision, 1), 2.0);
      QCOMPARE(spacing4SpringForce(collision, 9), 10.0);
      QCOMPARE(spacing4SpringForce(collision, 0), 0.0);
      QCOMPARE(spacing4SpringForce({}, 1), 0.0);
      QCOMPARE(spacing4SpringForce({ { 1, 0 } }, 1), 0.0);
      }

void TestSpacing::fixedOffsetPlan()
      {
      MasterScore* score = readScore(DIR + "v0.59-spacing-polyrhythm-sustained-natural.mscx");
      QVERIFY(score);
      score->doLayout();
      Measure* measure = score->firstMeasure();
      QVERIFY(measure);
      std::vector<Segment*> segments;
      for (Segment& s : measure->segments()) {
            if (s.isChordRestType() && segments.size() < 3) segments.push_back(&s);
            }
      QCOMPARE(static_cast<int>(segments.size()), 3);
      const qreal nativeWidth = measure->width();
      const qreal nativeX[] = { segments[0]->x(), segments[1]->x(), segments[2]->x() };
      const qreal offsets[][3] = { { 0, 0, 0 }, { 1, 2, .5 }, { 1, -2, .5 } };
      const qreal flexible[] = { 4, 8, 5 };
      const qreal stretches[] = { 1, 2, .5 };
      for (const auto& row : offsets) {
            for (qreal extra : { qreal(3), qreal(24) }) {
                  SpacingPlan4 plan;
                  plan.measures.push_back(SpacingMeasurePlan4(measure));
                  plan.measures.front().x = 10;
                  qreal cursor = 10;
                  for (int i = 0; i < 3; ++i) {
                        SpacingSegmentPlan4 s(segments[i]);
                        s.x = cursor;
                        s.widthOffset = row[i];
                        s.width = flexible[i] + row[i];
                        s.stretch = stretches[i];
                        cursor += s.width;
                        plan.segments.push_back(s);
                        }
                  plan.measures.front().width = cursor - 10;
                  plan.width = cursor;
                  const qreal target = cursor + extra;
                  QVERIFY(justifySpacingPlan4(plan, target));
                  // Hand-solved source spring cases: at +3 the third spring
                  // stays at its floor; at +24 all three springs are active.
                  const qreal expected[] = { extra == 3 ? 5 : 82.0 / 7,
                        extra == 3 ? 10 : 164.0 / 7, extra == 3 ? 5 : 41.0 / 7 };
                  cursor = 10;
                  for (int i = 0; i < 3; ++i) {
                        QCOMPARE(plan.segments[i].widthOffset, row[i]);
                        QVERIFY(qAbs(plan.segments[i].width - expected[i] - row[i]) < 1e-9);
                        QVERIFY(qAbs(plan.segments[i].x - cursor) < 1e-9);
                        cursor += plan.segments[i].width;
                        }
                  QVERIFY(qAbs(cursor - target) < 1e-9);
                  QVERIFY(qAbs(plan.width - target) < 1e-9);
                  QVERIFY(qAbs(plan.measures.front().width - (target - 10)) < 1e-9);
                  QCOMPARE(plan.measures.front().x, qreal(10));
                  SpacingPlan4 invalid = plan;
                  invalid.segments.front().widthOffset = invalid.segments.front().width + 1;
                  const qreal savedWidth = invalid.segments.front().width;
                  QVERIFY(!justifySpacingPlan4(invalid, target + 1));
                  QCOMPARE(invalid.width, plan.width);
                  QCOMPARE(invalid.segments.front().width, savedWidth);
                  QCOMPARE(invalid.segments.front().widthOffset, savedWidth + 1);
                  }
            }
      QCOMPARE(measure->width(), nativeWidth);
      for (int i = 0; i < 3; ++i) QCOMPARE(segments[i]->x(), nativeX[i]);
      delete score;
      }

void TestSpacing::geometryResolution()
      {
      MasterScore* score = readScore(DIR + "studio-spacing-natural-h1.mscx");
      QVERIFY(score);
      {
      Chord context(score);
      context.setParent(score->firstMeasure()->first(SegmentType::ChordRest));
      context.setTrack(0);
      context.setDurationType(TDuration::DurationType::V_EIGHTH);
      Note* ownedNote = new Note(score);
      context.add(ownedNote);
      Note& note = *ownedNote;
      note.setbbox(QRectF(1, 2, 3, 4));
      Accidental* accidental = new Accidental(score);
      accidental->setParent(&note);
      accidental->setTrack(0);
      accidental->setbbox(QRectF(-2, -1, 0.5, 2));
      accidental->setPos(-1, 0);
      note.setAccidental(accidental);
      NoteDot* dot = new NoteDot(score);
      dot->setParent(&note);
      dot->setTrack(0);
      dot->setPos(6, 0);
      note.dots().push_back(dot);
      const SpacingShape4 tagged = spacingElementShape4(&note);
      QCOMPARE(static_cast<int>(tagged.elements.size()), 3);
      QVERIFY(tagged.complete());
      QCOMPARE(tagged.elements[0].item, static_cast<const Element*>(&note));
      QCOMPARE(tagged.elements[1].item, static_cast<const Element*>(dot));
      QCOMPARE(tagged.elements[2].item, static_cast<const Element*>(accidental));
      const QPointF offset(7, -3);
      const SpacingShape4 shifted = tagged.translated(offset);
      for (size_t i = 0; i < tagged.elements.size(); ++i) {
            QCOMPARE(shifted.elements[i].rect, tagged.elements[i].rect.translated(offset));
            QCOMPARE(shifted.elements[i].item, tagged.elements[i].item);
            QCOMPARE(int(shifted.elements[i].identity), int(tagged.elements[i].identity));
            }
      Shape native;
      const QRectF rect(1, 2, 3, 4);
      native.add(rect);
      native.add(QRectF(-5, 0, 2, 0)); // synthetic horizontal spacer, no inferred owner
      SpacingShape4 candidates;
      candidates.elements.push_back(SpacingShapeElement4(rect, &note));
      SpacingShape4 resolved = spacingResolveShape4(native, candidates);
      QCOMPARE(resolved.elements[0].item, static_cast<const Element*>(&note));
      QVERIFY(!resolved.complete());
      QCOMPARE(int(resolved.elements[1].identity), int(SpacingIdentity4::UNCLASSIFIED));
      candidates.elements.push_back(SpacingShapeElement4(rect, accidental));
      resolved = spacingResolveShape4(native, candidates);
      QCOMPARE(int(resolved.elements[0].identity), int(SpacingIdentity4::AMBIGUOUS));
      QVERIFY(!resolved.elements[0].item);
      std::reverse(candidates.elements.begin(), candidates.elements.end());
      QCOMPARE(int(spacingResolveShape4(native, candidates).elements[0].identity), int(SpacingIdentity4::AMBIGUOUS));
      // Unknown provenance must not be overridden by an overlapping known item.
      candidates.elements.clear();
      candidates.elements.push_back(SpacingShapeElement4(rect, &note));
      candidates.elements.push_back(SpacingShapeElement4(rect));
      QCOMPARE(int(spacingResolveShape4(native, candidates).elements[0].identity), int(SpacingIdentity4::UNCLASSIFIED));
      candidates.elements.clear();
      candidates.elements.push_back(SpacingShapeElement4(rect.translated(1e-12, 0), &note));
      QCOMPARE(int(spacingResolveShape4(native, candidates).elements[0].identity), int(SpacingIdentity4::UNCLASSIFIED));
      native.add(rect); // only one of two identical primitives has an owner
      candidates.elements.clear();
      candidates.elements.push_back(SpacingShapeElement4(rect, &note));
      resolved = spacingResolveShape4(native, candidates);
      QCOMPARE(int(resolved.elements[0].identity), int(SpacingIdentity4::UNCLASSIFIED));
      QCOMPARE(int(resolved.elements[2].identity), int(SpacingIdentity4::UNCLASSIFIED));
      // Producer-tagged aggregates remain in the snapshot but have a separate
      // collision projection. Arbitrary zero-height rectangles stay opaque.
      const SpacingPaddingTable4 table(score);
      SpacingShape4 aggregate = tagged;
      SpacingShapeElement4 spacer(QRectF(-1, 0, 8, 0), &context);
      spacer.identity = SpacingIdentity4::LEGACY_SPACER;
      aggregate.elements.push_back(spacer);
      SpacingShape4 projected;
      QVERIFY(spacingCollisionProjection4(table, aggregate, score->spatium(), projected));
      QCOMPARE(static_cast<int>(aggregate.elements.size()), 4);
      QCOMPARE(static_cast<int>(projected.elements.size()), 3);
      QVERIFY(projected.complete());
      aggregate.elements.back().identity = SpacingIdentity4::UNCLASSIFIED;
      aggregate.elements.back().item = nullptr;
      QVERIFY(!spacingCollisionProjection4(table, aggregate, score->spatium(), projected));
      QCOMPARE(static_cast<int>(projected.elements.size()), 3);
      SpacingShape4 orphan;
      orphan.elements.push_back(spacer);
      QVERIFY(!spacingCollisionProjection4(table, orphan, score->spatium(), projected));
      // Two voices can emit the same legacy aggregate. Preserve all owners
      // only when native/candidate multiplicities match exactly.
      Chord secondContext(score);
      secondContext.setParent(context.parent());
      secondContext.setTrack(1);
      secondContext.setDurationType(TDuration::DurationType::V_EIGHTH);
      Note* secondNote = new Note(score);
      secondContext.add(secondNote);
      secondNote->setbbox(QRectF(2, 8, 3, 4));
      // Shared direct producers are admitted only at staff assembly, with
      // exact counts. Projection keeps every note's pair policy available.
      Shape directNative;
      directNative.add(rect);
      directNative.add(rect);
      SpacingShape4 directOwners;
      directOwners.elements.push_back(SpacingShapeElement4(rect, &note));
      directOwners.elements.push_back(SpacingShapeElement4(rect, secondNote));
      QCOMPARE(int(spacingResolveShape4(directNative, directOwners).elements.front().identity),
            int(SpacingIdentity4::AMBIGUOUS));
      SpacingShape4 directShared = spacingResolveShape4(directNative, directOwners, true);
      QVERIFY(!directShared.complete());
      for (const SpacingShapeElement4& e : directShared.elements) {
            QCOMPARE(int(e.identity), int(SpacingIdentity4::SHARED_ITEM));
            QVERIFY(!e.item && e.aggregateOwners.empty());
            QCOMPARE(static_cast<int>(e.sharedItems.size()), 2);
            }
      std::reverse(directOwners.elements.begin(), directOwners.elements.end());
      QVERIFY(directShared.elements.front().sharedItems
            == spacingResolveShape4(directNative, directOwners, true).elements.front().sharedItems);
      QVERIFY(directShared.translated(offset).elements.front().sharedItems == directShared.elements.front().sharedItems);
      QVERIFY(spacingCollisionProjection4(table, directShared, score->spatium(), projected));
      QCOMPARE(static_cast<int>(projected.elements.size()), 4);
      QVERIFY(projected.complete());
      for (const Element* owner : { static_cast<const Element*>(&note), static_cast<const Element*>(secondNote) }) {
            int count = 0;
            for (const SpacingShapeElement4& e : projected.elements) count += e.item == owner;
            QCOMPARE(count, 2);
            }
      SpacingShape4 malformed = directShared;
      malformed.elements.front().sharedItems.back() = malformed.elements.front().sharedItems.front();
      QVERIFY(!spacingCollisionProjection4(table, malformed, score->spatium(), projected));
      QCOMPARE(static_cast<int>(projected.elements.size()), 4);
      malformed = directShared;
      malformed.elements.front().sharedItems.back() = accidental; // shared non-note stays unsupported
      QVERIFY(!spacingCollisionProjection4(table, malformed, score->spatium(), projected));
      directOwners.elements.push_back(SpacingShapeElement4(rect));
      QVERIFY(spacingResolveShape4(directNative, directOwners, true).elements.front().identity != SpacingIdentity4::SHARED_ITEM);
      directOwners.elements.pop_back();
      directNative.add(rect);
      QVERIFY(spacingResolveShape4(directNative, directOwners, true).elements.front().identity != SpacingIdentity4::SHARED_ITEM);
      Shape sharedNative;
      sharedNative.add(spacer.rect);
      sharedNative.add(spacer.rect);
      SpacingShape4 owners;
      owners.elements.push_back(spacer);
      SpacingShapeElement4 otherSpacer = spacer;
      otherSpacer.item = &secondContext;
      owners.elements.push_back(otherSpacer);
      SpacingShape4 shared = spacingResolveShape4(sharedNative, owners);
      for (const SpacingShapeElement4& entry : shared.elements) {
            QCOMPARE(int(entry.identity), int(SpacingIdentity4::LEGACY_SPACER));
            QVERIFY(!entry.item);
            QCOMPARE(static_cast<int>(entry.aggregateOwners.size()), 2);
            }
      std::reverse(owners.elements.begin(), owners.elements.end());
      const SpacingShape4 permuted = spacingResolveShape4(sharedNative, owners);
      QVERIFY(shared.elements.front().aggregateOwners == permuted.elements.front().aggregateOwners);
      aggregate = tagged;
      aggregate.elements.push_back(SpacingShapeElement4(secondNote->bbox(), secondNote));
      aggregate.elements.insert(aggregate.elements.end(), shared.elements.begin(), shared.elements.end());
      QVERIFY(spacingCollisionProjection4(table, aggregate, score->spatium(), projected));
      QCOMPARE(static_cast<int>(projected.elements.size()), 4);
      aggregate.elements.erase(aggregate.elements.begin() + tagged.elements.size());
      QVERIFY(!spacingCollisionProjection4(table, aggregate, score->spatium(), projected));
      QCOMPARE(static_cast<int>(projected.elements.size()), 4); // missing owner's footprint cannot be discarded
      sharedNative.add(spacer.rect);
      shared = spacingResolveShape4(sharedNative, owners);
      QVERIFY(shared.elements.front().identity != SpacingIdentity4::LEGACY_SPACER);
      owners.elements.push_back(SpacingShapeElement4(spacer.rect));
      shared = spacingResolveShape4(sharedNative, owners);
      QVERIFY(shared.elements.front().identity != SpacingIdentity4::LEGACY_SPACER);
      }
      delete score;
      }

void TestSpacing::geometryInputs_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<bool>("modernBeams");
      for (bool beams : { false, true }) {
            const QString suffix = beams ? "-studio-beams" : "-legacy-beams";
            QTest::newRow(qPrintable("ordinary" + suffix)) << QString("studio-spacing-baseline.mscx") << beams;
            QTest::newRow(qPrintable("accidentals-cross-staff" + suffix)) << QString("v0.40-spacing-accidentals.mscx") << beams;
            QTest::newRow(qPrintable("mixed-voices" + suffix)) << QString("v0.38-spacing-voices.mscx") << beams;
            }
      }

void TestSpacing::geometryInputs()
      {
      QFETCH(QString, file);
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      int heads = 0;
      int signs = 0;
      int stems = 0;
      int crossStaffHeads = 0;
      int itemPairQueries = 0;
      int notePairQueries = 0;
      int opaqueShapeFallbacks = 0;
      const SpacingPaddingTable4 paddingTable(score);
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            const qreal measureWidth = m->width();
            for (Segment& seg : m->segments()) {
                  const qreal x = seg.x(), width = seg.width(), offset = seg.spacingWidthOffset4();
                  const std::vector<Shape> native = seg.shapes();
                  for (int staffIdx = 0; staffIdx < score->nstaves(); ++staffIdx) {
                        const SpacingShape4 tagged = spacingStaffShape4(&seg, staffIdx);
                        QCOMPARE(static_cast<int>(tagged.elements.size()), static_cast<int>(native[staffIdx].size()));
                        for (size_t i = 0; i < tagged.elements.size(); ++i) {
                              const SpacingShapeElement4& e = tagged.elements[i];
                              QCOMPARE(e.rect, static_cast<const QRectF&>(native[staffIdx][i]));
                              QCOMPARE(static_cast<const QRectF&>(seg.staffShape(staffIdx)[i]), static_cast<const QRectF&>(native[staffIdx][i]));
                              if (e.identity == SpacingIdentity4::LEGACY_SPACER) {
                                    if (e.item) {
                                          QVERIFY(e.aggregateOwners.empty());
                                          QVERIFY(e.item->isChord() || e.item->isBarLine());
                                          }
                                    else {
                                          QVERIFY(e.aggregateOwners.size() > 1);
                                          for (const Element* owner : e.aggregateOwners)
                                                QVERIFY(owner && (owner->isChord() || owner->isBarLine()));
                                          }
                                    continue;
                                    }
                              if (e.identity != SpacingIdentity4::ITEM) {
                                    QVERIFY(!e.item);
                                    continue;
                                    }
                              QVERIFY(e.item);
                              if (e.item->isNote()) {
                                    ++heads;
                                    const Chord* chord = toNote(e.item)->chord();
                                    QVERIFY(chord);
                                    QCOMPARE(chord->vStaffIdx(), staffIdx);
                                    QVERIFY(e.rect == e.item->bbox().translated(e.item->pos()).translated(chord->pos()));
                                    if (chord->staffMove())
                                          ++crossStaffHeads;
                                    }
                              if (e.item->isAccidental())
                                    ++signs;
                              if (e.item->isStem())
                                    ++stems;
                              }
                        Segment* next = seg.next(SegmentType::ChordRest);
                        if (seg.isChordRestType() && next) {
                              const SpacingShape4 following = spacingStaffShape4(next, staffIdx);
                              if (!tagged.elements.empty() && !following.elements.empty()) {
                                    const auto opaque = [](const SpacingShapeElement4& e) {
                                          return !e.rect.isNull() && (e.identity != SpacingIdentity4::ITEM || !e.item);
                                          };
                                    const bool hasOpaque = std::any_of(tagged.elements.begin(), tagged.elements.end(), opaque)
                                          || std::any_of(following.elements.begin(), following.elements.end(), opaque);
                                    qreal distance = 123;
                                    const bool supported = spacingShapeDistance4(paddingTable, tagged, following, m->spatium(), 1, distance);
                                    // Native chord shapes include legacy horizontal
                                    // spacers. Whole-shape fallback is intentional.
                                    if (hasOpaque) {
                                          QVERIFY(!supported);
                                          ++opaqueShapeFallbacks;
                                          }
                                    if (supported) {
                                          QVERIFY(qIsFinite(distance));
                                          qreal squeezed = 123;
                                          QVERIFY(spacingShapeDistance4(paddingTable, tagged, following, m->spatium(), 0.6, squeezed));
                                          QVERIFY(squeezed <= distance + 1e-6);
                                          }
                                    else
                                          QCOMPARE(distance, 123.0);
                                    // Exercise identified live items separately.
                                    // These probes never become a whole-shape floor.
                                    for (const SpacingShapeElement4& a : tagged.elements) {
                                          if (opaque(a) || a.rect.isNull())
                                                continue;
                                          for (const SpacingShapeElement4& b : following.elements) {
                                                if (opaque(b) || b.rect.isNull())
                                                      continue;
                                                SpacingShape4 one, two;
                                                one.elements.push_back(a);
                                                two.elements.push_back(b);
                                                qreal pairDistance = 123;
                                                const bool accepts = spacingShapeDistance4(paddingTable, one, two, m->spatium(), 1, pairDistance);
                                                if (a.item->isNote() && b.item->isNote()) {
                                                      QVERIFY2(accepts, "Plain note pair unexpectedly rejected by common-notation policy");
                                                      ++notePairQueries;
                                                      }
                                                if (accepts) {
                                                      ++itemPairQueries;
                                                      QVERIFY(qIsFinite(pairDistance));
                                                      qreal squeezed = 123;
                                                      QVERIFY(spacingShapeDistance4(paddingTable, one, two, m->spatium(), 0.6, squeezed));
                                                      QVERIFY(squeezed <= pairDistance + 1e-6);
                                                      }
                                                else
                                                      QCOMPARE(pairDistance, 123.0);
                                                }
                                          }
                                    }
                              }
                        }
                  QCOMPARE(seg.x(), x);
                  QCOMPARE(seg.width(), width);
                  QCOMPARE(seg.spacingWidthOffset4(), offset);
                  }
            QCOMPARE(m->width(), measureWidth);
            }
      QVERIFY(heads > 0);
      QVERIFY(stems > 0);
      QVERIFY(itemPairQueries > 0);
      QVERIFY(notePairQueries > 0);
      QVERIFY(opaqueShapeFallbacks > 0);
      if (file.contains("accidentals")) {
            QVERIFY(signs > 0);
            QVERIFY(crossStaffHeads > 0);
            }
      QVERIFY(spacingStaffShape4(nullptr, 0).elements.empty());
      QVERIFY(spacingStaffShape4(score->firstMeasure()->first(), -1).elements.empty());
      QVERIFY(spacingStaffShape4(score->firstMeasure()->first(), score->nstaves()).elements.empty());
      delete score;
      }

void TestSpacing::collisionPolicy()
      {
      MasterScore* score = readScore(DIR + "v0.40-spacing-accidentals.mscx");
      QVERIFY(score);
      score->doLayout();
      const SpacingPaddingTable4 table(score);
      const qreal sp = score->spatium();
      qreal padding = 0;
      QVERIFY(table.padding(ElementType::NOTE, ElementType::ACCIDENTAL, padding));
      QVERIFY(qAbs(padding - qMax(score->styleP(Sid::accidentalNoteDistance), 0.35 * sp)) < 1e-9);
      QVERIFY(table.padding(ElementType::ACCIDENTAL, ElementType::NOTE, padding));
      QCOMPARE(padding, score->styleP(Sid::accidentalNoteDistance));
      QVERIFY(table.padding(ElementType::STEM, ElementType::STEM, padding));
      QVERIFY(qAbs(padding - 0.85 * sp) < 1e-9);
      padding = 123;
      QVERIFY(!table.padding(ElementType::LYRICS, ElementType::NOTE, padding));
      QCOMPARE(padding, 123.0);
      {
      Segment* first = score->firstMeasure()->first(SegmentType::ChordRest);
      QVERIFY(first);
      Segment* next = first->next(SegmentType::ChordRest);
      QVERIFY(next);
      Chord contextA(score), contextB(score);
      contextA.setParent(first);
      contextB.setParent(next);
      contextA.setTrack(0);
      contextB.setTrack(0);
      contextA.setDurationType(TDuration::DurationType::V_EIGHTH);
      contextB.setDurationType(TDuration::DurationType::V_EIGHTH);
      Note* a = new Note(score);
      Note* b = new Note(score);
      contextA.add(a);
      contextB.add(b);
      a->setbbox(QRectF(0, 0, sp, sp));
      b->setbbox(QRectF(0, 0, sp, sp));
      SpacingPairPolicy4 policy;
      QVERIFY(spacingPairPolicy4(table, a, b, sp, policy));
      QCOMPARE(int(policy.kerning), int(SpacingKerning4::NON_KERNING));
      QVERIFY(qAbs(policy.padding - score->styleP(Sid::minNoteDistance)) < 1e-9);
      const SpacingShape4 left = spacingElementShape4(a);
      const SpacingShape4 right = spacingElementShape4(b);
      qreal distance = 0;
      QVERIFY(spacingShapeDistance4(table, left, right, sp, 1, distance));
      QVERIFY(qAbs(distance - sp - score->styleP(Sid::minNoteDistance)) < 1e-9);
      QVERIFY(spacingShapeDistance4(table, left, right.translated(QPointF(0, 10 * sp)), sp, 1, distance));
      QVERIFY(qAbs(distance - sp - score->styleP(Sid::minNoteDistance)) < 1e-9);
      contextB.setTrack(1);
      b->setTrack(1);
      b->setPos(0, 10 * sp);
      QVERIFY(spacingPairPolicy4(table, a, b, sp, policy));
      QCOMPARE(int(policy.kerning), int(SpacingKerning4::KERNING));
      QVERIFY(spacingShapeDistance4(table, left, spacingElementShape4(b).translated(b->pos()), sp, 1, distance));
      QCOMPARE(distance, -std::numeric_limits<qreal>::max());
      a->setSmall(true);
      QVERIFY(spacingPairPolicy4(table, a, b, sp, policy));
      QVERIFY(qAbs(policy.padding - 0.1 * sp * (a->mag() + b->mag()) / 2) < 1e-9);
      a->setSmall(false);
      Accidental accidental(score);
      accidental.setParent(b);
      accidental.setTrack(1);
      QVERIFY(spacingPairPolicy4(table, a, &accidental, sp, policy));
      QVERIFY(qAbs(policy.verticalClearance - 0.1 * sp) < 1e-9);
      Clef clef(score);
      clef.setParent(first);
      clef.setTrack(0);
      QVERIFY(spacingPairPolicy4(table, a, &clef, sp, policy));
      QCOMPARE(int(policy.kerning), int(SpacingKerning4::NON_KERNING));
      clef.setParent(next);
      QVERIFY(spacingPairPolicy4(table, a, &clef, sp, policy));
      QCOMPARE(int(policy.kerning), int(SpacingKerning4::RIGHT_EDGE));
      Rest fullRest(score);
      fullRest.setParent(first);
      fullRest.setTrack(4);
      fullRest.setDurationType(TDuration::DurationType::V_MEASURE);
      QVERIFY(spacingPairPolicy4(table, &fullRest, a, sp, policy));
      QCOMPARE(int(policy.kerning), int(SpacingKerning4::ALLOW_COLLISION));
      // Failed queries must leave output untouched, including late failure.
      qreal untouched = 321;
      SpacingShape4 incomplete = right;
      incomplete.elements.push_back(SpacingShapeElement4(QRectF(2, 0, 1, 1)));
      QVERIFY(!spacingShapeDistance4(table, left, incomplete, sp, 1, untouched));
      QCOMPARE(untouched, 321.0);
      QVERIFY(!spacingShapeDistance4(table, left, right, sp, -0.1, untouched));
      QCOMPARE(untouched, 321.0);
      Tie tie(score);
      a->setTieFor(&tie);
      policy.padding = 123;
      const bool acceptsTie = spacingPairPolicy4(table, a, b, sp, policy);
      a->setTieFor(nullptr);
      QVERIFY(!acceptsTie);
      QCOMPARE(policy.padding, 123.0);
      contextA.setNoteType(NoteType::ACCIACCATURA);
      QVERIFY(!spacingPairPolicy4(table, a, b, sp, policy));
      }
      // Vertical-clearance boundary, zero-width and edge kerning constraints.
      SpacingPairPolicy4 policy;
      policy.padding = 0.01 * sp;
      policy.verticalClearance = 0.2 * sp;
      policy.kerning = SpacingKerning4::KERNING;
      const QRectF r(0, 0, sp, sp);
      QVERIFY(qAbs(spacingPairDistance4(r, QRectF(0, 1.1 * sp, sp, sp), policy, sp, 1) - 1.1 * sp) < 1e-9);
      QCOMPARE(spacingPairDistance4(r, QRectF(0, 2 * sp, sp, sp), policy, sp, 1), -std::numeric_limits<qreal>::max());
      policy.kerning = SpacingKerning4::RIGHT_EDGE;
      QCOMPARE(spacingPairDistance4(r, QRectF(0, 2 * sp, sp, sp), policy, sp, 1), sp);
      policy.kerning = SpacingKerning4::KERNING;
      QVERIFY(qAbs(spacingPairDistance4(QRectF(0, 0, 0, sp), QRectF(0, 2 * sp, sp, sp), policy, sp, 0.5) - 0.05 * sp) < 1e-9);
      delete score;
      }

void TestSpacing::placementPlan_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::newRow("legacy-beams") << false;
      QTest::newRow("studio-beams") << true;
      }

void TestSpacing::placementPlan()
      {
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + "v0.41-spacing-systems.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      struct Saved {
            Measure* measure;
            qreal x, width;
            std::vector<qreal> segments;
            };
      std::vector<Saved> saved;
      std::vector<System*> systems;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            Saved entry;
            entry.measure = m;
            entry.x = m->x();
            entry.width = m->width();
            for (Segment& seg : m->segments()) {
                  entry.segments.push_back(seg.x());
                  entry.segments.push_back(seg.width());
                  entry.segments.push_back(seg.stretch());
                  entry.segments.push_back(seg.spacingWidthOffset4());
                  }
            saved.push_back(entry);
            if (systems.empty() || systems.back() != m->system())
                  systems.push_back(m->system());
            }
      QCOMPARE(static_cast<int>(systems.size()), 3);
      for (System* system : systems) {
            SpacingPlan4 plan;
            QString reason;
            QVERIFY2(buildSpacingPlan4(system, plan, &reason), qPrintable(reason));
            QCOMPARE(static_cast<int>(plan.measures.size()), 3);
            QVERIFY(reason.isEmpty());
            QVERIFY(qIsFinite(plan.width) && plan.width > system->leftMargin());
            for (size_t i = 0; i < plan.measures.size(); ++i) {
                  const SpacingMeasurePlan4& m = plan.measures[i];
                  if (i > 0)
                        QVERIFY(qAbs(m.x - plan.measures[i - 1].x - plan.measures[i - 1].width) < 1e-6);
                  qreal cursor = m.x;
                  bool first = true;
                  for (const SpacingSegmentPlan4& seg : plan.segments) {
                        if (seg.segment->measure() != m.measure || seg.ignored)
                              continue;
                        QVERIFY(qIsFinite(seg.x) && qIsFinite(seg.width) && seg.width >= 0);
                        if (first) {
                              QVERIFY(seg.x >= m.x - 1e-6);
                              cursor = seg.x;
                              first = false;
                              }
                        QVERIFY(qAbs(seg.x - cursor) < 1e-6);
                        cursor += seg.width;
                        }
                  QVERIFY(!first);
                  QVERIFY(qAbs(cursor - m.x - m.width) < 1e-6);
                  }
            SpacingPlan4 repeated;
            QVERIFY2(buildSpacingPlan4(system, repeated, &reason), qPrintable(reason));
            QCOMPARE(repeated.width, plan.width);
            QCOMPARE(static_cast<int>(repeated.segments.size()), static_cast<int>(plan.segments.size()));
            for (size_t i = 0; i < plan.segments.size(); ++i) {
                  QCOMPARE(repeated.segments[i].x, plan.segments[i].x);
                  QCOMPARE(repeated.segments[i].width, plan.segments[i].width);
                  }
            SpacingPlan4 justified = plan;
            const qreal target = plan.width + 20 * score->spatium();
            QVERIFY(justifySpacingPlan4(justified, target));
            QVERIFY(qAbs(justified.width - target) < 1e-6);
            std::vector<SpacingSpring4> springs;
            for (const SpacingSegmentPlan4& seg : plan.segments) {
                  if (!seg.ignored && seg.stretch > 0)
                        springs.push_back({ seg.width, seg.stretch });
                  }
            const qreal force = spacing4SpringForce(springs, target - plan.width);
            QVERIFY(force > 0);
            for (size_t i = 0; i < plan.segments.size(); ++i) {
                  const SpacingSegmentPlan4& before = plan.segments[i];
                  if (before.ignored)
                        continue;
                  const qreal expected = before.stretch > 0 ? qMax(before.width, force * before.stretch) : before.width;
                  QVERIFY(qAbs(justified.segments[i].width - expected) < 1e-6);
                  }
            SpacingPlanOptions4 compressed;
            compressed.stretchReduction = 0.67;
            compressed.squeezeFactor = 0.8;
            QVERIFY2(buildSpacingPlan4(system, repeated, &reason, compressed), qPrintable(reason));
            QVERIFY(repeated.width <= plan.width + 1e-6);
            const qreal oldMinimum = score->styleP(Sid::minMeasureWidth) / score->spatium();
            score->setStyleValue(Sid::minMeasureWidth, Spatium(80));
            const bool minimumOK = buildSpacingPlan4(system, repeated, &reason);
            score->setStyleValue(Sid::minMeasureWidth, Spatium(oldMinimum));
            QVERIFY2(minimumOK, qPrintable(reason));
            QVERIFY(repeated.width > plan.width);
            const qreal minimum = qMin(80 * score->spatium(), system->width() - system->leftMargin());
            for (const SpacingMeasurePlan4& m : repeated.measures)
                  QVERIFY(m.width + 1e-6 >= minimum);
            SpacingPlanOptions4 invalid;
            invalid.squeezeFactor = -1;
            repeated = plan;
            QVERIFY(!buildSpacingPlan4(system, repeated, &reason, invalid));
            QCOMPARE(repeated.width, plan.width);
            QCOMPARE(repeated.segments.front().x, plan.segments.front().x);
            QVERIFY(!justifySpacingPlan4(repeated, plan.width - 1));
            QCOMPARE(repeated.width, plan.width);
            }
      for (const Saved& entry : saved) {
            QCOMPARE(entry.measure->x(), entry.x);
            QCOMPARE(entry.measure->width(), entry.width);
            size_t i = 0;
            for (Segment& seg : entry.measure->segments()) {
                  QCOMPARE(seg.x(), entry.segments[i++]);
                  QCOMPARE(seg.width(), entry.segments[i++]);
                  QCOMPARE(seg.stretch(), entry.segments[i++]);
                  QCOMPARE(seg.spacingWidthOffset4(), entry.segments[i++]);
                  }
            }
      delete score;
      // Unsupported systems must leave an existing output untouched.
      score = readScore(DIR + "v0.40-spacing-accidentals.mscx");
      QVERIFY(score);
      score->doLayout();
      Measure* cross = score->firstMeasure();
      for (int i = 0; i < 3; ++i)
            cross = cross->nextMeasure(); // H24
      QVERIFY(cross);
      SpacingPlan4 unchanged;
      unchanged.width = 123;
      QString reason;
      QVERIFY(!buildSpacingPlan4(cross->system(), unchanged, &reason));
      QCOMPARE(unchanged.width, 123.0);
      QVERIFY(!reason.isEmpty());
      QVERIFY(!buildSpacingPlan4(nullptr, unchanged, &reason));
      QCOMPARE(unchanged.width, 123.0);
      delete score;
      }

void TestSpacing::finalPlacement_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<bool>("modernBeams");
      QTest::newRow("systems-legacy-beams") << QString("v0.47-spacing-placement.mscx") << false;
      QTest::newRow("systems-studio-beams") << QString("v0.47-spacing-placement.mscx") << true;
      QTest::newRow("natural-h5-legacy-beams") << QString("v0.47-spacing-natural-h5.mscx") << false;
      QTest::newRow("natural-h5-studio-beams") << QString("v0.47-spacing-natural-h5.mscx") << true;
      QTest::newRow("natural-h8-legacy-beams") << QString("v0.47-spacing-natural-h8.mscx") << false;
      QTest::newRow("natural-h8-studio-beams") << QString("v0.47-spacing-natural-h8.mscx") << true;
      }

void TestSpacing::finalPlacement()
      {
      QFETCH(QString, file);
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      QVERIFY(score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto snapshot = [score]() -> std::vector<qreal> {
            std::vector<qreal> result;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  result.push_back(m->x());
                  result.push_back(m->width());
                  for (Segment& s : m->segments()) {
                        result.push_back(s.x());
                        result.push_back(s.width());
                        }
                  }
            return result;
            };
      const std::vector<qreal> legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      std::vector<System*> systems;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            if (systems.empty() || systems.back() != m->system())
                  systems.push_back(m->system());
            }
      QCOMPARE(static_cast<int>(systems.size()), file.contains("placement") ? 3 : 1);
      const qreal available = score->styleD(Sid::pagePrintableWidth) * DPI;
      for (System* system : systems) {
            SpacingPlan4 expected;
            QString reason;
            QVERIFY2(buildSpacingPlan4(system, expected, &reason), qPrintable(reason));
            QVERIFY(expected.width <= available);
            // The overview fills all three systems; the natural controls have
            // fill limit 1 and remain unfilled. Require the actual layout to
            // match the planner, including fixed header/barline spacing.
            if (file.contains("placement"))
                  QVERIFY(justifySpacingPlan4(expected, available));
            QVERIFY(qAbs(system->width() - expected.width) < 1e-6);
            for (const SpacingMeasurePlan4& m : expected.measures) {
                  QVERIFY(qAbs(m.measure->x() - m.x) < 1e-6);
                  QVERIFY(qAbs(m.measure->width() - m.width) < 1e-6);
                  }
            for (const SpacingSegmentPlan4& s : expected.segments) {
                  if (s.ignored)
                        continue;
                  const Measure* m = s.segment->measure();
                  QVERIFY2(qAbs(m->x() + s.segment->x() - s.x) < 1e-6,
                        qPrintable(QString("segment at tick %1 did not use final plan").arg(s.segment->tick().ticks())));
                  QVERIFY(qAbs(s.segment->width() - s.width) < 1e-6);
                  QCOMPARE(s.segment->stretch(), s.stretch);
                  }
            }
      const std::vector<qreal> modern = snapshot();
      QVERIFY(modern != legacy);
      score->doLayout();
      const std::vector<qreal> repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i)
            QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const std::vector<qreal> restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i)
            QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::fittingSwitch_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::newRow("legacy-beams") << false;
      QTest::newRow("studio-beams") << true;
      }

void TestSpacing::fittingSwitch()
      {
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + "v0.48-spacing-fitting.mscx");
      QVERIFY(score);
      QVERIFY(score->styleB(Sid::spacingLayout4));
      QVERIFY(!score->styleB(Sid::hideEmptyStaves));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      // Start with all nine measures in one natural-width system, then choose
      // a page width between the planner and old per-measure fitting widths.
      score->setStyleValue(Sid::pageWidth, 100.0);
      score->setStyleValue(Sid::pagePrintableWidth, 98.8);
      score->doLayout();
      System* system = score->firstMeasure()->system();
      QVERIFY(system);
      int count = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            QCOMPARE(m->system(), system);
            QVERIFY(!m->lineBreak() && !m->pageBreak() && !m->noBreak());
            ++count;
            }
      QCOMPARE(count, 9);
      SpacingPlan4 plan;
      QString reason;
      QVERIFY2(buildSpacingPlan4(system, plan, &reason), qPrintable(reason));
      const qreal modernWidth = plan.width;
      qreal oldFittingWidth = system->leftMargin();
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            m->computeMinWidth();
            oldFittingWidth += m->width();
            }
      QVERIFY2(qAbs(modernWidth - oldFittingWidth) > 1e-4,
            "The fitting control must distinguish group planning from per-measure widths");
      const qreal available = (modernWidth + oldFittingWidth) / 2;
      score->setStyleValue(Sid::pagePrintableWidth, available / DPI);
      score->setStyleValue(Sid::pageWidth, available / DPI + 1.2);
      score->doLayout();
      const auto snapshot = [score]() -> std::vector<qreal> {
            std::vector<qreal> values;
            int systemIndex = -1;
            const System* previous = nullptr;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  if (m->system() != previous) {
                        ++systemIndex;
                        previous = m->system();
                        }
                  values.push_back(systemIndex);
                  values.push_back(m->x());
                  values.push_back(m->width());
                  for (Segment& s : m->segments()) {
                        // Changing line breaks can create generated headers.
                        // removeSystemHeader retains them disabled for reuse;
                        // their presence is not a difference in score layout.
                        if (!s.enabled() && (s.header() || s.trailer()))
                              continue;
                        values.push_back(static_cast<int>(s.segmentType()));
                        values.push_back(s.tick().ticks());
                        values.push_back(s.enabled() ? 1 : 0);
                        values.push_back(s.x());
                        values.push_back(s.width());
                        }
                  }
            return values;
            };
      const auto systemCount = [score]() {
            int n = 0;
            const System* previous = nullptr;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  if (m->system() != previous) {
                        previous = m->system();
                        ++n;
                        }
                  }
            return n;
            };
      if (modernWidth < oldFittingWidth)
            QCOMPARE(systemCount(), 1);
      else
            QVERIFY(systemCount() > 1);
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            if (m != m->system()->firstMeasure())
                  continue;
            QVERIFY2(buildSpacingPlan4(m->system(), plan, &reason), qPrintable(reason));
            QVERIFY(plan.width <= available + 1e-6);
            QVERIFY(m->system()->width() <= available + 1e-6);
            }
      const std::vector<qreal> modern = snapshot();
      score->doLayout();
      const std::vector<qreal> repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i)
            QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const std::vector<qreal> legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      const std::vector<qreal> reenabled = snapshot();
      QCOMPARE(static_cast<int>(reenabled.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i)
            QVERIFY(qAbs(reenabled[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const std::vector<qreal> restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i)
            QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::squeezePlan_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::newRow("legacy-beams") << false;
      QTest::newRow("studio-beams") << true;
      }

void TestSpacing::squeezePlan()
      {
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + "v0.49-spacing-squeeze-h5.mscx");
      QVERIFY(score);
      QVERIFY(score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->setStyleValue(Sid::pageWidth, 100.0);
      score->setStyleValue(Sid::pagePrintableWidth, 98.8);
      score->doLayout();
      Measure* measure = score->firstMeasure();
      QVERIFY(measure && !measure->nextMeasure());
      QCOMPARE(measure->userStretch(), qreal(2));
      const auto snapshot = [measure]() -> std::vector<qreal> {
            std::vector<qreal> values = { measure->x(), measure->width() };
            for (Segment& seg : measure->segments()) {
                  values.push_back(seg.x());
                  values.push_back(seg.width());
                  values.push_back(seg.stretch());
                  }
            return values;
            };
      const std::vector<qreal> saved = snapshot();
      SpacingPlan4 natural;
      QString reason;
      QVERIFY2(buildSpacingPlan4(measure->system(), natural, &reason), qPrintable(reason));
      SpacingPlan4 result;
      QVERIFY(squeezeSpacingPlan4(measure->system(), natural.width, result, &reason));
      QCOMPARE(result.width, natural.width);
      SpacingPlanOptions4 options;
      options.minimumMeasureWidths = false;
      options.stretchReduction = 1 - 0.33;
      options.squeezeFactor = 1 - 0.2;
      qreal previous = natural.width;
      qreal productionTarget = 0;
      for (int pass = 0; pass < 3; ++pass) {
            SpacingPlan4 expected;
            QVERIFY2(buildSpacingPlan4(measure->system(), expected, &reason, options), qPrintable(reason));
            QVERIFY(expected.width < previous - 1e-6);
            const qreal target = (previous + expected.width) / 2;
            if (pass == 0)
                  productionTarget = target;
            QVERIFY2(squeezeSpacingPlan4(measure->system(), target, result, &reason), qPrintable(reason));
            QVERIFY(reason.isEmpty());
            QVERIFY(qAbs(result.width - expected.width) < 1e-6);
            QCOMPARE(static_cast<int>(result.segments.size()), static_cast<int>(expected.segments.size()));
            for (size_t i = 0; i < result.segments.size(); ++i) {
                  QVERIFY(qAbs(result.segments[i].x - expected.segments[i].x) < 1e-6);
                  QVERIFY(qAbs(result.segments[i].width - expected.segments[i].width) < 1e-6);
                  }
            previous = expected.width;
            options.stretchReduction -= 0.33;
            options.squeezeFactor -= 0.2;
            }
      const qreal unchanged = result.width;
      QVERIFY(!squeezeSpacingPlan4(measure->system(), 0, result, &reason));
      QCOMPARE(result.width, unchanged);
      QVERIFY(!squeezeSpacingPlan4(measure->system(), measure->system()->leftMargin() + 1e-5, result, &reason));
      QCOMPARE(result.width, unchanged);
      QVERIFY(!reason.isEmpty());
      QVERIFY(snapshot() == saved); // queries, including failed ones, never publish geometry

      // Force the actual final-placement path to compress this one measure,
      // then justify its solved widths to the requested page width.
      score->setStyleValue(Sid::pagePrintableWidth, productionTarget / DPI);
      score->setStyleValue(Sid::pageWidth, productionTarget / DPI + 1.2);
      score->doLayout();
      QVERIFY2(buildSpacingPlan4(measure->system(), natural, &reason), qPrintable(reason));
      QVERIFY(natural.width > productionTarget);
      QVERIFY2(squeezeSpacingPlan4(measure->system(), productionTarget, result, &reason), qPrintable(reason));
      QVERIFY(justifySpacingPlan4(result, productionTarget));
      QVERIFY(qAbs(measure->system()->width() - productionTarget) < 1e-6);
      QVERIFY(qAbs(measure->width() - result.measures.front().width) < 1e-6);
      for (const SpacingSegmentPlan4& seg : result.segments) {
            if (seg.ignored)
                  continue;
            QVERIFY(qAbs(measure->x() + seg.segment->x() - seg.x) < 1e-6);
            QVERIFY(qAbs(seg.segment->width() - seg.width) < 1e-6);
            }
      const std::vector<qreal> modern = snapshot();
      score->doLayout();
      const std::vector<qreal> repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i)
            QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const std::vector<qreal> legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const std::vector<qreal> restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i)
            QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::courtesyPlan_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::newRow("legacy-beams") << false;
      QTest::newRow("studio-beams") << true;
      }

void TestSpacing::courtesyPlan()
      {
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + "v0.50-spacing-courtesy.mscx");
      QVERIFY(score);
      QVERIFY(score->styleB(Sid::spacingLayout4));
      int inputMeasures = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++inputMeasures)
            QVERIFY2(m->lineBreak() == (inputMeasures < 3), "Courtesy fixture must load three explicit line breaks");
      QCOMPARE(inputMeasures, 4);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      const auto snapshot = [score]() -> std::vector<qreal> {
            std::vector<qreal> values;
            int index = -1;
            const System* previous = nullptr;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  if (m->system() != previous) {
                        previous = m->system();
                        ++index;
                        }
                  values.push_back(index);
                  values.push_back(m->x());
                  values.push_back(m->width());
                  for (Segment& seg : m->segments()) {
                        if (!seg.enabled() && (seg.header() || seg.trailer()))
                              continue;
                        values.push_back(static_cast<int>(seg.segmentType()));
                        values.push_back(seg.tick().ticks());
                        values.push_back(seg.x());
                        values.push_back(seg.width());
                        }
                  }
            return values;
            };
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const std::vector<qreal> legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      const bool keys[] = { true, false, true, false };
      const bool times[] = { false, true, true, false };
      const int onsets[] = { 8, 8, 6, 4 };
      const qreal available = score->styleD(Sid::pagePrintableWidth) * DPI;
      int row = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++row) {
            QVERIFY(row < 4);
            QCOMPARE(m->system()->firstMeasure(), m);
            QCOMPARE(m->system()->lastMeasure(), m);
            int notes = 0;
            bool key = false, time = false;
            for (Segment& seg : m->segments()) {
                  if (!seg.enabled() || !seg.visible() || seg.allElementsInvisible())
                        continue;
                  notes += seg.isChordRestType();
                  if (!seg.isKeySigAnnounceType() && !seg.isTimeSigAnnounceType())
                        continue;
                  QVERIFY(seg.trailer());
                  QVERIFY(seg.rtick() == m->ticks());
                  key = key || seg.isKeySigAnnounceType();
                  time = time || seg.isTimeSigAnnounceType();
                  }
            QCOMPARE(notes, onsets[row]);
            QCOMPARE(key, keys[row]);
            QCOMPARE(time, times[row]);
            SpacingPlan4 plan;
            QString reason;
            QVERIFY2(buildSpacingPlan4(m->system(), plan, &reason), qPrintable(reason));
            const qreal naturalWidth = plan.width;
            if (row < 3) {
                  for (const SpacingSegmentPlan4& entry : plan.segments) {
                        Segment* trailer = entry.segment;
                        if (entry.ignored || (!trailer->isKeySigAnnounceType() && !trailer->isTimeSigAnnounceType()))
                              continue;
                        trailer->setTrailer(false);
                        SpacingPlan4 unchanged;
                        unchanged.width = 123;
                        const bool accepted = buildSpacingPlan4(m->system(), unchanged, &reason);
                        trailer->setTrailer(true);
                        QVERIFY(!accepted);
                        QCOMPARE(unchanged.width, qreal(123));
                        }
                  // The terminal margin is a fixed source value, not squeezed
                  // padding or rhythmic spring space. Query without DOM edits.
                  const qreal oldMargin = score->styleP(Sid::systemTrailerRightMargin) / score->spatium();
                  score->setStyleValue(Sid::systemTrailerRightMargin, Spatium(oldMargin + .75));
                  SpacingPlan4 changed;
                  const bool built = buildSpacingPlan4(m->system(), changed, &reason);
                  score->setStyleValue(Sid::systemTrailerRightMargin, Spatium(oldMargin));
                  QVERIFY2(built, qPrintable(reason));
                  QVERIFY(qAbs(changed.width - naturalWidth - .75 * score->spatium()) < 1e-6);
                  QVERIFY(justifySpacingPlan4(plan, available));
                  }
            QVERIFY(qAbs(m->system()->width() - plan.width) < 1e-6);
            QVERIFY(qAbs(m->width() - plan.measures.front().width) < 1e-6);
            for (const SpacingSegmentPlan4& seg : plan.segments) {
                  if (seg.ignored)
                        continue;
                  QVERIFY(qAbs(m->x() + seg.segment->x() - seg.x) < 1e-6);
                  QVERIFY(qAbs(seg.segment->width() - seg.width) < 1e-6);
                  }
            }
      QCOMPARE(row, 4);
      const std::vector<qreal> modern = snapshot();
      score->doLayout();
      const std::vector<qreal> repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i)
            QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const std::vector<qreal> restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i)
            QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::polyrhythmInputs_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<bool>("natural");
      QTest::addColumn<bool>("modernBeams");
      QTest::newRow("overview-legacy-beams") << QString("v0.51-spacing-polyrhythms.mscx") << false << false;
      QTest::newRow("overview-studio-beams") << QString("v0.51-spacing-polyrhythms.mscx") << false << true;
      QTest::newRow("natural-legacy-beams") << QString("v0.51-spacing-polyrhythm-natural.mscx") << true << false;
      QTest::newRow("natural-studio-beams") << QString("v0.51-spacing-polyrhythm-natural.mscx") << true << true;
      QTest::newRow("accidentals-natural-legacy-beams") << QString("v0.54-spacing-polyrhythm-accidentals-natural.mscx") << true << false;
      QTest::newRow("accidentals-natural-studio-beams") << QString("v0.54-spacing-polyrhythm-accidentals-natural.mscx") << true << true;
      QTest::newRow("accidentals-justified-legacy-beams") << QString("v0.54-spacing-polyrhythm-accidentals.mscx") << false << false;
      QTest::newRow("accidentals-justified-studio-beams") << QString("v0.54-spacing-polyrhythm-accidentals.mscx") << false << true;
      QTest::newRow("seconds-natural-legacy-beams") << QString("v0.55-spacing-polyrhythm-seconds-natural.mscx") << true << false;
      QTest::newRow("seconds-natural-studio-beams") << QString("v0.55-spacing-polyrhythm-seconds-natural.mscx") << true << true;
      QTest::newRow("seconds-justified-legacy-beams") << QString("v0.55-spacing-polyrhythm-seconds.mscx") << false << false;
      QTest::newRow("seconds-justified-studio-beams") << QString("v0.55-spacing-polyrhythm-seconds.mscx") << false << true;
      QTest::newRow("unisons-natural-legacy-beams") << QString("v0.56-spacing-polyrhythm-unisons-natural.mscx") << true << false;
      QTest::newRow("unisons-natural-studio-beams") << QString("v0.56-spacing-polyrhythm-unisons-natural.mscx") << true << true;
      QTest::newRow("unisons-justified-legacy-beams") << QString("v0.56-spacing-polyrhythm-unisons.mscx") << false << false;
      QTest::newRow("unisons-justified-studio-beams") << QString("v0.56-spacing-polyrhythm-unisons.mscx") << false << true;
      QTest::newRow("rests-natural-legacy-beams") << QString("v0.58-spacing-polyrhythm-rests-natural.mscx") << true << false;
      QTest::newRow("rests-natural-studio-beams") << QString("v0.58-spacing-polyrhythm-rests-natural.mscx") << true << true;
      QTest::newRow("rests-justified-legacy-beams") << QString("v0.58-spacing-polyrhythm-rests.mscx") << false << false;
      QTest::newRow("rests-justified-studio-beams") << QString("v0.58-spacing-polyrhythm-rests.mscx") << false << true;
      QTest::newRow("sustained-natural-legacy-beams") << QString("v0.59-spacing-polyrhythm-sustained-natural.mscx") << true << false;
      QTest::newRow("sustained-natural-studio-beams") << QString("v0.59-spacing-polyrhythm-sustained-natural.mscx") << true << true;
      QTest::newRow("sustained-justified-legacy-beams") << QString("v0.59-spacing-polyrhythm-sustained.mscx") << false << false;
      QTest::newRow("sustained-justified-studio-beams") << QString("v0.59-spacing-polyrhythm-sustained.mscx") << false << true;
      }

void TestSpacing::polyrhythmInputs()
      {
      QFETCH(QString, file);
      QFETCH(bool, natural);
      QFETCH(bool, modernBeams);
      const bool seconds = file.startsWith("v0.55-");
      const bool unisons = file.startsWith("v0.56-");
      const bool rests = file.startsWith("v0.58-");
      const bool sustained = file.startsWith("v0.59-");
      const bool accidentals = seconds || file.startsWith("v0.54-");
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score && score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      const auto snapshot = [score]() -> std::vector<qreal> {
            std::vector<qreal> values;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  values.push_back(m->x()); values.push_back(m->width());
                  for (Segment& seg : m->segments()) {
                        if (!seg.enabled() && (seg.header() || seg.trailer()))
                              continue;
                        values.push_back(static_cast<int>(seg.segmentType()));
                        values.push_back(seg.rtick().toDouble());
                        values.push_back(seg.x()); values.push_back(seg.width());
                        }
                  }
            return values;
            };
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const std::vector<qreal> legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      const int tracks[] = { 0, 1, VOICES, VOICES + 1 };
      const int counts[3][4] = { { 3, 2, 2, 3 }, { 5, 4, 4, 5 }, { 6, 8, 10, 12 } };
      int measureCount = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++measureCount) {
            const int row = natural || accidentals || unisons || rests || sustained ? 2 : measureCount;
            QVERIFY(row < 3);
            QVERIFY(m->ticks() == Fraction(1, 2));
            QCOMPARE(m->system()->firstMeasure(), m);
            QCOMPARE(m->system()->lastMeasure(), m);
            for (int v = 0; v < 4; ++v) {
                  Fraction elapsed(0, 1);
                  int n = 0;
                  int signs = 0;
                  for (Segment& seg : m->segments()) {
                        if (!seg.isChordRestType()) continue;
                        const ChordRest* cr = seg.cr(tracks[v]);
                        if (!cr) continue;
                        if (rests && n == 1) {
                              QVERIFY(cr->isRest());
                              QVERIFY(!toRest(cr)->isGap() && !toRest(cr)->isFullMeasureRest());
                              QVERIFY(cr->beam());
                              }
                        else
                              QVERIFY(cr->isChord());
                        if (sustained && (v == 1 || v == 3)) {
                              QVERIFY(!cr->tuplet() && !cr->beam());
                              const Chord* chord = toChord(cr);
                              QCOMPARE(static_cast<int>(chord->notes().size()), 1);
                              QCOMPARE(chord->notes().front()->pitch(), v == 1 ? 65 : 41);
                              QCOMPARE(chord->notes().front()->tpc(), static_cast<int>(Tpc::TPC_F));
                              }
                        if (unisons) {
                              const Chord* chord = toChord(cr);
                              QCOMPARE(static_cast<int>(chord->notes().size()), 1);
                              const Note* note = chord->notes().front();
                              QCOMPARE(note->pitch(), v < 2 ? 77 : 53);
                              QCOMPARE(note->tpc(), static_cast<int>(Tpc::TPC_F));
                              QVERIFY(!note->accidental());
                              }
                        if (accidentals) {
                              const Chord* chord = toChord(cr);
                              QCOMPARE(static_cast<int>(chord->notes().size()), seconds ? 2 : 1);
                              const int pitches[] = { 77, 65, 53, 41 };
                              const int alterations[] = { 1, 0, -1, 0 };
                              const int spellings[] = { Tpc::TPC_F_S, Tpc::TPC_F, Tpc::TPC_F_B, Tpc::TPC_F };
                              int matched = 0;
                              int mirrored = 0;
                              for (const Note* note : chord->notes()) {
                                    mirrored += note->mirror();
                                    const int offset = note->pitch() - pitches[v] - alterations[n % 4];
                                    QVERIFY(offset == 0 || (seconds && offset == 2));
                                    const int bit = offset == 0 ? 1 : 2;
                                    QVERIFY(!(matched & bit));
                                    matched |= bit;
                                    const int secondSpellings[] = { Tpc::TPC_G_S, Tpc::TPC_G, Tpc::TPC_G_B, Tpc::TPC_G };
                                    QCOMPARE(note->tpc(), offset == 0 ? spellings[n % 4] : secondSpellings[n % 4]);
                                    QCOMPARE(note->pitch() % 12, tpc2pitch(note->tpc()));
                                    QCOMPARE(tpc2step(note->tpc()), tpc2step(offset == 0 ? Tpc::TPC_F : Tpc::TPC_G));
                                    if (n == 0 || n == 2) {
                                          QVERIFY(note->accidental());
                                          QCOMPARE(static_cast<int>(note->accidental()->accidentalType()),
                                                static_cast<int>(n == 0 ? AccidentalType::SHARP : AccidentalType::FLAT));
                                          }
                                    signs += note->accidental() != nullptr;
                                    }
                              QCOMPARE(matched, seconds ? 3 : 1);
                              if (seconds) QVERIFY(mirrored > 0);
                              }
                        QVERIFY(seg.rtick() == elapsed);
                        Fraction duration;
                        if (sustained && (v == 1 || v == 3)) duration = Fraction(1, 2);
                        else if (row == 0) duration = (v == 0 || v == 3) ? Fraction(1, 6) : Fraction(1, 4);
                        else if (row == 1) duration = (v == 0 || v == 3) ? Fraction(1, 10) : Fraction(1, 8);
                        else if (v == 0) duration = n < 4 ? Fraction(1, 16) : Fraction(1, 8);
                        else if (v == 1) duration = n < 5 ? Fraction(1, 20) : Fraction(1, 12);
                        else if (v == 2) duration = n < 6 ? Fraction(1, 24) : Fraction(1, 16);
                        else duration = n < 7 ? Fraction(1, 28) : Fraction(1, 20);
                        QVERIFY(cr->actualTicks() == duration);
                        elapsed += duration;
                        ++n;
                        }
                  QCOMPARE(n, sustained && (v == 1 || v == 3) ? 1 : counts[row][v]);
                  if (accidentals) QVERIFY(signs > 0);
                  QVERIFY(elapsed == Fraction(1, 2));
                  }
            SpacingPlan4 applied;
            QString reason;
            if (unisons) {
                  Segment* first = m->first(SegmentType::ChordRest);
                  QVERIFY(first);
                  for (int staff = 0; staff < 2; ++staff) {
                        const SpacingShape4 shape = spacingStaffShape4(first, staff);
                        bool shared = false;
                        for (const SpacingShapeElement4& item : shape.elements) {
                              if (item.identity == SpacingIdentity4::SHARED_ITEM) {
                                    QVERIFY(!item.item);
                                    QCOMPARE(static_cast<int>(item.sharedItems.size()), 2);
                                    shared = true;
                                    }
                              }
                        QVERIFY(shared);
                        }
                  }
            QVERIFY2(buildSpacingPlan4(m->system(), applied, &reason), qPrintable(reason));
            if (!natural)
                  QVERIFY(justifySpacingPlan4(applied, score->styleD(Sid::pagePrintableWidth) * DPI));
            QVERIFY(qAbs(m->system()->width() - applied.width) < 1e-6);
            for (const SpacingSegmentPlan4& entry : applied.segments) {
                  if (entry.ignored) continue;
                  QVERIFY(qAbs(m->x() + entry.segment->x() - entry.x) < 1e-6);
                  QVERIFY(qAbs(entry.segment->width() - entry.width) < 1e-6);
                  }
            // Verify the multi-voice rhythmic branch against the pinned
            // duration formula, including the preceding non-adjacent onset.
            const auto shortest = [](const Segment* seg) {
                  Fraction value = seg->measure()->ticks();
                  for (const Element* e : seg->elist()) {
                        if (e && e->isChordRest() && toChordRest(e)->actualTicks() < value)
                              value = toChordRest(e)->actualTicks();
                        }
                  return value;
                  };
            for (Segment& seg : m->segments()) {
                  if (!seg.isChordRestType()) continue;
                  const Segment* prev = seg.prev(SegmentType::ChordRest);
                  const Fraction currentShortest = shortest(&seg);
                  Fraction reference = seg.ticks();
                  double proportion = 1;
                  if (currentShortest != seg.ticks()) {
                        const Fraction previousShortest = prev ? shortest(prev) : Fraction(0, 1);
                        reference = prev && previousShortest != prev->ticks() && previousShortest < currentShortest
                              ? previousShortest : currentShortest;
                        proportion = (seg.ticks() / reference).toDouble();
                        }
                  const qreal expected = std::pow(score->styleD(Sid::measureSpacing),
                        std::log2(reference.toDouble() / .25)) * proportion;
                  QVERIFY(qAbs(segmentSpacingStretch4(&seg) - expected) < 1e-9);
                  QVERIFY(qIsFinite(seg.x()) && qIsFinite(seg.width()) && seg.width() >= 0);
                  }
            }
      QCOMPARE(measureCount, natural || accidentals || unisons || rests || sustained ? 1 : 3);
      const std::vector<qreal> modern = snapshot();
      score->doLayout();
      const std::vector<qreal> repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const std::vector<qreal> restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::crossStaffPlanBoundary_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<bool>("modernBeams");
      for (bool beams : { false, true }) {
            const QString suffix = beams ? "-studio-beams" : "-legacy-beams";
            QTest::newRow(qPrintable("natural" + suffix)) << QString("v0.60-spacing-cross-staff-natural.mscx") << beams;
            QTest::newRow(qPrintable("justified" + suffix)) << QString("v0.60-spacing-cross-staff.mscx") << beams;
            QTest::newRow(qPrintable("runs-natural" + suffix)) << QString("v0.65-spacing-cross-staff-runs-natural.mscx") << beams;
            QTest::newRow(qPrintable("runs-justified" + suffix)) << QString("v0.65-spacing-cross-staff-runs.mscx") << beams;
            QTest::newRow(qPrintable("reverse-runs-natural" + suffix)) << QString("v0.66-spacing-cross-staff-reverse-runs-natural.mscx") << beams;
            QTest::newRow(qPrintable("reverse-runs-justified" + suffix)) << QString("v0.66-spacing-cross-staff-reverse-runs.mscx") << beams;
            }
      }

void TestSpacing::crossStaffPlanBoundary()
      {
      QFETCH(QString, file);
      QFETCH(bool, modernBeams);
      const bool reverse = file.startsWith("v0.66-");
      const bool runs = reverse || file.startsWith("v0.65-");
      const auto expectedStaff = [runs, reverse](int i) {
            const int staff = runs ? i / 2 : i % 2;
            return reverse ? 1 - staff : staff;
            };
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score && score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      const auto snapshot = [score]() {
            std::vector<qreal> result;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  result.push_back(m->x()); result.push_back(m->width());
                  for (Segment& s : m->segments()) {
                        if (!s.enabled() && (s.header() || s.trailer())) continue;
                        result.push_back(static_cast<int>(s.segmentType()));
                        result.push_back(s.rtick().toDouble());
                        result.push_back(s.x()); result.push_back(s.width());
                        result.push_back(s.spacingWidthOffset4());
                        }
                  }
            return result;
            };
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      Measure* m = score->firstMeasure();
      QVERIFY(m && !m->nextMeasure() && m->ticks() == Fraction(1, 2));
      int onset = 0;
      for (Segment& s : m->segments()) {
            QVERIFY(qIsFinite(s.x()) && qIsFinite(s.width()) && s.width() >= 0);
            if (!s.isChordRestType()) continue;
            QVERIFY(s.rtick() == Fraction(onset, 8));
            const ChordRest* cr = s.cr(0);
            QVERIFY(cr && cr->isChord() && cr->actualTicks() == Fraction(1, 8));
            const Chord* chord = toChord(cr);
            QCOMPARE(chord->staffMove(), expectedStaff(onset));
            QCOMPARE(static_cast<int>(chord->notes().size()), 1);
            QCOMPARE(chord->notes().front()->pitch(), expectedStaff(onset) ? 53 : 65);
            QVERIFY(chord->beam() && chord->beam()->cross());
            if (onset == 0) {
                  for (int track : { 1, VOICES }) {
                        const ChordRest* held = s.cr(track);
                        QVERIFY(held && held->isChord() && held->actualTicks() == Fraction(1, 2));
                        }
                  }
            ++onset;
            }
      QCOMPARE(onset, 4);
      const auto modern = snapshot();
      const Chord* firstChord = toChord(m->first(SegmentType::ChordRest)->cr(0));
      std::vector<bool> stemSides;
      for (const ChordRest* cr : firstChord->beam()->elements()) stemSides.push_back(cr->up());
      CrossStaffContext4 context;
      QString contextReason;
      QVERIFY2(buildCrossStaffContext4(firstChord->beam(), context, &contextReason), qPrintable(contextReason));
      QCOMPARE(static_cast<int>(context.chords.size()), 4);
      QCOMPARE(static_cast<int>(context.pairs.size()), 3);
      const qreal displacement = score->noteHeadWidth() - score->styleP(Sid::stemWidth);
      for (int i = 0; i < 4; ++i) {
            QCOMPARE(context.chords[i].visualStaff, expectedStaff(i));
            QCOMPARE(context.chords[i].up, expectedStaff(i) != 0);
            QCOMPARE(context.chords[i].chord->up(), static_cast<bool>(stemSides[i]));
            const Chord* chord = context.chords[i].chord;
            QCOMPARE(chord->up(), context.chords[i].up);
            QVERIFY(qAbs(context.chords[i].stemX - chord->x() - chord->stem()->x()) < 1e-6);
            }
      for (int i = 0; i < 3; ++i) {
            QVERIFY(context.pairs[i].adjacentSegments && context.pairs[i].canAdjust);
            const bool up = expectedStaff(i) != 0;
            const bool nextUp = expectedStaff(i + 1) != 0;
            const qreal offset = up == nextUp ? 0 : (up ? displacement : -displacement);
            QVERIFY(qAbs(context.pairs[i].widthOffset - offset) < 1e-6);
            if (up && !nextUp) {
                  QVERIFY(context.pairs[i].minimumWidth >= 2 * displacement);
                  qreal stemPadding = 0;
                  const SpacingPaddingTable4 table(score);
                  QVERIFY(table.padding(ElementType::STEM, ElementType::STEM, stemPadding));
                  const qreal expected = qMax(qreal(0), context.chords[i].stemX - context.chords[i + 1].stemX + stemPadding);
                  QVERIFY(qAbs(context.pairs[i].minimumStemDistance - expected) < 1e-6);
                  QVERIFY(context.pairs[i].minimumWidth >= expected);
                  }
            else {
                  QCOMPARE(context.pairs[i].minimumWidth, qreal(0));
                  QCOMPARE(context.pairs[i].minimumStemDistance, qreal(0));
                  }
            }
      QVERIFY(snapshot() == modern);
      Chord* edited = toChord(m->first(SegmentType::ChordRest)->cr(0));
      const Direction savedDirection = edited->stemDirection();
      edited->setStemDirection(Direction::UP);
      CrossStaffContext4 rejected;
      rejected.beam = firstChord->beam(); // sentinel survives failure
      QVERIFY(!buildCrossStaffContext4(firstChord->beam(), rejected, &contextReason));
      QCOMPARE(rejected.beam, static_cast<const Beam*>(firstChord->beam()));
      QVERIFY(rejected.chords.empty() && rejected.pairs.empty());
      SpacingPlan4 rejectedPlan;
      rejectedPlan.width = -123;
      QVERIFY(!buildSpacingPlan4(m->system(), rejectedPlan, &contextReason));
      QCOMPARE(rejectedPlan.width, qreal(-123));
      QVERIFY(rejectedPlan.segments.empty() && rejectedPlan.measures.empty());
      edited->setStemDirection(savedDirection);
      QVERIFY(snapshot() == modern);
      SpacingPlan4 plan;
      QString reason;
      QVERIFY2(buildSpacingPlan4(m->system(), plan, &reason), qPrintable(reason));
      if (!file.contains("-natural"))
            QVERIFY(justifySpacingPlan4(plan, score->styleD(Sid::pagePrintableWidth) * DPI));
      QVERIFY(qAbs(plan.width - m->system()->width()) < 1e-6);
      int fixedOffsets = 0;
      for (const SpacingSegmentPlan4& e : plan.segments) {
            if (e.ignored) continue;
            QVERIFY(qAbs(m->x() + e.segment->x() - e.x) < 1e-6);
            QVERIFY(qAbs(e.segment->width() - e.width) < 1e-6);
            QVERIFY(qAbs(e.segment->spacingWidthOffset4() - e.widthOffset) < 1e-6);
            fixedOffsets += e.widthOffset != 0;
            }
      QCOMPARE(fixedOffsets, runs ? 1 : 3);
      QVERIFY(snapshot() == modern);
      score->doLayout();
      const auto repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::crossStaffInterleaved_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<bool>("modernBeams");
      for (bool beams : { false, true }) {
            const QString suffix = beams ? "-studio-beams" : "-legacy-beams";
            QTest::newRow(qPrintable("natural" + suffix)) << QString("v0.67-spacing-cross-staff-interleaved-natural.mscx") << beams;
            QTest::newRow(qPrintable("justified" + suffix)) << QString("v0.67-spacing-cross-staff-interleaved.mscx") << beams;
            QTest::newRow(qPrintable("cross-accidentals-natural" + suffix)) << QString("v0.69-spacing-cross-accidentals-natural.mscx") << beams;
            QTest::newRow(qPrintable("cross-accidentals-justified" + suffix)) << QString("v0.69-spacing-cross-accidentals.mscx") << beams;
            QTest::newRow(qPrintable("companion-accidentals-natural" + suffix)) << QString("v0.69-spacing-companion-accidentals-natural.mscx") << beams;
            QTest::newRow(qPrintable("companion-accidentals-justified" + suffix)) << QString("v0.69-spacing-companion-accidentals.mscx") << beams;
            QTest::newRow(qPrintable("chords-natural" + suffix)) << QString("v0.70-spacing-cross-chords-natural.mscx") << beams;
            QTest::newRow(qPrintable("chords-justified" + suffix)) << QString("v0.70-spacing-cross-chords.mscx") << beams;
            QTest::newRow(qPrintable("accidental-chords-natural" + suffix)) << QString("v0.70-spacing-cross-accidentals-chords-natural.mscx") << beams;
            QTest::newRow(qPrintable("accidental-chords-justified" + suffix)) << QString("v0.70-spacing-cross-accidentals-chords.mscx") << beams;
            QTest::newRow(qPrintable("two-voices-natural" + suffix)) << QString("v0.71-spacing-cross-chords-two-voices-natural.mscx") << beams;
            QTest::newRow(qPrintable("two-voices-justified" + suffix)) << QString("v0.71-spacing-cross-chords-two-voices.mscx") << beams;
            QTest::newRow(qPrintable("two-voices-accidentals-natural" + suffix)) << QString("v0.71-spacing-cross-chords-two-voices-accidentals-natural.mscx") << beams;
            QTest::newRow(qPrintable("two-voices-accidentals-justified" + suffix)) << QString("v0.71-spacing-cross-chords-two-voices-accidentals.mscx") << beams;
            }
      }

void TestSpacing::crossStaffInterleaved()
      {
      QFETCH(QString, file);
      QFETCH(bool, modernBeams);
      const bool crossAccidentals = file.contains("spacing-cross-accidentals");
      const bool twoVoices = file.contains("two-voices");
      const bool companionAccidentals = file.contains("spacing-companion-accidentals")
            || file.contains("two-voices-accidentals");
      const bool crossChords = file.contains("-chords");
      const int alterations[] = { 1, 0, -1, 0 };
      const int fSpellings[] = { Tpc::TPC_F_S, Tpc::TPC_F, Tpc::TPC_F_B, Tpc::TPC_F };
      const int cSpellings[] = { Tpc::TPC_C_S, Tpc::TPC_C, Tpc::TPC_C_B, Tpc::TPC_C };
      const int aSpellings[] = { Tpc::TPC_A_S, Tpc::TPC_A, Tpc::TPC_A_B, Tpc::TPC_A };
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score && score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      const auto snapshot = [score]() {
            std::vector<qreal> result;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  result.push_back(m->x()); result.push_back(m->width());
                  for (Segment& s : m->segments()) {
                        if (!s.enabled() && (s.header() || s.trailer())) continue;
                        result.push_back(s.x()); result.push_back(s.width());
                        result.push_back(s.spacingWidthOffset4());
                        }
                  }
            return result;
            };
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      Measure* m = score->firstMeasure();
      QVERIFY(m && !m->nextMeasure() && m->ticks() == Fraction(1, 2));
      int onset = 0;
      int accidentals = 0;
      const Chord* first = nullptr;
      for (Segment& s : m->segments()) {
            QVERIFY(qIsFinite(s.x()) && qIsFinite(s.width()) && s.width() >= 0);
            if (!s.isChordRestType()) continue;
            QVERIFY(s.rtick() == Fraction(onset, 16));
            QVERIFY(s.ticks() == Fraction(1, 16));
            const ChordRest* companion = s.cr(1);
            QVERIFY(companion && companion->isChord() && !companion->beam());
            QVERIFY(companion->actualTicks() == Fraction(1, 16));
            QCOMPARE(static_cast<int>(toChord(companion)->notes().size()), 1);
            const Note* companionNote = toChord(companion)->notes().front();
            QCOMPARE(companionNote->pitch(), 60 + (companionAccidentals ? alterations[onset % 4] : 0));
            QCOMPARE(companionNote->tpc(), companionAccidentals ? cSpellings[onset % 4] : static_cast<int>(Tpc::TPC_C));
            QCOMPARE(companionNote->pitch() % 12, (tpc2pitch(companionNote->tpc()) + 12) % 12);
            QCOMPARE(tpc2step(companionNote->tpc()), tpc2step(Tpc::TPC_C));
            accidentals += companionNote->accidental() != nullptr;
            if (twoVoices) {
                  const ChordRest* lower = s.cr(VOICES);
                  QVERIFY(lower && lower->isChord() && !lower->beam());
                  QVERIFY(lower->actualTicks() == Fraction(1, 16));
                  QCOMPARE(static_cast<int>(toChord(lower)->notes().size()), 1);
                  const Note* note = toChord(lower)->notes().front();
                  QCOMPARE(note->pitch(), 48 + (companionAccidentals ? alterations[onset % 4] : 0));
                  QCOMPARE(note->tpc(), companionAccidentals ? cSpellings[onset % 4] : static_cast<int>(Tpc::TPC_C));
                  QCOMPARE(note->pitch() % 12, (tpc2pitch(note->tpc()) + 12) % 12);
                  QCOMPARE(tpc2step(note->tpc()), tpc2step(Tpc::TPC_C));
                  accidentals += note->accidental() != nullptr;
                  }
            if (onset % 2 == 0) {
                  const ChordRest* cr = s.cr(0);
                  QVERIFY(cr && cr->isChord() && cr->actualTicks() == Fraction(1, 8));
                  const Chord* c = toChord(cr);
                  QCOMPARE(c->staffMove(), onset < 4 ? 1 : 0);
                  QCOMPARE(static_cast<int>(c->notes().size()), crossChords ? 2 : 1);
                  int noteIndex = 0;
                  for (const Note* beamNote : c->notes()) {
                        const int base = (onset < 4 ? 53 : 65) + 4 * noteIndex;
                        QCOMPARE(beamNote->pitch(), base + (crossAccidentals ? alterations[onset / 2] : 0));
                        const int spelling = crossAccidentals ? (noteIndex ? aSpellings[onset / 2] : fSpellings[onset / 2])
                              : static_cast<int>(noteIndex ? Tpc::TPC_A : Tpc::TPC_F);
                        QCOMPARE(beamNote->tpc(), spelling);
                        QCOMPARE(beamNote->pitch() % 12, (tpc2pitch(beamNote->tpc()) + 12) % 12);
                        QCOMPARE(tpc2step(beamNote->tpc()), tpc2step(noteIndex ? Tpc::TPC_A : Tpc::TPC_F));
                        QVERIFY(!beamNote->mirror());
                        accidentals += beamNote->accidental() != nullptr;
                        ++noteIndex;
                        }
                  QVERIFY(c->beam() && c->beam()->cross());
                  if (!first) first = c;
                  }
            else QVERIFY(!s.cr(0));
            ++onset;
            }
      QCOMPARE(onset, 8);
      if (crossAccidentals || companionAccidentals) QVERIFY(accidentals >= 2);
      QVERIFY(first);
      const auto modern = snapshot();
      CrossStaffContext4 context;
      QString reason;
      QVERIFY2(buildCrossStaffContext4(first->beam(), context, &reason), qPrintable(reason));
      QCOMPARE(static_cast<int>(context.chords.size()), 4);
      QCOMPARE(static_cast<int>(context.pairs.size()), 3);
      for (int i = 0; i < 4; ++i) {
            const Chord* chord = context.chords[i].chord;
            QCOMPARE(context.chords[i].visualStaff, i < 2 ? 1 : 0);
            QCOMPARE(context.chords[i].up, i < 2);
            QCOMPARE(chord->up(), context.chords[i].up);
            QVERIFY(qAbs(context.chords[i].stemX - chord->x() - chord->stem()->x()) < 1e-6);
            }
      for (int i = 0; i < 3; ++i) {
            const CrossStaffPair4& pair = context.pairs[i];
            QVERIFY(!pair.adjacentSegments && !pair.canAdjust);
            QCOMPARE(pair.widthOffset, qreal(0));
            QCOMPARE(pair.minimumWidth, qreal(0));
            if (i == 1) {
                  qreal padding = 0;
                  const SpacingPaddingTable4 table(score);
                  QVERIFY(table.padding(ElementType::STEM, ElementType::STEM, padding));
                  const qreal expected = qMax(qreal(0), context.chords[i].stemX - context.chords[i + 1].stemX + padding);
                  QVERIFY(expected > 0 && qAbs(pair.minimumStemDistance - expected) < 1e-6);
                  const qreal actual = pair.second->segment()->x() + context.chords[i + 1].stemX
                        - pair.first->segment()->x() - context.chords[i].stemX;
                  QVERIFY(actual + 1e-6 >= padding);
                  }
            else QCOMPARE(pair.minimumStemDistance, qreal(0));
            }
      SpacingPlan4 plan;
      QVERIFY2(buildSpacingPlan4(m->system(), plan, &reason), qPrintable(reason));
      if (!file.contains("-natural"))
            QVERIFY(justifySpacingPlan4(plan, score->styleD(Sid::pagePrintableWidth) * DPI));
      QVERIFY(qAbs(plan.width - m->system()->width()) < 1e-6);
      for (const SpacingSegmentPlan4& e : plan.segments) {
            if (e.ignored) continue;
            QCOMPARE(e.widthOffset, qreal(0));
            QVERIFY(qAbs(m->x() + e.segment->x() - e.x) < 1e-6);
            QVERIFY(qAbs(e.segment->width() - e.width) < 1e-6);
            QVERIFY(qAbs(e.segment->spacingWidthOffset4()) < 1e-6);
            }
      Chord* edited = const_cast<Chord*>(first);
      const Direction saved = edited->stemDirection();
      edited->setStemDirection(Direction::UP);
      SpacingPlan4 rejected;
      rejected.width = -123;
      QVERIFY(!buildSpacingPlan4(m->system(), rejected, &reason));
      QCOMPARE(rejected.width, qreal(-123));
      QVERIFY(rejected.segments.empty() && rejected.measures.empty());
      edited->setStemDirection(saved);
      Chord* companion = toChord(m->first(SegmentType::ChordRest)->cr(1));
      const Fraction savedTicks = companion->ticks();
      companion->setTicks(Fraction(1, 8));
      QVERIFY(!buildSpacingPlan4(m->system(), rejected, &reason));
      QCOMPARE(rejected.width, qreal(-123));
      QVERIFY(rejected.segments.empty() && rejected.measures.empty());
      companion->setTicks(savedTicks);
      if (twoVoices) {
            Chord* lower = toChord(m->first(SegmentType::ChordRest)->cr(VOICES));
            const Fraction savedLowerTicks = lower->ticks();
            lower->setTicks(Fraction(1, 8));
            QVERIFY(!buildSpacingPlan4(m->system(), rejected, &reason));
            QCOMPARE(rejected.width, qreal(-123));
            QVERIFY(rejected.segments.empty() && rejected.measures.empty());
            lower->setTicks(savedLowerTicks);
            }
      if (crossChords) {
            Note* second = edited->notes().back();
            const QPointF savedOffset = second->offset();
            second->setOffset(QPointF(.5, 0));
            QVERIFY(!buildSpacingPlan4(m->system(), rejected, &reason));
            QCOMPARE(rejected.width, qreal(-123));
            QVERIFY(rejected.segments.empty() && rejected.measures.empty());
            second->setOffset(savedOffset);
            }
      QVERIFY(snapshot() == modern);
      score->doLayout();
      const auto repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::crossStaffSystems_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<bool>("modernBeams");
      for (bool beams : { false, true }) {
            const QString suffix = beams ? "-studio-beams" : "-legacy-beams";
            QTest::newRow(qPrintable("natural" + suffix)) << QString("v0.72-spacing-cross-staff-system-natural.mscx") << beams;
            QTest::newRow(qPrintable("justified" + suffix)) << QString("v0.72-spacing-cross-staff-system.mscx") << beams;
            }
      }

void TestSpacing::crossStaffSystems()
      {
      QFETCH(QString, file);
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score && score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      const auto snapshot = [score]() {
            std::vector<qreal> result;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  result.push_back(m->x()); result.push_back(m->width());
                  result.push_back(m->system()->width());
                  for (Segment& s : m->segments()) {
                        if (!s.enabled() && (s.header() || s.trailer())) continue;
                        result.push_back(static_cast<int>(s.segmentType()));
                        result.push_back(s.rtick().toDouble());
                        result.push_back(s.x()); result.push_back(s.width());
                        result.push_back(s.spacingWidthOffset4());
                        }
                  }
            return result;
            };
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      Measure* first = score->firstMeasure();
      QVERIFY(first && first->nextMeasure() && !first->nextMeasure()->nextMeasure());
      Measure* second = first->nextMeasure();
      QCOMPARE(first->system(), second->system());
      QVERIFY(first->ticks() == Fraction(1, 2) && second->ticks() == Fraction(1, 2));
      QCOMPARE(static_cast<int>(first->system()->measures().size()), 2);
      int measureIndex = 0;
      const Beam* previousBeam = nullptr;
      for (Measure* m : { first, second }) {
            int onset = 0;
            for (Segment& segment : m->segments()) {
                  if (!segment.isChordRestType()) continue;
                  QVERIFY(segment.rtick() == Fraction(onset, 16));
                  QVERIFY(segment.ticks() == Fraction(1, 16));
                  for (int track : { 1, VOICES }) {
                        const ChordRest* cr = segment.cr(track);
                        QVERIFY(cr && cr->isChord() && !cr->beam() && cr->actualTicks() == Fraction(1, 16));
                        const Note* note = toChord(cr)->notes().front();
                        const int delta[] = { 1, 0, -1, 0 };
                        QCOMPARE(note->pitch(), (track == 1 ? 60 : 48) + (measureIndex ? delta[onset % 4] : 0));
                        QCOMPARE(tpc2step(note->tpc()), tpc2step(Tpc::TPC_C));
                        }
                  if (onset % 2 == 0) {
                        const ChordRest* cr = segment.cr(0);
                        QVERIFY(cr && cr->isChord() && cr->actualTicks() == Fraction(1, 8));
                        const Chord* c = toChord(cr);
                        QCOMPARE(c->staffMove(), onset < 4 ? 1 : 0);
                        QCOMPARE(static_cast<int>(c->notes().size()), 2);
                        QCOMPARE(c->notes().front()->pitch(), onset < 4 ? 53 : 65);
                        QCOMPARE(c->notes().back()->pitch(), onset < 4 ? 57 : 69);
                        QVERIFY(c->beam() && c->beam()->cross());
                        if (onset == 0) {
                              QVERIFY(c->beam() != previousBeam);
                              previousBeam = c->beam();
                              }
                        }
                  else QVERIFY(!segment.cr(0));
                  ++onset;
                  }
            QCOMPARE(onset, 8);
            ++measureIndex;
            }
      const auto modern = snapshot();
      SpacingPlan4 natural;
      QString reason;
      QVERIFY2(buildSpacingPlan4(first->system(), natural, &reason), qPrintable(reason));
      QCOMPARE(static_cast<int>(natural.measures.size()), 2);
      SpacingPlan4 placed = natural;
      if (!file.contains("-natural"))
            QVERIFY(justifySpacingPlan4(placed, score->styleD(Sid::pagePrintableWidth) * DPI));
      QVERIFY(qAbs(placed.width - first->system()->width()) < 1e-6);
      for (const SpacingMeasurePlan4& m : placed.measures) {
            QVERIFY(qAbs(m.measure->x() - m.x) < 1e-6);
            QVERIFY(qAbs(m.measure->width() - m.width) < 1e-6);
            }
      for (const SpacingSegmentPlan4& e : placed.segments) {
            if (e.ignored) continue;
            QVERIFY(qIsFinite(e.x) && qIsFinite(e.width) && e.width >= 0);
            QCOMPARE(e.widthOffset, qreal(0));
            QVERIFY(qAbs(e.segment->measure()->x() + e.segment->x() - e.x) < 1e-6);
            QVERIFY(qAbs(e.segment->width() - e.width) < 1e-6);
            QVERIFY(qAbs(e.segment->spacingWidthOffset4()) < 1e-6);
            }
      // Exercise a real compression pass across both measure contexts without
      // changing the live page size or borrowing the justified native widths.
      SpacingPlanOptions4 options;
      options.minimumMeasureWidths = false;
      options.stretchReduction = 1 - 0.33;
      options.squeezeFactor = 1 - 0.2;
      SpacingPlan4 reduced;
      QVERIFY2(buildSpacingPlan4(first->system(), reduced, &reason, options), qPrintable(reason));
      QVERIFY(reduced.width < natural.width);
      const qreal target = (reduced.width + natural.width) / 2;
      SpacingPlan4 compressed;
      QVERIFY2(squeezeSpacingPlan4(first->system(), target, compressed, &reason), qPrintable(reason));
      QCOMPARE(static_cast<int>(compressed.measures.size()), 2);
      QVERIFY(compressed.width <= target + 1e-6);
      QVERIFY(justifySpacingPlan4(compressed, target));
      QVERIFY(qAbs(compressed.width - target) < 1e-6);
      for (Measure* m : { first, second }) {
            const Chord* c = toChord(m->first(SegmentType::ChordRest)->cr(0));
            CrossStaffContext4 context;
            QVERIFY(buildCrossStaffContext4(c->beam(), context));
            const CrossStaffPair4& pair = context.pairs[1];
            qreal a = 0, b = 0;
            bool foundA = false, foundB = false;
            for (const SpacingSegmentPlan4& e : compressed.segments) {
                  QCOMPARE(e.widthOffset, qreal(0));
                  if (e.segment == pair.first->segment()) { a = e.x; foundA = true; }
                  if (e.segment == pair.second->segment()) { b = e.x; foundB = true; }
                  }
            QVERIFY(foundA && foundB && b - a + 1e-6 >= pair.minimumStemDistance);
            }
      Chord* edited = toChord(second->first(SegmentType::ChordRest)->cr(0));
      const Direction saved = edited->stemDirection();
      edited->setStemDirection(Direction::UP);
      SpacingPlan4 rejected;
      rejected.width = -123;
      QVERIFY(!buildSpacingPlan4(first->system(), rejected, &reason));
      QCOMPARE(rejected.width, qreal(-123));
      QVERIFY(rejected.segments.empty() && rejected.measures.empty());
      edited->setStemDirection(saved);
      QVERIFY(snapshot() == modern);
      score->doLayout();
      const auto repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::crossStaffFitting_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<QString>("file");
      for (bool beams : { false, true }) {
            const QString suffix = beams ? "-studio-beams" : "-legacy-beams";
            QTest::newRow(qPrintable("original" + suffix)) << beams << QString("v0.73-spacing-cross-staff-fitting.mscx");
            QTest::newRow(qPrintable("explicit-headers" + suffix)) << beams << QString("v0.76-spacing-header-control.mscx");
            }
      }

void TestSpacing::crossStaffFitting()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score && score->styleB(Sid::spacingLayout4));
      QCOMPARE(score->styleD(Sid::lastSystemFillLimit), qreal(0));
      if (file.startsWith("v0.76-")) {
            QCOMPARE(score->styleS(Sid::clefLeftMargin).val(), qreal(.75));
            QCOMPARE(score->styleS(Sid::clefKeyDistance).val(), qreal(.75));
            QCOMPARE(score->styleS(Sid::systemHeaderDistance).val(), qreal(2.5));
            QCOMPARE(score->styleS(Sid::systemHeaderTimeSigDistance).val(), qreal(2));
            QVERIFY(score->styleB(Sid::genClef) && score->styleB(Sid::genKeysig));
            }
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->setStyleValue(Sid::pageWidth, 100.0);
      score->setStyleValue(Sid::pagePrintableWidth, 98.8);
      score->doLayout();
      System* wideSystem = score->firstMeasure()->system();
      int measureCount = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            QCOMPARE(m->system(), wideSystem);
            QVERIFY(!m->lineBreak() && !m->pageBreak() && !m->noBreak());
            QVERIFY(m->ticks() == Fraction(1, 2));
            int onset = 0;
            for (Segment& segment : m->segments()) {
                  if (!segment.isChordRestType()) continue;
                  QVERIFY(segment.rtick() == Fraction(onset, 16));
                  for (int track : { 1, VOICES }) {
                        const ChordRest* cr = segment.cr(track);
                        QVERIFY(cr && cr->isChord() && !cr->beam() && cr->actualTicks() == Fraction(1, 16));
                        const int delta[] = { 1, 0, -1, 0 };
                        const Note* note = toChord(cr)->notes().front();
                        QCOMPARE(note->pitch(), (track == 1 ? 60 : 48) + (measureCount % 2 ? delta[onset % 4] : 0));
                        }
                  if (onset % 2 == 0) {
                        const ChordRest* cr = segment.cr(0);
                        QVERIFY(cr && cr->isChord() && cr->actualTicks() == Fraction(1, 8));
                        const Chord* c = toChord(cr);
                        QCOMPARE(c->staffMove(), onset < 4 ? 1 : 0);
                        QCOMPARE(static_cast<int>(c->notes().size()), 2);
                        QVERIFY(c->beam() && c->beam()->cross());
                        }
                  ++onset;
                  }
            QCOMPARE(onset, 8);
            ++measureCount;
            }
      QCOMPARE(measureCount, 6);
      SpacingPlan4 natural;
      QString reason;
      QVERIFY2(buildSpacingPlan4(wideSystem, natural, &reason), qPrintable(reason));
      QCOMPARE(static_cast<int>(natural.measures.size()), 6);
      const qreal naturalWidth = natural.width;
      const auto snapshot = [score]() {
            std::vector<qreal> result;
            const System* previous = nullptr;
            int systemIndex = -1;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  if (m->system() != previous) { previous = m->system(); ++systemIndex; }
                  result.push_back(systemIndex);
                  result.push_back(m->x()); result.push_back(m->width());
                  for (Segment& segment : m->segments()) {
                        if (!segment.enabled() && (segment.header() || segment.trailer())) continue;
                        result.push_back(static_cast<int>(segment.segmentType()));
                        result.push_back(segment.tick().toDouble());
                        result.push_back(segment.enabled() ? 1 : 0);
                        result.push_back(segment.x()); result.push_back(segment.width());
                        result.push_back(segment.spacingWidthOffset4());
                        }
                  }
            return result;
            };
      int previousCount = 0;
      for (qreal ratio : { qreal(1.1), qreal(.55), qreal(.35) }) {
            const qreal available = naturalWidth * ratio;
            score->setStyleValue(Sid::pagePrintableWidth, available / DPI);
            score->setStyleValue(Sid::pageWidth, available / DPI + 1.2);
            score->setStyleValue(Sid::spacingLayout4, false);
            score->doLayout();
            const auto legacy = snapshot();
            score->setStyleValue(Sid::spacingLayout4, true);
            score->doLayout();
            int systems = 0;
            int total = 0;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  ++total;
                  if (m != m->system()->firstMeasure()) continue;
                  ++systems;
                  SpacingPlan4 plan;
                  QVERIFY2(buildSpacingPlan4(m->system(), plan, &reason), qPrintable(reason));
                  QVERIFY(plan.width <= available + 1e-6);
                  QVERIFY(justifySpacingPlan4(plan, available));
                  QVERIFY(qAbs(plan.width - m->system()->width()) < 1e-6);
                  for (const SpacingMeasurePlan4& e : plan.measures) {
                        QVERIFY(qAbs(e.measure->x() - e.x) < 1e-6);
                        QVERIFY(qAbs(e.measure->width() - e.width) < 1e-6);
                        }
                  for (const SpacingSegmentPlan4& e : plan.segments) {
                        if (e.ignored) continue;
                        QVERIFY(qIsFinite(e.x) && qIsFinite(e.width) && e.width >= 0);
                        QCOMPARE(e.widthOffset, qreal(0));
                        QVERIFY(qAbs(e.segment->measure()->x() + e.segment->x() - e.x) < 1e-6);
                        QVERIFY(qAbs(e.segment->width() - e.width) < 1e-6);
                        QVERIFY(qAbs(e.segment->spacingWidthOffset4()) < 1e-6);
                        }
                  }
            QCOMPARE(total, 6);
            if (ratio > 1) QCOMPARE(systems, 1);
            else QVERIFY(systems > 1);
            QVERIFY(systems >= previousCount);
            previousCount = systems;
            const auto modern = snapshot();
            score->doLayout();
            const auto repeated = snapshot();
            QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
            for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
            score->setStyleValue(Sid::spacingLayout4, false);
            score->doLayout();
            const auto restored = snapshot();
            QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
            for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
            }
      delete score;
      }

void TestSpacing::crossBeamSwitch_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::newRow("legacy-beams") << false;
      QTest::newRow("studio-beams") << true;
      }

void TestSpacing::crossBeamSwitch()
      {
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + "studio-spacing-cross-staff.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      std::vector<qreal> legacy;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
            for (Segment& seg : m->segments()) {
                  legacy.push_back(seg.x());
                  legacy.push_back(seg.width());
                  }
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      // H9/H10 start with down stems; H11/H12 start with up stems.
      // A fixed stem-side correction survives justification of equal durations.
      const int signs[4][8] = {
            { 0, -1, 0, 1, 0, -1, 0, 0 },
            { -1, 1, -1, 1, -1, 1, -1, 0 },
            { 0, 1, 0, -1, 0, 1, 0, 0 },
            { 1, -1, 1, -1, 1, -1, 1, 0 }
            };
      const qreal displacement = score->noteHeadWidth() - score->styleP(Sid::stemWidth);
      QVERIFY(displacement > 0);
      std::vector<qreal> modern;
      int row = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++row) {
            QVERIFY(row < 4);
            int column = 0;
            qreal coreWidth = 0;
            for (Segment& seg : m->segments()) {
                  modern.push_back(seg.x());
                  modern.push_back(seg.width());
                  QVERIFY(qIsFinite(seg.x()) && qIsFinite(seg.width()) && seg.width() >= 0);
                  if (!seg.isChordRestType()) {
                        QCOMPARE(seg.spacingWidthOffset4(), qreal(0));
                        continue;
                        }
                  QVERIFY(column < 8);
                  const qreal offset = signs[row][column++] * displacement;
                  QVERIFY2(qAbs(seg.spacingWidthOffset4() - offset) < 1e-6,
                        qPrintable(QString("H%1 note %2: offset %3, expected %4")
                              .arg(row + 9).arg(column).arg(seg.spacingWidthOffset4(), 0, 'g', 15)
                              .arg(offset, 0, 'g', 15)));
                  // These wide systems have no active collision floors.
                  const qreal width = seg.width() - offset;
                  if (coreWidth > 0)
                        QVERIFY(qAbs(width - coreWidth) < 1e-6);
                  coreWidth = width;
                  }
            QCOMPARE(column, 8);
            }
      QCOMPARE(row, 4);
      score->doLayout();
      size_t i = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
            for (Segment& seg : m->segments()) {
                  QVERIFY(i + 1 < modern.size());
                  QVERIFY(qAbs(seg.x() - modern[i++]) < 1e-6);
                  QVERIFY(qAbs(seg.width() - modern[i++]) < 1e-6);
                  }
      QCOMPARE(static_cast<int>(i), static_cast<int>(modern.size()));
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      i = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
            for (Segment& seg : m->segments()) {
                  QVERIFY(i + 1 < legacy.size());
                  QVERIFY(qAbs(seg.x() - legacy[i++]) < 1e-6);
                  QVERIFY(qAbs(seg.width() - legacy[i++]) < 1e-6);
                  QCOMPARE(seg.spacingWidthOffset4(), qreal(0));
                  }
      QCOMPARE(static_cast<int>(i), static_cast<int>(legacy.size()));
      delete score;
      }

void TestSpacing::crossBeamCorners_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<int>("measures");
      QTest::addColumn<bool>("modernBeams");
      QTest::newRow("corners-legacy-beams") << QString("studio-spacing-cross-staff-corners.mscx") << 6 << false;
      QTest::newRow("corners-studio-beams") << QString("studio-spacing-cross-staff-corners.mscx") << 6 << true;
      QTest::newRow("natural-legacy-beams") << QString("studio-spacing-natural-h13.mscx") << 1 << false;
      QTest::newRow("natural-studio-beams") << QString("studio-spacing-natural-h13.mscx") << 1 << true;
      }

void TestSpacing::crossBeamCorners()
      {
      QFETCH(QString, file);
      QFETCH(int, measures);
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      QVERIFY(!score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      auto snapshot = [score]() -> std::vector<qreal> {
            std::vector<qreal> values;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  values.push_back(m->width());
                  for (Segment& seg : m->segments()) {
                        values.push_back(seg.x());
                        values.push_back(seg.width());
                        }
                  }
            return values;
            };
      const std::vector<qreal> legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      int row = 0;
      const int expectedChords[] = { 6, 6, 8, 6, 8, 11 };
      const int signs[4][8] = {
            { -1, 1, 0, -1, 1, 0, 0, 0 },
            { 1, -1, 0, 1, -1, 0, 0, 0 },
            { -1, 1, -1, 1, -1, 1, -1, 0 },
            { -1, 0, -1, 0, 0, 0, 0, 0 }
            };
      const qreal displacement = score->noteHeadWidth() - score->styleP(Sid::stemWidth);
      QVERIFY(displacement > 0);
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++row) {
            QVERIFY(row < measures);
            QVERIFY(m->ticks() == Fraction(1, 2));
            const int sourceTrack = (row == 1 || row == 5) ? VOICES : 0;
            Segment* first = m->findSegmentR(SegmentType::ChordRest, Fraction(0, 1));
            QVERIFY(first);
            ChordRest* primary = first->cr(sourceTrack);
            QVERIFY(primary && primary->isChord() && primary->beam());
            QVERIFY(primary->beam()->cross());
            int chords = 0;
            int breaks = 0;
            int onset = 0;
            for (Segment& seg : m->segments()) {
                  QVERIFY(qIsFinite(seg.x()) && qIsFinite(seg.width()) && seg.width() >= 0);
                  if (seg.isChordRestType()) {
                        QVERIFY(onset < 8);
                        const qreal expected = row < 4 ? signs[row][onset] * displacement : 0;
                        QVERIFY2(qAbs(seg.spacingWidthOffset4() - expected) < 1e-6,
                              qPrintable(QString("H%1 onset %2: offset %3, expected %4")
                                    .arg(row + 13).arg(onset + 1).arg(seg.spacingWidthOffset4(), 0, 'g', 15)
                                    .arg(expected, 0, 'g', 15)));
                        // H13's lower-to-upper isolated secondary beams require
                        // clearance for both beamlets, even in the short system.
                        if (row == 0 && (onset == 1 || onset == 4)) {
                              const qreal floor = displacement + 2 * score->styleP(Sid::beamMinLen)
                                    + 0.35 * m->spatium();
                              QVERIFY(seg.width() + 1e-6 >= floor);
                              }
                        ++onset;
                        }
                  else
                        QCOMPARE(seg.spacingWidthOffset4(), qreal(0));
                  for (Element* element : seg.elist()) {
                        if (!element || !element->isChord())
                              continue;
                        ++chords;
                        if (toChordRest(element)->beamMode() == Beam::Mode::BEGIN32)
                              ++breaks;
                        }
                  }
            QCOMPARE(chords, expectedChords[row]);
            QCOMPARE(breaks, row < 2 ? 2 : 0);
            QCOMPARE(onset, row < 2 ? 6 : (row == 2 || row == 5 ? 8 : 4));
            }
      QCOMPARE(row, measures);
      const std::vector<qreal> modern = snapshot();
      score->doLayout();
      const std::vector<qreal> repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i)
            QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const std::vector<qreal> restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i)
            QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::sustainedVoiceSwitch_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<bool>("natural");
      QTest::addColumn<bool>("modernBeams");
      QTest::newRow("natural-legacy-beams") << QString("v0.37-spacing-natural-h16.mscx") << true << false;
      QTest::newRow("natural-studio-beams") << QString("v0.37-spacing-natural-h16.mscx") << true << true;
      QTest::newRow("mirrored-legacy-beams") << QString("v0.38-spacing-voices.mscx") << false << false;
      QTest::newRow("mirrored-studio-beams") << QString("v0.38-spacing-voices.mscx") << false << true;
      }

void TestSpacing::sustainedVoiceSwitch()
      {
      QFETCH(QString, file);
      QFETCH(bool, natural);
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      QVERIFY(!score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      auto snapshot = [score]() -> std::vector<qreal> {
            std::vector<qreal> values;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  values.push_back(m->width());
                  for (Segment& seg : m->segments()) {
                        values.push_back(seg.x());
                        values.push_back(seg.width());
                        }
                  }
            return values;
            };
      const std::vector<qreal> legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      const qreal displacement = score->noteHeadWidth() - score->styleP(Sid::stemWidth);
      QVERIFY(displacement > 0);
      const int signs[3][4] = { { -1, 0, -1, 0 }, { 1, 0, 1, 0 }, { 0, 0, 0, 0 } };
      int row = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++row) {
            QVERIFY(row < (natural ? 1 : 2));
            QVERIFY(m->ticks() == Fraction(1, 2));
            const int profile = natural ? 0 : row + 1;
            int onset = 0;
            int extraChords = 0;
            const int primaryTrack = profile == 1 ? VOICES : 0;
            Segment* first = m->findSegmentR(SegmentType::ChordRest, Fraction(0, 1));
            QVERIFY(first && first->cr(primaryTrack) && first->cr(primaryTrack)->beam());
            QVERIFY(first->cr(primaryTrack)->beam()->cross());
            for (Segment& seg : m->segments()) {
                  QVERIFY(qIsFinite(seg.x()) && qIsFinite(seg.width()) && seg.width() >= 0);
                  if (!seg.isChordRestType())
                        continue;
                  QVERIFY(onset < 4);
                  ChordRest* extra = seg.cr(primaryTrack + 1);
                  if (extra && extra->isChord()) {
                        ++extraChords;
                        QVERIFY(!extra->beam());
                        if (!natural)
                              QCOMPARE(int(extra->beamMode()), int(Beam::Mode::NONE));
                        }
                  const qreal expected = signs[profile][onset] * displacement;
                  QVERIFY2(qAbs(seg.spacingWidthOffset4() - expected) < 1e-6,
                        qPrintable(QString("voice profile %1 onset %2: offset %3, expected %4")
                              .arg(profile).arg(onset + 1).arg(seg.spacingWidthOffset4(), 0, 'g', 15)
                              .arg(expected, 0, 'g', 15)));
                  // Even a blocked signed adjustment retains up-to-down clearance.
                  if ((profile != 1 && onset == 1) || (profile == 1 && onset % 2 == 0))
                        QVERIFY(seg.width() + 1e-6 >= 2 * displacement);
                  ++onset;
                  }
            QCOMPARE(onset, 4);
            QCOMPARE(extraChords, profile == 2 ? 4 : 2);
            }
      QCOMPARE(row, natural ? 1 : 2);
      const std::vector<qreal> modern = snapshot();
      score->doLayout();
      const std::vector<qreal> repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i)
            QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const std::vector<qreal> restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i)
            QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::accidentalInputs_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<bool>("natural");
      QTest::addColumn<bool>("modernBeams");
      QTest::newRow("overview-legacy-beams") << QString("v0.40-spacing-accidentals.mscx") << false << false;
      QTest::newRow("overview-studio-beams") << QString("v0.40-spacing-accidentals.mscx") << false << true;
      QTest::newRow("natural-legacy-beams") << QString("v0.40-spacing-natural-h24.mscx") << true << false;
      QTest::newRow("natural-studio-beams") << QString("v0.40-spacing-natural-h24.mscx") << true << true;
      }

void TestSpacing::accidentalInputs()
      {
      QFETCH(QString, file);
      QFETCH(bool, natural);
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      QVERIFY(score->styleB(Sid::spacingLayout4)); // visual inputs open ready for comparison
      score->setStyleValue(Sid::spacingLayout4, false);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      auto snapshot = [score]() -> std::vector<qreal> {
            std::vector<qreal> values;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  values.push_back(m->width());
                  for (Segment& seg : m->segments()) {
                        values.push_back(seg.x());
                        values.push_back(seg.width());
                        }
                  }
            return values;
            };
      const std::vector<qreal> legacy = snapshot();
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            Segment* first = m->findSegmentR(SegmentType::ChordRest, Fraction(0, 1));
            QVERIFY(first && first->nextActive());
            QCOMPARE(first->minHorizontalDistance(first->nextActive(), false, true),
                  first->minHorizontalDistance(first->nextActive(), false));
            }
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      const int firstPitches[] = { 73, 75, 61, 73, 49, 73 };
      const int firstTpcs[] = { 21, 11, 21, 21, 21, 21 };
      const int signs[3][4] = { { -1, 1, -1, 0 }, { 1, -1, 1, 0 }, { -1, 0, -1, 0 } };
      const qreal displacement = score->noteHeadWidth() - score->styleP(Sid::stemWidth);
      int row = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++row) {
            QVERIFY(row < (natural ? 1 : 6));
            const int input = natural ? 3 : row;
            QVERIFY(m->ticks() == Fraction(1, 2));
            Segment* first = m->findSegmentR(SegmentType::ChordRest, Fraction(0, 1));
            QVERIFY(first);
            ChordRest* primary = first->cr(input == 4 ? VOICES : 0);
            QVERIFY(primary && primary->isChord() && primary->beam());
            QCOMPARE(primary->beam()->cross(), input >= 3);
            const Chord* chord = toChord(primary);
            QCOMPARE(static_cast<int>(chord->notes().size()), input == 2 ? 2 : 1);
            QCOMPARE(chord->notes().front()->pitch(), firstPitches[input]);
            QCOMPARE(chord->notes().front()->tpc1(), firstTpcs[input]);
            for (const Note* note : chord->notes()) {
                  QVERIFY(note->accidental()); // prove the chromatic input actually generated signs
                  QCOMPARE(int(note->accidental()->accidentalType()),
                        int(input == 1 ? AccidentalType::FLAT : AccidentalType::SHARP));
                  }
            if (input == 3 || input == 5) {
                  Segment* next = first->nextActive();
                  QVERIFY(next && next->isChordRestType());
                  QVERIFY(first->spacingWidthOffset4() < 0);
                  const qreal plain = first->minHorizontalDistance(next, false);
                  const qreal kerned = first->minHorizontalDistance(next, false, true);
                  const qreal geometry = first->minHorizontalCollidingDistance(next);
                  const qreal floor = qMax(qreal(0), geometry) + score->styleP(Sid::minNoteDistance)
                        + next->extraLeadingSpace().val() * first->spatium();
                  QVERIFY(qAbs(kerned - floor) < 1e-6);
                  QVERIFY(kerned <= plain + 1e-6);
                  QVERIFY(first->width() + 1e-6 >= kerned);
                  if (natural)
                        QVERIFY(first->width() + 1e-6 < plain);
                  }
            int onset = 0;
            for (Segment& seg : m->segments()) {
                  QVERIFY(qIsFinite(seg.x()) && qIsFinite(seg.width()) && seg.width() >= 0);
                  if (!seg.isChordRestType())
                        continue;
                  QVERIFY(onset < 4);
                  const qreal expected = input >= 3 ? signs[input - 3][onset] * displacement : 0;
                  QVERIFY2(qAbs(seg.spacingWidthOffset4() - expected) < 1e-6,
                        qPrintable(QString("H%1 onset %2: offset %3, expected %4")
                              .arg(input + 21).arg(onset + 1).arg(seg.spacingWidthOffset4(), 0, 'g', 15)
                              .arg(expected, 0, 'g', 15)));
                  ++onset;
                  }
            QCOMPARE(onset, 4);
            }
      QCOMPARE(row, natural ? 1 : 6);
      const std::vector<qreal> modern = snapshot();
      score->doLayout();
      const std::vector<qreal> repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i)
            QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const std::vector<qreal> restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i)
            QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::systemInputs_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::newRow("legacy-beams") << false;
      QTest::newRow("studio-beams") << true;
      }

void TestSpacing::systemInputs()
      {
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + "v0.41-spacing-systems.mscx");
      QVERIFY(score);
      QVERIFY(score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      std::vector<Measure*> measures;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
            measures.push_back(m);
      QCOMPARE(static_cast<int>(measures.size()), 9);
      auto grouped = [&measures]() {
            for (size_t i = 0; i < measures.size(); ++i) {
                  if (!measures[i]->system())
                        return false;
                  if (i && ((measures[i]->system() == measures[i - 1]->system()) != (i % 3 != 0)))
                        return false;
                  }
            return true;
            };
      auto snapshot = [&measures]() -> std::vector<qreal> {
            std::vector<qreal> values;
            for (Measure* m : measures) {
                  values.push_back(m->x());
                  values.push_back(m->width());
                  for (Segment& seg : m->segments()) {
                        values.push_back(seg.x());
                        values.push_back(seg.width());
                        }
                  }
            return values;
            };
      QVERIFY(grouped());
      const std::vector<qreal> legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      QVERIFY(grouped());
      const int counts[] = { 4, 8, 16, 16, 8, 4, 7, 16, 12 };
      qreal systemForce = 0;
      for (size_t i = 0; i < measures.size(); ++i) {
            if (i % 3 == 0)
                  systemForce = 0;
            Measure* m = measures[i];
            QVERIFY(m->ticks() == Fraction(1, 1));
            QVERIFY(qIsFinite(m->x()) && qIsFinite(m->width()) && m->width() > 0);
            int onsets = 0;
            int signs = 0;
            for (Segment& seg : m->segments()) {
                  QVERIFY(qIsFinite(seg.x()) && qIsFinite(seg.width()) && seg.width() >= 0);
                  QCOMPARE(seg.spacingWidthOffset4(), qreal(0));
                  if (!seg.isChordRestType())
                        continue;
                  ++onsets;
                  ChordRest* cr = seg.cr(0);
                  QVERIFY(cr && cr->isChord());
                  // The first two spacious systems have no active collision
                  // floors: one rhythmic spring force must span all 3 measures.
                  if (i < 6) {
                        QVERIFY(seg.stretch() > 0);
                        const qreal force = seg.width() / seg.stretch();
                        if (systemForce > 0)
                              QVERIFY2(qAbs(force - systemForce) < 1e-6,
                                    qPrintable(QString("H%1: spring force %2, system force %3")
                                          .arg(i + 27).arg(force, 0, 'g', 15).arg(systemForce, 0, 'g', 15)));
                        systemForce = force;
                        }
                  if (i == 8) {
                        QVERIFY(cr->tuplet());
                        QVERIFY(cr->actualTicks() == Fraction(1, 12));
                        }
                  for (const Note* note : toChord(cr)->notes())
                        signs += note->accidental() != nullptr;
                  }
            QCOMPARE(onsets, counts[i]);
            if (i == 7)
                  QVERIFY(signs > 0);
            }
      const std::vector<qreal> modern = snapshot();
      score->doLayout();
      QVERIFY(grouped());
      const std::vector<qreal> repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i)
            QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      QVERIFY(grouped());
      const std::vector<qreal> restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i)
            QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::graceBeforePlan_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::newRow("legacy-beams") << false;
      QTest::newRow("studio-beams") << true;
      }

void TestSpacing::graceBeforePlan()
      {
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + "v0.79-spacing-grace-before.mscx");
      QVERIFY(score);
      QVERIFY(score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      const SpacingPaddingTable4 table(score);
      int index = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++index) {
            Segment* s = m->first(SegmentType::ChordRest);
            QVERIFY(s && s->element(0) && s->element(0)->isChord());
            Chord* main = toChord(s->element(0));
            const auto graces = main->graceNotesBefore();
            QCOMPARE(graces.size(), index < 4 ? 4 : 1);
            const bool up = index < 2 || index == 4;
            if (index < 4) {
                  QVERIFY(graces.front()->beam());
                  // Grace beams choose their own direction independently of
                  // explicit per-chord stem directions in Evolution.
                  QVERIFY(graces.front()->beam()->beamDirection() == (up ? Direction::UP : Direction::DOWN));
                  }
            else
                  QVERIFY(!graces.front()->beam());
            std::vector<QPointF> native;
            for (const Chord* grace : graces) {
                  QCOMPARE(grace->up(), up);
                  native.push_back(grace->pos());
                  }
            const qreal nativeWidth = m->width();
            const QPointF nativeMain = main->pos();
            GraceBeforePlan4 plan;
            QString reason;
            QVERIFY2(buildGraceBeforePlan4(main, plan, &reason), qPrintable(reason));
            QCOMPARE(static_cast<int>(plan.positions.size()), graces.size());
            QVERIFY(plan.left <= plan.positions.front().x + 1e-8);
            const qreal terminalBound = -(0.45 * score->spatium()
                  + graces.back()->notes().front()->headWidth() / 2) - main->pos().x();
            QVERIFY(plan.positions.back().x <= terminalBound + 1e-8);
            for (int i = 0; i < graces.size(); ++i) {
                  QCOMPARE(plan.positions[i].chord, static_cast<const Chord*>(graces[i]));
                  QCOMPARE(graces[i]->pos(), native[i]);
                  if (i) QVERIFY(plan.positions[i - 1].x < plan.positions[i].x);
                  }
            QCOMPARE(main->pos(), nativeMain);
            QCOMPARE(m->width(), nativeWidth);
            SpacingPairPolicy4 policy;
            const Note* last = graces.back()->notes().front();
            QVERIFY(!spacingPairPolicy4(table, last, main->notes().front(), main->spatium(), policy));
            QVERIFY(spacingGracePairPolicy4(table, main, last, main->notes().front(), main->spatium(), policy));
            QVERIFY(policy.padding >= .45 * score->spatium() - 1e-8);
            if (graces.size() > 1) {
                  QVERIFY(spacingGracePairPolicy4(table, main, graces.front()->notes().front(),
                        last, main->spatium(), policy));
                  QVERIFY(policy.padding >= .3 * score->spatium() - 1e-8);
                  }
            // Check every notehead pair, including nonadjacent group members,
            // against its independently queried collision constraint.
            for (int a = 0; a < graces.size(); ++a) {
                  const Note* first = graces[a]->notes().front();
                  for (int b = a + 1; b <= graces.size(); ++b) {
                        const Note* second = b == graces.size() ? main->notes().front()
                              : graces[b]->notes().front();
                        QVERIFY(spacingGracePairPolicy4(table, main, first, second, main->spatium(), policy));
                        const qreal required = spacingPairDistance4(first->bbox().translated(first->pos()),
                              second->bbox().translated(second->pos()), policy, main->spatium(), 1);
                        const qreal rightX = b == graces.size() ? 0 : plan.positions[b].x;
                        QVERIFY(rightX - plan.positions[a].x >= required - 1e-8);
                        }
                  }
            // Neither old grace x positions nor a second query may feed back
            // into the plan through the native main chord's composite shape.
            for (Chord* grace : graces) grace->rxpos() -= 100 * score->spatium();
            GraceBeforePlan4 repeated;
            QVERIFY2(buildGraceBeforePlan4(main, repeated, &reason), qPrintable(reason));
            QCOMPARE(repeated.left, plan.left);
            for (int i = 0; i < graces.size(); ++i) {
                  QCOMPARE(repeated.positions[i].x, plan.positions[i].x);
                  graces[i]->setPos(native[i]);
                  }
            // Unsupported edits reject atomically, preserving a caller's plan.
            Note* note = graces.front()->notes().front();
            const QPointF oldOffset = note->offset();
            note->setOffset(QPointF(.5, 0));
            QVERIFY(!buildGraceBeforePlan4(main, repeated, &reason));
            QVERIFY(!reason.isEmpty());
            QCOMPARE(repeated.left, plan.left);
            QCOMPARE(repeated.positions.front().x, plan.positions.front().x);
            note->setOffset(oldOffset);
            main->setNoStem(true);
            QVERIFY(!buildGraceBeforePlan4(main, repeated, &reason));
            main->setNoStem(false);
            }
      QCOMPARE(index, 6);
      GraceBeforePlan4 unchanged;
      unchanged.left = 123;
      QVERIFY(!buildGraceBeforePlan4(nullptr, unchanged));
      QCOMPARE(unchanged.left, qreal(123));
      delete score;
      }

void TestSpacing::graceBeforeLayout_data()
      {
      graceBeforePlan_data();
      }

void TestSpacing::graceBeforeLayout()
      {
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + "v0.80-spacing-grace-before.mscx");
      QVERIFY(score);
      QVERIFY(score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      const auto measureSnapshot = [](const Measure* measure) {
            std::vector<qreal> values = { measure->width() };
            for (const Segment& s : measure->segments()) {
                  if (!s.enabled() || !s.isChordRestType())
                        continue;
                  values.insert(values.end(), {s.x(), s.width(), s.staffShape(0).left()});
                  const Element* item = s.element(0);
                  if (!item || !item->isChord())
                        continue;
                  const Chord* chord = toChord(item);
                  values.insert(values.end(), {chord->x(), chord->spacingLeft4(), chord->spacingRight4()});
                  for (const Chord* grace : chord->graceNotesBefore())
                        values.push_back(grace->x());
                  }
            return values;
            };
      const auto snapshot = [&]() {
            std::vector<qreal> values;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  const auto part = measureSnapshot(m);
                  values.insert(values.end(), part.begin(), part.end());
                  }
            return values;
            };
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      int count = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++count) {
            Segment* s = m->first(SegmentType::ChordRest);
            QVERIFY(s && s->element(0) && s->element(0)->isChord());
            const Chord* main = toChord(s->element(0));
            GraceBeforePlan4 plan;
            QString reason;
            QVERIFY2(buildGraceBeforePlan4(main, plan, &reason), qPrintable(reason));
            for (const GracePosition4& p : plan.positions) {
                  QVERIFY(qAbs(p.chord->x() - p.x) < 1e-6);
                  const Note* note = p.chord->notes().front();
                  const QRectF expected = note->bbox().translated(note->pos() + p.chord->pos() + main->pos());
                  bool found = false;
                  for (const QRectF& rect : s->staffShape(0)) {
                        if (rect == expected) found = true;
                        }
                  QVERIFY(found); // The segment shape was rebuilt after publication.
                  }
            const qreal leftExtent = -plan.left - main->x();
            QVERIFY(main->spacingLeft4() >= leftExtent - 1e-6);
            QVERIFY(s->staffShape(0).left() >= leftExtent - 1e-6);
            QVERIFY(s->x() + main->x() + plan.left >= -1e-6);
            QVERIFY(m->width() >= s->x() + main->x() + main->spacingRight4() - 1e-6);
            // v0.81 admits this validated group, including header/group
            // and group/following-barline geometry, to the full system plan.
            SpacingPlan4 expectedPlan;
            QVERIFY2(buildSpacingPlan4(m->system(), expectedPlan, &reason), qPrintable(reason));
            QVERIFY(expectedPlan.width <= m->system()->width() + 1e-6);
            if (expectedPlan.width < m->system()->width() - 1e-6)
                  QVERIFY(justifySpacingPlan4(expectedPlan, m->system()->width()));
            for (const SpacingSegmentPlan4& entry : expectedPlan.segments) {
                  if (entry.ignored) continue;
                  QVERIFY(qAbs(entry.segment->measure()->x() + entry.segment->x() - entry.x) < 1e-6);
                  QVERIFY(qAbs(entry.segment->width() - entry.width) < 1e-6);
                  }
            }
      QCOMPARE(count, 6);
      const auto modern = snapshot();
      QCOMPARE(static_cast<int>(modern.size()), static_cast<int>(legacy.size()));
      bool changed = false;
      for (size_t i = 0; i < modern.size(); ++i)
            changed = changed || qAbs(modern[i] - legacy[i]) > 1e-6;
      QVERIFY(changed);
      score->doLayout();
      const auto repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i)
            QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i)
            QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      // A manual grace offset stays on the native path in either style state.
      Measure* second = score->firstMeasure()->nextMeasure();
      Chord* main = toChord(second->first(SegmentType::ChordRest)->element(0));
      Note* note = main->graceNotesBefore()[1]->notes().front();
      note->setOffset(QPointF(.5, 0));
      score->doLayout();
      const auto unsupportedLegacy = measureSnapshot(second);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      GraceBeforePlan4 rejected;
      QVERIFY(!buildGraceBeforePlan4(main, rejected));
      const auto unsupportedModern = measureSnapshot(second);
      QCOMPARE(static_cast<int>(unsupportedModern.size()), static_cast<int>(unsupportedLegacy.size()));
      for (size_t i = 0; i < unsupportedLegacy.size(); ++i)
            QVERIFY(qAbs(unsupportedModern[i] - unsupportedLegacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::graceNeighborPlan_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<QString>("file");
      for (bool modern : {false, true}) {
            const QString tag = modern ? "studio-" : "legacy-";
            QTest::newRow(qPrintable(tag + "justified")) << modern << QString("v0.81-spacing-grace-neighbors.mscx");
            QTest::newRow(qPrintable(tag + "natural")) << modern << QString("v0.81-spacing-grace-neighbors-natural.mscx");
            QTest::newRow(qPrintable(tag + "slurred-accidentals")) << modern << QString("v0.86-spacing-grace-accidental-neighbors.mscx");
            QTest::newRow(qPrintable(tag + "slurred-accidentals-natural")) << modern << QString("v0.86-spacing-grace-accidental-neighbors-natural.mscx");
            }
      }

void TestSpacing::graceNeighborPlan()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      QVERIFY(score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      const auto snapshot = [&]() {
            std::vector<qreal> values;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  values.insert(values.end(), {m->x(), m->width()});
                  for (const Segment& s : m->segments()) {
                        if (!s.enabled()) continue;
                        values.insert(values.end(), {s.x(), s.width(), s.spacingWidthOffset4()});
                        for (const Element* item : s.elist()) {
                              if (!item || !item->isChord()) continue;
                              const Chord* chord = toChord(item);
                              values.push_back(chord->spacingLeft4());
                              for (const Chord* grace : chord->graceNotes()) values.insert(values.end(), {grace->x(), grace->y()});
                              }
                        }
                  }
            return values;
            };
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      const auto modern = snapshot();
      const SpacingPaddingTable4 table(score);
      SpacingGraceContext4 context;
      std::vector<Chord*> mains;
      int count = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++count) {
            int chords = 0;
            Chord* main = nullptr;
            for (const Segment& s : m->segments()) {
                  if (!s.isChordRestType()) continue;
                  ++chords;
                  Chord* chord = toChord(s.element(0));
                  QVERIFY(chord);
                  if (!chord->graceNotes().empty()) main = chord;
                  }
            QCOMPARE(chords, 3);
            QVERIFY(main);
            QCOMPARE(main->rtick().ticks(), DIVISION);
            if (file.startsWith("v0.86-")) {
                  QVERIFY(count < 6);
                  const int mainPitch[] = {72, 73, 72, 73, 73, 61};
                  const int previousPitch[] = {73, 69, 73, 69, 66, 66};
                  const int previousTpc[] = {21, 17, 21, 17, 20, 20};
                  const int nextPitch[] = {67, 72, 67, 72, 65, 65};
                  const int nextTpc[] = {15, 14, 15, 14, 13, 13};
                  const Note* n = main->notes().front();
                  QCOMPARE(n->pitch(), mainPitch[count]);
                  QCOMPARE(n->tpc(), count == 0 || count == 2 ? 14 : 21);
                  QVERIFY(n->accidental());
                  QCOMPARE(int(n->accidental()->accidentalType()),
                        int(count == 0 || count == 2 ? AccidentalType::NATURAL : AccidentalType::SHARP));
                  const Chord* previous = toChord(main->segment()->prev(SegmentType::ChordRest)->element(0));
                  const Chord* next = toChord(main->segment()->next(SegmentType::ChordRest)->element(0));
                  QCOMPARE(previous->notes().front()->pitch(), previousPitch[count]);
                  QCOMPARE(previous->notes().front()->tpc(), previousTpc[count]);
                  QCOMPARE(next->notes().front()->pitch(), nextPitch[count]);
                  QCOMPARE(next->notes().front()->tpc(), nextTpc[count]);
                  if (count != 1 && count != 3) {
                        QVERIFY(previous->notes().front()->accidental());
                        QCOMPARE(int(previous->notes().front()->accidental()->accidentalType()), int(AccidentalType::SHARP));
                        }
                  if (count != 0 && count != 2) {
                        QVERIFY(next->notes().front()->accidental());
                        QCOMPARE(int(next->notes().front()->accidental()->accidentalType()), int(AccidentalType::NATURAL));
                        }
                  Slur* local = nullptr;
                  for (const auto& entry : score->spannerMap().map()) {
                        if (entry.second->isSlur() && entry.second->endElement() == main)
                              local = toSlur(entry.second);
                        }
                  QVERIFY(local);
                  QCOMPARE(local->startElement(), static_cast<Element*>(main->graceNotesBefore().front()));
                  QVERIFY(spacingLocalGraceSlur4(local, main));
                  }
            context.mains.push_back(main);
            mains.push_back(main);
            GraceBeforePlan4 group;
            QString reason;
            QVERIFY2(buildGraceBeforePlan4(main, group, &reason), qPrintable(reason));
            for (const GracePosition4& p : group.positions) QVERIFY(qAbs(p.chord->x() - p.x) < 1e-6);
            if (m != m->system()->firstMeasure()) continue;
            SpacingPlan4 natural;
            QVERIFY2(buildSpacingPlan4(m->system(), natural, &reason), qPrintable(reason));
            SpacingPlan4 expected = natural;
            QVERIFY(expected.width <= m->system()->width() + 1e-6);
            if (expected.width < m->system()->width() - 1e-6)
                  QVERIFY(justifySpacingPlan4(expected, m->system()->width()));
            bool measureMismatch = false;
            for (const SpacingMeasurePlan4& e : expected.measures)
                  measureMismatch = measureMismatch || qAbs(e.measure->x() - e.x) >= 1e-6
                        || qAbs(e.measure->width() - e.width) >= 1e-6;
            if (measureMismatch) {
                  // Qt 5.9 truncates a single QVERIFY2 message. Emit short
                  // separate messages so segment details survive in result.xml.
                  const QString report = QString("%1: natural=%2 system=%3 expected=%4")
                        .arg(file).arg(natural.width, 0, 'g', 17)
                        .arg(m->system()->width(), 0, 'g', 17).arg(expected.width, 0, 'g', 17);
                  qDebug("%s", qPrintable(report));
                  for (size_t i = 0; i < expected.measures.size(); ++i) {
                        const SpacingMeasurePlan4& e = expected.measures[i];
                        const QString line = QString("measure tick=%1: x actual=%2 expected=%3; width actual=%4 expected=%5 natural=%6")
                              .arg(e.measure->tick().ticks()).arg(e.measure->x(), 0, 'g', 17)
                              .arg(e.x, 0, 'g', 17).arg(e.measure->width(), 0, 'g', 17)
                              .arg(e.width, 0, 'g', 17).arg(natural.measures[i].width, 0, 'g', 17);
                        qDebug("%s", qPrintable(line));
                        }
                  for (const SpacingSegmentPlan4& e : natural.segments) {
                        if (e.ignored) continue;
                        const QString line = QString("segment tick=%1 type=%2: x actual=%3 natural=%4; width actual=%5 natural=%6")
                              .arg(e.segment->tick().ticks()).arg(int(e.segment->segmentType()))
                              .arg(e.segment->measure()->x() + e.segment->x(), 0, 'g', 17)
                              .arg(e.x, 0, 'g', 17).arg(e.segment->width(), 0, 'g', 17).arg(e.width, 0, 'g', 17);
                        qDebug("%s", qPrintable(line));
                        }
                  QVERIFY2(!measureMismatch, qPrintable(report));
                  }
            for (const SpacingMeasurePlan4& e : expected.measures) {
                  QVERIFY(qAbs(e.measure->x() - e.x) < 1e-6);
                  QVERIFY(qAbs(e.measure->width() - e.width) < 1e-6);
                  }
            for (const SpacingSegmentPlan4& e : expected.segments) {
                  if (e.ignored) continue;
                  QVERIFY(qAbs(e.segment->measure()->x() + e.segment->x() - e.x) < 1e-6);
                  QVERIFY(qAbs(e.segment->width() - e.width) < 1e-6);
                  }
            SpacingPlanOptions4 options;
            options.stretchReduction = .67;
            options.squeezeFactor = .8;
            options.minimumMeasureWidths = false;
            SpacingPlan4 compressed;
            QVERIFY2(buildSpacingPlan4(m->system(), compressed, &reason, options), qPrintable(reason));
            QVERIFY(compressed.width < natural.width);
            const qreal target = (compressed.width + natural.width) / 2;
            SpacingPlan4 squeezed;
            QVERIFY2(squeezeSpacingPlan4(m->system(), target, squeezed, &reason), qPrintable(reason));
            QVERIFY(justifySpacingPlan4(squeezed, target));
            QVERIFY(qAbs(squeezed.width - target) < 1e-6);
            }
      QCOMPARE(count, 6);
      // Default queries still reject grace input. Only the prepared context
      // admits its roots, including pairs with preceding/following ordinary notes.
      Chord* main = mains.front();
      const SpacingShape4 shape = spacingStaffShape4(main->segment(), 0);
      SpacingShape4 projected;
      const qreal sp = main->spatium();
      QVERIFY(!spacingCollisionProjection4(table, shape, sp, projected));
      QVERIFY(spacingCollisionProjection4(table, shape, sp, projected, &context));
      const Note* grace = main->graceNotesBefore().front()->notes().front();
      const Note* previous = toChord(main->segment()->prev(SegmentType::ChordRest)->element(0))->notes().front();
      SpacingPairPolicy4 policy;
      QVERIFY(!spacingPairPolicy4(table, previous, grace, sp, policy));
      QVERIFY(spacingPairPolicy4(table, previous, grace, sp, policy, &context));
      QVERIFY(policy.padding >= .45 * score->spatium() - 1e-6);
      SpacingGraceContext4 onlyFirst;
      onlyFirst.mains.push_back(main);
      const Note* other = mains[1]->graceNotesBefore().front()->notes().front();
      QVERIFY(!spacingPairPolicy4(table, grace, other, sp, policy, &onlyFirst));
      QVERIFY(spacingPairPolicy4(table, grace, other, sp, policy, &context));
      QVERIFY(snapshot() == modern); // Successful and compressed queries are read-only.
      // A stale member position rejects the entire system plan atomically.
      Chord* moved = main->graceNotesBefore().front();
      const qreal oldX = moved->x();
      moved->rxpos() += sp;
      SpacingPlan4 rejected;
      rejected.width = 123;
      QString reason;
      QVERIFY(!buildSpacingPlan4(main->measure()->system(), rejected, &reason));
      QCOMPARE(rejected.width, qreal(123));
      moved->rxpos() = oldX;
      // A manual edit in the last group of a system also rejects the whole plan.
      Measure* last = main->measure()->system()->lastMeasure();
      Chord* lastMain = nullptr;
      for (const Segment& s : last->segments()) {
            if (s.isChordRestType() && s.element(0) && s.element(0)->isChord()
                && !toChord(s.element(0))->graceNotes().empty()) lastMain = toChord(s.element(0));
            }
      QVERIFY(lastMain);
      Note* edited = lastMain->graceNotesBefore().front()->notes().front();
      const QPointF oldOffset = edited->offset();
      edited->setOffset(QPointF(.5, 0));
      QVERIFY(!buildSpacingPlan4(main->measure()->system(), rejected, &reason));
      QCOMPARE(rejected.width, qreal(123));
      edited->setOffset(oldOffset);
      score->doLayout();
      const auto repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

// Include system membership and active geometry, but not cached disabled headers
// left behind by earlier system-break choices.
static std::vector<qreal> graceLayoutSnapshot4(Score* score)
      {
      std::vector<qreal> values;
      const System* previous = nullptr;
      int systemIndex = -1;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            if (m->system() != previous) { previous = m->system(); ++systemIndex; }
            values.insert(values.end(), {qreal(systemIndex), m->x(), m->width()});
            for (const Segment& s : m->segments()) {
                  if (!s.enabled() && (s.header() || s.trailer())) continue;
                  values.insert(values.end(), {qreal(static_cast<int>(s.segmentType())), s.tick().toDouble(),
                        s.enabled() ? qreal(1) : qreal(0), s.x(), s.width(), s.spacingWidthOffset4()});
                  for (const Element* item : s.elist()) {
                        if (!item || !item->isChord()) continue;
                        const Chord* chord = toChord(item);
                        values.insert(values.end(), {chord->x(), chord->spacingLeft4(), chord->spacingRight4()});
                        for (const Chord* grace : chord->graceNotesBefore()) values.push_back(grace->x());
                        }
                  }
            }
      return values;
      }

void TestSpacing::graceAfterSystem_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<QString>("file");
      for (const QString& file : {QString("v0.97-spacing-after-grace-system.mscx"),
                                  QString("v0.97-spacing-after-grace-system-natural.mscx")}) {
            for (bool beams : {false, true}) {
                  const QByteArray tag = (file + (beams ? "-studio" : "-legacy")).toLatin1();
                  QTest::newRow(tag.constData()) << beams << file;
                  }
            }
      }

void TestSpacing::graceAfterSystem()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score && score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = graceLayoutSnapshot4(score);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      const auto modern = graceLayoutSnapshot4(score);
      QString reason;
      int count = 0, groups = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++count) {
            QVERIFY(count < 3);
            QVERIFY(m->ticks() == Fraction(1, 1));
            int onsets = 0;
            for (Segment& segment : m->segments()) {
                  if (!segment.isChordRestType()) continue;
                  ++onsets;
                  Element* item = segment.element(0);
                  QVERIFY(item && item->isChord());
                  const Chord* chord = toChord(item);
                  if (chord->graceNotesAfter().empty()) continue;
                  ++groups;
                  QCOMPARE(chord->graceNotesAfter().size(), count == 0 ? 1 : 4);
                  GraceAfterPlan4 group;
                  QVERIFY2(buildGraceAfterPlan4(chord, group, &reason), qPrintable(reason));
                  for (const auto& p : group.positions) {
                        const qreal actual = chord->segment()->x() + chord->x() + p.chord->x() - group.target->x();
                        QVERIFY2(qAbs(actual - p.x) < 1e-6,
                              qPrintable(QString("AG%1 target-relative x actual=%2 expected=%3")
                                    .arg(count + 1).arg(actual, 0, 'g', 17).arg(p.x, 0, 'g', 17)));
                        }
                  }
            QCOMPARE(onsets, 3);
            if (m != m->system()->firstMeasure()) continue;
            SpacingPlan4 natural;
            QVERIFY2(buildSpacingPlan4(m->system(), natural, &reason), qPrintable(reason));
            QVERIFY(!natural.afterGraces.empty());
            SpacingPlan4 expected = natural;
            QVERIFY2(expected.width <= m->system()->width() + 1e-6,
                  qPrintable(QString("after-grace natural width=%1 exceeds final system=%2 at tick=%3")
                        .arg(expected.width, 0, 'g', 17).arg(m->system()->width(), 0, 'g', 17)
                        .arg(m->tick().ticks())));
            if (expected.width < m->system()->width() - 1e-6) QVERIFY(justifySpacingPlan4(expected, m->system()->width()));
            for (const SpacingMeasurePlan4& e : expected.measures) {
                  QVERIFY2(qAbs(e.measure->width() - e.width) < 1e-6,
                        qPrintable(QString("measure tick=%1 width actual=%2 expected=%3 natural total=%4 system=%5")
                              .arg(e.measure->tick().ticks()).arg(e.measure->width(), 0, 'g', 17)
                              .arg(e.width, 0, 'g', 17).arg(natural.width, 0, 'g', 17)
                              .arg(m->system()->width(), 0, 'g', 17)));
                  QVERIFY(qAbs(e.measure->x() - e.x) < 1e-6);
                  }
            for (const SpacingSegmentPlan4& e : expected.segments) {
                  if (e.ignored) continue;
                  QVERIFY(qAbs(e.segment->measure()->x() + e.segment->x() - e.x) < 1e-6);
                  QVERIFY(qAbs(e.segment->width() - e.width) < 1e-6);
                  }
            SpacingPlanOptions4 options;
            options.minimumMeasureWidths = false;
            options.stretchReduction = .67;
            options.squeezeFactor = .8;
            SpacingPlan4 compressed;
            QVERIFY2(buildSpacingPlan4(m->system(), compressed, &reason, options), qPrintable(reason));
            QVERIFY(compressed.width < natural.width);
            QCOMPARE(static_cast<int>(compressed.afterGraces.size()), static_cast<int>(natural.afterGraces.size()));
            QVERIFY(graceLayoutSnapshot4(score) == modern);
            }
      QCOMPARE(count, 3);
      QCOMPARE(groups, 3);
      score->doLayout();
      const auto repeated = graceLayoutSnapshot4(score);
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto restored = graceLayoutSnapshot4(score);
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::graceAfterGeometry_data()
      {
      graceAfterBaseline_data();
      }

void TestSpacing::graceAfterGeometry()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      const auto native = graceLayoutSnapshot4(score);
      const auto belongsTo = [](const Element* item, const Chord* chord) {
            for (; item; item = item->parent()) {
                  if (item->isChord()) return item == chord;
                  }
            return false;
            };
      const auto sameShape = [](const SpacingShape4& a, const SpacingShape4& b) {
            if (a.elements.size() != b.elements.size()) return false;
            for (size_t i = 0; i < a.elements.size(); ++i) {
                  if (a.elements[i].item != b.elements[i].item || a.elements[i].identity != b.elements[i].identity
                      || a.elements[i].rect != b.elements[i].rect) return false;
                  }
            return true;
            };
      int count = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++count) {
            QVERIFY(count < 6);
            Chord* owner = nullptr;
            for (Segment& segment : m->segments()) {
                  Element* item = segment.element(0);
                  if (item && item->isChord() && !toChord(item)->graceNotesAfter().empty()) owner = toChord(item);
                  }
            QVERIFY(owner);
            GraceAfterGeometry4 geometry;
            geometry.plan.left = 123;
            geometry.source.elements.push_back(SpacingShapeElement4(QRectF(1, 2, 3, 4), owner));
            geometry.target = geometry.source;
            const SpacingShape4 sentinel = geometry.source;
            QString reason;
            const bool built = buildGraceAfterGeometry4(owner, geometry, &reason);
            if (count >= 3) {
                  QVERIFY(!built);
                  QCOMPARE(geometry.plan.left, qreal(123));
                  QVERIFY(!geometry.plan.owner && !geometry.plan.target && geometry.plan.positions.empty());
                  QVERIFY(sameShape(geometry.source, sentinel) && sameShape(geometry.target, sentinel));
                  continue;
                  }
            QVERIFY2(built, qPrintable(reason));
            QVERIFY(geometry.source.complete() && geometry.target.complete());
            QCOMPARE(geometry.plan.owner, static_cast<const Chord*>(owner));
            const Chord* target = toChord(geometry.plan.target->element(owner->track()));
            for (const auto& e : geometry.source.elements) QVERIFY(belongsTo(e.item, owner));
            int followingCopies = 0;
            const Note* followingNote = target->notes().front();
            for (const auto& e : geometry.target.elements) {
                  if (e.item == followingNote) {
                        ++followingCopies;
                        QCOMPARE(e.rect, followingNote->bbox().translated(followingNote->pos() + target->pos()));
                        }
                  }
            QCOMPARE(followingCopies, 1);
            for (const GraceAfterPosition4& position : geometry.plan.positions) {
                  const Note* note = position.chord->notes().front();
                  int sourceCopies = 0, targetCopies = 0;
                  for (const auto& e : geometry.source.elements) sourceCopies += e.item == note;
                  for (const auto& e : geometry.target.elements) {
                        if (e.item == note) {
                              ++targetCopies;
                              QCOMPARE(e.rect, note->bbox().translated(note->pos() + QPointF(position.x, 0)));
                              }
                        }
                  QCOMPARE(sourceCopies, 0);
                  QCOMPARE(targetCopies, 1); // No duplicate old source footprint.
                  }
            for (const auto& e : geometry.target.elements) {
                  bool known = belongsTo(e.item, target);
                  for (const auto& p : geometry.plan.positions) known = known || belongsTo(e.item, p.chord);
                  QVERIFY(known);
                  }
            const auto graces = owner->graceNotesAfter();
            std::vector<QPointF> saved;
            for (Chord* grace : graces) { saved.push_back(grace->pos()); grace->rxpos() -= 100 * score->spatium(); }
            const QPointF sourcePos = owner->segment()->pos();
            Segment* targetSegment = m->tick2segment(owner->tick() + owner->actualTicks(), SegmentType::ChordRest);
            QVERIFY(targetSegment);
            const QPointF targetPos = targetSegment->pos();
            owner->segment()->rxpos() -= 17 * score->spatium();
            targetSegment->rxpos() += 23 * score->spatium();
            GraceAfterGeometry4 repeated;
            const bool rebuilt = buildGraceAfterGeometry4(owner, repeated, &reason);
            owner->segment()->setPos(sourcePos);
            targetSegment->setPos(targetPos);
            for (int i = 0; i < graces.size(); ++i) graces[i]->setPos(saved[i]);
            QVERIFY2(rebuilt, qPrintable(reason));
            QVERIFY(sameShape(geometry.source, repeated.source) && sameShape(geometry.target, repeated.target));
            const QPointF offset = owner->notes().front()->offset();
            owner->notes().front()->setOffset(QPointF(.5, 0));
            const bool editedAccepted = buildGraceAfterGeometry4(owner, repeated, &reason);
            owner->notes().front()->setOffset(offset);
            QVERIFY(!editedAccepted);
            QVERIFY(sameShape(geometry.source, repeated.source) && sameShape(geometry.target, repeated.target));
            QCOMPARE(repeated.plan.owner, geometry.plan.owner);
            QCOMPARE(repeated.plan.target, geometry.plan.target);
            QCOMPARE(repeated.plan.left, geometry.plan.left);
            const SpacingPaddingTable4 table(score);
            qreal distance = 123;
            QVERIFY(!spacingShapeDistance4(table, geometry.source, geometry.target, owner->spatium(), 1, distance));
            QCOMPARE(distance, qreal(123)); // Default/system admission still requires a later context.
            }
      QCOMPARE(count, 6);
      QVERIFY(graceLayoutSnapshot4(score) == native);
      delete score;
      }

void TestSpacing::graceAfterLayout_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<QString>("file");
      for (const QString& file : {QString("v0.95-spacing-after-grace-packing.mscx"),
                                  QString("v0.95-spacing-after-grace-packing-natural.mscx")}) {
            for (bool beams : {false, true}) {
                  const QByteArray tag = (file + (beams ? "-studio" : "-legacy")).toLatin1();
                  QTest::newRow(tag.constData()) << beams << file;
                  }
            }
      }

void TestSpacing::graceAfterLayout()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score && score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      // Give every ordinary group ample room. Default/natural-page rejection
      // and stability are also covered by graceAfterBaseline/graceAfterPlan.
      score->setStyleValue(Sid::pageWidth, 100.0);
      score->setStyleValue(Sid::pagePrintableWidth, 98.8);
      score->setStyleValue(Sid::lastSystemFillLimit, 0.0);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = graceLayoutSnapshot4(score);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      int count = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++count) {
            QVERIFY(count < 6);
            Chord* owner = nullptr;
            for (Segment& segment : m->segments()) {
                  Element* item = segment.element(0);
                  if (item && item->isChord() && !toChord(item)->graceNotesAfter().empty()) owner = toChord(item);
                  }
            QVERIFY(owner);
            const auto native = graceLayoutSnapshot4(score);
            QString reason;
            if (count >= 3) {
                  QVERIFY(!applyGraceAfterPlan4(owner, &reason));
                  QVERIFY(graceLayoutSnapshot4(score) == native);
                  continue;
                  }
            GraceAfterPlan4 plan;
            QVERIFY2(buildGraceAfterPlan4(owner, plan, &reason), qPrintable(reason));
            for (const GraceAfterPosition4& p : plan.positions) {
                  const qreal actual = owner->segment()->x() + owner->x() + p.chord->x() - plan.target->x();
                  QVERIFY2(qAbs(actual - p.x) < 1e-6,
                        qPrintable(QString("AG%1 applied x=%2 expected=%3").arg(count + 1)
                              .arg(actual, 0, 'g', 17).arg(p.x, 0, 'g', 17)));
                  }
            const qreal width = m->width();
            QVERIFY2(applyGraceAfterPlan4(owner, &reason), qPrintable(reason));
            QCOMPARE(m->width(), width);
            QVERIFY(graceLayoutSnapshot4(score) == native); // Idempotent local publication.
            Segment* target = m->tick2segment(owner->tick() + owner->actualTicks(), SegmentType::ChordRest);
            QVERIFY(target);
            const QPointF old = target->pos();
            target->rxpos() = owner->segment()->x() - 100 * score->spatium();
            const auto cramped = graceLayoutSnapshot4(score);
            const bool accepted = applyGraceAfterPlan4(owner, &reason);
            const auto after = graceLayoutSnapshot4(score);
            target->setPos(old);
            QVERIFY(!accepted);
            QVERIFY(!reason.isEmpty());
            QVERIFY(after == cramped); // Not a single member moves on failed clearance.
            QVERIFY(graceLayoutSnapshot4(score) == native);
            }
      QCOMPARE(count, 6);
      const auto modern = graceLayoutSnapshot4(score);
      score->doLayout();
      const auto repeated = graceLayoutSnapshot4(score);
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto restored = graceLayoutSnapshot4(score);
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::graceAfterPlan_data()
      {
      graceAfterBaseline_data();
      }

void TestSpacing::graceAfterPlan()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      const auto native = graceLayoutSnapshot4(score);
      const SpacingPaddingTable4 table(score);
      int count = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++count) {
            QVERIFY(count < 6);
            Chord* owner = nullptr;
            for (Segment& segment : m->segments()) {
                  const Element* item = segment.element(0);
                  if (item && item->isChord() && !toChord(item)->graceNotesAfter().empty()) owner = toChord(segment.element(0));
                  }
            QVERIFY(owner);
            const auto graces = owner->graceNotesAfter();
            GraceAfterPlan4 plan;
            plan.left = 123;
            plan.right = 456;
            QString reason;
            const bool built = buildGraceAfterPlan4(owner, plan, &reason);
            if (count >= 3) {
                  QVERIFY(!built);
                  QVERIFY(!reason.isEmpty());
                  QCOMPARE(plan.left, qreal(123));
                  QCOMPARE(plan.right, qreal(456));
                  QVERIFY(!plan.owner && !plan.target && plan.positions.empty());
                  continue;
                  }
            QVERIFY2(built, qPrintable(reason));
            QVERIFY(reason.isEmpty());
            QCOMPARE(plan.owner, static_cast<const Chord*>(owner));
            const Segment* targetSegment = m->tick2segment(owner->tick() + owner->actualTicks(), SegmentType::ChordRest);
            QVERIFY(targetSegment && targetSegment->element(0) && targetSegment->element(0)->isChord());
            const Chord* target = toChord(targetSegment->element(0));
            QCOMPARE(plan.target, targetSegment);
            QCOMPARE(static_cast<int>(plan.positions.size()), graces.size());
            QVERIFY(qIsFinite(plan.left) && qIsFinite(plan.right) && plan.right > plan.left);
            for (int i = 0; i < graces.size(); ++i) {
                  QCOMPARE(plan.positions[i].chord, static_cast<const Chord*>(graces[i]));
                  QVERIFY(qIsFinite(plan.positions[i].x));
                  if (i) QVERIFY(plan.positions[i].x > plan.positions[i - 1].x);
                  }
            const qreal bound = -(0.45 * score->spatium() + graces.back()->notes().front()->headWidth() / 2);
            QVERIFY(plan.positions.back().x <= bound + 1e-6);
            SpacingPairPolicy4 policy;
            QVERIFY(!spacingPairPolicy4(table, graces.front()->notes().front(), target->notes().front(),
                  graces.front()->spatium(), policy));
            QVERIFY(spacingAfterGracePairPolicy4(table, owner, target, graces.front()->notes().front(),
                  target->notes().front(), graces.front()->spatium(), policy));
            QVERIFY(policy.padding >= .45 * score->spatium());
            QVERIFY(!spacingAfterGracePairPolicy4(table, owner, target, owner->notes().front(),
                  target->notes().front(), owner->spatium(), policy));
            // Target-relative output must not depend on stale native after-x,
            // owner x, or the unsolved source/target segment spacing.
            std::vector<QPointF> saved;
            for (Chord* grace : graces) { saved.push_back(grace->pos()); grace->rxpos() -= 100 * score->spatium(); }
            const QPointF ownerPos = owner->pos();
            const QPointF sourcePos = owner->segment()->pos();
            const QPointF targetPos = targetSegment->pos();
            owner->rxpos() += 11 * score->spatium();
            owner->segment()->rxpos() -= 17 * score->spatium();
            Segment* writableTarget = m->tick2segment(owner->tick() + owner->actualTicks(), SegmentType::ChordRest);
            writableTarget->rxpos() += 23 * score->spatium();
            GraceAfterPlan4 repeated;
            const bool rebuilt = buildGraceAfterPlan4(owner, repeated, &reason);
            owner->setPos(ownerPos);
            owner->segment()->setPos(sourcePos);
            writableTarget->setPos(targetPos);
            for (int i = 0; i < graces.size(); ++i) graces[i]->setPos(saved[i]);
            QVERIFY2(rebuilt, qPrintable(reason));
            QCOMPARE(repeated.left, plan.left);
            QCOMPARE(repeated.right, plan.right);
            QCOMPARE(static_cast<int>(repeated.positions.size()), static_cast<int>(plan.positions.size()));
            for (size_t i = 0; i < plan.positions.size(); ++i) QCOMPARE(repeated.positions[i].x, plan.positions[i].x);
            // An edited group/target rejects without replacing a prior result.
            for (Note* note : {graces.front()->notes().front(), target->notes().front()}) {
                  const QPointF old = note->offset();
                  note->setOffset(QPointF(.5, 0));
                  const bool accepted = buildGraceAfterPlan4(owner, repeated, &reason);
                  note->setOffset(old);
                  QVERIFY(!accepted);
                  QCOMPARE(repeated.left, plan.left);
                  QCOMPARE(repeated.right, plan.right);
                  QCOMPARE(repeated.owner, plan.owner);
                  QCOMPARE(repeated.target, plan.target);
                  QCOMPARE(static_cast<int>(repeated.positions.size()), static_cast<int>(plan.positions.size()));
                  for (size_t i = 0; i < plan.positions.size(); ++i) QCOMPARE(repeated.positions[i].x, plan.positions[i].x);
                  }
            Stem* stem = graces.front()->stem();
            const qreal userLength = stem->userLen();
            stem->setUserLen(score->spatium());
            const bool stemAccepted = buildGraceAfterPlan4(owner, repeated, &reason);
            stem->setUserLen(userLength);
            QVERIFY(!stemAccepted);
            QCOMPARE(repeated.left, plan.left);
            QCOMPARE(repeated.right, plan.right);
            SpacingPlan4 systemPlan;
            systemPlan.width = 123;
            const bool systemBuilt = buildSpacingPlan4(m->system(), systemPlan);
            QCOMPARE(systemBuilt, !file.contains("-natural.mscx"));
            if (systemBuilt) QCOMPARE(static_cast<int>(systemPlan.afterGraces.size()), 1);
            else QCOMPARE(systemPlan.width, qreal(123));
            }
      QCOMPARE(count, 6);
      GraceAfterPlan4 rejected;
      rejected.left = 123;
      QVERIFY(!buildGraceAfterPlan4(nullptr, rejected));
      QCOMPARE(rejected.left, qreal(123));
      QVERIFY(graceLayoutSnapshot4(score) == native);
      delete score;
      }

void TestSpacing::graceAfterBaseline_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<QString>("file");
      for (const QString& file : {QString("v0.92-spacing-after-grace-baseline.mscx"),
                                  QString("v0.92-spacing-after-grace-baseline-natural.mscx")}) {
            for (bool beams : {false, true}) {
                  const QByteArray tag = (file + (beams ? "-studio" : "-legacy")).toLatin1();
                  QTest::newRow(tag.constData()) << beams << file;
                  }
            }
      }

void TestSpacing::graceAfterBaseline()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score && score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      const auto measureSnapshot = [](const Measure* m) {
            std::vector<qreal> values = {m->width()};
            for (const Segment& segment : m->segments()) {
                  if (!segment.enabled()) continue;
                  values.insert(values.end(), {qreal(int(segment.segmentType())), segment.x(), segment.width()});
                  for (const Element* item : segment.elist()) {
                        if (!item || !item->isChord()) continue;
                        const Chord* chord = toChord(item);
                        values.insert(values.end(), {chord->x(), chord->y()});
                        for (const Chord* grace : chord->graceNotes())
                              values.insert(values.end(), {grace->x(), grace->y(), qreal(grace->noteType())});
                        }
                  }
            return values;
            };
      const auto afterTargetSnapshot = [](Measure* m) {
            std::vector<qreal> values;
            for (const Segment& segment : m->segments()) {
                  if (!segment.enabled() || !segment.isChordRestType()) continue;
                  for (const Element* item : segment.elist()) {
                        if (!item || !item->isChord()) continue;
                        const Chord* owner = toChord(item);
                        const auto after = owner->graceNotesAfter();
                        if (after.empty()) continue;
                        const Segment* target = m->tick2segment(owner->tick() + owner->actualTicks(), SegmentType::All);
                        if (!target) return std::vector<qreal>();
                        for (const Chord* grace : after)
                              values.insert(values.end(), {segment.x() + owner->x() + grace->x() - target->x(), grace->y()});
                        }
                  }
            return values;
            };
      const auto snapshot = [&]() {
            std::vector<qreal> values;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  const auto part = measureSnapshot(m);
                  values.insert(values.end(), part.begin(), part.end());
                  }
            return values;
            };
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = snapshot();
      std::vector<std::vector<qreal>> legacyAfterTargets;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
            legacyAfterTargets.push_back(afterTargetSnapshot(m));
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      const auto modern = snapshot();
      int count = 0;
      std::vector<std::vector<int>> afterPitches;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++count) {
            QVERIFY(count < 6);
            QVERIFY(m->ticks() == Fraction(1, 1));
            std::vector<ChordRest*> onsets;
            for (Segment& segment : m->segments()) {
                  if (segment.isChordRestType()) onsets.push_back(segment.cr(0));
                  }
            QCOMPARE(static_cast<int>(onsets.size()), 3);
            QVERIFY(onsets[0] && onsets[1] && onsets[2]);
            QVERIFY(onsets[0]->rtick() == Fraction(0, 1));
            QVERIFY(onsets[1]->rtick() == Fraction(1, 4));
            QVERIFY(onsets[2]->rtick() == Fraction(1, 2));
            ChordRest* ownerRest = onsets[count == 3 ? 2 : 1];
            QVERIFY(ownerRest->isChord());
            Chord* owner = toChord(ownerRest);
            const auto after = owner->graceNotesAfter();
            QCOMPARE(after.size(), count == 0 ? 1 : (count == 1 || count == 2 ? 4 : 2));
            QVERIFY(owner->graceNotesBefore().empty());
            const int plainPitch[] = {77, 74, 71, 76};
            const int plainTpc[] = {13, 16, 19, 18};
            const int alteredPitch[] = {78, 73, 71, 76};
            const int alteredTpc[] = {20, 9, 19, 18};
            for (int i = 0; i < after.size(); ++i) {
                  const Chord* grace = after[i];
                  QCOMPARE(grace->parent(), static_cast<Element*>(owner));
                  QCOMPARE(grace->segment(), owner->segment());
                  QVERIFY(grace->isGraceAfter());
                  QVERIFY(grace->tick() == owner->tick());
                  QCOMPARE(grace->track(), owner->track());
                  QVERIFY(grace->up() == (count != 2));
                  const int pitch = count == 0 ? 76 : count == 1 ? alteredPitch[i]
                        : count == 2 ? plainPitch[i] : i == 0 ? 74 : 76;
                  const int tpc = count == 0 ? 18 : count == 1 ? alteredTpc[i]
                        : count == 2 ? plainTpc[i] : i == 0 ? 16 : 18;
                  QCOMPARE(grace->notes().front()->pitch(), pitch);
                  QCOMPARE(grace->notes().front()->tpc(), tpc);
                  QVERIFY(qIsFinite(grace->x()) && qIsFinite(grace->y()));
                  if (i) QVERIFY(grace->x() > after[i - 1]->x());
                  if (after.size() > 1) {
                        QVERIFY(grace->beam());
                        QCOMPARE(grace->beam(), after.front()->beam());
                        }
                  }
            std::vector<int> pitches;
            for (const Chord* grace : after) {
                  pitches.push_back(grace->notes().front()->pitch());
                  pitches.push_back(grace->notes().front()->tpc());
                  }
            afterPitches.push_back(pitches);
            // Fixture target semantics only; this is not a new target adapter.
            Segment* target = m->tick2segment(owner->tick() + owner->actualTicks(), SegmentType::All);
            QVERIFY(target && target != owner->segment());
            if (count == 3)
                  QVERIFY(target->isEndBarLineType());
            else {
                  QCOMPARE(target, onsets[2]->segment());
                  QVERIFY(onsets[2]->isRest() == (count == 4));
                  if (count == 5) {
                        QVERIFY(onsets[2]->isChord());
                        QCOMPARE(toChord(onsets[2])->graceNotesBefore().size(), 2);
                        }
                  }
            GraceBeforePlan4 group;
            group.left = 123;
            QVERIFY(!buildGraceBeforePlan4(owner, group));
            QCOMPARE(group.left, qreal(123));
            SpacingPlan4 rejected;
            rejected.width = 123;
            const bool built = buildSpacingPlan4(m->system(), rejected);
            QCOMPARE(built, count < 3 && !file.contains("-natural.mscx"));
            if (built) QCOMPARE(static_cast<int>(rejected.afterGraces.size()), 1);
            else {
                  QCOMPARE(rejected.width, qreal(123));
                  QVERIFY(rejected.measures.empty() && rejected.segments.empty() && rejected.afterGraces.empty());
                  }
            if (count == 3 || count == 4) {
                  // Rhythmic spacing still acts when the full plan rejects
                  // after-graces. AG4/AG5 retain native target-relative packing,
                  // not whole-measure widths or absolute chord positions.
                  // AG6's target has before-graces with their own local adapter.
                  const auto current = afterTargetSnapshot(m);
                  QCOMPARE(static_cast<int>(current.size()), 2 * after.size());
                  QCOMPARE(static_cast<int>(current.size()), static_cast<int>(legacyAfterTargets[count].size()));
                  for (size_t i = 0; i < current.size(); ++i) {
                        QVERIFY2(qAbs(current[i] - legacyAfterTargets[count][i]) < 1e-6,
                              qPrintable(QString("AG%1 target-relative component %2: on=%3 off=%4")
                                    .arg(count + 1).arg(static_cast<int>(i))
                                    .arg(current[i], 0, 'g', 17).arg(legacyAfterTargets[count][i], 0, 'g', 17)));
                        }
                  }
            }
      QCOMPARE(count, 6);
      QVERIFY(snapshot() == modern); // All rejected queries were read-only.
      score->doLayout();
      const auto repeated = snapshot();
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      const QString output = modernBeams ? "v0.92-after-grace-studio-roundtrip.mscx"
            : "v0.92-after-grace-legacy-roundtrip.mscx";
      QVERIFY(saveScore(score, output));
      MasterScore* loaded = readCreatedScore(output);
      QVERIFY(loaded);
      QVERIFY(loaded->styleB(Sid::spacingLayout4));
      QCOMPARE(loaded->styleB(Sid::beamLayout4), modernBeams);
      loaded->doLayout();
      int loadedCount = 0;
      for (Measure* m = loaded->firstMeasure(); m; m = m->nextMeasure(), ++loadedCount) {
            QVERIFY(loadedCount < 6);
            Chord* owner = nullptr;
            for (Segment& segment : m->segments()) {
                  Element* item = segment.element(0);
                  if (item && item->isChord() && !toChord(item)->graceNotesAfter().empty()) owner = toChord(item);
                  }
            QVERIFY(owner);
            const auto after = owner->graceNotesAfter();
            QCOMPARE(after.size(), loadedCount == 0 ? 1 : (loadedCount < 3 ? 4 : 2));
            std::vector<int> pitches;
            for (const Chord* grace : after) {
                  QCOMPARE(grace->parent(), static_cast<Element*>(owner));
                  QVERIFY(grace->isGraceAfter() && grace->tick() == owner->tick());
                  QVERIFY(grace->up() == (loadedCount != 2));
                  pitches.push_back(grace->notes().front()->pitch());
                  pitches.push_back(grace->notes().front()->tpc());
                  }
            QVERIFY(pitches == afterPitches[loadedCount]);
            }
      QCOMPARE(loadedCount, 6);
      delete loaded;
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto restored = snapshot();
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::graceFitting_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<QString>("file");
      for (const QString& file : {QString("v0.82-spacing-grace-fitting.mscx"),
                                  QString("v0.90-spacing-grace-accidental-fitting.mscx")}) {
            for (bool beams : {false, true}) {
                  const QByteArray tag = (file + (beams ? "-studio" : "-legacy")).toLatin1();
                  QTest::newRow(tag.constData()) << beams << file;
                  }
            }
      }

void TestSpacing::graceFitting()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score && score->styleB(Sid::spacingLayout4));
      QCOMPARE(score->styleD(Sid::lastSystemFillLimit), qreal(0));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->setStyleValue(Sid::pageWidth, 100.0);
      score->setStyleValue(Sid::pagePrintableWidth, 98.8);
      score->doLayout();
      System* wideSystem = score->firstMeasure()->system();
      int count = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++count) {
            QCOMPARE(m->system(), wideSystem);
            QVERIFY(!m->lineBreak() && !m->pageBreak() && !m->noBreak());
            QVERIFY(m->ticks() == Fraction(1, 1));
            int onsets = 0;
            for (const Segment& s : m->segments()) {
                  if (!s.isChordRestType()) continue;
                  QVERIFY(s.rtick() == Fraction(onsets, 4));
                  const Element* item = s.element(0);
                  QVERIFY(item && item->isChord());
                  const Chord* chord = toChord(item);
                  if (onsets == 1) QCOMPARE(chord->graceNotesBefore().size(), count < 4 ? 4 : 1);
                  else QVERIFY(chord->graceNotes().empty());
                  ++onsets;
                  }
            QCOMPARE(onsets, 3);
            }
      QCOMPARE(count, 6);
      SpacingPlan4 wide;
      QString reason;
      QVERIFY2(buildSpacingPlan4(wideSystem, wide, &reason), qPrintable(reason));
      const qreal naturalWidth = wide.width;
      int previousCount = 0;
      for (qreal ratio : {qreal(1.1), qreal(.55), qreal(.35)}) {
            const qreal available = naturalWidth * ratio;
            score->setStyleValue(Sid::pagePrintableWidth, available / DPI);
            score->setStyleValue(Sid::pageWidth, available / DPI + 1.2);
            score->setStyleValue(Sid::spacingLayout4, false);
            score->doLayout();
            const auto legacy = graceLayoutSnapshot4(score);
            score->setStyleValue(Sid::spacingLayout4, true);
            score->doLayout();
            int total = 0;
            int systems = 0;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  ++total;
                  for (const Segment& s : m->segments()) {
                        if (!s.isChordRestType()) continue;
                        const Chord* chord = toChord(s.element(0));
                        if (chord->graceNotes().empty()) continue;
                        GraceBeforePlan4 group;
                        QVERIFY2(buildGraceBeforePlan4(chord, group, &reason), qPrintable(reason));
                        for (const GracePosition4& p : group.positions) QVERIFY(qAbs(p.chord->x() - p.x) < 1e-6);
                        }
                  if (m != m->system()->firstMeasure()) continue;
                  ++systems;
                  SpacingPlan4 plan;
                  QVERIFY2(buildSpacingPlan4(m->system(), plan, &reason), qPrintable(reason));
                  QVERIFY(plan.width <= available + 1e-6);
                  QVERIFY(justifySpacingPlan4(plan, available));
                  QVERIFY(qAbs(m->system()->width() - available) < 1e-6);
                  for (const SpacingMeasurePlan4& e : plan.measures) {
                        QVERIFY(qAbs(e.measure->x() - e.x) < 1e-6);
                        QVERIFY(qAbs(e.measure->width() - e.width) < 1e-6);
                        }
                  for (const SpacingSegmentPlan4& e : plan.segments) {
                        if (e.ignored) continue;
                        QVERIFY(qAbs(e.segment->measure()->x() + e.segment->x() - e.x) < 1e-6);
                        QVERIFY(qAbs(e.segment->width() - e.width) < 1e-6);
                        QCOMPARE(e.widthOffset, qreal(0));
                        }
                  }
            QCOMPARE(total, 6);
            if (ratio > 1) QCOMPARE(systems, 1);
            else QVERIFY(systems > 1);
            QVERIFY(systems >= previousCount);
            previousCount = systems;
            const auto modern = graceLayoutSnapshot4(score);
            score->doLayout();
            const auto repeated = graceLayoutSnapshot4(score);
            QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
            for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
            score->setStyleValue(Sid::spacingLayout4, false);
            score->doLayout();
            const auto restored = graceLayoutSnapshot4(score);
            QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
            for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
            }
      delete score;
      }

void TestSpacing::graceSqueeze_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<QString>("file");
      for (const QString& file : {QString("v0.82-spacing-grace-squeeze.mscx"),
                                  QString("v0.90-spacing-grace-accidental-squeeze.mscx"),
                                  QString("v0.97-spacing-after-grace-squeeze.mscx")}) {
            for (bool beams : {false, true}) {
                  const QByteArray tag = (file + (beams ? "-studio" : "-legacy")).toLatin1();
                  QTest::newRow(tag.constData()) << beams << file;
                  }
            }
      }

void TestSpacing::graceSqueeze()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score && score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->setStyleValue(Sid::pageWidth, 100.0);
      score->setStyleValue(Sid::pagePrintableWidth, 98.8);
      score->doLayout();
      Measure* measure = score->firstMeasure();
      QVERIFY(measure && !measure->nextMeasure());
      QCOMPARE(measure->userStretch(), qreal(2));
      SpacingPlan4 natural;
      QString reason;
      QVERIFY2(buildSpacingPlan4(measure->system(), natural, &reason), qPrintable(reason));
      SpacingPlanOptions4 options;
      options.minimumMeasureWidths = false;
      options.stretchReduction = 1 - .33;
      options.squeezeFactor = 1 - .2;
      SpacingPlan4 compressed;
      QVERIFY2(buildSpacingPlan4(measure->system(), compressed, &reason, options), qPrintable(reason));
      QVERIFY(compressed.width < natural.width);
      const qreal target = (natural.width + compressed.width) / 2;
      score->setStyleValue(Sid::pagePrintableWidth, target / DPI);
      score->setStyleValue(Sid::pageWidth, target / DPI + 1.2);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = graceLayoutSnapshot4(score);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      QVERIFY2(buildSpacingPlan4(measure->system(), natural, &reason), qPrintable(reason));
      QVERIFY(natural.width > target);
      SpacingPlan4 expected;
      QVERIFY2(squeezeSpacingPlan4(measure->system(), target, expected, &reason), qPrintable(reason));
      QVERIFY(justifySpacingPlan4(expected, target));
      QVERIFY(qAbs(measure->system()->width() - target) < 1e-6);
      for (const SpacingMeasurePlan4& e : expected.measures) {
            QVERIFY(qAbs(e.measure->x() - e.x) < 1e-6);
            QVERIFY(qAbs(e.measure->width() - e.width) < 1e-6);
            }
      for (const SpacingSegmentPlan4& e : expected.segments) {
            if (e.ignored) continue;
            QVERIFY2(qAbs(e.segment->measure()->x() + e.segment->x() - e.x) < 1e-6,
                  qPrintable(QString("grace squeeze segment tick=%1 type=%2 actual x=%3 expected x=%4 natural=%5 target=%6")
                        .arg(e.segment->tick().ticks()).arg(int(e.segment->segmentType()))
                        .arg(e.segment->measure()->x() + e.segment->x(), 0, 'g', 17)
                        .arg(e.x, 0, 'g', 17).arg(natural.width, 0, 'g', 17).arg(target, 0, 'g', 17)));
            QVERIFY(qAbs(e.segment->width() - e.width) < 1e-6);
            }
      Chord* main = nullptr;
      for (const Segment& s : measure->segments()) {
            if (s.isChordRestType() && s.element(0) && !toChord(s.element(0))->graceNotes().empty())
                  main = toChord(s.element(0));
            }
      QVERIFY(main);
      if (!main->graceNotesAfter().empty()) {
            GraceAfterPlan4 group;
            QVERIFY2(buildGraceAfterPlan4(main, group, &reason), qPrintable(reason));
            QCOMPARE(static_cast<int>(expected.afterGraces.size()), 1);
            for (const GraceAfterPosition4& p : group.positions)
                  QVERIFY(qAbs(main->segment()->x() + main->x() + p.chord->x() - group.target->x() - p.x) < 1e-6);
            }
      else {
            GraceBeforePlan4 group;
            QVERIFY2(buildGraceBeforePlan4(main, group, &reason), qPrintable(reason));
            for (const GracePosition4& p : group.positions) QVERIFY(qAbs(p.chord->x() - p.x) < 1e-6);
            }
      const auto modern = graceLayoutSnapshot4(score);
      SpacingPlan4 rejected;
      rejected.width = 123;
      QVERIFY(!squeezeSpacingPlan4(measure->system(), 0, rejected, &reason));
      QCOMPARE(rejected.width, qreal(123));
      QVERIFY(graceLayoutSnapshot4(score) == modern);
      score->doLayout();
      const auto repeated = graceLayoutSnapshot4(score);
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto restored = graceLayoutSnapshot4(score);
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::graceSlurSpacing_data()
      {
      graceBeforePlan_data();
      }

void TestSpacing::graceSlurSpacing()
      {
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + "v0.84-spacing-grace-slurs.mscx");
      MasterScore* plain = readScore(DIR + "v0.84-spacing-grace-slurs-plain.mscx");
      QVERIFY(score && plain);
      QVERIFY(score->styleB(Sid::spacingLayout4) && plain->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      plain->setStyleValue(Sid::beamLayout4, modernBeams);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = graceLayoutSnapshot4(score);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      plain->doLayout();
      std::vector<Slur*> slurs;
      for (const auto& entry : score->spannerMap().map()) {
            if (entry.second->isSlur()) slurs.push_back(toSlur(entry.second));
            }
      QCOMPARE(static_cast<int>(slurs.size()), 6);
      int count = 0;
      const SpacingPaddingTable4 table(score);
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++count) {
            const Segment* s = m->first(SegmentType::ChordRest);
            QVERIFY(s && s->element(0) && s->element(0)->isChord());
            Chord* main = toChord(s->element(0));
            Slur* slur = nullptr;
            for (Slur* candidate : slurs) {
                  if (candidate->endElement() == main) slur = candidate;
                  }
            QVERIFY(slur);
            QCOMPARE(slur->startElement(), static_cast<Element*>(main->graceNotesBefore().front()));
            QVERIFY(slur->tick() == main->tick() && slur->tick2() == main->tick());
            QVERIFY(spacingLocalGraceSlur4(slur, main));
            QCOMPARE(static_cast<int>(slur->nsegments()), 1);
            GraceBeforePlan4 group;
            QString reason;
            QVERIFY2(buildGraceBeforePlan4(main, group, &reason), qPrintable(reason));
            for (const GracePosition4& p : group.positions) QVERIFY(qAbs(p.chord->x() - p.x) < 1e-6);
            SpacingGraceContext4 context;
            context.mains.push_back(main);
            const Note* note = main->notes().front();
            SpacingPairPolicy4 policy;
            QVERIFY(!spacingPairPolicy4(table, note, note, note->spatium(), policy));
            QVERIFY(spacingPairPolicy4(table, note, note, note->spatium(), policy, &context));
            SpacingPlan4 expected;
            QVERIFY2(buildSpacingPlan4(m->system(), expected, &reason), qPrintable(reason));
            QVERIFY(expected.width <= m->system()->width() + 1e-6);
            if (expected.width < m->system()->width() - 1e-6)
                  QVERIFY(justifySpacingPlan4(expected, m->system()->width()));
            for (const SpacingSegmentPlan4& e : expected.segments) {
                  if (e.ignored) continue;
                  QVERIFY(qAbs(e.segment->measure()->x() + e.segment->x() - e.x) < 1e-6);
                  QVERIFY(qAbs(e.segment->width() - e.width) < 1e-6);
                  }
            }
      QCOMPARE(count, 6);
      const auto modern = graceLayoutSnapshot4(score);
      const auto withoutSlurs = graceLayoutSnapshot4(plain);
      QCOMPARE(static_cast<int>(withoutSlurs.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(withoutSlurs[i] - modern[i]) < 1e-6);
      // Handle edits and an unresolved endpoint must not inherit this allowance.
      Slur* first = slurs.front();
      Chord* main = toChord(first->endElement());
      SlurSegment* segment = first->frontSegment();
      const QPointF oldOffset = segment->ups(Grip::BEZIER1).off;
      segment->ups(Grip::BEZIER1).off = QPointF(.5, 0);
      QVERIFY(!spacingLocalGraceSlur4(first, main));
      GraceBeforePlan4 rejectedGroup;
      rejectedGroup.left = 123;
      QVERIFY(!buildGraceBeforePlan4(main, rejectedGroup));
      QCOMPARE(rejectedGroup.left, qreal(123));
      SpacingPlan4 rejected;
      rejected.width = 123;
      QVERIFY(!buildSpacingPlan4(main->measure()->system(), rejected));
      QCOMPARE(rejected.width, qreal(123));
      segment->ups(Grip::BEZIER1).off = oldOffset;
      Element* oldEnd = first->endElement();
      first->setEndElement(nullptr);
      QVERIFY(!spacingLocalGraceSlur4(first, main));
      QVERIFY(!buildGraceBeforePlan4(main, rejectedGroup));
      QCOMPARE(rejectedGroup.left, qreal(123));
      first->setEndElement(oldEnd);
      QVERIFY(graceLayoutSnapshot4(score) == modern); // Queries and rejected allowances do not publish positions.
      QVERIFY(saveScore(score, "v0.84-grace-slurs-roundtrip.mscx"));
      MasterScore* loaded = readCreatedScore("v0.84-grace-slurs-roundtrip.mscx");
      QVERIFY(loaded);
      loaded->doLayout();
      int loadedSlurs = 0;
      for (const auto& entry : loaded->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            Slur* slur = toSlur(entry.second);
            QVERIFY(slur->endElement() && slur->endElement()->isChord());
            Chord* parent = toChord(slur->endElement());
            QVERIFY(!parent->graceNotesBefore().empty());
            QCOMPARE(slur->startElement(), static_cast<Element*>(parent->graceNotesBefore().front()));
            QVERIFY(spacingLocalGraceSlur4(slur, parent));
            ++loadedSlurs;
            }
      QCOMPARE(loadedSlurs, 6);
      score->doLayout();
      const auto repeated = graceLayoutSnapshot4(score);
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto restored = graceLayoutSnapshot4(score);
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete loaded;
      delete plain;
      delete score;
      }

QTEST_MAIN(TestSpacing)
#include "tst_spacing.moc"
