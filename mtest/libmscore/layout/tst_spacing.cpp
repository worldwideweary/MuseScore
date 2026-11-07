// MuseScore — optional Studio spacing regression tests.
// Licensed under GNU GPL version 2, as published in LICENSE.GPL.
#include <QtTest/QtTest>
#include "mtest/testutils.h"
#include "libmscore/score.h"
#include "libmscore/xml.h"
#include "libmscore/undo.h"
#include "libmscore/measure.h"
#include "libmscore/segment.h"
#include "libmscore/spacinglayout4.h"
#include "libmscore/spacinggeometry4.h"
#include "libmscore/spacingcollision4.h"
#include "libmscore/spacingplan4.h"
#include "libmscore/spacinginput4.h"
#include "libmscore/layout.h"
#include "libmscore/navigate.h"
#include "libmscore/gracespacing4.h"
#include "libmscore/graceslurlayout4.h"
#include "libmscore/system.h"
#include "libmscore/shape.h"
#include "libmscore/notedot.h"
#include "libmscore/ledgerline.h"
#include "libmscore/clef.h"
#include "libmscore/breath.h"
#include "libmscore/rest.h"
#include "libmscore/tie.h"
#include <limits>
#include <array>
#include <memory>
#include "libmscore/chordrest.h"
#include "libmscore/beam.h"
#include "libmscore/barline.h"
#include "libmscore/keysig.h"
#include "libmscore/timesig.h"
#include "libmscore/beamlayout4.h"
#include "libmscore/chord.h"
#include "libmscore/note.h"
#include "libmscore/accidental.h"
#include "libmscore/pitchspelling.h"
#include "libmscore/stem.h"
#include "libmscore/slur.h"
#include "libmscore/harmony.h"
#include "libmscore/stemslash.h"
#include "libmscore/hook.h"
#include "libmscore/spannermap.h"

using namespace Ms;

#define DIR QString("libmscore/layout/")

// Standalone layout probes are not score-owned spanners. Their rendered
// segments join System's non-owning list and must leave it before deletion.
class SlurProbeRegistration4 {
      Slur& _slur;
public:
      explicit SlurProbeRegistration4(Slur& slur) : _slur(slur) {}
      SlurProbeRegistration4(const SlurProbeRegistration4&) = delete;
      SlurProbeRegistration4& operator=(const SlurProbeRegistration4&) = delete;
      ~SlurProbeRegistration4() {
            for (SpannerSegment* segment : _slur.spannerSegments()) segment->setSystem(nullptr);
            // Spanner::pushUnusedSegment already detaches pooled segments.
            }
      };

class TestSpacing : public QObject, public MTest {
      Q_OBJECT
   private slots:
      void initTestCase() { initMTest(); }
      void slurProbeLifetime4();
      void spacingInputSnapshot4_data();
      void spacingInputSnapshot4();
      void spacingSlurAttachments4();
      void preparedSpacingProduction4_data();
      void preparedSpacingProduction4();
      void spacingSlurDirections4_data();
      void spacingSlurDirections4();
      void slurDirectionPriority4();
      void slurRendererDirection4_data();
      void slurRendererDirection4();
      void breathCaesuraPlanning4_data();
      void breathCaesuraPlanning4();
      void repeatBarlinePlanning4_data();
      void repeatBarlinePlanning4();
      void repeatCollection4_data();
      void repeatCollection4();
      void signatureCollection4_data();
      void signatureCollection4();
      void pendingCourtesyTrial4();
      void clefSignaturePlanning4_data();
      void clefSignaturePlanning4();
      void smallNotation4_data();
      void smallNotation4();
      void styleRoundTrip();
      void durationPolicy();
      void springForce();
      void graceKerningBeforeSystem4();
      void graceBeforePlan_data();
      void graceBeforePlan();
      void graceBeforeLayout_data();
      void graceBeforeLayout();
      void graceNeighborPlan_data();
      void graceNeighborPlan();
      void graceAfterBarlineSystem_data();
      void graceAfterBarlineSystem();
      void graceMergedSlurs_data();
      void graceMergedSlurs();
      void graceContextCachePolicies();
      void graceMergedSlurObstacles_data();
      void graceMergedSlurObstacles();
      void graceSlurCurveFit();
      void graceMergedSlurCurves_data();
      void graceMergedSlurCurves();
      void graceMergedLaterSlurCurves4();
      void graceSlurSiblingAnchors4();
      void continuedSlurStemAnchors4();
      void beamSlurAnchorHeights4_data();
      void beamSlurAnchorHeights4();
      void beamEndSlurAnchors4();
      void spacingDecisionScopes4();
      void spacingNativeDecisions4();
      void spacingDecisionCorpus4_data();
      void spacingDecisionCorpus4();
      void measureRestPlanning4_data();
      void measureRestPlanning4();
      void systemSpacingLifecycle4_data();
      void systemSpacingLifecycle4();
      void acciaccaturaSlash4_data();
      void acciaccaturaSlash4();
      void slurOffsetUndoAdmission4();
      void mainStartSlurSpacing4_data();
      void mainStartSlurSpacing4();
      void slurAnchorSystems4_data();
      void slurAnchorSystems4();
      void slurAnchorTraversal4_data();
      void slurAnchorTraversal4();
      void slurAnchorSpacing4_data();
      void slurAnchorSpacing4();
      void slurOffsetLayout4_data();
      void slurOffsetLayout4();
      void graceMergedSlurBeamAnchors_data();
      void graceMergedSlurBeamAnchors();
      void graceMergedSystem_data();
      void graceMergedSystem();
      void graceMergedGeometry_data();
      void graceMergedGeometry();
      void graceAfterBoundaryBaseline_data();
      void graceAfterBoundaryBaseline();
      void graceAfterRestStaticSqueeze_data();
      void graceAfterRestStaticSqueeze();
      void graceAfterRest_data();
      void graceAfterRest();
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
      QVERIFY(score->styleB(Sid::scaleRythmicSpacingForSmallNotes));
      for (bool enabled : {false, true}) {
            MStyle imported;
            const QByteArray bytes = QByteArray("<Style><scaleRythmicSpacingForSmallNotes>")
                  + (enabled ? "1" : "0") + "</scaleRythmicSpacingForSmallNotes></Style>";
            XmlReader reader(bytes);
            QVERIFY(reader.readNextStartElement());
            imported.load(reader, 475);
            QCOMPARE(imported.value(Sid::scaleRythmicSpacingForSmallNotes).toBool(), enabled);
            }
      QVERIFY(!score->styleB(Sid::spacingLayout4));
      QVERIFY(score->styleB(Sid::beamLayout4));
      score->setStyleValue(Sid::spacingLayout4, true);
      score->setStyleValue(Sid::beamLayout4, false);
      score->setStyleValue(Sid::scaleRythmicSpacingForSmallNotes, false);
      QVERIFY(saveScore(score, "studio-spacing-roundtrip.mscx"));
      MasterScore* loaded = readCreatedScore("studio-spacing-roundtrip.mscx");
      QVERIFY(loaded);
      QVERIFY(loaded->styleB(Sid::spacingLayout4));
      QVERIFY(!loaded->styleB(Sid::beamLayout4));
      QVERIFY(!loaded->styleB(Sid::scaleRythmicSpacingForSmallNotes));
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
      // The native producer emits identities with each primitive, including
      // coincident primitives. No rectangle matching or owner selection occurs.
      SpacingShape4 produced;
      const Shape emitted = note.shapeWithItems4(&produced);
      SpacingShape4 captured;
      QVERIFY(captureProducedSpacingShape4(emitted, produced, captured));
      QCOMPARE(int(captured.elements.size()), 3);
      QCOMPARE(captured.elements[0].item, static_cast<const Element*>(&note));
      QCOMPARE(captured.elements[1].item, static_cast<const Element*>(dot));
      QCOMPARE(captured.elements[2].item, static_cast<const Element*>(accidental));
      const QRectF accidentalBox = accidental->bbox();
      const QPointF accidentalPos = accidental->pos();
      accidental->setbbox(note.bbox());
      accidental->setPos(QPointF());
      SpacingShape4 coincident;
      const Shape coincidentNative = note.shapeWithItems4(&coincident);
      accidental->setbbox(accidentalBox);
      accidental->setPos(accidentalPos);
      QVERIFY(captureProducedSpacingShape4(coincidentNative, coincident, captured));
      QCOMPARE(captured.elements[0].rect, captured.elements[2].rect);
      QCOMPARE(captured.elements[0].item, static_cast<const Element*>(&note));
      QCOMPARE(captured.elements[2].item, static_cast<const Element*>(accidental));
      for (int mode = 0; mode < 9; ++mode) {
            SpacingShape4 broken = produced;
            switch (mode) {
            case 0: broken.elements.pop_back(); break;
            case 1: broken.elements.push_back(broken.elements.front()); break;
            case 2: std::swap(broken.elements[0], broken.elements[1]); break;
            case 3: broken.elements.front().rect.translate(1e-12, 0); break;
            case 4: broken.elements.front().item = nullptr; break;
            case 5: broken.elements.front().identity = SpacingIdentity4::AMBIGUOUS; break;
            case 6: broken.elements.front().sharedItems.push_back(accidental); break;
            case 7: broken.elements.front().aggregateOwners.push_back(accidental); break;
            case 8: broken.elements.front().identity = SpacingIdentity4::LEGACY_SPACER; break;
            }
            QVERIFY(!captureProducedSpacingShape4(emitted, broken, captured));
            QCOMPARE(captured.elements[0].rect, captured.elements[2].rect);
            QCOMPARE(captured.elements[0].item, static_cast<const Element*>(&note));
            QCOMPARE(captured.elements[2].item, static_cast<const Element*>(accidental));
            }
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
      // Resolve again exactly as chord geometry is reconciled with its staff
      // shape. Coincident unpositioned after-grace bounds must retain the full
      // producer set, rather than becoming unclassified at this second level.
      SpacingShape4 nested = spacingResolveShape4(sharedNative, shared, true);
      for (const auto& entry : nested.elements) {
            QCOMPARE(int(entry.identity), int(SpacingIdentity4::LEGACY_SPACER));
            QVERIFY(!entry.item && entry.sharedItems.empty());
            QVERIFY(entry.aggregateOwners == shared.elements.front().aggregateOwners);
            }
      SpacingShape4 reversed = shared;
      for (auto& entry : reversed.elements) std::reverse(entry.aggregateOwners.begin(), entry.aggregateOwners.end());
      QVERIFY(spacingResolveShape4(sharedNative, reversed, true).elements.front().aggregateOwners
            == nested.elements.front().aggregateOwners);
      Shape translatedNative;
      for (const QRectF& r : sharedNative) translatedNative.add(r.translated(offset));
      const SpacingShape4 translatedNested = spacingResolveShape4(translatedNative, nested.translated(offset), true);
      QVERIFY(translatedNested.elements.front().aggregateOwners == nested.elements.front().aggregateOwners);
      for (int mode = 0; mode < 7; ++mode) {
            SpacingShape4 broken = shared;
            Shape wrongNative = sharedNative;
            switch (mode) {
            case 0: broken.elements.front().aggregateOwners.back() = broken.elements.front().aggregateOwners.front(); break;
            case 1: broken.elements.front().aggregateOwners.back() = nullptr; break;
            case 2: broken.elements.front().aggregateOwners.back() = accidental; break; // inconsistent producer sets
            case 3: broken.elements.front().item = &secondContext; break;
            case 4: broken.elements.front().sharedItems.push_back(&note); break;
            case 5: broken.elements.pop_back(); break;
            case 6: wrongNative.add(spacer.rect); break;
            }
            const SpacingShape4 rejected = spacingResolveShape4(wrongNative, broken, true);
            QVERIFY(rejected.elements.front().identity != SpacingIdentity4::LEGACY_SPACER);
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
      int producedStaffs = 0;
      bool checkedStale = false;
      const SpacingPaddingTable4 paddingTable(score);
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            const qreal measureWidth = m->width();
            for (Segment& seg : m->segments()) {
                  const qreal x = seg.x(), width = seg.width(), offset = seg.spacingWidthOffset4();
                  const std::vector<Shape> native = seg.shapes();
                  for (int staffIdx = 0; staffIdx < score->nstaves(); ++staffIdx) {
                        const bool visible = seg.visible();
                        SpacingShape4 produced, captured;
                        const Shape emitted = seg.shapeWithItems4(staffIdx, &produced);
                        QCOMPARE(emitted.size(), produced.elements.size());
                        for (size_t i = 0; i < emitted.size(); ++i)
                              QCOMPARE(static_cast<const QRectF&>(emitted[i]), produced.elements[i].rect);
                        // Native cached shapes can have omitted barline owners
                        // or later layout writes. Complete/current captures
                        // retain exact source owners; all others stay opaque.
                        const bool current = captureProducedSpacingShape4(emitted, produced, captured)
                              && captureProducedSpacingShape4(native[staffIdx], produced, captured);
                        producedStaffs += current && !captured.elements.empty();
                        const SpacingShape4 tagged = spacingStaffShape4(&seg, staffIdx);
                        QCOMPARE(seg.visible(), visible);
                        if (!checkedStale && current && seg.isChordRestType() && !native[staffIdx].empty()) {
                              for (int mode = 0; mode < 2; ++mode) {
                                    Shape& changed = seg.staffShape(staffIdx);
                                    if (mode == 0) changed.front().translate(.125 * score->spatium(), 0);
                                    else changed.add(native[staffIdx].front()); // uncovered coincident primitive
                                    const Shape stale = changed;
                                    const SpacingShape4 rejected = spacingStaffShape4(&seg, staffIdx);
                                    changed = native[staffIdx]; // restore before assertions
                                    QCOMPARE(rejected.elements.size(), stale.size());
                                    for (size_t i = 0; i < rejected.elements.size(); ++i) {
                                          QCOMPARE(rejected.elements[i].rect, static_cast<const QRectF&>(stale[i]));
                                          QVERIFY(rejected.elements[i].identity == SpacingIdentity4::UNCLASSIFIED);
                                          QVERIFY(!rejected.elements[i].item);
                                          QVERIFY(rejected.elements[i].aggregateOwners.empty() && rejected.elements[i].sharedItems.empty());
                                          }
                                    }
                              checkedStale = true;
                              }
                        QCOMPARE(static_cast<int>(tagged.elements.size()), static_cast<int>(native[staffIdx].size()));
                        for (size_t i = 0; i < tagged.elements.size(); ++i) {
                              const SpacingShapeElement4& e = tagged.elements[i];
                              if (current) {
                                    QCOMPARE(e.item, captured.elements[i].item);
                                    QVERIFY(e.identity == captured.elements[i].identity);
                                    }
                              else {
                                    QVERIFY(!e.item);
                                    QVERIFY(e.identity == SpacingIdentity4::UNCLASSIFIED);
                                    }
                              QVERIFY(e.aggregateOwners.empty() && e.sharedItems.empty());
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
      QVERIFY(producedStaffs > 0);
      QVERIFY(checkedStale);
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
                        const Note* owners[2];
                        for (int voice = 0; voice < 2; ++voice) {
                              const ChordRest* cr = first->cr(staff * VOICES + voice);
                              QVERIFY(cr && cr->isChord());
                              QCOMPARE(int(toChord(cr)->notes().size()), 1);
                              owners[voice] = toChord(cr)->notes().front();
                              }
                        QVERIFY(owners[0] != owners[1]);
                        // Native emission keeps each coincident primitive's
                        // actual owner; SHARED_ITEM belonged to the old
                        // rectangle-matching adapter. Both owners must survive
                        // raw capture and collision projection exactly once.
                        SpacingShape4 projected;
                        const SpacingPaddingTable4 table(score);
                        QVERIFY(spacingCollisionProjection4(table, shape, first->spatium(), projected));
                        QVERIFY(projected.complete());
                        const SpacingShape4* views[] = { &shape, &projected };
                        for (const SpacingShape4* view : views) {
                              int counts[2] = { 0, 0 };
                              QRectF rectangles[2];
                              for (const SpacingShapeElement4& item : view->elements) {
                                    for (int voice = 0; voice < 2; ++voice) {
                                          if (item.item != owners[voice]) continue;
                                          QVERIFY(item.identity == SpacingIdentity4::ITEM);
                                          QVERIFY(item.aggregateOwners.empty() && item.sharedItems.empty());
                                          const Note* note = owners[voice];
                                          QCOMPARE(item.rect, note->bbox().translated(note->pos()).translated(note->chord()->pos()));
                                          rectangles[voice] = item.rect;
                                          ++counts[voice];
                                          }
                                    }
                              QCOMPARE(counts[0], 1);
                              QCOMPARE(counts[1], 1);
                              QCOMPARE(rectangles[0], rectangles[1]);
                              }
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

// Policy queries before system collection must not require page coordinates.
void TestSpacing::graceKerningBeforeSystem4()
      {
      MasterScore* score = readScore(DIR + "v0.79-spacing-grace-before.mscx");
      QVERIFY(score);
      score->doLayout();
      Measure* measure = score->firstMeasure();
      QVERIFY(measure);
      Chord* main = toChord(measure->first(SegmentType::ChordRest)->cr(0));
      QVERIFY(main && !main->notes().empty());
      const SpacingPaddingTable4 table(score);
      {
      LedgerLine ledger(score);
      ledger.setParent(main);
      ledger.setTrack(main->track());
      Element* parent = measure->parent();
      measure->setParent(nullptr);
      SpacingPairPolicy4 fresh, cached;
      SpacingContextCache4 cache;
      const bool accepted = spacingGracePairPolicy4(table, main, main->notes().front(), &ledger,
            score->spatium(), fresh);
      const bool reused = spacingGracePairPolicy4(table, main, main->notes().front(), &ledger,
            score->spatium(), cached, &cache);
      measure->setParent(parent);
      QVERIFY(accepted && reused);
      QCOMPARE(int(fresh.kerning), int(SpacingKerning4::KERNING));
      QCOMPARE(int(cached.kerning), int(fresh.kerning));
      ledger.setParent(nullptr);
      }
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

// Include the rendered after-grace state for barline publication/restoration.
static std::vector<qreal> graceBarlineSnapshot4(Score* score)
      {
      auto values = graceLayoutSnapshot4(score);
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (const Segment& segment : m->segments()) {
                  const Element* item = segment.element(0);
                  if (!item || !item->isChord()) continue;
                  for (const Chord* grace : toChord(item)->graceNotesAfter()) {
                        values.insert(values.end(), {grace->x(), grace->y(),
                              grace->stem() ? grace->stem()->len() : 0});
                        if (grace->beam() && grace->beam()->primarySegment4()) {
                              const QLineF& beam = *grace->beam()->primarySegment4();
                              values.insert(values.end(), {beam.x1(), beam.y1(), beam.x2(), beam.y2()});
                              }
                        }
                  }
            }
      return values;
      }

static std::vector<qreal> graceMergedSnapshot4(Score* score)
      {
      auto values = graceLayoutSnapshot4(score);
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (const Segment& seg : m->segments()) {
                  const Element* item = seg.element(0);
                  if (!item || !item->isChord()) continue;
                  for (const Chord* grace : toChord(item)->graceNotes()) {
                        values.insert(values.end(), {grace->x(), grace->y(),
                              qreal(grace->up()), grace->stem() ? grace->stem()->len() : 0});
                        if (grace->beam() && grace->beam()->primarySegment4()) {
                              const QLineF& beam = *grace->beam()->primarySegment4();
                              values.insert(values.end(), {beam.x1(), beam.y1(), beam.x2(), beam.y2()});
                              }
                        }
                  }
            }
      return values;
      }

// Test-only full fixture capture. Production factories must use the active
// measure/system domain and its producer declarations, never pair-loop scans.
static bool captureGraceFixture160(const Chord* main, SpacingAttachmentPreparation4& owner, QString* reason)
      {
      if (!main || !main->score()) return false;
      SpacingInput4 input;
      if (!buildSpacingInput4(main->score(), input, reason)) return false;
      std::vector<Direction> stems;
      for (const auto& a : input.anchors()) stems.push_back(a.up ? Direction::UP : Direction::DOWN);
      return prepareSpacingAttachments4(std::move(input), stems, owner, reason);
      }

static bool spacingShapesEqual159(const SpacingShape4& a, const SpacingShape4& b)
      {
      if (a.elements.size() != b.elements.size()) return false;
      for (size_t i = 0; i < a.elements.size(); ++i) {
            const auto& x = a.elements[i];
            const auto& y = b.elements[i];
            if (x.rect != y.rect || x.item != y.item || x.identity != y.identity
                || x.aggregateOwners != y.aggregateOwners || x.sharedItems != y.sharedItems) return false;
            }
      return true;
      }

static bool spacingPlansEqual154(const SpacingPlan4& a, const SpacingPlan4& b)
      {
      if (a.width != b.width || a.measures.size() != b.measures.size()
          || a.segments.size() != b.segments.size() || a.afterGraces.size() != b.afterGraces.size()
          || a.mergedGraces.size() != b.mergedGraces.size()) return false;
      for (size_t i = 0; i < a.measures.size(); ++i)
            if (a.measures[i].measure != b.measures[i].measure || a.measures[i].x != b.measures[i].x
                || a.measures[i].width != b.measures[i].width) return false;
      for (size_t i = 0; i < a.segments.size(); ++i) {
            const auto& x = a.segments[i];
            const auto& y = b.segments[i];
            if (x.segment != y.segment || x.x != y.x || x.width != y.width || x.widthOffset != y.widthOffset
                || x.stretch != y.stretch || x.ignored != y.ignored) return false;
            }
      for (size_t i = 0; i < a.afterGraces.size(); ++i) {
            const auto& x = a.afterGraces[i];
            const auto& y = b.afterGraces[i];
            if (x.owner != y.owner || x.target != y.target || x.positions.size() != y.positions.size()
                || x.slurPolicy != y.slurPolicy) return false;
            for (size_t j = 0; j < x.positions.size(); ++j)
                  if (x.positions[j].chord != y.positions[j].chord || x.positions[j].x != y.positions[j].x) return false;
            }
      for (size_t i = 0; i < a.mergedGraces.size(); ++i) {
            const auto& x = a.mergedGraces[i];
            const auto& y = b.mergedGraces[i];
            if (x.owner != y.owner || x.target != y.target || x.positions.size() != y.positions.size()
                || x.slurPolicy != y.slurPolicy) return false;
            for (size_t j = 0; j < x.positions.size(); ++j)
                  if (x.positions[j].chord != y.positions[j].chord || x.positions[j].x != y.positions[j].x) return false;
            }
      return true;
      }

// Independent DOM traversal of pinned Studio computeUp/isDirectionMixture.
// Caller-provided directions let us test the rules without changing live stems.
static Direction slurDirectionOracle153(const Slur* slur, const SpacingInput4& input,
                                        const std::vector<Direction>& directions)
      {
      if (slur->slurDirection() != Direction::AUTO) return slur->slurDirection();
      const Element* a = slur->startElement();
      const Element* b = slur->endElement();
      if (!a || !b || !a->isChordRest() || !b->isChordRest()) return Direction::AUTO;
      const ChordRest* start = toChordRest(a);
      const ChordRest* end = toChordRest(b);
      const Chord* c1 = start->isChord() ? toChord(start) : nullptr;
      const Chord* c2 = end->isChord() ? toChord(end) : nullptr;
      const auto up = [&](const ChordRest* chord) {
            return directions[size_t(input.anchor(chord) - input.anchors().data())] == Direction::UP;
            };
      if (c1 && c1->beam() && c1->beam()->cross()) return Direction::UP;
      for (const Measure* m = start->measure(); m && m->tick() <= end->tick(); m = m->nextMeasure())
            if (c1 && m->hasVoices(start->staffIdx(), slur->tick(), slur->ticks() + end->ticks()))
                  return start->voice() > 0 || end->voice() > 0 ? Direction::DOWN : Direction::UP;
      const auto mixture = [&]() {
            if (c1->track() != c2->track()) return false;
            if (c2->isGrace() && up(c2) != up(c1)) return true;
            if (c1->isGraceBefore() && c2->isGraceAfter() && c1->parent() == c2->parent()) {
                  const Chord* parent = toChord(c1->parent());
                  if (parent->stem() && up(parent) != up(c1)) return true;
                  }
            for (const Segment* seg = c1->segment(); seg && seg->tick() <= c2->tick();
                 seg = seg->next1(SegmentType::ChordRest)) {
                  if ((c1->isGrace() || c2->isGraceBefore()) && seg->tick() == c2->tick()) return false;
                  const Element* item = seg->element(c1->track());
                  if (item && item->isChord() && up(toChord(item)) != up(c1)) return true;
                  }
            return false;
            };
      if (c1 && c2 && (!c1->isGrace() || c2 != c1->parent()) && mixture()) return Direction::UP;
      return up(start) ? Direction::DOWN : Direction::UP;
      }

static bool slurDirectionsMatch153(const SpacingInput4& input)
      {
      for (int mode = 0; mode < 4; ++mode) {
            std::vector<Direction> stems;
            for (const auto& a : input.anchors()) stems.push_back(
                  (mode == 0 ? a.up : mode == 1 ? true : mode == 2 ? false : (a.orderInTrack % 2) != 0)
                        ? Direction::UP : Direction::DOWN);
            std::vector<Direction> actual;
            if (!buildSpacingSlurDirections4(input, stems, actual) || actual.size() != input.slurs().size()) return false;
            for (size_t i = 0; i < actual.size(); ++i)
                  if (actual[i] != slurDirectionOracle153(input.slurs()[i].slur, input, stems)) return false;
            SpacingSlurAttachments4 sides;
            if (!buildSpacingSlurAttachments4(input, sides, &actual)) return false;
            }
      // No cached stem/slur direction can fill a pending producer input.
      std::vector<Direction> stems(input.anchors().size(), Direction::AUTO), actual;
      if (!buildSpacingSlurDirections4(input, stems, actual)) return false;
      for (size_t i = 0; i < actual.size(); ++i) {
            const auto& root = input.slurs()[i];
            Direction expected = root.requestedDirection;
            if (expected == Direction::AUTO && root.start.anchorIndex != SPACING_INPUT_NONE4
                && root.end.anchorIndex != SPACING_INPUT_NONE4
                && root.start.kind != SpacingEndpointKind4::NOTE && root.end.kind != SpacingEndpointKind4::NOTE) {
                  const auto& start = input.anchors()[root.start.anchorIndex];
                  const auto& end = input.anchors()[root.end.anchorIndex];
                  if (start.crossStaffBeam) expected = Direction::UP;
                  else if (root.multipleVoices)
                        expected = start.track % VOICES || end.track % VOICES ? Direction::DOWN : Direction::UP;
                  }
            if (actual[i] != expected) return false;
            }
      const auto saved = actual;
      stems.push_back(Direction::UP);
      if (buildSpacingSlurDirections4(input, stems, actual) || actual != saved) return false;
      stems.pop_back();
      if (!stems.empty()) {
            stems[0] = static_cast<Direction>(99);
            if (buildSpacingSlurDirections4(input, stems, actual) || actual != saved) return false;
            }
      return true;
      }

void TestSpacing::spacingInputSnapshot4_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<bool>("hasTies");
      QTest::addColumn<bool>("hasNoteSpanners");
      QTest::newRow("grace-one-system") << QString(DIR + "v0.139-spacing-two-staff-slur-offsets.mscx") << false << false;
      QTest::newRow("grace-two-systems") << QString(DIR + "v0.145-spacing-slur-anchor-systems.mscx") << false << false;
      QTest::newRow("ties-and-rests") << QString("libmscore/midi/testTieTrill.mscx") << true << false;
      QTest::newRow("note-spanners-and-rests") << QString("libmscore/spanners/glissando-cloning01-ref.mscx") << false << true;
      }

void TestSpacing::spacingInputSnapshot4()
      {
      QFETCH(QString, file);
      QFETCH(bool, hasTies);
      QFETCH(bool, hasNoteSpanners);
      MasterScore* score = readScore(file);
      QVERIFY(score);
      score->doLayout();
      {
      LayoutContext context(score);
      QCOMPARE(bool(context.spacingDirections4), score->styleB(Sid::spacingLayout4));
      }
      const auto state = [score]() {
            auto values = graceLayoutSnapshot4(score);
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
                  for (const Segment& segment : m->segments())
                        for (const Element* item : segment.elist()) {
                              if (!item || !item->isChord()) continue;
                              const Chord* main = toChord(item);
                              QVector<const Chord*> chords {main};
                              for (const Chord* grace : main->graceNotes()) chords.push_back(grace);
                              for (const Chord* chord : chords) {
                                    values.insert(values.end(), {chord->x(), chord->y(), qreal(chord->up()),
                                          chord->stem() ? chord->stem()->len() : 0});
                                    if (chord->beam() && chord->beam()->primarySegment4()) {
                                          const QLineF& beam = *chord->beam()->primarySegment4();
                                          values.insert(values.end(), {beam.x1(), beam.y1(), beam.x2(), beam.y2()});
                                          }
                                    }
                              }
            for (const auto& entry : score->spannerMap().map()) {
                  if (!entry.second->isSlur()) continue;
                  for (const SpannerSegment* item : entry.second->spannerSegments()) {
                        const SlurSegment* segment = toSlurSegment(item);
                        values.insert(values.end(), {segment->x(), segment->y()});
                        for (Grip grip : {Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END}) {
                              const QPointF point = segment->ups(grip).pos();
                              values.insert(values.end(), {point.x(), point.y()});
                              }
                        for (const QRectF& rect : segment->shape())
                              values.insert(values.end(), {rect.x(), rect.y(), rect.width(), rect.height()});
                        }
                  }
            return values;
            };
      const auto before = state();
      const bool spacingStyle = score->styleB(Sid::spacingLayout4);
      const bool beamStyle = score->styleB(Sid::beamLayout4);
      const int history = score->undoStack()->getCurIdx();
      const auto selection = score->selection().elements();
      const auto selectionState = score->selection().state();
      const auto updateMode = score->cmdState().updateMode();
      const quint64 generation = score->cmdState().generation4();
      QString reason;
      SpacingInput4 input;
      QVERIFY2(buildSpacingInput4(score, input, &reason), qPrintable(reason));
      QCOMPARE(input.score(), static_cast<const Score*>(score));
      QVERIFY(!input.anchors().empty());
      // Independent oracle: actual editor navigation, rather than another
      // copy of the collector's segment/grace flattening algorithm.
      std::unordered_map<int, const ChordRest*> next;
      std::unordered_map<int, size_t> ordinal;
      for (const auto& anchor : input.anchors()) {
            if (!ordinal.count(anchor.track)) next[anchor.track] = anchor.item;
            QCOMPARE(anchor.item, next[anchor.track]);
            QVERIFY(anchor.orderInTrack == ordinal[anchor.track]++);
            next[anchor.track] = nextChordRest(const_cast<ChordRest*>(anchor.item));
            QCOMPARE(input.anchor(anchor.item), &anchor);
            std::vector<const Slur*> expectedRoots;
            for (const auto& entry : score->spannerMap().map()) {
                  if (!entry.second->isSlur()) continue;
                  const Slur* slur = toSlur(entry.second);
                  if (slur->startElement() == anchor.item || slur->endElement() == anchor.item
                      || (slur->track() == anchor.track && slur->tick() == anchor.tick)
                      || (slur->track2() == anchor.track && slur->tick2() == anchor.tick)) expectedRoots.push_back(slur);
                  }
            std::vector<const Slur*> indexedRoots;
            for (size_t index : anchor.relatedSlurs) indexedRoots.push_back(input.slurs()[index].slur);
            QVERIFY(indexedRoots == expectedRoots); // Preserve identity/tick aliases and root order.
            }
      for (const auto& entry : next) QVERIFY(!entry.second);
      bool foundTie = false;
      bool foundNoteSpanner = false;
      for (const auto& note : input.notes()) {
            QCOMPARE(input.note(note.note), &note);
            QVERIFY(note.anchorIndex < input.anchors().size());
            QCOMPARE(static_cast<const ChordRest*>(note.note->chord()), input.anchors()[note.anchorIndex].item);
            QCOMPARE(note.tieFor, static_cast<const Tie*>(note.note->tieFor()));
            QCOMPARE(note.tieBack, static_cast<const Tie*>(note.note->tieBack()));
            foundTie = foundTie || note.tieFor || note.tieBack;
            foundNoteSpanner = foundNoteSpanner || !note.spannersFor.empty() || !note.spannersBack.empty();
            }
      QVERIFY(!hasTies || foundTie);
      QVERIFY(!hasNoteSpanners || foundNoteSpanner);
      for (const auto& slur : input.slurs()) {
            QCOMPARE(input.slur(slur.slur), &slur);
            QCOMPARE(slur.start.element, static_cast<const Element*>(slur.slur->startElement()));
            QCOMPARE(slur.end.element, static_cast<const Element*>(slur.slur->endElement()));
            if (slur.start.anchorIndex != SPACING_INPUT_NONE4) {
                  const auto& indices = input.anchors()[slur.start.anchorIndex].startingSlurs;
                  QVERIFY(std::any_of(indices.begin(), indices.end(), [&](size_t i) { return input.slurs()[i].slur == slur.slur; }));
                  }
            }
      QString difference;
      const auto queriesMatch = [score, &difference](const SpacingInput4& prepared) {
            difference = "Prepared Studio slur directions differ from DOM source-rule oracle";
            if (!slurDirectionsMatch153(prepared)) return false;
            // Independent root scan: expected membership uses exact captured
            // element identity, never the compatibility tick-alias index.
            for (int mode = 0; mode < 3; ++mode) {
                  std::vector<Direction> directions;
                  for (const auto& root : prepared.slurs()) directions.push_back(
                        root.requestedDirection != Direction::AUTO ? root.requestedDirection
                        : mode == 0 ? Direction::AUTO : mode == 1 ? Direction::UP : Direction::DOWN);
                  SpacingSlurAttachments4 flags;
                  if (!buildSpacingSlurAttachments4(prepared, flags, mode ? &directions : nullptr)) {
                        difference = "Failed to prepare slur-side flags";
                        return false;
                        }
                  if (flags.anchors.size() != prepared.anchors().size()) return false;
                  size_t missingStarts = 0, missingEnds = 0;
                  for (size_t i = 0; i < prepared.slurs().size(); ++i) {
                        const auto& root = prepared.slurs()[i];
                        missingStarts += root.start.anchorIndex == SPACING_INPUT_NONE4;
                        missingEnds += root.end.anchorIndex == SPACING_INPUT_NONE4;
                        }
                  bool complete = !missingStarts && !missingEnds;
                  if (flags.unmappedStarts != missingStarts || flags.unmappedEnds != missingEnds) return false;
                  for (size_t a = 0; a < prepared.anchors().size(); ++a) {
                        SpacingSlurSides4 expected;
                        const ChordRest* chord = prepared.anchors()[a].item;
                        for (size_t i = 0; i < prepared.slurs().size(); ++i) {
                              const auto& root = prepared.slurs()[i];
                              const auto owner = [](const Element* item) -> const Element* {
                                    return item && item->isNote() ? toNote(item)->chord() : item;
                                    };
                              const bool start = owner(root.start.element) == chord;
                              const bool end = owner(root.end.element) == chord;
                              if (directions[i] == Direction::AUTO) {
                                    expected.pendingStarts += start;
                                    expected.pendingEnds += end;
                                    }
                              else if (directions[i] == Direction::UP) {
                                    expected.startUp = expected.startUp || start;
                                    expected.endUp = expected.endUp || end;
                                    }
                              else {
                                    expected.startDown = expected.startDown || start;
                                    expected.endDown = expected.endDown || end;
                                    }
                              }
                        const auto& actual = flags.anchors[a];
                        difference = "General slur-side flags differ from exact root scan";
                        if (actual.startUp != expected.startUp || actual.startDown != expected.startDown
                            || actual.endUp != expected.endUp || actual.endDown != expected.endDown
                            || actual.pendingStarts != expected.pendingStarts || actual.pendingEnds != expected.pendingEnds) return false;
                        for (bool up : {false, true}) {
                              const unsigned blocked = up ? unsigned(expected.startDown || expected.endDown) * 2
                                    : unsigned(expected.startUp || expected.endUp);
                              if (actual.blockedKerning(up) != blocked) return false;
                              }
                        complete = complete && expected.complete();
                        }
                  if (flags.complete() != complete) return false;
                  }
            SpacingGraceContext4 context;
            for (const auto& anchor : prepared.anchors()) {
                  if (anchor.kind != SpacingMemberKind4::MAIN || !anchor.main) continue;
                  const Chord* main = anchor.main;
                  if (!main->graceNotesBefore().empty()) context.mains.push_back(main);
                  if (!main->graceNotesAfter().empty()) {
                        context.afterMains.push_back(main);
                        const ChordRest* next = nextChordRest(const_cast<Chord*>(main), true);
                        if (next && next->isChord() && !toChord(next)->graceNotesBefore().empty())
                              context.mergedPairs.push_back({main, toChord(next)});
                        }
                  }
            SpacingInput4 ownedInput = prepared;
            std::vector<Direction> stems;
            for (const auto& a : prepared.anchors()) stems.push_back(a.up ? Direction::UP : Direction::DOWN);
            SpacingAttachmentPreparation4 owner;
            difference = "Failed to compose caller-owned attachment preparation";
            if (!prepareSpacingAttachments4(std::move(ownedInput), stems, owner)
                || ownedInput.score() || !ownedInput.anchors().empty() || !ownedInput.slurs().empty()) return false;
            std::vector<Direction> expectedDirections;
            if (!buildSpacingSlurDirections4(prepared, stems, expectedDirections)
                || owner.slurDirections() != expectedDirections || owner.anchorDirections() != stems) return false;
            for (const auto& a : owner.input().anchors())
                  if (owner.sides(a.item) != &owner.attachments().anchors[size_t(&a - owner.input().anchors().data())]) return false;
            for (size_t i = 0; i < owner.input().slurs().size(); ++i)
                  if (owner.direction(owner.input().slurs()[i].slur) != &owner.slurDirections()[i]) return false;
            if (owner.sides(nullptr) || owner.direction(nullptr)) return false;
            // Identity-based capture is an explicit post-producer operation.
            // Reverse declaration order to ensure it is not snapshot-index based.
            std::vector<SpacingPreparedAnchor4> declarations;
            for (const auto& a : prepared.anchors()) declarations.emplace_back(a.item, a.up ? Direction::UP : Direction::DOWN);
            std::reverse(declarations.begin(), declarations.end());
            SpacingAttachmentPreparation4 captured;
            difference = "Post-producer attachment capture differs from composed kernels";
            if (!captureSpacingAttachments4(score, declarations, captured)
                || captured.anchorDirections() != owner.anchorDirections()
                || captured.slurDirections() != owner.slurDirections() || captured.complete() != owner.complete()) return false;
            const auto* saved = captured.sides(prepared.anchors().front().item);
            const auto preserved = [&]() {
                  return captured.sides(prepared.anchors().front().item) == saved
                        && captured.anchorDirections() == owner.anchorDirections() && captured.slurDirections() == owner.slurDirections();
                  };
            auto bad = declarations;
            bad.push_back(bad.front());
            if (captureSpacingAttachments4(score, bad, captured) || !preserved()) return false;
            bad = declarations;
            bad[0].direction = static_cast<Direction>(99);
            if (captureSpacingAttachments4(score, bad, captured) || !preserved()) return false;
            bad[0] = SpacingPreparedAnchor4(nullptr, Direction::UP);
            if (captureSpacingAttachments4(score, bad, captured) || !preserved()) return false;
            if (captureSpacingAttachments4(nullptr, declarations, captured) || !preserved()) return false;
            {
            Chord foreign(nullptr);
            bad[0] = SpacingPreparedAnchor4(&foreign, Direction::UP);
            if (captureSpacingAttachments4(score, bad, captured) || !preserved()) return false;
            }
            SpacingAttachmentPreparation4 omitted;
            if (!captureSpacingAttachments4(score, {}, omitted)) return false;
            for (Direction d : omitted.anchorDirections()) if (d != Direction::AUTO) return false;
            // Subsequent planner/cache tests borrow the factory-created owner.
            owner = std::move(captured);

            // Failed replacement preserves both the existing owner and source.
            ownedInput = prepared;
            const auto* first = owner.sides(prepared.anchors().front().item);
            auto invalid = stems;
            invalid.push_back(Direction::AUTO);
            if (prepareSpacingAttachments4(std::move(ownedInput), invalid, owner)
                || ownedInput.score() != prepared.score() || ownedInput.anchors().size() != prepared.anchors().size()
                || owner.sides(prepared.anchors().front().item) != first) return false;
            ownedInput = SpacingInput4();
            // A moved-from owner must not appear complete or retain score data.
            SpacingAttachmentPreparation4 moved(std::move(owner));
            if (owner.input().score() || owner.complete() || owner.sides(prepared.anchors().front().item)) return false;
            owner = std::move(moved);
            if (moved.input().score() || moved.complete() || moved.direction(nullptr)) return false;
            SpacingPaddingTable4 table(score);
            SpacingContextCache4 ownedQuery(owner);
            if (ownedQuery.input() != &owner.input() || ownedQuery.preparation() != &owner) return false;
            SpacingContextCache4 oldQuery;
            SpacingContextCache4 indexedQuery(&prepared);
            for (const auto& anchor : prepared.anchors()) {
                  if (!anchor.item->isChord()) continue;
                  const Chord* chord = toChord(anchor.item);
                  difference = QString("Attachment query track %1 tick %2 member %3")
                        .arg(anchor.track).arg(anchor.tick.ticks()).arg(int(anchor.memberIndex));
                  if (oldQuery.forwardSlurKerning(chord) != indexedQuery.forwardSlurKerning(chord)
                      || oldQuery.forwardSlurKerning(chord) != ownedQuery.forwardSlurKerning(chord)) return false;
                  for (const Note* note : chord->notes()) {
                        SpacingPairPolicy4 oldPolicy, indexedPolicy, ownedPolicy;
                        oldPolicy.padding = indexedPolicy.padding = ownedPolicy.padding = -123;
                        const bool oldAccepted = spacingPairPolicy4(table, note, note, chord->spatium(), oldPolicy, &context, &oldQuery);
                        const bool indexedAccepted = spacingPairPolicy4(table, note, note, chord->spatium(), indexedPolicy, &context, &indexedQuery);
                        const bool ownedAccepted = spacingPairPolicy4(table, note, note, chord->spatium(), ownedPolicy, &context, &ownedQuery);
                        if (oldAccepted != ownedAccepted || oldPolicy.padding != ownedPolicy.padding
                            || oldPolicy.verticalClearance != ownedPolicy.verticalClearance || oldPolicy.kerning != ownedPolicy.kerning) return false;
                        if (oldAccepted != indexedAccepted || oldPolicy.padding != indexedPolicy.padding
                            || oldPolicy.verticalClearance != indexedPolicy.verticalClearance
                            || oldPolicy.kerning != indexedPolicy.kerning) return false;
                        }
                  }
            for (System* system : score->systems()) {
                  if (system->vbox()) continue;
                  SpacingAttachmentPreparation4 systemOwner;
                  difference = "Failed to capture finalized-system musical inputs";
                  if (!captureSystemSpacingAttachments4(system, {}, systemOwner)) return false;
                  std::vector<const ChordRest*> expectedAnchors;
                  for (const auto& a : prepared.anchors())
                        if (a.measure->system() == system) expectedAnchors.push_back(a.item);
                  if (systemOwner.input().anchors().size() != expectedAnchors.size()) return false;
                  {
                  SpacingContextCache4 indexQuery(systemOwner);
                  for (size_t i = 0; i < expectedAnchors.size(); ++i) {
                        const auto& a = systemOwner.input().anchors()[i];
                        if (a.item != expectedAnchors[i]) return false;
                        const auto* global = prepared.anchor(a.item);
                        if (!global || a.track != global->track || a.tick != global->tick || a.kind != global->kind
                            || a.memberIndex != global->memberIndex || a.main != global->main) return false;
                        std::vector<const Slur*> scopedRoots, allRoots;
                        for (size_t index : a.relatedSlurs) scopedRoots.push_back(systemOwner.input().slurs()[index].slur);
                        for (size_t index : global->relatedSlurs) allRoots.push_back(prepared.slurs()[index].slur);
                        difference = "System interval capture changed local root identity or native map order";
                        if (scopedRoots != allRoots) return false;
                        if (a.item->isChord()) {
                              const Chord* chord = toChord(a.item);
                              if (oldQuery.forwardSlurKerning(chord) != indexQuery.forwardSlurKerning(chord)) return false;
                              for (const Note* note : chord->notes()) {
                                    SpacingPairPolicy4 oldPolicy, localPolicy;
                                    oldPolicy.padding = localPolicy.padding = -123;
                                    const bool oldOK = spacingPairPolicy4(table, note, note, chord->spatium(), oldPolicy, &context, &oldQuery);
                                    const bool localOK = spacingPairPolicy4(table, note, note, chord->spatium(), localPolicy, &context, &indexQuery);
                                    if (oldOK != localOK || oldPolicy.padding != localPolicy.padding
                                        || oldPolicy.kerning != localPolicy.kerning || oldPolicy.verticalClearance != localPolicy.verticalClearance) return false;
                                    }
                              }
                        }
                  }
                  // Omitted readiness stays pending, never certified by capture.
                  for (Direction d : systemOwner.anchorDirections()) if (d != Direction::AUTO) return false;
                  // Existing geometry is not enough to certify readiness. Only
                  // completed native measure production supplies the tokens.
                  SpacingDirectionReadiness4 readiness;
                  if (!readiness.declarations(system).empty()) return false;
                  const Measure* firstMeasure = system->firstMeasure();
                  readiness.produced(firstMeasure);
                  const auto partial = readiness.declarations(system);
                  std::vector<const ChordRest*> partialExpected;
                  for (const auto& a : prepared.anchors())
                        if (a.measure == firstMeasure) partialExpected.push_back(a.item);
                  if (partial.size() != partialExpected.size()) return false;
                  for (size_t i = 0; i < partial.size(); ++i)
                        if (partial[i].item != partialExpected[i] || partial[i].direction == Direction::AUTO) return false;
                  SpacingAttachmentPreparation4 partialOwner;
                  if (!captureSystemSpacingAttachments4(system, partial, partialOwner)) return false;
                  for (size_t i = 0; i < partialOwner.input().anchors().size(); ++i) {
                        const auto& a = partialOwner.input().anchors()[i];
                        const Direction expected = a.measure == firstMeasure ? (a.up ? Direction::UP : Direction::DOWN) : Direction::AUTO;
                        if (partialOwner.anchorDirections()[i] != expected) return false;
                        }
                  readiness.invalidate(firstMeasure);
                  if (!readiness.declarations(system).empty()) return false;
                  for (const MeasureBase* mb : system->measures())
                        if (mb->isMeasure()) readiness.produced(toMeasure(mb));
                  const auto allDirections = readiness.declarations(system);
                  if (allDirections.size() != expectedAnchors.size()) return false;
                  for (size_t i = 0; i < allDirections.size(); ++i)
                        if (allDirections[i].item != expectedAnchors[i]) return false;
                  SpacingAttachmentPreparation4 readyOwner;
                  if (!captureSystemSpacingAttachments4(system, allDirections, readyOwner)) return false;
                  for (size_t i = 0; i < readyOwner.input().anchors().size(); ++i) {
                        const auto& a = readyOwner.input().anchors()[i];
                        if (readyOwner.anchorDirections()[i] != (a.up ? Direction::UP : Direction::DOWN)) return false;
                        }
                  for (size_t i = 0; i < readyOwner.input().slurs().size(); ++i) {
                        const auto& root = readyOwner.input().slurs()[i];
                        if (root.start.anchorIndex != SPACING_INPUT_NONE4 && root.end.anchorIndex != SPACING_INPUT_NONE4) {
                              const Direction* global = owner.direction(root.slur);
                              if (!global || *global != readyOwner.slurDirections()[i]) return false;
                              }
                        }
                  SpacingDirectionReadiness4 freshPass;
                  if (!freshPass.declarations(system).empty()) return false;
                  // Bind the positively certified owner for the plan/fitting
                  // parity checks below, just as production now does.
                  systemOwner = std::move(readyOwner);
                  const auto* savedLocal = expectedAnchors.empty() ? nullptr : systemOwner.sides(expectedAnchors.front());
                  if (captureSystemSpacingAttachments4(nullptr, {}, systemOwner)
                      || (!expectedAnchors.empty() && systemOwner.sides(expectedAnchors.front()) != savedLocal)) return false;
                  for (const auto& outside : prepared.anchors()) {
                        if (outside.measure->system() == system) continue;
                        if (captureSystemSpacingAttachments4(system, {SpacingPreparedAnchor4(outside.item, Direction::UP)}, systemOwner)
                            || (!expectedAnchors.empty() && systemOwner.sides(expectedAnchors.front()) != savedLocal)) return false;
                        break;
                        }
                  SpacingContextCache4 systemQuery(systemOwner);
                  difference = "Nested grace query changed packing, projection or admission";
                  for (const auto& a : systemOwner.input().anchors()) {
                        if (a.kind != SpacingMemberKind4::MAIN || !a.main) continue;
                        const Chord* main = a.main;
                        if (!main->graceNotesBefore().empty()) {
                              GraceBeforePlan4 oldBefore, sharedBefore;
                              oldBefore.left = sharedBefore.left = -159;
                              const bool oldOK = buildGraceBeforePlan4(main, oldBefore);
                              const bool sharedOK = buildGraceBeforePlan4(main, sharedBefore, nullptr, &systemQuery);
                              if (oldOK != sharedOK || oldBefore.left != sharedBefore.left
                                  || oldBefore.positions.size() != sharedBefore.positions.size()) return false;
                              for (size_t i = 0; i < oldBefore.positions.size(); ++i)
                                    if (oldBefore.positions[i].chord != sharedBefore.positions[i].chord
                                        || oldBefore.positions[i].x != sharedBefore.positions[i].x) return false;
                              }
                        if (!main->graceNotesAfter().empty()) {
                              GraceAfterGeometry4 oldAfter, sharedAfter;
                              oldAfter.plan.left = sharedAfter.plan.left = -159;
                              const bool oldAfterOK = buildGraceAfterGeometry4(main, oldAfter);
                              const bool sharedAfterOK = buildGraceAfterGeometry4(main, sharedAfter, nullptr, &systemQuery);
                              SpacingPlan4 oldPlan, sharedPlan;
                              oldPlan.width = sharedPlan.width = 0;
                              oldPlan.afterGraces.push_back(oldAfter.plan);
                              sharedPlan.afterGraces.push_back(sharedAfter.plan);
                              if (oldAfterOK != sharedAfterOK || !spacingPlansEqual154(oldPlan, sharedPlan)
                                  || oldAfter.plan.left != sharedAfter.plan.left || oldAfter.plan.right != sharedAfter.plan.right
                                  || !spacingShapesEqual159(oldAfter.source, sharedAfter.source)
                                  || !spacingShapesEqual159(oldAfter.target, sharedAfter.target)) return false;
                              GraceMergedGeometry4 oldMerged, sharedMerged;
                              oldMerged.plan.left = sharedMerged.plan.left = -159;
                              const bool oldMergedOK = buildGraceMergedGeometry4(main, oldMerged);
                              const bool sharedMergedOK = buildGraceMergedGeometry4(main, sharedMerged, nullptr, &systemQuery);
                              oldPlan.afterGraces.clear();
                              sharedPlan.afterGraces.clear();
                              oldPlan.mergedGraces.push_back(oldMerged.plan);
                              sharedPlan.mergedGraces.push_back(sharedMerged.plan);
                              if (oldMergedOK != sharedMergedOK || !spacingPlansEqual154(oldPlan, sharedPlan)
                                  || oldMerged.plan.left != sharedMerged.plan.left || oldMerged.plan.right != sharedMerged.plan.right
                                  || !spacingShapesEqual159(oldMerged.source, sharedMerged.source)
                                  || !spacingShapesEqual159(oldMerged.target, sharedMerged.target)) return false;
                              }
                        }
                  SpacingPlan4 oldPlan, indexedPlan, ownedPlan, systemPlan;
                  oldPlan.width = indexedPlan.width = ownedPlan.width = systemPlan.width = -456;
                  const bool oldAccepted = buildSpacingPlan4(system, oldPlan);
                  const bool indexedAccepted = buildSpacingPlan4(system, indexedPlan, nullptr, SpacingPlanOptions4(), nullptr, &prepared);
                  const bool ownedAccepted = buildSpacingPlan4(system, ownedPlan, owner);
                  const bool systemAccepted = buildSpacingPlan4(system, systemPlan, systemOwner);
                  difference = "System-scoped preparation changed complete natural plan or admission";
                  if (oldAccepted != systemAccepted || !spacingPlansEqual154(oldPlan, systemPlan)) return false;
                  difference = "Owned preparation changed complete natural plan or admission";
                  if (oldAccepted != ownedAccepted || !spacingPlansEqual154(oldPlan, ownedPlan)) return false;
                  if (oldAccepted) {
                        std::vector<GraceMergedGeometry4> mismatched;
                        for (const auto& a : systemOwner.input().anchors()) {
                              if (a.kind != SpacingMemberKind4::MAIN || !a.main || a.main->graceNotesAfter().empty()) continue;
                              GraceMergedGeometry4 handoff;
                              if (!buildGraceMergedGeometry4(a.main, handoff)) continue;
                              handoff.plan.slurPolicy = SpacingSlurPolicy4::PREPARED;
                              mismatched.push_back(handoff);
                              }
                        if (!mismatched.empty()) {
                              SpacingPlan4 rebuilt;
                              difference = "Mismatched packing-policy provenance was reused";
                              if (!buildSpacingPlan4(system, rebuilt, nullptr, SpacingPlanOptions4(), &mismatched)
                                  || !spacingPlansEqual154(oldPlan, rebuilt)) return false;
                              }
                        for (qreal ratio : {qreal(1), qreal(0.95)}) {
                              SpacingPlan4 oldFit, ownedFit, systemFit;
                              oldFit.width = ownedFit.width = systemFit.width = -987;
                              const bool oldFitted = squeezeSpacingPlan4(system, oldPlan.width * ratio, oldFit);
                              const bool ownedFitted = squeezeSpacingPlan4(system, oldPlan.width * ratio, ownedFit, owner);
                              const bool systemFitted = squeezeSpacingPlan4(system, oldPlan.width * ratio, systemFit, systemOwner);
                              difference = "System-scoped preparation changed fitting outcome or complete plan";
                              if (oldFitted != systemFitted || !spacingPlansEqual154(oldFit, systemFit)) return false;
                              difference = "Owned preparation changed fitting outcome or complete plan";
                              if (oldFitted != ownedFitted || !spacingPlansEqual154(oldFit, ownedFit)) return false;
                              }
                        }
                  difference = "Prepared inputs changed system plan admission or width";
                  if (oldAccepted != indexedAccepted || oldPlan.width != indexedPlan.width) return false;
                  if (!oldAccepted) continue;
                  difference = "Prepared inputs changed measure or segment placement";
                  if (oldPlan.measures.size() != indexedPlan.measures.size()
                      || oldPlan.segments.size() != indexedPlan.segments.size()) return false;
                  for (size_t i = 0; i < oldPlan.measures.size(); ++i) {
                        const auto& old = oldPlan.measures[i];
                        const auto& indexed = indexedPlan.measures[i];
                        if (old.measure != indexed.measure || old.x != indexed.x || old.width != indexed.width) return false;
                        }
                  for (size_t i = 0; i < oldPlan.segments.size(); ++i) {
                        const auto& old = oldPlan.segments[i];
                        const auto& indexed = indexedPlan.segments[i];
                        if (old.segment != indexed.segment || old.x != indexed.x || old.width != indexed.width
                            || old.widthOffset != indexed.widthOffset || old.stretch != indexed.stretch || old.ignored != indexed.ignored) return false;
                        }
                  difference = "Prepared inputs changed planned grace positions";
                  if (oldPlan.afterGraces.size() != indexedPlan.afterGraces.size()
                      || oldPlan.mergedGraces.size() != indexedPlan.mergedGraces.size()) return false;
                  for (size_t i = 0; i < oldPlan.afterGraces.size(); ++i) {
                        const auto& old = oldPlan.afterGraces[i];
                        const auto& indexed = indexedPlan.afterGraces[i];
                        if (old.owner != indexed.owner || old.target != indexed.target || old.positions.size() != indexed.positions.size()) return false;
                        for (size_t j = 0; j < old.positions.size(); ++j)
                              if (old.positions[j].chord != indexed.positions[j].chord || old.positions[j].x != indexed.positions[j].x) return false;
                        }
                  for (size_t i = 0; i < oldPlan.mergedGraces.size(); ++i) {
                        const auto& old = oldPlan.mergedGraces[i];
                        const auto& indexed = indexedPlan.mergedGraces[i];
                        if (old.owner != indexed.owner || old.target != indexed.target || old.positions.size() != indexed.positions.size()) return false;
                        for (size_t j = 0; j < old.positions.size(); ++j)
                              if (old.positions[j].chord != indexed.positions[j].chord || old.positions[j].x != indexed.positions[j].x) return false;
                        }
                  }
            difference.clear();
            return true;
            };
      QVERIFY2(queriesMatch(input), qPrintable(difference));
      {
      SpacingInput4 foreign;
      SpacingPlan4 untouched;
      untouched.width = -789;
      QVERIFY(!buildSpacingPlan4(score->firstMeasure()->system(), untouched, &reason, SpacingPlanOptions4(), nullptr, &foreign));
      QCOMPARE(untouched.width, qreal(-789));
      SpacingAttachmentPreparation4 missing;
      QVERIFY(!buildSpacingPlan4(score->firstMeasure()->system(), untouched, missing));
      QVERIFY(!squeezeSpacingPlan4(score->firstMeasure()->system(), 100, untouched, missing));
      QCOMPARE(untouched.width, qreal(-789));
      }
      {
      SpacingInput4 unchanged = input;
      const auto* original = unchanged.anchor(input.anchors().front().item);
      SpacingInputScope4 invalid;
      invalid.measures = {score->firstMeasure(), score->firstMeasure()};
      QVERIFY(!buildSpacingInput4(score, invalid, unchanged, &reason));
      QCOMPARE(unchanged.anchor(input.anchors().front().item), original);
      if (!input.slurs().empty()) {
            invalid.measures = {score->firstMeasure()};
            invalid.slurs = {input.slurs().front().slur, input.slurs().front().slur};
            QVERIFY(!buildSpacingInput4(score, invalid, unchanged, &reason));
            QCOMPARE(unchanged.anchor(input.anchors().front().item), original);
            }
      }
      QVERIFY(state() == before);
      QCOMPARE(score->undoStack()->getCurIdx(), history);
      QVERIFY(score->selection().elements() == selection && score->selection().state() == selectionState);
      QVERIFY(score->cmdState().updateMode() == updateMode && score->cmdState().generation4() == generation);
      QCOMPARE(score->styleB(Sid::spacingLayout4), spacingStyle);
      QCOMPARE(score->styleB(Sid::beamLayout4), beamStyle);
      const size_t anchors = input.anchors().size();
      QVERIFY(!buildSpacingInput4(nullptr, input, &reason));
      QCOMPARE(input.score(), static_cast<const Score*>(score));
      QVERIFY(input.anchors().size() == anchors); // Failed build preserves prior output.
      input = SpacingInput4(); // End the borrowed view before a structural edit.
      {
      // Keep native insertion keys while changing current ticks. The ordered
      // interval query must still follow actual map order after invalidation.
      Slur first(score), second(score);
      first.setTrack(0);
      first.setTick(score->firstMeasure()->tick());
      first.setTicks(Fraction(1, 4));
      second.setTrack(0);
      second.setTick(score->lastMeasure()->tick());
      second.setTicks(Fraction(1, 4));
      score->addSpanner(&first);
      score->addSpanner(&second);
      const int stop = score->lastMeasure()->endTick().ticks() + Fraction(1, 2).ticks();
      const auto orderedMatches = [&]() {
            const auto actual = score->spannerMap().findOverlappingInMapOrder(0, stop);
            std::vector<Spanner*> expected;
            for (const auto& entry : score->spannerMap().map())
                  if (entry.second->tick().ticks() <= stop && entry.second->tick2().ticks() >= 0)
                        expected.push_back(entry.second);
            return actual == expected;
            };
      const bool initialOrder = orderedMatches();
      first.setTick(score->lastMeasure()->endTick());
      const bool movedOrder = orderedMatches();
      score->removeSpanner(&second);
      const bool removedOrder = orderedMatches();
      score->removeSpanner(&first);
      QVERIFY(initialOrder && movedOrder && removedOrder);
      }
      {
      Slur unresolved(score);
      unresolved.setTrack(0);
      unresolved.setTrack2(0);
      unresolved.setTick(score->firstMeasure()->tick());
      unresolved.setTick2(score->lastMeasure()->tick());
      score->addSpanner(&unresolved);
      const bool collected = buildSpacingInput4(score, input, &reason);
      const SpacingSlurInput4* captured = input.slur(&unresolved);
      const bool stillMissing = captured && captured->start.kind == SpacingEndpointKind4::MISSING
            && captured->end.kind == SpacingEndpointKind4::MISSING
            && !unresolved.startElement() && !unresolved.endElement();
      bool unknownRejected = true;
      if (collected) {
            const Chord* alias = nullptr;
            std::vector<Direction> stems;
            for (const auto& a : input.anchors()) {
                  stems.push_back(a.up ? Direction::UP : Direction::DOWN);
                  if (a.track == unresolved.track() && a.tick == unresolved.tick() && a.item->isChord()) alias = toChord(a.item);
                  }
            SpacingAttachmentPreparation4 owner;
            unknownRejected = prepareSpacingAttachments4(std::move(input), stems, owner);
            if (unknownRejected && alias) {
                  SpacingContextCache4 query(owner, SpacingSlurPolicy4::PREPARED);
                  unknownRejected = !query.preparedSlursReady(alias);
                  }
            }
      input = SpacingInput4();
      score->removeSpanner(&unresolved);
      QVERIFY2(collected, qPrintable(reason));
      QVERIFY(stillMissing); // Never invoke lazy startCR/endCR to complete it.
      QVERIFY(unknownRejected); // Unknown tick aliases cannot become absent attachments.
      }
      Slur* selected = nullptr;
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            Slur* slur = toSlur(entry.second);
            if (!slur->startElement() || !slur->startElement()->isChord()) continue;
            const Chord* start = toChord(slur->startElement());
            if (start->isGraceAfter() && toChord(start->parent())->graceNotesAfter().size() > 1
                && toChord(start->parent())->graceNotesAfter().front() == start) { selected = slur; break; }
            }
      if (selected) {
            const Chord* original = toChord(selected->startElement());
            const Chord* expected = toChord(original->parent())->graceNotesAfter().at(1);
            QVERIFY2(buildSpacingInput4(score, input, &reason), qPrintable(reason));
            const size_t originalOrder = input.anchor(original)->orderInTrack;
            const Fraction originalTick = input.anchor(original)->tick;
            input = SpacingInput4();
            EditData edit;
            edit.key = Qt::Key_Right;
            edit.modifiers = Qt::ShiftModifier;
            edit.curGrip = Grip::START;
            score->startCmd();
            QVERIFY(selected->frontSegment()->edit(edit));
            score->endCmd();
            QCOMPARE(static_cast<const Element*>(selected->startElement()), static_cast<const Element*>(expected));
            const auto moved = state();
            QVERIFY2(buildSpacingInput4(score, input, &reason), qPrintable(reason));
            const auto* slur = input.slur(selected);
            QCOMPARE(slur->start.element, static_cast<const Element*>(expected));
            const auto& anchor = input.anchors()[slur->start.anchorIndex];
            QCOMPARE(anchor.tick, originalTick); // Same tick, different exact grace identity/order.
            QVERIFY(anchor.orderInTrack == originalOrder + 1);
            QVERIFY2(queriesMatch(input), qPrintable(difference));
            QVERIFY(state() == moved);
            input = SpacingInput4();
            score->undoRedo(true, nullptr);
            QVERIFY2(buildSpacingInput4(score, input, &reason), qPrintable(reason));
            QCOMPARE(input.slur(selected)->start.element, static_cast<const Element*>(original));
            QVERIFY2(queriesMatch(input), qPrintable(difference));
            }
      input = SpacingInput4();
      delete score;
      }

void TestSpacing::preparedSpacingProduction4_data()
      {
      QTest::addColumn<QString>("file");
      QTest::newRow("retained") << QString("v0.139-spacing-two-staff-slur-offsets.mscx");
      QTest::newRow("natural-root-boundary") << QString("v0.167-grace-policy-natural.mscx");
      QTest::newRow("system-root-boundary") << QString("v0.167-grace-policy-systems.mscx");
      }

void TestSpacing::preparedSpacingProduction4()
      {
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      score->doLayout();
      const auto native = graceLayoutSnapshot4(score);
      int checked = 0;
      for (System* system : score->systems()) {
            if (system->vbox()) continue;
            SpacingAttachmentPreparation4 owner;
            QString reason;
            QVERIFY2(snapshotLaidOutSystemSpacing4(system, owner, &reason), qPrintable(reason));
            SpacingPlanOptions4 options;
            options.slurPolicy = SpacingSlurPolicy4::PREPARED;
            SpacingPlan4 untouched;
            untouched.width = -166;
            QVERIFY(!buildSpacingPlan4(system, untouched, &reason, options));
            QCOMPARE(untouched.width, qreal(-166));
            SpacingPlan4 plan;
            QVERIFY2(buildSpacingPlan4(system, plan, owner, &reason, options), qPrintable(reason));
            for (const auto& group : plan.afterGraces) QVERIFY(group.slurPolicy == SpacingSlurPolicy4::PREPARED);
            for (const auto& group : plan.mergedGraces) QVERIFY(group.slurPolicy == SpacingSlurPolicy4::PREPARED);
            SpacingPlan4 published;
            QVERIFY2(squeezeSpacingPlan4(system, system->width(), published, owner, &reason, nullptr,
                                         SpacingSlurPolicy4::PREPARED), qPrintable(reason));
            QVERIFY(justifySpacingPlan4(published, system->width()));
            QVERIFY2(spacingPlanMatches4(published, &reason), qPrintable(reason));
            SpacingPlan4 fitted;
            QVERIFY(squeezeSpacingPlan4(system, plan.width, fitted, owner, &reason, nullptr, SpacingSlurPolicy4::PREPARED));
            QVERIFY(spacingPlansEqual154(plan, fitted));
            const auto* old = owner.input().anchors().empty() ? nullptr : owner.input().anchors().front().item;
            QVERIFY(!snapshotLaidOutSystemSpacing4(nullptr, owner));
            if (old) QVERIFY(owner.input().anchor(old));
            ++checked;
            }
      QVERIFY(checked > 0);
      QVERIFY(graceLayoutSnapshot4(score) == native); // All proof queries were read-only.
      QVERIFY(!score->graceQueryFactory4()); // No borrowed binding escaped layout.
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      const auto toggled = graceLayoutSnapshot4(score);
      QCOMPARE(toggled.size(), native.size());
      for (size_t i = 0; i < native.size(); ++i) QVERIFY(qAbs(native[i] - toggled[i]) < 1e-6);
      int lookAheadChecked = 0;
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            Slur* root = toSlur(entry.second);
            if (root->slurDirection() != Direction::AUTO || !root->startElement() || !root->endElement()
                || !root->startElement()->isChord() || !root->endElement()->isChord()) continue;
            Chord* start = toChord(root->startElement());
            Chord* end = toChord(root->endElement());
            if (start->measure() == end->measure() || end->notes().empty()) continue;
            const int line = end->notes().front()->line();
            const bool up = end->up(), startUp = start->up();
            SpacingDirectionReadiness4 readiness;
            readiness.produced(start->measure());
            end->notes().front()->setLine(line + 8);
            end->setUp(!up);
            Element* firstParent = start->measure()->parent();
            Element* lastParent = end->measure()->parent();
            start->measure()->setParent(nullptr);
            end->measure()->setParent(nullptr);
            const bool produced = readiness.prepareRoot(root);
            start->measure()->setParent(firstParent);
            end->measure()->setParent(lastParent); // Restore before assertions.
            QVERIFY(produced && readiness.rootPrepared(root));
            QCOMPARE(end->notes().front()->line(), line);
            QCOMPARE(end->up(), up);
            QCOMPARE(start->up(), startUp);
            QVERIFY(readiness.declarations(end->measure()).empty()); // No false measure certificate.
            ++lookAheadChecked;
            }
      if (file.startsWith("v0.167")) QVERIFY(lookAheadChecked > 0);
      delete score;
      }

void TestSpacing::spacingSlurAttachments4()
      {
      MasterScore* score = readScore(DIR + "v0.139-spacing-two-staff-slur-offsets.mscx");
      QVERIFY(score);
      score->doLayout();
      SpacingInput4 input;
      QVERIFY(buildSpacingInput4(score, input));
      const Chord* start = nullptr;
      const Chord* end = nullptr;
      for (const auto& anchor : input.anchors()) {
            if (anchor.kind == SpacingMemberKind4::AFTER && anchor.memberIndex == 1 && !start)
                  start = toChord(anchor.item);
            if (anchor.item->isChord()) end = toChord(anchor.item);
            }
      QVERIFY(start && end && start != end);
      // Page translation and a missing system must not change local side tests.
      const Note* first = start->notes().front();
      const Note* second = toChord(start->parent())->notes().front();
      const unsigned expectedSide = second->pagePos().y() < first->pagePos().y() ? 1 : 2;
      unsigned placedSide = 0, detachedSide = 0;
      QVERIFY(spacingLocalSlurSide4(first, second, placedSide));
      Measure* localMeasure = start->measure();
      SpacingDirectionReadiness4 measureReady;
      QVERIFY(measureReady.declarations(localMeasure).empty());
      measureReady.produced(localMeasure);
      const auto localDeclarations = measureReady.declarations(localMeasure);
      SpacingAttachmentPreparation4 measureOwner;
      QVERIFY(captureMeasureSpacingAttachments4(localMeasure, localDeclarations, measureOwner));
      const size_t localAnchors = measureOwner.input().anchors().size();
      QVERIFY(localAnchors > 0);
      Element* savedParent = localMeasure->parent();
      localMeasure->setParent(nullptr);
      const bool detachedAccepted = spacingLocalSlurSide4(first, second, detachedSide);
      SpacingAttachmentPreparation4 detachedOwner;
      const bool detachedCaptured = captureMeasureSpacingAttachments4(localMeasure, localDeclarations, detachedOwner);
      localMeasure->setParent(savedParent); // Restore before any assertion exits.
      QVERIFY(detachedCaptured);
      QCOMPARE(detachedOwner.input().anchors().size(), localAnchors);
      for (const auto& anchor : detachedOwner.input().anchors()) QVERIFY(anchor.measure == localMeasure);
      const auto* savedSides = measureOwner.sides(start);
      QVERIFY(!captureMeasureSpacingAttachments4(nullptr, localDeclarations, measureOwner));
      QCOMPARE(measureOwner.sides(start), savedSides);
      measureReady.invalidate(localMeasure);
      QVERIFY(measureReady.declarations(localMeasure).empty());
      QVERIFY(detachedAccepted);
      QCOMPARE(placedSide, expectedSide);
      QCOMPARE(detachedSide, expectedSide);
      input = SpacingInput4();
      // All directions, visibility and line styles coexist at a non-first
      // grace endpoint. No first-member, count, same-track or curve gate.
      std::vector<Slur*> roots;
      for (Direction direction : {Direction::AUTO, Direction::UP, Direction::DOWN})
            for (bool visible : {false, true})
                  for (int line : {0, 1}) {
                        Slur* root = new Slur(score);
                        root->setTrack(start->track());
                        root->setTrack2(end->track());
                        root->setTick(start->tick());
                        root->setTick2(end->tick());
                        root->setStartElement(const_cast<Chord*>(start));
                        root->setEndElement(const_cast<Chord*>(end));
                        root->setSlurDirection(direction);
                        root->setVisible(visible);
                        root->setLineType(line);
                        score->addSpanner(root);
                        roots.push_back(root);
                        }
      QVERIFY(buildSpacingInput4(score, input));
      SpacingSlurAttachments4 flags;
      QVERIFY(buildSpacingSlurAttachments4(input, flags));
      bool stale = false;
      for (const auto& root : input.slurs())
            stale = stale || (root.requestedDirection != Direction::AUTO
                  && root.cachedUp != (root.requestedDirection == Direction::UP));
      QVERIFY(stale); // Explicit requests changed; no layout resolved the cache.
      const size_t a = size_t(input.anchor(start) - input.anchors().data());
      QVERIFY(flags.anchors[a].startUp && flags.anchors[a].startDown);
      QVERIFY(flags.anchors[a].pendingStarts >= 4);
      QVERIFY(!flags.complete());
      std::vector<Direction> directions(input.slurs().size(), Direction::UP);
      for (size_t i = 0; i < input.slurs().size(); ++i)
            if (input.slurs()[i].requestedDirection != Direction::AUTO)
                  directions[i] = input.slurs()[i].requestedDirection;
      QVERIFY(buildSpacingSlurAttachments4(input, flags, &directions));
      QVERIFY(flags.complete());
      QVERIFY(flags.anchors[a].complete());
      QCOMPARE(flags.anchors[a].blockedKerning(false), 1u);
      QCOMPARE(flags.anchors[a].blockedKerning(true), 2u);
      // Validation must not partially publish a new attachment table.
      const size_t previous = flags.anchors.size();
      directions.push_back(Direction::AUTO);
      QVERIFY(!buildSpacingSlurAttachments4(input, flags, &directions));
      QVERIFY(flags.anchors.size() == previous && flags.complete());
      directions.pop_back();
      directions[0] = static_cast<Direction>(99);
      QVERIFY(!buildSpacingSlurAttachments4(input, flags, &directions));
      for (size_t i = 0; i < input.slurs().size(); ++i) {
            directions[i] = input.slurs()[i].requestedDirection;
            if (directions[i] == Direction::UP || directions[i] == Direction::DOWN) {
                  directions[i] = directions[i] == Direction::UP ? Direction::DOWN : Direction::UP;
                  QVERIFY(!buildSpacingSlurAttachments4(input, flags, &directions));
                  break;
                  }
            }
      QVERIFY(flags.anchors.size() == previous && flags.complete());
      {
      SpacingInput4 declaredInput;
      QVERIFY(buildSpacingInput4(score, declaredInput));
      std::vector<Direction> stems(declaredInput.anchors().size(), Direction::UP);
      SpacingAttachmentPreparation4 declared;
      std::vector<SpacingPreparedSlur4> rootDeclarations { { roots.front(), Direction::DOWN } };
      QVERIFY(prepareSpacingAttachments4(std::move(declaredInput), stems, declared, nullptr, &rootDeclarations));
      QVERIFY(declared.direction(roots.front()) && *declared.direction(roots.front()) == Direction::DOWN);
      const auto* saved = declared.sides(start);
      QVERIFY(buildSpacingInput4(score, declaredInput));
      rootDeclarations.push_back(rootDeclarations.front());
      QVERIFY(!prepareSpacingAttachments4(std::move(declaredInput), stems, declared, nullptr, &rootDeclarations));
      QVERIFY(declaredInput.score() == score && declared.sides(start) == saved);
      rootDeclarations = { { roots.front(), static_cast<Direction>(99) } };
      QVERIFY(!prepareSpacingAttachments4(std::move(declaredInput), stems, declared, nullptr, &rootDeclarations));
      QVERIFY(declaredInput.score() == score && declared.sides(start) == saved);
      rootDeclarations = { { roots[4], Direction::DOWN } }; // Explicit UP cannot be overridden.
      QVERIFY(!prepareSpacingAttachments4(std::move(declaredInput), stems, declared, nullptr, &rootDeclarations));
      rootDeclarations = { { nullptr, Direction::UP } };
      QVERIFY(!prepareSpacingAttachments4(std::move(declaredInput), stems, declared, nullptr, &rootDeclarations));
      QVERIFY(declaredInput.score() == score && declared.sides(start) == saved);
      rootDeclarations = { { roots[4], Direction::AUTO } }; // A literal UP request stays resolved.
      QVERIFY(prepareSpacingAttachments4(std::move(declaredInput), stems, declared, nullptr, &rootDeclarations));
      QVERIFY(declared.direction(roots[4]) && *declared.direction(roots[4]) == Direction::UP);
      }
      input = SpacingInput4();
      {
      const auto geometryBefore = graceBarlineSnapshot4(score);
      QVERIFY(buildSpacingInput4(score, input));
      std::vector<Direction> stems;
      for (const auto& anchor : input.anchors()) stems.push_back(anchor.up ? Direction::UP : Direction::DOWN);
      SpacingAttachmentPreparation4 owner;
      QVERIFY(prepareSpacingAttachments4(std::move(input), stems, owner));
      SpacingContextCache4 compatibility(owner);
      SpacingContextCache4 prepared(owner, SpacingSlurPolicy4::PREPARED);
      QVERIFY(compatibility.slurPolicy() == SpacingSlurPolicy4::COMPATIBILITY);
      QVERIFY(prepared.preparedSlursReady(start));
      QCOMPARE(prepared.forwardSlurKerning(start), start->up() ? 2u : 1u);
      const Chord* main = toChord(start->parent());
      GraceMergedGeometry4 rejected, candidate;
      rejected.plan.left = -159;
      QVERIFY(!buildGraceMergedGeometry4(main, rejected, nullptr, &compatibility));
      QCOMPARE(rejected.plan.left, qreal(-159));
      QString reason;
      QVERIFY2(buildGraceMergedGeometry4(main, candidate, &reason, &prepared), qPrintable(reason));
      QVERIFY(candidate.plan.owner == main && candidate.source.complete() && candidate.target.complete());
      QVERIFY(!candidate.plan.positions.empty());
      // A missing certificate cannot fall back to compatibility inside this mode.
      SpacingAttachmentPreparation4 pending;
      QVERIFY(captureSpacingAttachments4(score, {}, pending));
      SpacingContextCache4 pendingQuery(pending, SpacingSlurPolicy4::PREPARED);
      QVERIFY(!pendingQuery.preparedSlursReady(start));
      GraceMergedGeometry4 untouched;
      untouched.plan.left = -159;
      QVERIFY(!buildGraceMergedGeometry4(main, untouched, nullptr, &pendingQuery));
      QCOMPARE(untouched.plan.left, qreal(-159));
      QVERIFY(geometryBefore == graceBarlineSnapshot4(score));
      }
      {
      const Chord* main = toChord(start->parent());
      int captures = 0;
      SpacingGraceQueryFactory4 factory([&](const Chord* chord, SpacingAttachmentPreparation4& owner, QString* reason) {
            ++captures;
            return captureGraceFixture160(chord, owner, reason);
            }, SpacingSlurPolicy4::PREPARED);
      QVERIFY(!score->graceQueryFactory4());
      {
      SpacingGraceQueryScope4 scope(score, &factory);
      QCOMPARE(score->graceQueryFactory4(), &factory);
      GraceMergedGeometry4 geometry;
      QString reason;
      QVERIFY2(queryGraceMergedGeometry4(main, geometry, &reason), qPrintable(reason));
      QVERIFY(geometry.plan.slurPolicy == SpacingSlurPolicy4::PREPARED);
      QCOMPARE(captures, 1);
      GraceBeforePlan4 before;
      QVERIFY2(queryGraceBeforePlan4(geometry.plan.target, before, &reason), qPrintable(reason));
      QVERIFY(before.slurPolicy == SpacingSlurPolicy4::PREPARED);
      QCOMPARE(captures, 2);
      {
      SpacingGraceQueryScope4 disabled(score, nullptr);
      QVERIFY(!score->graceQueryFactory4());
      GraceMergedGeometry4 rejected;
      rejected.plan.left = -160;
      QVERIFY(!queryGraceMergedGeometry4(main, rejected));
      QCOMPARE(rejected.plan.left, qreal(-160));
      QCOMPARE(captures, 2);
      }
      QCOMPARE(score->graceQueryFactory4(), &factory);
      std::vector<Chord*> members;
      for (Chord* grace : main->graceNotesAfter()) members.push_back(grace);
      for (Chord* grace : geometry.plan.target->graceNotesBefore()) members.push_back(grace);
      std::vector<qreal> positions;
      for (Chord* grace : members) positions.push_back(grace->x());
      const int previousCaptures = captures;
      GraceMergedGeometry4 prepared;
      QVERIFY2(prepareGraceMergedBeams4(const_cast<Chord*>(main), &reason, &prepared), qPrintable(reason));
      QVERIFY(prepared.plan.slurPolicy == SpacingSlurPolicy4::PREPARED);
      QVERIFY(captures >= previousCaptures + 2); // Fresh query before/after native beam writes.
      for (size_t i = 0; i < members.size(); ++i) QCOMPARE(members[i]->x(), positions[i]);
      SpacingGraceQueryFactory4 failed([](const Chord*, SpacingAttachmentPreparation4&, QString*) { return false; },
                                      SpacingSlurPolicy4::PREPARED);
      const auto native = graceBarlineSnapshot4(score);
      {
      SpacingGraceQueryScope4 failure(score, &failed);
      GraceBeforePlan4 untouched;
      untouched.left = -160;
      QVERIFY(!queryGraceBeforePlan4(geometry.plan.target, untouched));
      QCOMPARE(untouched.left, qreal(-160));
      QVERIFY(!applyGraceAfterPlan4(const_cast<Chord*>(main)));
      QVERIFY(graceBarlineSnapshot4(score) == native);
      }
      QCOMPARE(score->graceQueryFactory4(), &factory);
      }
      QVERIFY(!score->graceQueryFactory4());
      }
      for (Slur* root : roots) {
            score->removeSpanner(root);
            delete root;
            }
      delete score;
      }

void TestSpacing::spacingSlurDirections4_data()
      {
      QTest::addColumn<QString>("file");
      QTest::newRow("grace-cross-system") << QString("v0.145-spacing-slur-anchor-systems.mscx");
      QTest::newRow("multiple-voices") << QString("v0.38-spacing-voices.mscx");
      QTest::newRow("cross-staff-beams") << QString("v0.71-spacing-cross-chords-two-voices.mscx");
      }

void TestSpacing::spacingSlurDirections4()
      {
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      score->doLayout();
      SpacingInput4 input;
      QVERIFY(buildSpacingInput4(score, input));
      std::vector<const ChordRest*> anchors;
      for (const auto& a : input.anchors()) anchors.push_back(a.item);
      QVERIFY(!anchors.empty());
      input = SpacingInput4();
      std::vector<Slur*> roots;
      // A root starts on every member/voice/rest. Endpoints cycle through
      // later/equal-tick members, across systems and back to earlier anchors.
      // No native curve/layout call is made on these synthetic roots.
      for (size_t i = 0; i < anchors.size(); ++i) {
            Slur* root = new Slur(score);
            const ChordRest* start = anchors[i];
            const ChordRest* end = anchors[(i * 7 + anchors.size() / 2) % anchors.size()];
            root->setTrack(start->track());
            root->setTrack2(end->track());
            root->setTick(start->tick());
            root->setTick2(end->tick());
            root->setStartElement(const_cast<ChordRest*>(start));
            root->setEndElement(const_cast<ChordRest*>(end));
            score->addSpanner(root);
            roots.push_back(root);
            }
      QVERIFY(buildSpacingInput4(score, input));
      QVERIFY(slurDirectionsMatch153(input));
      const auto before = graceLayoutSnapshot4(score);
      QVERIFY(slurDirectionsMatch153(input));
      QVERIFY(before == graceLayoutSnapshot4(score));
      input = SpacingInput4();
      for (Slur* root : roots) {
            score->removeSpanner(root);
            delete root;
            }
      delete score;
      }

void TestSpacing::slurDirectionPriority4()
      {
      for (Direction requested : {Direction::AUTO, Direction::UP, Direction::DOWN})
            for (Direction stem : {Direction::AUTO, Direction::UP, Direction::DOWN})
                  for (SpacingStemMixture4 mixture : {SpacingStemMixture4::NONE, SpacingStemMixture4::MIXED, SpacingStemMixture4::PENDING})
                        for (int bits = 0; bits < 16; ++bits) {
                              SpacingSlurDirectionFacts4 f;
                              f.requested = requested;
                              f.startStem = stem;
                              f.mixture = mixture;
                              f.anchorsReady = bits & 1;
                              f.crossStaffStart = bits & 2;
                              f.multipleVoices = bits & 4;
                              f.secondaryVoice = bits & 8;
                              Direction expected = requested;
                              if (requested == Direction::AUTO && f.anchorsReady) {
                                    if (f.crossStaffStart) expected = Direction::UP;
                                    else if (f.multipleVoices) expected = f.secondaryVoice ? Direction::DOWN : Direction::UP;
                                    else if (stem != Direction::AUTO) {
                                          if (mixture == SpacingStemMixture4::MIXED) expected = Direction::UP;
                                          else if (mixture == SpacingStemMixture4::NONE)
                                                expected = stem == Direction::UP ? Direction::DOWN : Direction::UP;
                                          }
                                    }
                              QVERIFY(resolveSpacingSlurDirection4(f) == expected);
                              }
      }

void TestSpacing::slurRendererDirection4_data()
      {
      spacingSlurDirections4_data();
      }

void TestSpacing::slurRendererDirection4()
      {
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      SpacingInput4 input;
      QVERIFY(buildSpacingInput4(score, input));
      // Add a forward main-note AUTO span on each actual track, including
      // secondary voices and cross-staff beam starts. Existing grace spans
      // exercise the non-main endpoints; no synthetic backward curves.
      std::map<int, std::pair<const Chord*, const Chord*>> bounds;
      for (const auto& anchor : input.anchors()) {
            if (anchor.kind != SpacingMemberKind4::MAIN || !anchor.main) continue;
            auto& pair = bounds[anchor.track];
            if (!pair.first) pair.first = anchor.main;
            pair.second = anchor.main;
            }
      input = SpacingInput4();
      for (const auto& entry : bounds) {
            const Chord* start = entry.second.first;
            const Chord* end = entry.second.second;
            if (start == end || start->tick() >= end->tick()) continue;
            Slur* root = new Slur(score);
            root->setTrack(start->track());
            root->setTrack2(end->track());
            root->setTick(start->tick());
            root->setTick2(end->tick());
            root->setStartElement(const_cast<Chord*>(start));
            root->setEndElement(const_cast<Chord*>(end));
            score->addSpanner(root);
            }
      std::vector<Slur*> roots;
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            Slur* root = toSlur(entry.second);
            root->setSlurDirection(Direction::AUTO);
            roots.push_back(root);
            }
      QVERIFY(!roots.empty());
      score->doLayout(); // Native layoutSystem path.
      const auto expected = [&]() {
            std::map<const Slur*, Direction> values;
            SpacingInput4 musical;
            if (!buildSpacingInput4(score, musical)) return values;
            std::vector<SpacingPreparedAnchor4> declarations;
            for (const auto& a : musical.anchors()) declarations.emplace_back(a.item, a.up ? Direction::UP : Direction::DOWN);
            musical = SpacingInput4();
            SpacingAttachmentPreparation4 owner;
            if (!captureSpacingAttachments4(score, declarations, owner)) return values;
            for (size_t i = 0; i < owner.input().slurs().size(); ++i)
                  values[owner.input().slurs()[i].slur] = owner.slurDirections()[i];
            return values;
            };
      auto directions = expected(); // Owner ends before native producer writes.
      QVERIFY(directions.size() == roots.size());
      const auto readState = [score]() {
            auto values = graceBarlineSnapshot4(score);
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
                  for (const Segment& segment : m->segments())
                        for (const Element* item : segment.elist()) {
                              if (!item || !item->isChord()) continue;
                              const Chord* main = toChord(item);
                              QVector<const Chord*> chords {main};
                              for (const Chord* grace : main->graceNotes()) chords.push_back(grace);
                              for (const Chord* chord : chords) {
                                    values.push_back(qreal(chord->up()));
                                    for (const Note* note : chord->notes())
                                          values.insert(values.end(), {qreal(note->line()), note->x(), note->y()});
                                    }
                              }
            for (const auto& entry : score->spannerMap().map())
                  if (entry.second->isSlur()) values.push_back(qreal(toSlur(entry.second)->up()));
            return values;
            };
      const auto beforeQuery = readState();
      const int queryHistory = score->undoStack()->getCurIdx();
      const auto queryMode = score->cmdState().updateMode();
      const auto queryGeneration = score->cmdState().generation4();
      for (const Slur* root : roots) {
            Direction readOnly = Direction::AUTO;
            QVERIFY(resolvePreparedSlurDirection4(root, readOnly));
            QVERIFY(readOnly == directions[root]);
            }
      QVERIFY(readState() == beforeQuery);
      QCOMPARE(score->undoStack()->getCurIdx(), queryHistory);
      QVERIFY(score->cmdState().updateMode() == queryMode && score->cmdState().generation4() == queryGeneration);
      for (Slur* root : roots) {
            QVERIFY(directions[root] != Direction::AUTO);
            QCOMPARE(root->up(), directions[root] == Direction::UP);
            Direction produced = Direction::AUTO;
            QVERIFY(prepareSlurDirection4(root, produced));
            QVERIFY(produced == directions[root]);
            root->layout(); // Native immediate-edit path.
            QCOMPARE(root->up(), directions[root] == Direction::UP);
            }
      score->doLayout();
      directions = expected();
      QVERIFY(directions.size() == roots.size());
      for (Slur* root : roots) QCOMPARE(root->up(), directions[root] == Direction::UP);
      // Native producer validation does not resolve missing raw anchors or
      // create history entries; literal explicit directions need no geometry.
      {
      const int history = score->undoStack()->getCurIdx();
      Direction unchanged = Direction::DOWN;
      QVERIFY(!prepareSlurDirection4(nullptr, unchanged));
      QVERIFY(!resolvePreparedSlurDirection4(nullptr, unchanged));
      QVERIFY(unchanged == Direction::DOWN);
      Slur missing(score);
      QVERIFY(!prepareSlurDirection4(&missing, unchanged));
      QVERIFY(!resolvePreparedSlurDirection4(&missing, unchanged));
      QVERIFY(!missing.startElement() && !missing.endElement() && unchanged == Direction::DOWN);
      missing.setSlurDirection(Direction::UP);
      QVERIFY(prepareSlurDirection4(&missing, unchanged) && unchanged == Direction::UP);
      QCOMPARE(score->undoStack()->getCurIdx(), history);
      }
      delete score;
      }

// Combined same-direction groups use the optional plan; M2 retains fallback.
void TestSpacing::graceMergedGeometry_data()
      {
      graceAfterBoundaryBaseline_data();
      }

void TestSpacing::graceMergedGeometry()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score && score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      const auto original = graceMergedSnapshot4(score);
      const auto belongsTo = [](const Element* item, const Chord* chord) {
            for (; item; item = item->parent()) {
                  if (item == chord) return true;
                  if (item->isChord()) return false;
                  }
            return false;
            };
      std::vector<Chord*> supported;
      int count = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++count) {
            QVERIFY(count < 6);
            Chord* owner = nullptr;
            for (const Segment& seg : m->segments()) {
                  Element* item = seg.element(0);
                  if (item && item->isChord() && !toChord(item)->graceNotesAfter().empty()) owner = toChord(item);
                  }
            QVERIFY(owner);
            GraceMergedGeometry4 geometry;
            geometry.plan.left = 123;
            geometry.source.elements.push_back(SpacingShapeElement4(QRectF(99, 0, 1, 1), owner));
            geometry.target = geometry.source;
            QString reason;
            const bool built = buildGraceMergedGeometry4(owner, geometry, &reason);
            QVERIFY2(built == (count == 3 || count == 5), qPrintable(reason));
            if (!built) {
                  QCOMPARE(geometry.plan.left, qreal(123));
                  QVERIFY(!geometry.plan.owner && !geometry.plan.target && geometry.plan.positions.empty());
                  QCOMPARE(static_cast<int>(geometry.source.elements.size()), 1);
                  QCOMPARE(static_cast<int>(geometry.target.elements.size()), 1);
                  QCOMPARE(geometry.source.elements.front().rect.x(), qreal(99));
                  QCOMPARE(geometry.target.elements.front().rect.x(), qreal(99));
                  GraceMergedGeometry4 unprepared;
                  unprepared.plan.left = 789;
                  QVERIFY(!prepareGraceMergedBeams4(owner, nullptr, &unprepared));
                  QCOMPARE(unprepared.plan.left, qreal(789));
                  QVERIFY(unprepared.source.elements.empty() && unprepared.target.elements.empty());
                  continue;
                  }
            supported.push_back(owner);
            QVERIFY(geometry.source.complete() && geometry.target.complete());
            const Chord* target = geometry.plan.target;
            for (const auto& e : geometry.source.elements) QVERIFY(belongsTo(e.item, owner));
            // Independent fresh extraction must match the packing snapshot,
            // including component identity/order and every translated rectangle.
            SpacingShape4 reference;
            SpacingShapeCache4 shapeCache;
            spacingElementShape4(owner, &shapeCache); // Populate nested after-graces first.
            const auto appendReference = [&](const Chord* chord, const QPointF& offset) {
                  const SpacingShape4 uncached = spacingElementShape4(chord);
                  const SpacingShape4 cached = spacingElementShape4(chord, &shapeCache);
                  if (cached.elements.size() != uncached.elements.size()) return false;
                  for (size_t i = 0; i < uncached.elements.size(); ++i) {
                        const auto& x = cached.elements[i];
                        const auto& y = uncached.elements[i];
                        if (x.item != y.item || x.identity != y.identity || x.rect != y.rect
                            || x.aggregateOwners != y.aggregateOwners || x.sharedItems != y.sharedItems) return false;
                        }
                  for (const auto& e : uncached.elements) {
                        if (e.identity != SpacingIdentity4::ITEM || !belongsTo(e.item, chord)) continue;
                        SpacingShapeElement4 component = e;
                        component.rect.translate(offset);
                        reference.elements.push_back(component);
                        }
                  return true;
                  };
            QVERIFY(appendReference(target, target->pos()));
            for (const auto& position : geometry.plan.positions)
                  QVERIFY(appendReference(position.chord, QPointF(position.x, 0)));
            QCOMPARE(int(geometry.target.elements.size()), int(reference.elements.size()));
            for (size_t i = 0; i < reference.elements.size(); ++i) {
                  const auto& actual = geometry.target.elements[i];
                  const auto& expected = reference.elements[i];
                  QCOMPARE(actual.item, expected.item);
                  QCOMPARE(int(actual.identity), int(expected.identity));
                  QCOMPARE(actual.rect, expected.rect);
                  QVERIFY(actual.aggregateOwners == expected.aggregateOwners);
                  QVERIFY(actual.sharedItems == expected.sharedItems);
                  }
            const Note* targetNote = target->notes().front();
            int mainCopies = 0;
            for (const auto& e : geometry.target.elements) {
                  if (e.item == targetNote) {
                        ++mainCopies;
                        QCOMPARE(e.rect, targetNote->bbox().translated(targetNote->pos() + target->pos()));
                        }
                  bool known = belongsTo(e.item, target);
                  for (const auto& p : geometry.plan.positions) known = known || belongsTo(e.item, p.chord);
                  QVERIFY(known);
                  }
            QCOMPARE(mainCopies, 1);
            for (const auto& p : geometry.plan.positions) {
                  const Note* note = p.chord->notes().front();
                  int oldCopies = 0, newCopies = 0;
                  for (const auto& e : geometry.source.elements) oldCopies += e.item == note;
                  for (const auto& e : geometry.target.elements) {
                        if (e.item == note) {
                              ++newCopies;
                              QCOMPARE(e.rect, note->bbox().translated(note->pos() + QPointF(p.x, 0)));
                              }
                        }
                  QCOMPARE(oldCopies, 0);
                  QCOMPARE(newCopies, 1);
                  QCOMPARE(static_cast<const Element*>(p.chord->parent()),
                        static_cast<const Element*>(p.chord->isGraceAfter() ? owner : target));
                  }
            // An edited target must not partially overwrite a previous projection.
            const QPointF offset = targetNote->offset();
            Note* edited = target->notes().front();
            edited->setOffset(QPointF(.5, 0));
            GraceMergedGeometry4 rejected;
            rejected.plan.left = 123;
            rejected.source.elements.push_back(SpacingShapeElement4(QRectF(99, 0, 1, 1), owner));
            const bool accepted = buildGraceMergedGeometry4(owner, rejected);
            edited->setOffset(offset);
            QVERIFY(!accepted);
            QCOMPARE(rejected.plan.left, qreal(123));
            QCOMPARE(rejected.source.elements.front().rect.x(), qreal(99));
            QVERIFY(rejected.target.elements.empty());
            }
      QCOMPARE(count, 6);
      QCOMPARE(static_cast<int>(supported.size()), 2);
      QVERIFY(graceMergedSnapshot4(score) == original); // Projection queries are read-only.
      for (Chord* owner : supported) {
            GraceMergedPlan4 plan;
            QVERIFY(buildGraceMergedPlan4(owner, plan));
            std::vector<qreal> positions;
            for (const auto& p : plan.positions) positions.push_back(p.chord->x());
            QString reason;
            GraceMergedGeometry4 completed, fresh;
            QVERIFY2(prepareGraceMergedBeams4(owner, &reason, &completed), qPrintable(reason));
            QVERIFY2(buildGraceMergedGeometry4(owner, fresh, &reason), qPrintable(reason));
            QCOMPARE(completed.plan.owner, fresh.plan.owner);
            QCOMPARE(completed.plan.target, fresh.plan.target);
            QCOMPARE(completed.plan.left, fresh.plan.left);
            QCOMPARE(completed.plan.right, fresh.plan.right);
            QCOMPARE(int(completed.plan.positions.size()), int(fresh.plan.positions.size()));
            for (size_t i = 0; i < fresh.plan.positions.size(); ++i) {
                  QCOMPARE(completed.plan.positions[i].chord, fresh.plan.positions[i].chord);
                  QCOMPARE(completed.plan.positions[i].x, fresh.plan.positions[i].x);
                  }
            for (const auto& pair : {std::make_pair(&completed.source, &fresh.source),
                                     std::make_pair(&completed.target, &fresh.target)}) {
                  QCOMPARE(int(pair.first->elements.size()), int(pair.second->elements.size()));
                  for (size_t i = 0; i < pair.first->elements.size(); ++i) {
                        const auto& x = pair.first->elements[i];
                        const auto& y = pair.second->elements[i];
                        QCOMPARE(x.item, y.item);
                        QCOMPARE(int(x.identity), int(y.identity));
                        QCOMPARE(x.rect, y.rect);
                        QVERIFY(x.aggregateOwners == y.aggregateOwners && x.sharedItems == y.sharedItems);
                        }
                  }
            for (size_t i = 0; i < plan.positions.size(); ++i) QCOMPARE(plan.positions[i].chord->x(), positions[i]);
            const auto prepared = graceMergedSnapshot4(score);
            QVERIFY2(prepareGraceMergedBeams4(owner, &reason), qPrintable(reason));
            const auto repeated = graceMergedSnapshot4(score);
            QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(prepared.size()));
            for (size_t i = 0; i < prepared.size(); ++i) QVERIFY(qAbs(prepared[i] - repeated[i]) < 1e-6);
            }
      // Normal live layout recreates its completed geometry after explicit preparation.
      score->doLayout();
      const auto restored = graceMergedSnapshot4(score);
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(original.size()));
      for (size_t i = 0; i < original.size(); ++i) QVERIFY(qAbs(restored[i] - original[i]) < 1e-6);
      delete score;
      }

void TestSpacing::graceAfterBoundaryBaseline_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<QString>("file");
      for (const QString& file : {QString("v0.107-spacing-after-grace-targets.mscx"),
                                  QString("v0.107-spacing-after-grace-targets-natural.mscx")}) {
            for (bool beams : {false, true}) {
                  const QByteArray tag = (file + (beams ? "-studio" : "-legacy")).toLatin1();
                  QTest::newRow(tag.constData()) << beams << file;
                  }
            }
      }

void TestSpacing::graceAfterBoundaryBaseline()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score && score->styleB(Sid::spacingLayout4) && score->styleB(Sid::beamLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      const auto snapshot = [&]() { return graceMergedSnapshot4(score); };
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      const auto modern = snapshot();
      int count = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++count) {
            QVERIFY(count < 6 && m->ticks() == Fraction(1, 1));
            QCOMPARE(m->userStretch(), qreal(1));
            const bool natural = file.contains("-natural.mscx");
            QVERIFY(m->lineBreak() == (!natural && count < 5));
            if (natural) QCOMPARE(m->system(), score->firstMeasure()->system());
            else QCOMPARE(m->system()->firstMeasure(), m->system()->lastMeasure());
            Segment* source = m->tick2segment(m->tick() + Fraction(1, count < 3 ? 2 : 4), SegmentType::ChordRest);
            QVERIFY(source && source->element(0) && source->element(0)->isChord());
            Chord* owner = toChord(source->element(0));
            const auto after = owner->graceNotesAfter();
            QCOMPARE(after.size(), (count == 2 || count == 5) ? 4 : 2);
            Segment* target = m->tick2segment(owner->tick() + owner->actualTicks(), SegmentType::All);
            QVERIFY(target && target->measure() == m);
            if (count < 3) QVERIFY(target->isEndBarLineType());
            else {
                  QVERIFY(target->isChordRestType() && target->element(0) && target->element(0)->isChord());
                  Chord* next = toChord(target->element(0));
                  const auto before = next->graceNotesBefore();
                  QCOMPARE(before.size(), 2);
                  for (const Chord* grace : before) {
                        QCOMPARE(grace->parent(), static_cast<Element*>(next));
                        QCOMPARE(grace->segment(), target);
                        QVERIFY(grace->isGraceBefore() && grace->up());
                        }
                  }
            for (int i = 0; i < after.size(); ++i) {
                  const Chord* grace = after[i];
                  QCOMPARE(grace->parent(), static_cast<Element*>(owner));
                  QCOMPARE(grace->segment(), source);
                  QCOMPARE(grace->track(), owner->track());
                  QVERIFY(grace->isGraceAfter() && grace->up() == (count != 1 && count != 4));
                  QVERIFY(qIsFinite(grace->x()) && qIsFinite(grace->y()));
                  if (i) QVERIFY(grace->x() > after[i - 1]->x());
                  }
            // Barline geometry is admitted; merged groups still reject atomically.
            GraceAfterPlan4 plan;
            plan.left = 123;
            QString reason;
            const bool built = buildGraceAfterPlan4(owner, plan, &reason);
            QCOMPARE(built, count < 3);
            GraceAfterGeometry4 geometry;
            geometry.plan.left = 123;
            const bool projected = buildGraceAfterGeometry4(owner, geometry, &reason);
            QCOMPARE(projected, count < 3);
            if (count >= 3) {
                  QCOMPARE(plan.left, qreal(123));
                  QVERIFY(plan.positions.empty() && !plan.owner && !plan.target);
                  QCOMPARE(geometry.plan.left, qreal(123));
                  QVERIFY(geometry.source.elements.empty() && geometry.target.elements.empty());
                  QVERIFY(!applyGraceAfterPlan4(owner));
                  }
            else {
                  QVERIFY(geometry.source.complete() && geometry.target.complete());
                  QCOMPARE(geometry.plan.target, static_cast<const Segment*>(target));
                  }
            GraceAfterPlan4 boundary;
            boundary.left = 123;
            const bool staged = buildGraceAfterBarlinePlan4(owner, boundary, &reason);
            QCOMPARE(staged, count < 3);
            if (staged) {
                  QCOMPARE(boundary.owner, static_cast<const Chord*>(owner));
                  QCOMPARE(boundary.target, static_cast<const Segment*>(target));
                  QCOMPARE(static_cast<int>(boundary.positions.size()), after.size());
                  QVERIFY(qIsFinite(boundary.left) && qIsFinite(boundary.right) && boundary.left < boundary.right);
                  QVERIFY(target->element(0) && target->element(0)->isBarLine());
                  BarLine* barline = toBarLine(target->element(0));
                  const SpacingPaddingTable4 table(score);
                  for (int i = 0; i < after.size(); ++i) {
                        QCOMPARE(boundary.positions[i].chord, static_cast<const Chord*>(after[i]));
                        QVERIFY(qIsFinite(boundary.positions[i].x));
                        if (i) QVERIFY(boundary.positions[i].x > boundary.positions[i - 1].x);
                        SpacingPairPolicy4 policy;
                        const Note* note = after[i]->notes().front();
                        QVERIFY(spacingAfterGracePairPolicy4(table, owner, barline, note, barline, score->spatium(), policy));
                        const qreal expectedPadding = score->styleP(Sid::noteBarDistance) * (note->mag() + barline->mag()) / 2;
                        QVERIFY(qAbs(policy.padding - expectedPadding) < 1e-6);
                        QVERIFY(!spacingPairPolicy4(table, note, barline, score->spatium(), policy));
                        QVERIFY(!spacingAfterGracePairPolicy4(table, owner, barline,
                              owner->notes().front(), note, score->spatium(), policy));
                        }
                  // Query rejection must be atomic for unsupported boundary edits.
                  for (int mode = 0; mode < 5; ++mode) {
                        const QPointF offset = barline->offset();
                        if (mode == 0) barline->setVisible(false);
                        if (mode == 1) barline->setSpanStaff(1);
                        if (mode == 2) barline->setBarLineType(BarLineType::DOUBLE);
                        if (mode == 3) barline->setOffset(QPointF(1, 0));
                        if (mode == 4) barline->setAutoplace(false);
                        GraceAfterPlan4 invalid;
                        invalid.left = 123;
                        const bool accepted = buildGraceAfterBarlinePlan4(owner, invalid);
                        if (mode == 0) barline->setVisible(true);
                        if (mode == 1) barline->setSpanStaff(0);
                        if (mode == 2) barline->setBarLineType(BarLineType::NORMAL);
                        if (mode == 3) barline->setOffset(offset);
                        if (mode == 4) barline->setAutoplace(true);
                        QVERIFY(!accepted);
                        QCOMPARE(invalid.left, qreal(123));
                        QVERIFY(!invalid.owner && !invalid.target && invalid.positions.empty());
                        }
                  }
            else {
                  QCOMPARE(boundary.left, qreal(123));
                  QVERIFY(!boundary.owner && !boundary.target && boundary.positions.empty());
                  }
            GraceMergedPlan4 joint;
            joint.left = 123;
            const bool jointBuilt = buildGraceMergedPlan4(owner, joint, &reason);
            QVERIFY2(jointBuilt == (count == 3 || count == 5), qPrintable(reason));
            if (jointBuilt) {
                  Chord* next = toChord(target->element(0));
                  auto combined = after;
                  for (Chord* grace : next->graceNotesBefore()) combined.push_back(grace);
                  QCOMPARE(joint.owner, static_cast<const Chord*>(owner));
                  QCOMPARE(joint.target, static_cast<const Chord*>(next));
                  QCOMPARE(static_cast<int>(joint.positions.size()), combined.size());
                  QVERIFY(qIsFinite(joint.left) && qIsFinite(joint.right) && joint.left < joint.right);
                  for (int i = 0; i < combined.size(); ++i) {
                        QCOMPARE(joint.positions[i].chord, static_cast<const Chord*>(combined[i]));
                        QCOMPARE(combined[i]->parent(), static_cast<Element*>(i < after.size() ? owner : next));
                        QVERIFY(qIsFinite(joint.positions[i].x));
                        if (i) QVERIFY(joint.positions[i].x > joint.positions[i - 1].x);
                        }
                  const SpacingPaddingTable4 table(score);
                  const Note* lastAfter = after.back()->notes().front();
                  const Note* firstBefore = next->graceNotesBefore().front()->notes().front();
                  SpacingPairPolicy4 policy;
                  QVERIFY(spacingMergedGracePairPolicy4(table, owner, next, lastAfter, firstBefore, score->spatium(), policy));
                  QVERIFY(policy.padding >= .3 * score->spatium());
                  const qreal boundaryDistance = spacingPairDistance4(lastAfter->bbox().translated(lastAfter->pos()),
                        firstBefore->bbox().translated(firstBefore->pos()), policy, score->spatium(), 1);
                  QVERIFY(joint.positions[after.size()].x - joint.positions[after.size() - 1].x + 1e-6 >= boundaryDistance);
                  QVERIFY(!spacingAfterGracePairPolicy4(table, owner, next, lastAfter, firstBefore, score->spatium(), policy));
                  QVERIFY(!spacingPairPolicy4(table, lastAfter, firstBefore, score->spatium(), policy));
                  QVERIFY(!spacingMergedGracePairPolicy4(table, owner, next,
                        owner->notes().front(), firstBefore, score->spatium(), policy));
                  // Native member x and segment intervals are not packing inputs.
                  std::vector<qreal> saved;
                  for (Chord* grace : combined) { saved.push_back(grace->x()); grace->rxpos() += 50 * score->spatium(); }
                  const QPointF oldTarget = target->pos();
                  target->rxpos() += 17 * score->spatium();
                  GraceMergedPlan4 repeated;
                  const bool repeatedBuilt = buildGraceMergedPlan4(owner, repeated, &reason);
                  target->setPos(oldTarget);
                  for (int i = 0; i < combined.size(); ++i) combined[i]->rxpos() = saved[i];
                  QVERIFY2(repeatedBuilt, qPrintable(reason));
                  QCOMPARE(static_cast<int>(repeated.positions.size()), static_cast<int>(joint.positions.size()));
                  for (size_t i = 0; i < joint.positions.size(); ++i)
                        QVERIFY(qAbs(repeated.positions[i].x - joint.positions[i].x) < 1e-6);
                  // Both owners' grace stems participate in eligibility.
                  for (Chord* grace : {after.front(), next->graceNotesBefore().front()}) {
                        const qreal userLength = grace->stem()->userLen();
                        grace->stem()->setUserLen(score->spatium());
                        GraceMergedPlan4 edited;
                        edited.left = 123;
                        const bool accepted = buildGraceMergedPlan4(owner, edited);
                        grace->stem()->setUserLen(userLength);
                        QVERIFY(!accepted);
                        QCOMPARE(edited.left, qreal(123));
                        QVERIFY(!edited.owner && !edited.target && edited.positions.empty());
                        }
                  }
            else {
                  QCOMPARE(joint.left, qreal(123));
                  QVERIFY(!joint.owner && !joint.target && joint.positions.empty());
                  }
            SpacingPlan4 system;
            system.width = 123;
            const bool systemBuilt = buildSpacingPlan4(m->system(), system, &reason);
            QVERIFY2(systemBuilt == (count != 4 && !natural), qPrintable(reason));
            if (systemBuilt) {
                  QCOMPARE(static_cast<int>(system.afterGraces.size()), count < 3 ? 1 : 0);
                  QCOMPARE(static_cast<int>(system.mergedGraces.size()), count == 3 || count == 5 ? 1 : 0);
                  }
            else {
                  QCOMPARE(system.width, qreal(123));
                  QVERIFY(system.measures.empty() && system.segments.empty() && system.afterGraces.empty() && system.mergedGraces.empty());
                  }
            }
      QCOMPARE(count, 6);
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

void TestSpacing::graceAfterRestStaticSqueeze_data()
      {
      graceBeforePlan_data();
      }

void TestSpacing::graceAfterRestStaticSqueeze()
      {
      QFETCH(bool, modernBeams);
      MasterScore* score = readScore(DIR + "v0.104-spacing-after-grace-rest-squeeze-match.mscx");
      QVERIFY(score && score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      Measure* measure = score->firstMeasure();
      QVERIFY(measure && !measure->nextMeasure() && !measure->lineBreak());
      QCOMPARE(measure->userStretch(), qreal(1));
      const auto placed = graceLayoutSnapshot4(score);
      QString reason;
      SpacingPlan4 natural, expected;
      QVERIFY2(buildSpacingPlan4(measure->system(), natural, &reason), qPrintable(reason));
      const qreal target = measure->system()->width();
      QVERIFY2(natural.width > target,
            qPrintable(QString("Matched rest fixture must squeeze: natural=%1 actual=%2")
                  .arg(natural.width, 0, 'g', 17).arg(target, 0, 'g', 17)));
      QVERIFY2(squeezeSpacingPlan4(measure->system(), target, expected, &reason), qPrintable(reason));
      QVERIFY(justifySpacingPlan4(expected, target));
      QCOMPARE(static_cast<int>(expected.afterGraces.size()), 1);
      for (const auto& e : expected.measures) {
            QVERIFY(qAbs(e.measure->x() - e.x) < 1e-6);
            QVERIFY(qAbs(e.measure->width() - e.width) < 1e-6);
            }
      for (const auto& e : expected.segments) {
            if (e.ignored) continue;
            QVERIFY(qAbs(e.segment->measure()->x() + e.segment->x() - e.x) < 1e-6);
            QVERIFY(qAbs(e.segment->width() - e.width) < 1e-6);
            }
      const GraceAfterPlan4& after = expected.afterGraces.front();
      QVERIFY(after.target->element(0) && after.target->element(0)->isRest());
      for (const auto& p : after.positions)
            QVERIFY(qAbs(after.owner->segment()->x() + after.owner->x() + p.chord->x() - after.target->x() - p.x) < 1e-6);
      // The v0.105 native probe found a quarter-space anchor shift on this
      // matched-input pair. Keep that result and the production selection covered.
      Beam* beam = after.positions.front().chord->beam();
      QVERIFY(beam);
      QVERIFY(beam->primarySegment4());
      const QLineF primary = *beam->primarySegment4();
      std::vector<qreal> stems;
      for (const auto& p : after.positions) stems.push_back(p.chord->stem()->len());
      GraceBeamAnchorProbe4 probe;
      QVERIFY(beam4ProbeGraceAnchors(beam, probe));
      QCOMPARE(static_cast<int>(probe.defaultStemLengths.size()), static_cast<int>(after.positions.size()));
      const qreal sp = score->spatium();
      QVERIFY(qAbs(probe.currentWidth / sp - .35) < 1e-6);
      QVERIFY(qAbs(probe.separatedWidth / sp - .5) < 1e-6);
      QVERIFY(qAbs((probe.separatedStart.y() - probe.currentStart.y()) / sp - .25) < 1e-6);
      QVERIFY(qAbs((probe.separatedEnd.y() - probe.currentEnd.y()) / sp - .25) < 1e-6);
      std::vector<ChordRest*> elements(beam->elements().begin(), beam->elements().end());
      QPointF start, end;
      qreal distance = beam->beamDist();
      QVERIFY(layoutBeam4(beam, elements, start, end, distance));
      QVERIFY(QLineF(start, probe.separatedStart).length() < 1e-6);
      QVERIFY(QLineF(end, probe.separatedEnd).length() < 1e-6);
      QVERIFY(qAbs(distance - beam->beamDist()) < 1e-6);
      if (modernBeams) {
            // Final segment placement must actually consume the nominal anchors.
            QVERIFY(qAbs(primary.dx()) > 1e-6);
            for (const QPointF& anchor : {start, end}) {
                  const qreal y = primary.y1() + (anchor.x() - primary.x1()) * primary.dy() / primary.dx()
                        + beam->pagePos().y();
                  QVERIFY(qAbs(y - anchor.y()) < 1e-6);
                  }
            }
      for (size_t j = 0; j < after.positions.size(); ++j)
            QCOMPARE(after.positions[j].chord->stem()->len(), stems[j]);
      QVERIFY(beam->primarySegment4());
      QCOMPARE(*beam->primarySegment4(), primary);
      GraceBeamAnchorProbe4 rejected;
      rejected.currentWidth = 123;
      QVERIFY(!beam4ProbeGraceAnchors(nullptr, rejected));
      QCOMPARE(rejected.currentWidth, qreal(123));
      QVERIFY(graceLayoutSnapshot4(score) == placed);
      delete score;
      }

void TestSpacing::graceAfterRest_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<QString>("file");
      for (const QString& file : {QString("v0.102-spacing-after-grace-rests.mscx"),
                                  QString("v0.102-spacing-after-grace-rests-natural.mscx")}) {
            for (bool beams : {false, true}) {
                  const QByteArray tag = (file + (beams ? "-studio" : "-legacy")).toLatin1();
                  QTest::newRow(tag.constData()) << beams << file;
                  }
            }
      }

void TestSpacing::graceAfterRest()
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
      const SpacingPaddingTable4 table(score);
      QString reason;
      int count = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++count) {
            QVERIFY(count < 4);
            if (file.contains("-natural.mscx")) {
                  QCOMPARE(m->system(), score->firstMeasure()->system());
                  QCOMPARE(m->system()->lastMeasure(), score->lastMeasure());
                  QVERIFY(!m->lineBreak());
                  }
            else {
                  QCOMPARE(m->system()->firstMeasure(), m);
                  QCOMPARE(m->system()->lastMeasure(), m);
                  }
            Segment* source = m->tick2segment(m->tick() + Fraction(1, 4), SegmentType::ChordRest);
            Segment* following = m->tick2segment(m->tick() + Fraction(1, 2), SegmentType::ChordRest);
            QVERIFY(source && following && source->element(0) && source->element(0)->isChord());
            QVERIFY(following->element(0) && following->element(0)->isRest());
            Chord* owner = toChord(source->element(0));
            Rest* rest = toRest(following->element(0));
            GraceAfterGeometry4 geometry;
            QVERIFY2(buildGraceAfterGeometry4(owner, geometry, &reason), qPrintable(reason));
            QCOMPARE(geometry.plan.target, static_cast<const Segment*>(following));
            QCOMPARE(static_cast<int>(geometry.plan.positions.size()), count == 2 ? 4 : count == 3 ? 1 : 2);
            int restCopies = 0;
            for (const auto& e : geometry.source.elements) QVERIFY(e.item != rest);
            for (const auto& e : geometry.target.elements) {
                  if (e.item == rest) {
                        ++restCopies;
                        QCOMPARE(e.rect, rest->bbox().translated(rest->pos()));
                        }
                  }
            QCOMPARE(restCopies, 1);
            for (const auto& p : geometry.plan.positions) {
                  const qreal actual = source->x() + owner->x() + p.chord->x() - following->x();
                  QVERIFY(qAbs(actual - p.x) < 1e-6);
                  SpacingPairPolicy4 policy;
                  QVERIFY(spacingAfterGracePairPolicy4(table, owner, rest, p.chord->notes().front(), rest,
                        p.chord->spatium(), policy));
                  QVERIFY(policy.padding >= .45 * score->spatium());
                  QVERIFY(!spacingPairPolicy4(table, p.chord->notes().front(), rest, p.chord->spatium(), policy));
                  }
            // Exercise the nominal anchor path on up/down and altered groups,
            // including the four-note contour. A query must not alter the beam.
            if (Beam* beam = geometry.plan.positions.front().chord->beam()) {
                  QVERIFY(beam->primarySegment4());
                  const QLineF primary = *beam->primarySegment4();
                  GraceBeamAnchorProbe4 probe;
                  QVERIFY(beam4ProbeGraceAnchors(beam, probe));
                  std::vector<ChordRest*> elements(beam->elements().begin(), beam->elements().end());
                  QPointF start, end;
                  qreal distance = beam->beamDist();
                  QVERIFY(layoutBeam4(beam, elements, start, end, distance));
                  QVERIFY(QLineF(start, probe.separatedStart).length() < 1e-6);
                  QVERIFY(QLineF(end, probe.separatedEnd).length() < 1e-6);
                  QCOMPARE(*beam->primarySegment4(), primary);
                  }
            // Every glyph/stem constraint, including owner-to-first grace,
            // must fit the actual source-to-target interval at natural spacing.
            qreal needed = 0;
            SpacingGraceContext4 context;
            context.afterMains.push_back(owner);
            QVERIFY(spacingShapeDistance4(table, geometry.source, geometry.target, owner->spatium(), 1, needed, &context));
            QVERIFY(following->x() - source->x() + 1e-6 >= needed);
            for (int mode = 0; mode < 4; ++mode) {
                  if (mode == 0) rest->setGap(true);
                  if (mode == 1) rest->setSmall(true);
                  if (mode == 2) rest->setVisible(false);
                  if (mode == 3) rest->setAutoplace(false);
                  GraceAfterPlan4 rejected;
                  rejected.left = 123;
                  const bool accepted = buildGraceAfterPlan4(owner, rejected, &reason);
                  if (mode == 0) rest->setGap(false);
                  if (mode == 1) rest->setSmall(false);
                  if (mode == 2) rest->setVisible(true);
                  if (mode == 3) rest->setAutoplace(true);
                  QVERIFY(!accepted);
                  QCOMPARE(rejected.left, qreal(123));
                  QVERIFY(rejected.positions.empty() && !rejected.owner && !rejected.target);
                  }
            QVERIFY(graceLayoutSnapshot4(score) == modern);
            if (m != m->system()->firstMeasure()) continue;
            SpacingPlan4 expected;
            QVERIFY2(buildSpacingPlan4(m->system(), expected, &reason), qPrintable(reason));
            QVERIFY(!expected.afterGraces.empty());
            QVERIFY(expected.width <= m->system()->width() + 1e-6);
            if (expected.width < m->system()->width() - 1e-6)
                  QVERIFY(justifySpacingPlan4(expected, m->system()->width()));
            for (const auto& e : expected.measures) {
                  QVERIFY(qAbs(e.measure->x() - e.x) < 1e-6);
                  QVERIFY(qAbs(e.measure->width() - e.width) < 1e-6);
                  }
            for (const auto& e : expected.segments) {
                  if (e.ignored) continue;
                  QVERIFY(qAbs(e.segment->measure()->x() + e.segment->x() - e.x) < 1e-6);
                  QVERIFY(qAbs(e.segment->width() - e.width) < 1e-6);
                  }
            }
      QCOMPARE(count, 4);
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

void TestSpacing::graceAfterBarlineSystem_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<QString>("file");
      for (const QString& file : {QString("v0.109-spacing-after-grace-barlines.mscx"),
                                  QString("v0.109-spacing-after-grace-barlines-natural.mscx")}) {
            for (bool beams : {false, true}) {
                  const QByteArray tag = (file + (beams ? "-studio" : "-legacy")).toLatin1();
                  QTest::newRow(tag.constData()) << beams << file;
                  }
            }
      }

void TestSpacing::graceAfterBarlineSystem()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score && score->styleB(Sid::spacingLayout4));
      // A plain closing measure keeps all three grace targets normal barlines.
      // The closing final barline has no after-graces and retains ordinary handling.
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = graceBarlineSnapshot4(score);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      const auto modern = graceBarlineSnapshot4(score);
      QString reason;
      int count = 0, groups = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++count) {
            QVERIFY(count < 4);
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
                  QCOMPARE(chord->graceNotesAfter().size(), count == 2 ? 4 : 2);
                  GraceAfterPlan4 group;
                  QVERIFY2(buildGraceAfterPlan4(chord, group, &reason), qPrintable(reason));
                  QVERIFY(group.target->isEndBarLineType());
                  QVERIFY(group.target->element(0) && group.target->element(0)->isBarLine());
                  GraceAfterGeometry4 geometry;
                  QVERIFY2(buildGraceAfterGeometry4(chord, geometry, &reason), qPrintable(reason));
                  const Element* barline = group.target->element(0);
                  int barlineCopies = 0;
                  for (const auto& e : geometry.source.elements) QVERIFY(e.item != barline);
                  for (const auto& e : geometry.target.elements) barlineCopies += e.item == barline;
                  QCOMPARE(barlineCopies, 1);
                  for (const auto& p : group.positions) {
                        int oldCopies = 0, newCopies = 0;
                        const Note* note = p.chord->notes().front();
                        for (const auto& e : geometry.source.elements) oldCopies += e.item == note;
                        for (const auto& e : geometry.target.elements) newCopies += e.item == note;
                        QCOMPARE(oldCopies, 0);
                        QCOMPARE(newCopies, 1);
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
            QCOMPARE(static_cast<int>(natural.afterGraces.size()),
                  file.contains("-natural.mscx") ? 3 : count < 3 ? 1 : 0);
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
            QVERIFY(graceBarlineSnapshot4(score) == modern);
            }
      QCOMPARE(count, 4);
      QCOMPARE(groups, 3);
      score->doLayout();
      const auto repeated = graceBarlineSnapshot4(score);
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto restored = graceBarlineSnapshot4(score);
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      }

void TestSpacing::graceMergedSlurBeamAnchors_data()
      {
      graceMergedSlurs_data();
      }

void TestSpacing::graceMergedSlurBeamAnchors()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, suffix);
      MasterScore* score = readScore(DIR + "v0.113-spacing-slurred-merged-" + suffix + ".mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      const auto original = graceMergedSnapshot4(score);
      int adapted = 0, preserved = 0;
      Slur* first = nullptr;
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            Slur* slur = toSlur(entry.second);
            Chord* start = toChord(slur->startElement());
            const Chord* target = toChord(slur->endElement());
            // Query both styles at the SAME published positions. Do not
            // conflate attachment differences with horizontal style toggling.
            score->setStyleValue(Sid::spacingLayout4, false);
            SlurPos legacy;
            slur->slurPos(&legacy);
            score->setStyleValue(Sid::spacingLayout4, true);
            SlurPos modern;
            slur->slurPos(&modern);
            QPointF a = legacy.p1, b = legacy.p2;
            QString reason;
            const bool accepted = adjustGraceMergedSlurBeamAnchors4(slur, a, b, &reason);
            // One read-only layout query shares validation across anchors and
            // obstacles, with identical physical components and atomic failure.
            {
                  GraceMergedSlurQuery4 query;
                  QPointF sharedA = legacy.p1, sharedB = legacy.p2;
                  QCOMPARE(adjustGraceMergedSlurBeamAnchors4(slur, sharedA, sharedB, nullptr, false, &query), accepted);
                  QCOMPARE(sharedA, a);
                  QCOMPARE(sharedB, b);
                  SpacingShape4 expectedObstacles, sharedObstacles;
                  const bool expected = buildGraceMergedSlurObstacles4(slur, expectedObstacles);
                  QCOMPARE(buildGraceMergedSlurObstacles4(slur, sharedObstacles, nullptr, false, &query), expected);
                  QCOMPARE(int(sharedObstacles.elements.size()), int(expectedObstacles.elements.size()));
                  for (size_t i = 0; i < expectedObstacles.elements.size(); ++i) {
                        const auto& x = expectedObstacles.elements[i];
                        const auto& y = sharedObstacles.elements[i];
                        QCOMPARE(y.rect, x.rect);
                        QCOMPARE(y.item, x.item);
                        QCOMPARE(int(y.identity), int(x.identity));
                        QVERIFY(y.aggregateOwners == x.aggregateOwners);
                        QVERIFY(y.sharedItems == x.sharedItems);
                        }
                  }
            QCOMPARE(accepted, start->up() == slur->up());
            QCOMPARE(a, modern.p1);
            QCOMPARE(b, modern.p2);
            if (!accepted) {
                  QCOMPARE(a, legacy.p1);
                  QCOMPARE(b, legacy.p2);
                  ++preserved;
                  continue;
                  }
            QVERIFY(reason.isEmpty());
            const qreal sp = score->spatium();
            const qreal sign = slur->up() ? -1 : 1;
            const QPointF startOrigin = start->pos() + start->parent()->pos()
                  + start->segment()->pos() + start->measure()->pos();
            const qreal startTipY = startOrigin.y() + start->stemPos().y() - start->pagePos().y()
                  + start->stem()->p2().y();
            // Measure the anchor's distance beyond the actual stem tip.
            // These fixtures have full-size members. Studio's intrinsic beam
            // magnitude is 1 even when the drawn grace beam is scaled to .7.
            for (const ChordRest* member : start->beam()->elements()) QVERIFY(!member->isSmall());
            const qreal sourceBeamTerm = score->styleS(Sid::beamWidth).val() * (sp / SPATIUM20) / 2;
            const qreal nominalHalfBeam = start->beam()->point(score->styleS(Sid::beamWidth)) / 2;
            const qreal sourceClearance = nominalHalfBeam + sourceBeamTerm + qMax(.35 * start->chordMag(), qreal(.2)) * sp;
            QVERIFY(qAbs(sign * (a.y() - startTipY) - sourceClearance) < 1e-6);
            QVERIFY(qAbs(a.y() - startTipY) < qAbs(legacy.p1.y() - startTipY));
            if (slur->up()) {
                  const QLineF* primary = start->beam()->primarySegment4();
                  QVERIFY(primary);
                  const qreal leadingEdge = start->beam()->pagePos().x() - start->measure()->system()->pagePos().x() + primary->x1();
                  const qreal capRadius = score->styleP(Sid::slurEndWidth) / 2;
                  QVERIFY2(qAbs(a.x() - capRadius - leadingEdge) < 1e-6,
                        qPrintable(QString("queried upper tip %1 versus final beam edge %2")
                              .arg(a.x() - capRadius, 0, 'g', 17).arg(leadingEdge, 0, 'g', 17)));
                  // A beam segment from an earlier preparation pass must not
                  // affect the query. Restore the cache before any assertion.
                  QLineF* cached = const_cast<QLineF*>(primary);
                  const QLineF savedBeam = *cached;
                  cached->translate(-3 * sp, 0);
                  QPointF cachedA = legacy.p1, cachedB = legacy.p2;
                  const bool cachedAccepted = adjustGraceMergedSlurBeamAnchors4(slur, cachedA, cachedB);
                  *cached = savedBeam;
                  QVERIFY(cachedAccepted);
                  QCOMPARE(cachedA, a);
                  QCOMPARE(cachedB, b);
                  // Likewise, moving the live first grace while leaving the
                  // beam cache alone must move the attachment exactly once.
                  const qreal savedX = start->x();
                  const qreal dx = .5 * sp;
                  start->rxpos() += dx;
                  QPointF movedA = legacy.p1, movedB = legacy.p2;
                  const bool movedAccepted = adjustGraceMergedSlurBeamAnchors4(slur, movedA, movedB);
                  start->rxpos() = savedX;
                  QVERIFY(movedAccepted);
                  QVERIFY(qAbs(movedA.x() - a.x() - dx) < 1e-6);
                  QVERIFY(qAbs(movedA.y() - a.y()) < 1e-6);
                  QCOMPARE(movedB, b);
                  }
            else {
                  const qreal startStemCenterX = startOrigin.x() + start->notes().front()->x()
                        + start->stemPosX() + sign * start->stem()->lineWidthMag() / 2;
                  QVERIFY(qAbs(a.x() - startStemCenterX - .15 * sp * start->chordMag()) < 1e-6);
                  }
            if (start->up() == target->up()) {
                  const QPointF endOrigin = target->pos() + target->segment()->pos() + target->measure()->pos();
                  const qreal endTipY = endOrigin.y() + target->stemPos().y() - target->pagePos().y()
                        + sign * target->stem()->len();
                  QVERIFY(qAbs(sign * (b.y() - endTipY) - .35 * sp) < 1e-6);
                  QVERIFY(qAbs(b.y() - endTipY) < qAbs(legacy.p2.y() - endTipY));
                  }
            else QCOMPARE(b, legacy.p2);
            if (!first) first = slur;
            ++adapted;
            }
      const int measures = suffix == "grace-squeeze" ? 1 : 4;
      QCOMPARE(adapted, measures);
      QCOMPARE(preserved, measures);
      QVERIFY(first);
      const QPointF sentinelA(11, 22), sentinelB(33, 44);
      QPointF a = sentinelA, b = sentinelB;
      const QPointF offset = first->offset();
      first->setOffset(QPointF(.5, 0));
      const bool edited = adjustGraceMergedSlurBeamAnchors4(first, a, b);
      first->setOffset(offset);
      QVERIFY(!edited);
      QCOMPARE(a, sentinelA);
      QCOMPARE(b, sentinelB);
      Element* anchor = first->startElement();
      Chord* start = toChord(anchor);
      first->setStartElement(start->beam()->elements().back());
      const bool last = adjustGraceMergedSlurBeamAnchors4(first, a, b);
      first->setStartElement(anchor);
      QVERIFY(!last);
      QCOMPARE(a, sentinelA);
      QCOMPARE(b, sentinelB);
      QVERIFY(!adjustGraceMergedSlurBeamAnchors4(nullptr, a, b));
      QCOMPARE(a, sentinelA);
      QCOMPARE(b, sentinelB);
      QVERIFY(graceMergedSnapshot4(score) == original);
      delete score;
      }

void TestSpacing::graceSlurCurveFit()
      {
      MasterScore* score = readScore(DIR + "v0.113-spacing-slurred-merged-grace-squeeze.mscx");
      QVERIFY(score);
      score->doLayout();
      Slur* slur = nullptr;
      for (const auto& entry : score->spannerMap().map()) if (entry.second->isSlur()) slur = toSlur(entry.second);
      QVERIFY(slur);
      const Chord* target = toChord(slur->endElement());
      const Note* note = toChord(slur->startElement())->notes().front();
      const auto nearPoint = [](const QPointF& a, const QPointF& b) { return (a - b).manhattanLength() < 1e-6; };
      for (bool up : {false, true}) {
            for (int floated = 0; floated < 4; ++floated) {
                  const qreal sign = up ? -1 : 1;
                  GraceSlurCurve4 curve;
                  curve.start = QPointF(0, 0);
                  curve.control1 = QPointF(3, .5 * sign);
                  curve.control2 = QPointF(7, .5 * sign);
                  curve.end = QPointF(10, 0);
                  const auto original = curve;
                  SpacingShape4 obstacles;
                  const QRectF obstacle = QRectF(QPointF(4, sign), QPointF(6, .4 * sign)).normalized();
                  obstacles.elements.push_back(SpacingShapeElement4(obstacle, note));
                  QString reason;
                  QVERIFY2(fitGraceSlurCurve4(obstacles, target, 1, up, curve, floated & 1, floated & 2, &reason), qPrintable(reason));
                  QVERIFY(reason.isEmpty());
                  const CubicBezier rendered(curve.start, curve.control1, curve.control2, curve.end);
                  QRectF padded = obstacle;
                  padded.adjust(0, up ? -.4 : 0, 0, up ? 0 : .4);
                  for (int sample = 0; sample <= 2000; ++sample) {
                        const QPointF p = rendered.pointAtPercent(sample / qreal(2000));
                        if (p.x() > padded.left() && p.x() < padded.right())
                              QVERIFY(up ? p.y() < padded.top() : p.y() > padded.bottom());
                        }
                  GraceSlurCurve4 translated = original;
                  const QPointF delta(15, -7);
                  translated.start += delta;
                  translated.control1 += delta;
                  translated.control2 += delta;
                  translated.end += delta;
                  QVERIFY(fitGraceSlurCurve4(obstacles.translated(delta), target, 1, up, translated, floated & 1, floated & 2));
                  QVERIFY(nearPoint(translated.start, curve.start + delta));
                  QVERIFY(nearPoint(translated.control1, curve.control1 + delta));
                  QVERIFY(nearPoint(translated.control2, curve.control2 + delta));
                  QVERIFY(nearPoint(translated.end, curve.end + delta));
                  SpacingShape4 impossible;
                  impossible.elements.push_back(SpacingShapeElement4(QRectF(0, -1000, 10, 2000), note));
                  auto rejected = original;
                  QVERIFY(!fitGraceSlurCurve4(impossible, target, 1, up, rejected));
                  QCOMPARE(rejected.start, original.start);
                  QCOMPARE(rejected.control1, original.control1);
                  QCOMPARE(rejected.control2, original.control2);
                  QCOMPARE(rejected.end, original.end);
                  QVERIFY(!fitGraceSlurCurve4(obstacles, target, 0, up, rejected));
                  QCOMPARE(rejected.control1, original.control1);
                  }
            }
      delete score;
      }

void TestSpacing::graceMergedSlurCurves_data()
      {
      graceMergedSlurs_data();
      }

void TestSpacing::graceMergedSlurCurves()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, suffix);
      MasterScore* score = readScore(DIR + "v0.113-spacing-slurred-merged-" + suffix + ".mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      const auto horizontal = graceMergedSnapshot4(score);
      const auto curves = [](Score* item) {
            std::vector<qreal> values;
            for (const auto& entry : item->spannerMap().map()) {
                  if (!entry.second->isSlur()) continue;
                  const Slur* slur = toSlur(entry.second);
                  values.push_back(slur->nsegments());
                  for (int i = 0; i < static_cast<int>(slur->nsegments()); ++i) {
                        const SlurSegment* segment = slur->segmentAt(i);
                        values.insert(values.end(), {segment->x(), segment->y(), qreal(segment->usesGraceCurve4())});
                        for (Grip grip : {Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END}) {
                              const QPointF p = segment->ups(grip).p;
                              const QPointF off = segment->ups(grip).off;
                              values.insert(values.end(), {p.x(), p.y(), off.x(), off.y()});
                              }
                        }
                  }
            return values;
            };
      const auto same = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i) if (qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      int count = 0;
      Slur* first = nullptr;
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            Slur* slur = toSlur(entry.second);
            QCOMPARE(static_cast<int>(slur->nsegments()), 1);
            const SlurSegment* segment = slur->frontSegment();
            QVERIFY(segment->usesGraceCurve4());
            QVERIFY(!segment->isEdited());
            const Chord* start = toChord(slur->startElement());
            if (slur->up() && start->up()) {
                  const QLineF* primary = start->beam()->primarySegment4();
                  QVERIFY(primary);
                  const qreal leadingEdge = start->beam()->pagePos().x() - start->measure()->system()->pagePos().x() + primary->x1();
                  // The fitter may lift the start vertically, but must retain
                  // the user's preferred visible horizontal edge alignment.
                  const qreal visibleTipX = segment->ups(Grip::START).pos().x() + segment->x()
                        - score->styleP(Sid::slurEndWidth) / 2;
                  QVERIFY2(qAbs(visibleTipX - leadingEdge) < 1e-6,
                        qPrintable(QString("rendered upper tip %1 versus final beam edge %2, tick %3")
                              .arg(visibleTipX, 0, 'g', 17).arg(leadingEdge, 0, 'g', 17).arg(slur->tick().ticks())));
                  }
            SpacingShape4 obstacles;
            QVERIFY(buildGraceMergedSlurObstacles4(slur, obstacles));
            const Chord* target = toChord(slur->endElement());
            const CubicBezier rendered(segment->ups(Grip::START).pos() + segment->pos(),
                  segment->ups(Grip::BEZIER1).pos() + segment->pos(),
                  segment->ups(Grip::BEZIER2).pos() + segment->pos(),
                  segment->ups(Grip::END).pos() + segment->pos());
            for (const auto& component : obstacles.elements) {
                  const Element* owner = component.item;
                  while (owner && !owner->isChord()) owner = owner->parent();
                  const qreal margin = owner == target ? 0 : (component.item->isNote() ? .4 : .1) * score->spatium();
                  QRectF padded = component.rect;
                  padded.adjust(0, slur->up() ? -margin : 0, 0, slur->up() ? 0 : margin);
                  for (int sample = 0; sample <= 1000; ++sample) {
                        const QPointF p = rendered.pointAtPercent(sample / qreal(1000));
                        if (p.x() > padded.left() && p.x() < padded.right())
                              QVERIFY2(slur->up() ? p.y() < padded.top() : p.y() > padded.bottom(),
                                    qPrintable(QString("curve clearance at tick %1, up %2, sample %3")
                                          .arg(slur->tick().ticks()).arg(slur->up()).arg(sample)));
                        }
                  }
            if (!first) first = slur;
            ++count;
            }
      QCOMPARE(count, suffix == "grace-squeeze" ? 2 : 8);
      const auto modern = curves(score);
      score->doLayout();
      QVERIFY(same(curves(score), modern));
      QVERIFY(same(graceMergedSnapshot4(score), horizontal));
      // Whole-slur movement preserves the modern base and note spacing.
      QVERIFY(first);
      const QPointF saved = first->offset();
      first->setOffset(QPointF(.5, 0));
      score->doLayout();
      QVERIFY(first->frontSegment()->usesGraceCurve4());
      QVERIFY(same(graceMergedSnapshot4(score), horizontal));
      first->setOffset(saved);
      score->doLayout();
      QVERIFY(same(curves(score), modern));
      // Reproduce keyboard/drag editing of the actual endpoint/control grips.
      // Every edit is relative to the same fitted automatic base, not a
      // legacy attachment. Repeated layout must not accumulate the edit.
      for (Grip grip : {Grip::START, Grip::END, Grip::BEZIER1, Grip::BEZIER2}) {
            SlurSegment* segment = first->frontSegment();
            const QPointF base = segment->ups(grip).pos();
            const QPointF old = segment->ups(grip).off;
            const QPointF delta(0, -.5 * score->spatium());
            segment->ups(grip).off = old + delta;
            segment->computeBezier(); // Direct editing preview, before full layout.
            QVERIFY((segment->ups(grip).pos() - base - delta).manhattanLength() < 1e-6);
            QVERIFY(same(graceMergedSnapshot4(score), horizontal));
            score->doLayout();
            segment = first->frontSegment();
            QVERIFY(segment->usesGraceCurve4());
            QCOMPARE(segment->ups(grip).off, old + delta);
            QVERIFY((segment->ups(grip).pos() - base - delta).manhattanLength() < 1e-6);
            QVERIFY(same(graceMergedSnapshot4(score), horizontal));
            SpacingPlan4 plan;
            QString reason;
            QVERIFY2(buildSpacingPlan4(toChord(first->endElement())->measure()->system(), plan, &reason), qPrintable(reason));
            QVERIFY(!plan.mergedGraces.empty());
            const auto edited = curves(score);
            score->doLayout();
            QVERIFY(same(curves(score), edited));
            QVERIFY(same(graceMergedSnapshot4(score), horizontal));
            first->frontSegment()->ups(grip).off = old;
            score->doLayout();
            QVERIFY(same(curves(score), modern));
            }
      // Exercise the user's left-node edit on every group and slur direction,
      // including altered M3 and the fitted upper M4 curve.
      std::vector<Slur*> allSlurs;
      for (const auto& entry : score->spannerMap().map()) if (entry.second->isSlur()) allSlurs.push_back(toSlur(entry.second));
      for (Slur* slur : allSlurs) {
            SlurSegment* segment = slur->frontSegment();
            const QPointF base = segment->ups(Grip::START).pos();
            const QPointF old = segment->ups(Grip::START).off;
            const QPointF delta(0, -.5 * score->spatium());
            segment->ups(Grip::START).off = old + delta;
            score->doLayout();
            segment = slur->frontSegment();
            QVERIFY(segment->usesGraceCurve4());
            QCOMPARE(segment->ups(Grip::START).off, old + delta);
            QVERIFY2((segment->ups(Grip::START).pos() - base - delta).manhattanLength() < 1e-6,
                  qPrintable(QString("manual start at tick %1: base (%2,%3), result (%4,%5), delta (%6,%7)")
                        .arg(slur->tick().ticks()).arg(base.x(), 0, 'g', 17).arg(base.y(), 0, 'g', 17)
                        .arg(segment->ups(Grip::START).pos().x(), 0, 'g', 17)
                        .arg(segment->ups(Grip::START).pos().y(), 0, 'g', 17)
                        .arg(delta.x(), 0, 'g', 17).arg(delta.y(), 0, 'g', 17)));
            QVERIFY(same(graceMergedSnapshot4(score), horizontal));
            segment->ups(Grip::START).off = old;
            score->doLayout();
            QVERIFY(same(curves(score), modern));
            }
      // Segment translation must translate the rendered base without making
      // the collision fitter compensate for the requested user displacement.
      SlurSegment* segment = first->frontSegment();
      const QPointF segmentOffset = segment->offset();
      const QPointF startBase = segment->ups(Grip::START).pos();
      segment->setOffset(segmentOffset + QPointF(.25, -.5) * score->spatium());
      score->doLayout();
      segment = first->frontSegment();
      QVERIFY(segment->usesGraceCurve4());
      QVERIFY((segment->ups(Grip::START).pos() - startBase).manhattanLength() < 1e-6);
      QVERIFY(same(graceMergedSnapshot4(score), horizontal));
      segment->setOffset(segmentOffset);
      score->doLayout();
      QVERIFY(same(curves(score), modern));
      // New layout-supplied raw anchors replace the prior editing frame.
      // Move the physical group, lay out this segment with fresh anchors, and
      // verify translation rather than reuse of an earlier frame's points.
      segment = first->frontSegment();
      std::vector<QPointF> basePoints, shiftedPoints;
      for (Grip grip : {Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END}) basePoints.push_back(segment->ups(grip).pos());
      Measure* measure = toChord(first->startElement())->measure();
      const QPointF savedMeasure = measure->pos();
      const QPointF shift(.5 * score->spatium(), .25 * score->spatium());
      measure->setPos(savedMeasure + shift);
      SlurPos shiftedAnchors;
      first->slurPos(&shiftedAnchors);
      segment->layoutSegment(shiftedAnchors.p1, shiftedAnchors.p2);
      const bool shiftedModern = segment->usesGraceCurve4();
      for (Grip grip : {Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END}) shiftedPoints.push_back(segment->ups(grip).pos());
      measure->setPos(savedMeasure);
      SlurPos restoredAnchors;
      first->slurPos(&restoredAnchors);
      segment->layoutSegment(restoredAnchors.p1, restoredAnchors.p2);
      QVERIFY(shiftedModern);
      for (size_t i = 0; i < basePoints.size(); ++i)
            QVERIFY((shiftedPoints[i] - basePoints[i] - shift).manhattanLength() < 1e-6);
      QVERIFY(same(curves(score), modern));
      QVERIFY(same(graceMergedSnapshot4(score), horizontal));
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      for (const auto& entry : score->spannerMap().map()) {
            if (entry.second->isSlur()) QVERIFY(!toSlur(entry.second)->frontSegment()->usesGraceCurve4());
            }
      const auto legacy = curves(score);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      QVERIFY(same(curves(score), modern));
      QVERIFY(same(graceMergedSnapshot4(score), horizontal));
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      QVERIFY(same(curves(score), legacy));
      delete score;
      }

// Later after-grace anchors must avoid the following group's physical music,
// even though compatibility spacing still recognizes only the first anchor.
void TestSpacing::graceMergedLaterSlurCurves4()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "v0.176-slur-beam-boundaries.mscx"));
      QVERIFY(score);
      score->doLayout();
      const auto notation = [&score]() {
            auto values = graceLayoutSnapshot4(score.get());
            const auto append = [&values](const Chord* chord) {
                  values.insert(values.end(), {chord->x(), chord->y(), qreal(chord->up()),
                        chord->stem() ? chord->stem()->len() : 0});
                  if (chord->beam() && chord->beam()->primarySegment4()) {
                        const QLineF& beam = *chord->beam()->primarySegment4();
                        values.insert(values.end(), {beam.x1(), beam.y1(), beam.x2(), beam.y2()});
                        }
                  };
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  for (const Segment& segment : m->segments()) {
                        for (const Element* item : segment.elist()) {
                              if (!item || !item->isChord()) continue;
                              const Chord* chord = toChord(item);
                              append(chord);
                              for (const Chord* grace : chord->graceNotes()) append(grace);
                              }
                        }
                  }
            return values;
            };
      const auto initialNotes = notation();
      const auto points = [](const SlurSegment* segment) {
            std::vector<QPointF> result {segment->pos()};
            for (Grip grip : {Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END})
                  result.push_back(segment->ups(grip).pos());
            return result;
            };
      const auto same = [](const std::vector<QPointF>& a, const std::vector<QPointF>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                  if ((a[i] - b[i]).manhattanLength() > 1e-6) return false;
            return true;
            };
      int upCount = 0, downCount = 0;
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            Slur* root = toSlur(entry.second);
            Chord* original = toChord(root->startElement());
            if (!original->isGraceAfter()) continue;
            Chord* owner = toChord(original->parent());
            const auto after = owner->graceNotesAfter();
            if (original == after.front()) continue; // The two G2 roots.
            Chord* target = toChord(root->endElement());
            QVERIFY(root->frontSegment()->usesGraceCurve4());
            const auto sibling = points(root->frontSegment());
            // Unobstructed reference spans: Studio's shoulders are one space
            // high over four spaces, and two over sixteen. Put the artificial
            // span left of the score so physical collisions cannot hide a
            // different initial-height rule. Exercise the real renderer in
            // both directions, then restore its actual published anchors.
            const qreal sp = score->spatium();
            const qreal sign = root->up() ? -1 : 1;
            const qreal reference[][4] = {{4, 1, 3, 1}, {16, 3.2, 12.8, 2}};
            for (const auto& shape : reference) {
                  const QPointF a(-20 * sp, sign * 12 * sp);
                  const QPointF b = a + QPointF(shape[0] * sp, 0);
                  SlurSegment* segment = root->frontSegment();
                  segment->layoutSegment(a, b);
                  const bool fitted = segment->usesGraceCurve4();
                  const auto actual = points(segment);
                  root->layout();
                  QVERIFY(fitted);
                  QVERIFY((actual[1] - a).manhattanLength() < 1e-6);
                  QVERIFY((actual[2] - a - QPointF(shape[1] * sp, sign * shape[3] * sp)).manhattanLength() < 1e-6);
                  QVERIFY((actual[3] - a - QPointF(shape[2] * sp, sign * shape[3] * sp)).manhattanLength() < 1e-6);
                  QVERIFY((actual[4] - b).manhattanLength() < 1e-6);
                  QVERIFY(same(points(root->frontSegment()), sibling));
                  QVERIFY(notation() == initialNotes);
                  }
            for (Chord* start : after) {
                  Slur probe(score.get());
                  SlurProbeRegistration4 registration(probe);
                  probe.setTrack(start->track());
                  probe.setTrack2(target->track());
                  probe.setTick(owner->tick());
                  probe.setTicks(target->tick() - owner->tick());
                  probe.setStartElement(start);
                  probe.setEndElement(target);
                  probe.setSlurDirection(root->slurDirection());
                  probe.layout();
                  SlurSegment* segment = probe.frontSegment();
                  QVERIFY2(segment && segment->usesGraceCurve4(),
                        qPrintable(QString("later grace curve, track %1, index %2")
                              .arg(start->track()).arg(start->graceIndex())));
                  QCOMPARE(spacingMergedGraceSlur4(&probe, owner, target), start == after.front());
                  SpacingShape4 obstacles;
                  QString reason;
                  QVERIFY2(buildGraceMergedSlurObstacles4(&probe, obstacles, &reason), qPrintable(reason));
                  const CubicBezier rendered(segment->ups(Grip::START).pos() + segment->pos(),
                        segment->ups(Grip::BEZIER1).pos() + segment->pos(),
                        segment->ups(Grip::BEZIER2).pos() + segment->pos(),
                        segment->ups(Grip::END).pos() + segment->pos());
                  bool beforeNoteSeen = false;
                  for (const auto& component : obstacles.elements) {
                        const Element* parent = component.item;
                        while (parent && !parent->isChord()) parent = parent->parent();
                        QVERIFY(parent && parent != start);
                        beforeNoteSeen = beforeNoteSeen || (component.item->isNote() && toChord(parent)->isGraceBefore());
                        const qreal margin = parent == target ? 0 : (component.item->isNote() ? .4 : .1) * score->spatium();
                        const QRectF padded = component.rect.adjusted(0, probe.up() ? -margin : 0,
                              0, probe.up() ? 0 : margin);
                        for (int sample = 0; sample <= 1000; ++sample) {
                              const QPointF p = rendered.pointAtPercent(sample / qreal(1000));
                              if (p.x() > padded.left() && p.x() < padded.right())
                                    QVERIFY2(probe.up() ? p.y() < padded.top() : p.y() > padded.bottom(),
                                          qPrintable(QString("later grace clearance: track %1, grace %2, sample %3")
                                                .arg(start->track()).arg(start->graceIndex()).arg(sample)));
                              }
                        }
                  QVERIFY(beforeNoteSeen);
                  const auto modern = points(segment);
                  const QPointF base = segment->ups(Grip::START).pos();
                  const QPointF delta(0, -.25 * score->spatium());
                  segment->ups(Grip::START).off = delta;
                  segment->computeBezier();
                  QVERIFY((segment->ups(Grip::START).pos() - base - delta).manhattanLength() < 1e-6);
                  probe.layout();
                  segment = probe.frontSegment();
                  QVERIFY(segment->usesGraceCurve4());
                  QCOMPARE(segment->ups(Grip::START).off, delta);
                  QVERIFY((segment->ups(Grip::START).pos() - base - delta).manhattanLength() < 1e-6);
                  segment->ups(Grip::START).off = QPointF();
                  probe.layout();
                  QVERIFY(same(points(probe.frontSegment()), modern));
                  score->setStyleValue(Sid::spacingLayout4, false);
                  probe.layout();
                  QVERIFY(!probe.frontSegment()->usesGraceCurve4());
                  const auto legacy = points(probe.frontSegment());
                  score->setStyleValue(Sid::spacingLayout4, true);
                  probe.layout();
                  QVERIFY(same(points(probe.frontSegment()), modern));
                  score->setStyleValue(Sid::spacingLayout4, false);
                  probe.layout();
                  QVERIFY(same(points(probe.frontSegment()), legacy));
                  score->setStyleValue(Sid::spacingLayout4, true);
                  QVERIFY(same(points(root->frontSegment()), sibling));
                  QVERIFY(notation() == initialNotes);
                  if (root->up()) ++upCount;
                  else ++downCount;
                  }
            }
      QCOMPARE(upCount, 4);
      QCOMPARE(downCount, 4);
      }

// Read-only collision input: no slur curve or published spacing changes yet.
void TestSpacing::graceMergedSlurObstacles_data()
      {
      graceMergedSlurs_data();
      }

void TestSpacing::graceMergedSlurObstacles()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, suffix);
      MasterScore* score = readScore(DIR + "v0.113-spacing-slurred-merged-" + suffix + ".mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      const auto original = graceMergedSnapshot4(score);
      int count = 0;
      Slur* first = nullptr;
      const auto closeRect = [](const QRectF& a, const QRectF& b) {
            return qAbs(a.x() - b.x()) < 1e-6 && qAbs(a.y() - b.y()) < 1e-6
                  && qAbs(a.width() - b.width()) < 1e-6 && qAbs(a.height() - b.height()) < 1e-6;
            };
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            Slur* slur = toSlur(entry.second);
            const Chord* start = toChord(slur->startElement());
            const Chord* owner = toChord(start->parent());
            const Chord* target = toChord(slur->endElement());
            SpacingShape4 obstacles;
            QString reason;
            QVERIFY2(buildGraceMergedSlurObstacles4(slur, obstacles, &reason), qPrintable(reason));
            QVERIFY(reason.isEmpty() && obstacles.complete() && !obstacles.elements.empty());
            std::vector<const Chord*> members;
            for (const Chord* chord : owner->graceNotesAfter()) if (chord != start) members.push_back(chord);
            for (const Chord* chord : target->graceNotesBefore()) members.push_back(chord);
            members.push_back(target);
            // Every intervening note survives, including those sharing an
            // endpoint's segment; the actual start anchor never survives.
            for (const Chord* chord : members) {
                  bool noteFound = false, accidentalFound = false;
                  const Note* note = chord->notes().front();
                  for (const auto& obstacle : obstacles.elements) {
                        noteFound = noteFound || obstacle.item == note;
                        accidentalFound = accidentalFound || obstacle.item == note->accidental();
                        }
                  QVERIFY(noteFound);
                  if (note->accidental() && note->accidental()->visible()) QVERIFY(accidentalFound);
                  }
            int notes = 0;
            for (const auto& obstacle : obstacles.elements) {
                  QVERIFY(obstacle.identity == SpacingIdentity4::ITEM && obstacle.item);
                  QVERIFY(obstacle.aggregateOwners.empty() && obstacle.sharedItems.empty());
                  QVERIFY(!obstacle.item->isLedgerLine());
                  const Element* parent = obstacle.item;
                  while (parent && !parent->isChord()) parent = parent->parent();
                  QVERIFY(parent && parent != start && parent != owner);
                  const Chord* chord = toChord(parent);
                  QVERIFY(std::find(members.begin(), members.end(), chord) != members.end());
                  if (obstacle.item->isNote()) ++notes;
                  // Verify against actual native local rectangles and parent
                  // chains, rather than the plan's temporary target offsets.
                  QPointF position = chord->pos() + chord->segment()->pos() + chord->measure()->pos();
                  if (chord->isGrace()) position += chord->parent()->pos();
                  const SpacingShape4 native = spacingElementShape4(chord);
                  bool found = false;
                  for (const auto& component : native.elements) {
                        if (component.item == obstacle.item && component.identity == SpacingIdentity4::ITEM
                              && closeRect(component.rect.translated(position), obstacle.rect)) found = true;
                        }
                  QVERIFY(found);
                  }
            QCOMPARE(notes, static_cast<int>(members.size()));
            if (!first) first = slur;
            ++count;
            }
      QCOMPARE(count, suffix == "grace-squeeze" ? 2 : 8);
      QVERIFY(first);
      QVERIFY(graceMergedSnapshot4(score) == original);
      Chord* start = toChord(first->startElement());
      Chord* owner = toChord(start->parent());
      Chord* target = toChord(first->endElement());
      Chord* moved = target->graceNotesBefore().front();
      SpacingShape4 before, after;
      QVERIFY(buildGraceMergedSlurObstacles4(first, before));
      const qreal dx = 0.75 * score->spatium();
      const qreal savedX = moved->x();
      moved->rxpos() += dx;
      const auto relocated = graceMergedSnapshot4(score);
      const bool movedAccepted = buildGraceMergedSlurObstacles4(first, after);
      const bool movedUnchanged = graceMergedSnapshot4(score) == relocated;
      moved->rxpos() = savedX;
      QVERIFY(movedAccepted && movedUnchanged);
      QCOMPARE(static_cast<int>(before.elements.size()), static_cast<int>(after.elements.size()));
      for (size_t i = 0; i < before.elements.size(); ++i) {
            const auto& old = before.elements[i];
            const auto& now = after.elements[i];
            QCOMPARE(now.item, old.item);
            const Element* parent = old.item;
            while (parent && !parent->isChord()) parent = parent->parent();
            QVERIFY(closeRect(now.rect, old.rect.translated(parent == moved ? QPointF(dx, 0) : QPointF())));
            }
      // Moving the measure tests translation of BOTH source-owned after-graces
      // and target-owned before-graces without changing their local offsets.
      Measure* measure = owner->measure();
      const QPointF savedPos = measure->pos();
      const QPointF delta(dx, 0.25 * score->spatium());
      measure->setPos(savedPos + delta);
      const bool translatedAccepted = buildGraceMergedSlurObstacles4(first, after);
      const bool measureUnchanged = measure->pos() == savedPos + delta;
      measure->setPos(savedPos);
      QVERIFY(translatedAccepted && measureUnchanged);
      QCOMPARE(static_cast<int>(before.elements.size()), static_cast<int>(after.elements.size()));
      for (size_t i = 0; i < before.elements.size(); ++i)
            QVERIFY(closeRect(after.elements[i].rect, before.elements[i].rect.translated(delta)));
      const QRectF sentinel(11, 22, 33, 44);
      SpacingShape4 rejected;
      rejected.elements.push_back(SpacingShapeElement4(sentinel, target));
      const QPointF savedOffset = first->offset();
      first->setOffset(QPointF(.5, 0));
      const bool editedAccepted = buildGraceMergedSlurObstacles4(first, rejected);
      first->setOffset(savedOffset);
      QVERIFY(!editedAccepted);
      QCOMPARE(static_cast<int>(rejected.elements.size()), 1);
      QCOMPARE(rejected.elements.front().rect, sentinel);
      QCOMPARE(rejected.elements.front().item, static_cast<const Element*>(target));
      const qreal savedLength = target->stem()->userLen();
      target->stem()->setUserLen(savedLength + score->spatium());
      const bool stemAccepted = buildGraceMergedSlurObstacles4(first, rejected);
      target->stem()->setUserLen(savedLength);
      QVERIFY(!stemAccepted);
      QCOMPARE(static_cast<int>(rejected.elements.size()), 1);
      QCOMPARE(rejected.elements.front().rect, sentinel);
      QVERIFY(!buildGraceMergedSlurObstacles4(nullptr, rejected));
      QVERIFY(graceMergedSnapshot4(score) == original);
      // Repeating the query is stable; enabling it has no rendering side effects.
      QVERIFY(buildGraceMergedSlurObstacles4(first, after));
      QCOMPARE(static_cast<int>(before.elements.size()), static_cast<int>(after.elements.size()));
      for (size_t i = 0; i < before.elements.size(); ++i) {
            QCOMPARE(after.elements[i].item, before.elements[i].item);
            QVERIFY(closeRect(after.elements[i].rect, before.elements[i].rect));
            }
      QVERIFY(graceMergedSnapshot4(score) == original);
      delete score;
      }

void TestSpacing::graceContextCachePolicies()
      {
      MasterScore* score = readScore(DIR + "v0.113-spacing-slurred-merged-graces-natural.mscx");
      QVERIFY(score);
      score->doLayout();
      Slur* selected = nullptr;
      const Chord* owner = nullptr;
      const Chord* target = nullptr;
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            const Slur* slur = toSlur(entry.second);
            const Chord* start = toChord(slur->startElement());
            if (!start || !start->isGraceAfter()) continue;
            owner = toChord(start->parent());
            target = toChord(slur->endElement());
            selected = toSlur(entry.second);
            break;
            }
      QVERIFY(owner && target);
      // The pair memo must preserve the full native shapes and local positions,
      // including both overlap and non-overlap. A new query sees geometry edits.
      Note* overlapNote = const_cast<Note*>(target->notes().front());
      const QPointF savedPosition = overlapNote->pos();
      const Element* overlapStem = target->stem();
      QVERIFY(overlapStem);
      bool sawOverlap = false, sawSeparation = false;
      for (qreal offset : {qreal(0), qreal(1000)}) {
            overlapNote->setPos(savedPosition + QPointF(offset, offset));
            SpacingContextCache4 query;
            for (const Element* a : {static_cast<const Element*>(overlapNote), overlapStem}) {
                  for (const Element* b : {static_cast<const Element*>(overlapNote), overlapStem}) {
                        const bool expected = a->shape().translated(a->pos()).intersects(b->shape().translated(b->pos()));
                        sawOverlap |= expected;
                        sawSeparation |= !expected;
                        for (int repeat = 0; repeat < 3; ++repeat)
                              QCOMPARE(query.shapesIntersect(a, b), expected);
                        }
                  }
            }
      overlapNote->setPos(savedPosition);
      QVERIFY(sawOverlap && sawSeparation);
      std::vector<const Element*> items {nullptr};
      for (const Chord* chord : {owner, target}) {
            items.push_back(chord->notes().front());
            items.push_back(chord->stem());
            for (const Chord* grace : chord->graceNotes()) {
                  items.push_back(grace->notes().front());
                  items.push_back(grace->stem());
                  if (grace->notes().front()->accidental()) items.push_back(grace->notes().front()->accidental());
                  }
            }
      // A component with an invalid track must still reject, even when its
      // owning chord already has a cached success from another component.
      {
            Stem invalid(score);
            invalid.setParent(const_cast<Chord*>(target));
            invalid.setTrack(-1);
            items.push_back(&invalid);
            SpacingPaddingTable4 table(score);
            SpacingContextCache4 cache;
            SpacingGraceContext4 joint;
            joint.mains.push_back(target);
            joint.afterMains.push_back(owner);
            joint.mergedPairs.push_back({owner, target});
            int accepted = 0, rejected = 0;
            for (int scope : {0, 1, 2, 3, 4, 5, 6, 0}) {
                  for (int repeat = 0; repeat < 2; ++repeat) {
                        for (const Element* a : items) {
                              for (const Element* b : items) {
                                    SpacingPairPolicy4 expected, actual;
                                    expected.padding = actual.padding = -123;
                                    expected.verticalClearance = actual.verticalClearance = -456;
                                    bool uncached, cached;
                                    if (scope == 0 || scope == 3) {
                                          uncached = spacingMergedGracePairPolicy4(table, owner, target, a, b, score->spatium(), expected, scope == 3);
                                          cached = spacingMergedGracePairPolicy4(table, owner, target, a, b, score->spatium(), actual, scope == 3, &cache);
                                          }
                                    else if (scope == 1) {
                                          uncached = spacingGracePairPolicy4(table, target, a, b, score->spatium(), expected);
                                          cached = spacingGracePairPolicy4(table, target, a, b, score->spatium(), actual, &cache);
                                          }
                                    else if (scope == 2) {
                                          uncached = spacingAfterGracePairPolicy4(table, owner, target, a, b, score->spatium(), expected);
                                          cached = spacingAfterGracePairPolicy4(table, owner, target, a, b, score->spatium(), actual, false, &cache);
                                          }
                                    else if (scope == 6) {
                                          // Switching roots must rebuild the prepared context and
                                          // invalidate any chord decisions from the original group.
                                          uncached = spacingMergedGracePairPolicy4(table, target, owner, a, b, score->spatium(), expected, true);
                                          cached = spacingMergedGracePairPolicy4(table, target, owner, a, b, score->spatium(), actual, true, &cache);
                                          }
                                    else {
                                          const SpacingGraceContext4* context = scope == 4 ? &joint : nullptr;
                                          uncached = spacingPairPolicy4(table, a, b, score->spatium(), expected, context);
                                          cached = spacingPairPolicy4(table, a, b, score->spatium(), actual, context, &cache);
                                          // Repeated components exercise the actual shape-distance cache.
                                          const QRectF rect(0, 0, 1, 1);
                                          SpacingShape4 left, right;
                                          for (int i = 0; i < 3; ++i) left.elements.push_back(SpacingShapeElement4(rect, a));
                                          for (int i = 0; i < 2; ++i) right.elements.push_back(SpacingShapeElement4(rect, b));
                                          qreal distance = 123;
                                          QCOMPARE(spacingShapeDistance4(table, left, right, score->spatium(), 1, distance, context), uncached);
                                          if (uncached)
                                                QCOMPARE(distance, spacingPairDistance4(rect, rect, expected, score->spatium(), 1));
                                          else
                                                QCOMPARE(distance, qreal(123));
                                          qreal sharedDistance = 123;
                                          QCOMPARE(spacingShapeDistance4(table, left, right, score->spatium(), 1, sharedDistance, context, &cache), uncached);
                                          QCOMPARE(sharedDistance, distance);
                                          SpacingPairPolicy4 self;
                                          const bool selfAccepted = spacingPairPolicy4(table, a, a, score->spatium(), self, context);
                                          SpacingShape4 projected;
                                          projected.elements.push_back(SpacingShapeElement4(QRectF(9, 9, 1, 1), target));
                                          QCOMPARE(spacingCollisionProjection4(table, left, score->spatium(), projected, context, &cache), selfAccepted);
                                          if (selfAccepted) {
                                                QCOMPARE(int(projected.elements.size()), 3);
                                                for (const auto& component : projected.elements) {
                                                      QCOMPARE(component.item, a);
                                                      QCOMPARE(component.rect, rect);
                                                      }
                                                }
                                          else {
                                                QCOMPARE(int(projected.elements.size()), 1);
                                                QCOMPARE(projected.elements.front().rect, QRectF(9, 9, 1, 1));
                                                }
                                          }
                                    QCOMPARE(cached, uncached);
                                    QCOMPARE(actual.padding, expected.padding);
                                    QCOMPARE(actual.verticalClearance, expected.verticalClearance);
                                    QCOMPARE(int(actual.kerning), int(expected.kerning));
                                    if (cached) ++accepted; else ++rejected;
                                    }
                              }
                        }
                  }
            QVERIFY(accepted > 0 && rejected > 0);
            // Existing graceMergedSlurs tests cover edits and third-slur rejection
            // through the complete planner with these local caches enabled.
            invalid.setParent(nullptr);
      }
      // Complete geometry queries must not retain eligibility into the next
      // edit. Rejection is atomic, and restoring the line restores the plan.
      GraceMergedGeometry4 baseline, rejected, restored;
      QVERIFY(buildGraceMergedGeometry4(owner, baseline));
      const int savedLine = selected->lineType();
      selected->setLineType(savedLine + 1);
      rejected.plan.owner = target;
      const bool admitted = buildGraceMergedGeometry4(owner, rejected);
      selected->setLineType(savedLine);
      QVERIFY(!admitted);
      QCOMPARE(rejected.plan.owner, target);
      QVERIFY(buildGraceMergedGeometry4(owner, restored));
      QCOMPARE(int(restored.plan.positions.size()), int(baseline.plan.positions.size()));
      for (size_t i = 0; i < baseline.plan.positions.size(); ++i) {
            QCOMPARE(restored.plan.positions[i].chord, baseline.plan.positions[i].chord);
            QCOMPARE(restored.plan.positions[i].x, baseline.plan.positions[i].x);
            }
      delete score;
      }

void TestSpacing::graceMergedSlurs_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<QString>("suffix");
      for (const QString& suffix : {QString("graces"), QString("graces-natural"), QString("grace-squeeze")}) {
            for (bool beams : {false, true}) {
                  const QByteArray tag = (suffix + (beams ? "-studio" : "-legacy")).toLatin1();
                  QTest::newRow(tag.constData()) << beams << suffix;
                  }
            }
      }

void TestSpacing::graceMergedSlurs()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, suffix);
      MasterScore* score = readScore(DIR + "v0.113-spacing-slurred-merged-" + suffix + ".mscx");
      MasterScore* plain = readScore(DIR + "v0.112-spacing-merged-" + suffix + ".mscx");
      QVERIFY(score && plain && score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      plain->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      plain->doLayout();
      // Compare published geometry, not the main chord's native spacing cache.
      // The plain target can use the standalone before adapter; paired slurs
      // deliberately require the merged adapter instead. Their _spaceLw values
      // therefore differ even though joint substitution replaces both bounds.
      const auto horizontal = [](Score* item) {
            std::vector<qreal> values;
            const System* previous = nullptr;
            int systemIndex = -1;
            for (Measure* m = item->firstMeasure(); m; m = m->nextMeasure()) {
                  if (m->system() != previous) { previous = m->system(); ++systemIndex; }
                  values.insert(values.end(), {qreal(systemIndex), m->x(), m->width()});
                  for (const Segment& seg : m->segments()) {
                        if (!seg.enabled() && (seg.header() || seg.trailer())) continue;
                        values.insert(values.end(), {qreal(static_cast<int>(seg.segmentType())), seg.tick().toDouble(),
                              seg.enabled() ? qreal(1) : qreal(0), seg.x(), seg.width(), seg.spacingWidthOffset4()});
                        for (const Element* e : seg.elist()) {
                              if (!e || !e->isChord()) continue;
                              const Chord* chord = toChord(e);
                              values.push_back(chord->x());
                              for (const Chord* grace : chord->graceNotes()) values.push_back(grace->x());
                              }
                        }
                  }
            return values;
            };
      int count = 0, up = 0, down = 0;
      Slur* first = nullptr;
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            Slur* slur = toSlur(entry.second);
            QVERIFY(slur->startElement() && slur->startElement()->isChord());
            QVERIFY(slur->endElement() && slur->endElement()->isChord());
            const Chord* start = toChord(slur->startElement());
            QVERIFY(start->isGraceAfter() && start->parent() && start->parent()->isChord());
            const Chord* owner = toChord(start->parent());
            const Chord* target = toChord(slur->endElement());
            QCOMPARE(start, static_cast<const Chord*>(owner->graceNotesAfter().front()));
            QVERIFY(spacingMergedGraceSlur4(slur, owner, target));
            GraceMergedGeometry4 geometry;
            QString reason;
            QVERIFY2(buildGraceMergedGeometry4(owner, geometry, &reason), qPrintable(reason));
            SpacingPlan4 plan;
            QVERIFY2(buildSpacingPlan4(owner->measure()->system(), plan, &reason), qPrintable(reason));
            QVERIFY(!plan.mergedGraces.empty());
            if (slur->slurDirection() == Direction::UP) ++up;
            if (slur->slurDirection() == Direction::DOWN) ++down;
            if (!first) first = slur;
            ++count;
            }
      const int measures = suffix == "grace-squeeze" ? 1 : 4;
      QCOMPARE(count, 2 * measures);
      QCOMPARE(up, measures);
      QCOMPARE(down, measures);
      QVERIFY(first);
      // Require the full-system adapter before comparing rendered positions.
      // Compare the replacement plan too: native cache differences must not
      // leak into its width or the combined groups' target-relative packing.
      Measure* plainMeasure = plain->firstMeasure();
      const System* previousSystem = nullptr;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), plainMeasure = plainMeasure->nextMeasure()) {
            QVERIFY(plainMeasure);
            if (m->system() == previousSystem) continue;
            previousSystem = m->system();
            SpacingPlan4 slurredPlan, plainPlan;
            QString reason;
            QVERIFY2(buildSpacingPlan4(m->system(), slurredPlan, &reason), qPrintable(reason));
            QVERIFY2(buildSpacingPlan4(plainMeasure->system(), plainPlan, &reason), qPrintable(reason));
            QVERIFY(qAbs(slurredPlan.width - plainPlan.width) < 1e-6);
            QCOMPARE(static_cast<int>(slurredPlan.mergedGraces.size()), static_cast<int>(plainPlan.mergedGraces.size()));
            for (size_t i = 0; i < plainPlan.mergedGraces.size(); ++i) {
                  const auto& actualGroup = slurredPlan.mergedGraces[i];
                  const auto& expectedGroup = plainPlan.mergedGraces[i];
                  QVERIFY(qAbs(actualGroup.left - expectedGroup.left) < 1e-6);
                  QVERIFY(qAbs(actualGroup.right - expectedGroup.right) < 1e-6);
                  QCOMPARE(static_cast<int>(actualGroup.positions.size()), static_cast<int>(expectedGroup.positions.size()));
                  for (size_t j = 0; j < expectedGroup.positions.size(); ++j)
                        QVERIFY(qAbs(actualGroup.positions[j].x - expectedGroup.positions[j].x) < 1e-6);
                  }
            }
      const auto expected = horizontal(plain);
      const auto actual = horizontal(score);
      QCOMPARE(static_cast<int>(actual.size()), static_cast<int>(expected.size()));
      for (size_t i = 0; i < expected.size(); ++i) {
            QVERIFY2(qAbs(actual[i] - expected[i]) < 1e-6,
                  qPrintable(QString("slurred/plain published horizontal component %1: %2 versus %3")
                        .arg(int(i)).arg(actual[i], 0, 'g', 17).arg(expected[i], 0, 'g', 17)));
            }

      const Chord* owner = toChord(first->startElement()->parent());
      const Chord* target = toChord(first->endElement());
      // A visual offset changes the curve, not horizontal eligibility.
      const QPointF offset = first->offset();
      first->setOffset(QPointF(.5, 0));
      GraceMergedPlan4 editedPlan;
      const bool accepted = buildGraceMergedPlan4(owner, editedPlan);
      const bool strict = spacingMergedGraceSlur4(first, owner, target);
      const bool spacing = spacingMergedGraceSlur4(first, owner, target, true);
      first->setOffset(offset);
      QVERIFY(accepted && !strict && spacing);
      QCOMPARE(editedPlan.owner, owner);
      QCOMPARE(editedPlan.target, target);
      QVERIFY(!editedPlan.positions.empty());
      // Three slurs still exceed this explicitly bounded structural scope.
      GraceMergedPlan4 rejected;
      rejected.left = 123;
      Slur* extra = new Slur(score);
      extra->setTrack(first->track());
      extra->setTrack2(first->track2());
      extra->setTick(first->tick());
      extra->setTick2(first->tick2());
      extra->setStartElement(first->startElement());
      extra->setEndElement(first->endElement());
      const bool extraEligible = spacingMergedGraceSlur4(extra, owner, target);
      score->addSpanner(extra);
      const bool thirdAccepted = buildGraceMergedPlan4(owner, rejected);
      score->removeSpanner(extra);
      delete extra;
      QVERIFY(extraEligible);
      QVERIFY(!thirdAccepted);
      QCOMPARE(rejected.left, qreal(123));
      QVERIFY(spacingMergedGraceSlur4(first, owner, target));
      QVERIFY(horizontal(score) == actual); // Queries/edit rejection leave positions unchanged.
      score->doLayout();
      const auto repeated = horizontal(score);
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(actual.size()));
      for (size_t i = 0; i < actual.size(); ++i) QVERIFY(qAbs(repeated[i] - actual[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = horizontal(score);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      const auto enabled = horizontal(score);
      QCOMPARE(static_cast<int>(enabled.size()), static_cast<int>(actual.size()));
      for (size_t i = 0; i < actual.size(); ++i) QVERIFY(qAbs(enabled[i] - actual[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto restored = horizontal(score);
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
      delete plain;
      }

void TestSpacing::graceMergedSystem_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<QString>("file");
      for (const QString& file : {QString("v0.112-spacing-merged-graces.mscx"),
                                  QString("v0.112-spacing-merged-graces-natural.mscx")}) {
            for (bool beams : {false, true}) {
                  const QByteArray tag = (file + (beams ? "-studio" : "-legacy")).toLatin1();
                  QTest::newRow(tag.constData()) << beams << file;
                  }
            }
      }

void TestSpacing::graceMergedSystem()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, file);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score && score->styleB(Sid::spacingLayout4));
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = graceMergedSnapshot4(score);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      const auto modern = graceMergedSnapshot4(score);
      QString reason;
      int count = 0, groups = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++count) {
            QVERIFY(count < 4);
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
                  QCOMPARE(chord->graceNotesAfter().size(), count % 2 ? 4 : 2);
                  GraceMergedPlan4 group;
                  QVERIFY2(buildGraceMergedPlan4(chord, group, &reason), qPrintable(reason));
                  for (const auto& p : group.positions) {
                        const Chord* parent = toChord(p.chord->parent());
                        QVERIFY(parent == chord || parent == group.target);
                        const qreal actual = parent->segment()->x() + parent->x() + p.chord->x() - group.target->segment()->x();
                        QVERIFY2(qAbs(actual - p.x) < 1e-6,
                              qPrintable(QString("AG%1 target-relative x actual=%2 expected=%3")
                                    .arg(count + 1).arg(actual, 0, 'g', 17).arg(p.x, 0, 'g', 17)));
                        }
                  }
            QCOMPARE(onsets, 3);
            if (m != m->system()->firstMeasure()) continue;
            SpacingPlan4 natural;
            QVERIFY2(buildSpacingPlan4(m->system(), natural, &reason), qPrintable(reason));
            QVERIFY(natural.afterGraces.empty());
            QCOMPARE(static_cast<int>(natural.mergedGraces.size()), file.contains("-natural.mscx") ? 4 : 1);
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
            QCOMPARE(static_cast<int>(compressed.mergedGraces.size()), static_cast<int>(natural.mergedGraces.size()));
            // The immediate preparation-to-plan handoff must match fresh
            // queries, including when only a suffix survived the last write.
            std::vector<GraceMergedGeometry4> readyMerged;
            for (const auto& group : natural.mergedGraces) {
                  GraceMergedGeometry4 ready;
                  QVERIFY(buildGraceMergedGeometry4(group.owner, ready));
                  readyMerged.push_back(std::move(ready));
                  }
            const auto samePlan = [](const SpacingPlan4& a, const SpacingPlan4& b) {
                  if (a.width != b.width || a.measures.size() != b.measures.size()
                      || a.segments.size() != b.segments.size() || a.afterGraces.size() != b.afterGraces.size()
                      || a.mergedGraces.size() != b.mergedGraces.size()) return false;
                  for (size_t i = 0; i < a.measures.size(); ++i) {
                        const auto& x = a.measures[i]; const auto& y = b.measures[i];
                        if (x.measure != y.measure || x.x != y.x || x.width != y.width) return false;
                        }
                  for (size_t i = 0; i < a.segments.size(); ++i) {
                        const auto& x = a.segments[i]; const auto& y = b.segments[i];
                        if (x.segment != y.segment || x.x != y.x || x.width != y.width
                            || x.widthOffset != y.widthOffset || x.stretch != y.stretch || x.ignored != y.ignored) return false;
                        }
                  for (size_t i = 0; i < a.mergedGraces.size(); ++i) {
                        const auto& x = a.mergedGraces[i]; const auto& y = b.mergedGraces[i];
                        if (x.owner != y.owner || x.target != y.target || x.left != y.left || x.right != y.right
                            || x.positions.size() != y.positions.size()) return false;
                        for (size_t j = 0; j < x.positions.size(); ++j) {
                              if (x.positions[j].chord != y.positions[j].chord || x.positions[j].x != y.positions[j].x) return false;
                              }
                        }
                  return true;
                  };
            for (int retained : {0, 1, 2}) {
                  auto handoff = readyMerged;
                  if (retained == 1) handoff.erase(handoff.begin());
                  if (retained == 2) handoff.clear();
                  SpacingPlan4 planned, squeezedExpected, squeezedActual;
                  QVERIFY(buildSpacingPlan4(m->system(), planned, &reason, SpacingPlanOptions4(), &handoff));
                  QVERIFY(samePlan(planned, natural));
                  QVERIFY(buildSpacingPlan4(m->system(), planned, &reason, options, &handoff));
                  QVERIFY(samePlan(planned, compressed));
                  squeezedExpected.width = squeezedActual.width = -123;
                  const bool fitted = squeezeSpacingPlan4(m->system(), compressed.width, squeezedExpected);
                  QCOMPARE(squeezeSpacingPlan4(m->system(), compressed.width, squeezedActual, nullptr, &handoff), fitted);
                  QVERIFY(samePlan(squeezedExpected, squeezedActual));
                  }
            QVERIFY(graceMergedSnapshot4(score) == modern);
            }
      QCOMPARE(count, 4);
      QCOMPARE(groups, 4);
      score->doLayout();
      const auto repeated = graceMergedSnapshot4(score);
      QCOMPARE(static_cast<int>(repeated.size()), static_cast<int>(modern.size()));
      for (size_t i = 0; i < modern.size(); ++i) QVERIFY(qAbs(repeated[i] - modern[i]) < 1e-6);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto restored = graceMergedSnapshot4(score);
      QCOMPARE(static_cast<int>(restored.size()), static_cast<int>(legacy.size()));
      for (size_t i = 0; i < legacy.size(); ++i) QVERIFY(qAbs(restored[i] - legacy[i]) < 1e-6);
      delete score;
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
      const auto belongsTo = [](const Element* item, const Element* chord) {
            for (; item; item = item->parent()) {
                  if (item == chord) return true;
                  if (item->isChord()) return false;
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
            if (count == 5) {
                  QVERIFY(!built);
                  QCOMPARE(geometry.plan.left, qreal(123));
                  QVERIFY(!geometry.plan.owner && !geometry.plan.target && geometry.plan.positions.empty());
                  QVERIFY(sameShape(geometry.source, sentinel) && sameShape(geometry.target, sentinel));
                  continue;
                  }
            QVERIFY2(built, qPrintable(reason));
            QVERIFY(geometry.source.complete() && geometry.target.complete());
            QCOMPARE(geometry.plan.owner, static_cast<const Chord*>(owner));
            const Element* target = geometry.plan.target->element(owner->track());
            for (const auto& e : geometry.source.elements) QVERIFY(belongsTo(e.item, owner));
            int followingCopies = 0;
            const Element* followingNote = target->isChord() ? static_cast<const Element*>(toChord(target)->notes().front()) : target;
            for (const auto& e : geometry.target.elements) {
                  if (e.item == followingNote) {
                        ++followingCopies;
                        QCOMPARE(e.rect, followingNote->bbox().translated(target->isChord() ? followingNote->pos() + target->pos() : target->pos()));
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
            Segment* targetSegment = m->tick2segment(owner->tick() + owner->actualTicks(), SegmentType::All);
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
            if (count == 5) {
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
            int captures = 0;
            SpacingGraceQueryFactory4 factory([&](const Chord* chord, SpacingAttachmentPreparation4& prepared, QString* why) {
                  ++captures;
                  return captureGraceFixture160(chord, prepared, why);
                  }, SpacingSlurPolicy4::COMPATIBILITY);
            {
            SpacingGraceQueryScope4 scope(score, &factory);
            QVERIFY2(applyGraceAfterPlan4(owner, &reason), qPrintable(reason));
            }
            QCOMPARE(captures, 1);
            QVERIFY(!score->graceQueryFactory4());
            QCOMPARE(m->width(), width);
            QVERIFY(graceLayoutSnapshot4(score) == native); // Idempotent local publication.
            Segment* target = m->tick2segment(owner->tick() + owner->actualTicks(), SegmentType::All);
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
            if (count == 5) {
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
            const Segment* targetSegment = m->tick2segment(owner->tick() + owner->actualTicks(), SegmentType::All);
            QVERIFY(targetSegment && targetSegment->element(0) && (targetSegment->element(0)->isChordRest() || targetSegment->element(0)->isBarLine()));
            Element* target = targetSegment->element(0);
            Element* targetPrimitive = target->isChord() ? static_cast<Element*>(toChord(target)->notes().front()) : target;
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
            QVERIFY(!spacingPairPolicy4(table, graces.front()->notes().front(), targetPrimitive,
                  graces.front()->spatium(), policy));
            QVERIFY(spacingAfterGracePairPolicy4(table, owner, target, graces.front()->notes().front(),
                  targetPrimitive, graces.front()->spatium(), policy));
            if (target->isBarLine()) {
                  const qreal expectedPadding = score->styleP(Sid::noteBarDistance)
                        * (graces.front()->notes().front()->mag() + target->mag()) / 2;
                  QVERIFY(qAbs(policy.padding - expectedPadding) < 1e-6);
                  }
            else QVERIFY(policy.padding >= .45 * score->spatium());
            QVERIFY(!spacingAfterGracePairPolicy4(table, owner, target, owner->notes().front(),
                  targetPrimitive, owner->spatium(), policy));
            // Target-relative output must not depend on stale native after-x,
            // owner x, or the unsolved source/target segment spacing.
            std::vector<QPointF> saved;
            for (Chord* grace : graces) { saved.push_back(grace->pos()); grace->rxpos() -= 100 * score->spatium(); }
            const QPointF ownerPos = owner->pos();
            const QPointF sourcePos = owner->segment()->pos();
            const QPointF targetPos = targetSegment->pos();
            owner->rxpos() += 11 * score->spatium();
            owner->segment()->rxpos() -= 17 * score->spatium();
            Segment* writableTarget = m->tick2segment(owner->tick() + owner->actualTicks(), SegmentType::All);
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
            std::vector<Note*> editedNotes { graces.front()->notes().front() };
            if (target->isChord()) editedNotes.push_back(toChord(target)->notes().front());
            for (Note* note : editedNotes) {
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
            const bool systemBuilt = buildSpacingPlan4(m->system(), systemPlan, &reason);
            QVERIFY2(systemBuilt, qPrintable(reason));
            QCOMPARE(static_cast<int>(systemPlan.afterGraces.size()), file.contains("-natural.mscx") ? 5 : 1);
            QCOMPARE(static_cast<int>(systemPlan.mergedGraces.size()), file.contains("-natural.mscx") ? 1 : 0);
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
            // Verify target semantics before checking optional system admission.
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
            QString reason;
            const bool built = buildSpacingPlan4(m->system(), rejected, &reason);
            QVERIFY2(built, qPrintable(reason));
            const bool combined = file.contains("-natural.mscx");
            QCOMPARE(static_cast<int>(rejected.afterGraces.size()), combined ? 5 : count < 5 ? 1 : 0);
            QCOMPARE(static_cast<int>(rejected.mergedGraces.size()), combined ? 1 : count == 5 ? 1 : 0);

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
                                  QString("v0.97-spacing-after-grace-squeeze.mscx"),
                                  QString("v0.102-spacing-after-grace-rest-squeeze.mscx"),
                                  QString("v0.104-spacing-after-grace-rest-squeeze-match.mscx"),
                                  QString("v0.109-spacing-after-grace-barline-squeeze.mscx"),
                                  QString("v0.112-spacing-merged-grace-squeeze.mscx"),
                                  QString("v0.113-spacing-slurred-merged-grace-squeeze.mscx")}) {
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
      const auto snapshot = [&]() {
            if (file.startsWith("v0.112-") || file.startsWith("v0.113-")) return graceMergedSnapshot4(score);
            return file.startsWith("v0.109-") ? graceBarlineSnapshot4(score) : graceLayoutSnapshot4(score);
            };
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->setStyleValue(Sid::pageWidth, 100.0);
      score->setStyleValue(Sid::pagePrintableWidth, 98.8);
      score->doLayout();
      Measure* measure = score->firstMeasure();
      QVERIFY(measure && !measure->nextMeasure());
      QCOMPARE(measure->userStretch(), (file.startsWith("v0.104-") || file.startsWith("v0.109-") || file.startsWith("v0.112-") || file.startsWith("v0.113-")) ? qreal(1) : qreal(2));
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
      const auto legacy = snapshot();
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
            if (s.isChordRestType() && s.element(0) && s.element(0)->isChord()
                && !toChord(s.element(0))->graceNotes().empty())
                  if (!main || !toChord(s.element(0))->graceNotesAfter().empty()) main = toChord(s.element(0));
            }
      QVERIFY(main);
      GraceMergedPlan4 joint;
      if (buildGraceMergedPlan4(main, joint, &reason)) {
            QCOMPARE(static_cast<int>(expected.mergedGraces.size()), 1);
            QVERIFY(expected.afterGraces.empty());
            for (const auto& p : joint.positions) {
                  const Chord* parent = toChord(p.chord->parent());
                  QVERIFY(qAbs(parent->segment()->x() + parent->x() + p.chord->x()
                        - joint.target->segment()->x() - p.x) < 1e-6);
                  }
            }
      else if (!main->graceNotesAfter().empty()) {
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
      const auto modern = snapshot();
      SpacingPlan4 rejected;
      rejected.width = 123;
      QVERIFY(!squeezeSpacingPlan4(measure->system(), 0, rejected, &reason));
      QCOMPARE(rejected.width, qreal(123));
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
      // Visual handle edits retain spacing; unresolved anchors still reject.
      Slur* first = slurs.front();
      Chord* main = toChord(first->endElement());
      SlurSegment* segment = first->frontSegment();
      const QPointF oldOffset = segment->ups(Grip::BEZIER1).off;
      segment->ups(Grip::BEZIER1).off = QPointF(.5, 0);
      QVERIFY(!spacingLocalGraceSlur4(first, main));
      QVERIFY(spacingLocalGraceSlur4(first, main, true));
      GraceBeforePlan4 editedGroup;
      QVERIFY(buildGraceBeforePlan4(main, editedGroup));
      SpacingPlan4 editedSystem;
      QVERIFY(buildSpacingPlan4(main->measure()->system(), editedSystem));
      QVERIFY(!editedGroup.positions.empty() && !editedSystem.segments.empty());
      segment->ups(Grip::BEZIER1).off = oldOffset;
      GraceBeforePlan4 rejectedGroup;
      rejectedGroup.left = 123;
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

void TestSpacing::slurOffsetUndoAdmission4()
      {
      MasterScore* score = readScore(DIR + "v0.139-spacing-two-staff-slur-offsets.mscx");
      QVERIFY(score);
      score->doLayout();
      SlurSegment* first = nullptr;
      SlurSegment* second = nullptr;
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            SlurSegment* segment = toSlur(entry.second)->frontSegment();
            if (!first) first = segment;
            else if (segment != first) { second = segment; break; }
            }
      QVERIFY(first && second);
      const auto addOffset = [first](UndoMacro& macro) {
            macro.appendChild(new ChangeProperty(first, Pid::SLUR_UOFF2, QPointF()));
            };
      {
      UndoMacro macro(score);
      QVERIFY(!macro.slurOffset4());
      addOffset(macro);
      macro.appendChild(new ChangeProperty(first, Pid::GENERATED, false));
      QCOMPARE(macro.slurOffset4(), static_cast<const SlurSegment*>(first));
      // Flattened same-segment key steps can retain the proof.
      macro.appendChild(new ChangeProperty(first, Pid::SLUR_UOFF3, QPointF()));
      QCOMPARE(macro.slurOffset4(), static_cast<const SlurSegment*>(first));
      }
      for (Pid property : {Pid::AUTOPLACE, Pid::SLUR_DIRECTION, Pid::SPANNER_TICKS, Pid::COLOR}) {
            UndoMacro macro(score);
            addOffset(macro);
            macro.appendChild(new ChangeProperty(first, property, first->getProperty(property)));
            QVERIFY(!macro.slurOffset4());
            }
      {
      UndoMacro macro(score);
      macro.appendChild(new ChangeProperty(first, Pid::GENERATED, false));
      QVERIFY(!macro.slurOffset4());
      addOffset(macro);
      macro.appendChild(new ChangeProperty(second, Pid::OFFSET, QPointF()));
      QVERIFY(!macro.slurOffset4());
      }
      {
      UndoMacro macro(score);
      auto* nested = new UndoMacro(score);
      addOffset(*nested);
      macro.appendChild(nested);
      QVERIFY(!macro.slurOffset4());
      }
      {
      // Subclasses cannot inherit a proof while overriding command execution.
      class DerivedProperty : public ChangeProperty {
         public:
            explicit DerivedProperty(SlurSegment* segment) : ChangeProperty(segment, Pid::OFFSET, QPointF()) {}
            };
      UndoMacro macro(score);
      macro.appendChild(new DerivedProperty(first));
      QVERIFY(!macro.slurOffset4());
      }
      {
      UndoMacro macro(score);
      auto* parent = new ChangeProperty(first, Pid::OFFSET, QPointF());
      parent->appendChild(new ChangeProperty(first, Pid::SLUR_UOFF1, QPointF()));
      macro.appendChild(parent);
      QVERIFY(!macro.slurOffset4());
      }
      delete score;
      }

void TestSpacing::mainStartSlurSpacing4_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<int>("startIndex");
      for (bool beams : {false, true})
            for (int index : {0, 1})
                  QTest::newRow(qPrintable(QString("beams-%1-start-%2").arg(beams).arg(index))) << beams << index;
      }

void TestSpacing::mainStartSlurSpacing4()
      {
      QFETCH(bool, modernBeams);
      QFETCH(int, startIndex);
      MasterScore* score = readScore(DIR + "v0.145-spacing-slur-anchor-systems.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      std::vector<Chord*> mains;
      for (const Segment& segment : score->firstMeasure()->segments()) {
            Element* item = segment.element(0);
            if (segment.isChordRestType() && item && item->isChord()) mains.push_back(toChord(item));
            }
      QCOMPARE(int(mains.size()), 3);
      const auto same = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                  if (!qIsFinite(a[i]) || !qIsFinite(b[i]) || qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      QString reason;
      const auto published = [score, &reason]() {
            for (System* system : score->systems()) {
                  if (system->vbox()) continue;
                  SpacingPlan4 plan;
                  if (!buildSpacingPlan4(system, plan, &reason)) return false;
                  if (system->lastMeasure() != score->lastMeasure()
                      && !justifySpacingPlan4(plan, system->width())) return false;
                  if (!spacingPlanMatches4(plan, &reason)) return false;
                  }
            return true;
            };
      QVERIFY2(published(), qPrintable(reason));
      const auto initial = graceLayoutSnapshot4(score);
      Slur* added = new Slur(score);
      added->setTrack(0);
      added->setTrack2(0);
      added->setTick(mains[startIndex]->tick());
      added->setTick2(mains[startIndex + 1]->tick());
      added->setStartElement(mains[startIndex]);
      added->setEndElement(mains[startIndex + 1]);
      added->setSlurDirection(Direction::AUTO);
      score->startCmd();
      score->undoAddElement(added);
      score->endCmd();
      QVERIFY2(published(), qPrintable(reason));
      const auto withAutomatic = graceLayoutSnapshot4(score);
      score->doLayout();
      QVERIFY(same(graceLayoutSnapshot4(score), withAutomatic));
      // Reproduce addition followed by X twice: the final direction becomes
      // explicit without changing the displayed side of the original AUTO slur.
      const bool automaticUp = added->up();
      for (int flip = 0; flip < 2; ++flip) {
            EditData edit;
            edit.key = Qt::Key_X;
            score->startCmd();
            QVERIFY(added->frontSegment()->edit(edit));
            score->endCmd();
            QVERIFY2(published(), qPrintable(reason));
            }
      QCOMPARE(added->up(), automaticUp);
      const auto explicitLayout = graceLayoutSnapshot4(score);
      score->doLayout();
      QVERIFY(same(graceLayoutSnapshot4(score), explicitLayout));
      // Visit grace endpoints without crossing the forced system boundary.
      // From A-to-C move forward into C's after-graces; from C-to-G move
      // backward into G's before-graces, reproducing both ownership cases.
      for (int step = 0; step < 2; ++step) {
            EditData edit;
            edit.key = startIndex == 0 ? Qt::Key_Right : Qt::Key_Left;
            edit.modifiers = Qt::ShiftModifier;
            edit.curGrip = Grip::END;
            score->startCmd();
            QVERIFY(added->frontSegment()->edit(edit));
            score->endCmd();
            QVERIFY(added->endElement()->isChord() && toChord(added->endElement())->isGrace());
            QVERIFY2(published(), qPrintable(reason));
            }
      const auto graceEndpoint = graceLayoutSnapshot4(score);
      score->doLayout();
      QVERIFY(same(graceLayoutSnapshot4(score), graceEndpoint));
      // Undo both moves, both flips and the addition, then redo addition alone.
      for (int command = 0; command < 5; ++command) score->undoRedo(true, nullptr);
      QVERIFY2(published(), qPrintable(reason));
      QVERIFY(same(graceLayoutSnapshot4(score), initial));
      score->undoRedo(false, nullptr);
      QVERIFY2(published(), qPrintable(reason));
      QVERIFY(same(graceLayoutSnapshot4(score), withAutomatic));
      delete score;
      }

void TestSpacing::beamEndSlurAnchors4()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "v0.113-spacing-slurred-merged-graces-natural.mscx"));
      QVERIFY(score);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      const auto notation = graceMergedSnapshot4(score.get());
      std::vector<Chord*> mains;
      std::vector<Beam*> beams;
      for (Measure* measure = score->firstMeasure(); measure; measure = measure->nextMeasure()) {
            for (const Segment& segment : measure->segments()) {
                  Element* item = segment.element(0);
                  if (!segment.isChordRestType() || !item || !item->isChord()) continue;
                  Chord* main = toChord(item);
                  mains.push_back(main);
                  for (Chord* grace : main->graceNotes()) {
                        Beam* beam = grace->beam();
                        if (beam && !beam->cross() && beam->elements().size() >= 2
                            && std::find(beams.begin(), beams.end(), beam) == beams.end()) beams.push_back(beam);
                        }
                  }
            }
      int startsUp = 0, startsDown = 0, endsUp = 0, endsDown = 0;
      const auto close = [](qreal a, qreal b) { return qIsFinite(a) && qIsFinite(b) && qAbs(a - b) < 1e-6; };
      for (Beam* beam : beams) {
            for (bool startEnd : {true, false}) {
                  ChordRest* member = startEnd ? beam->elements().back() : beam->elements().front();
                  QVERIFY(member->isChord());
                  Chord* anchor = toChord(member);
                  Chord* other = nullptr;
                  for (Chord* main : mains) {
                        if (main->beam() || !main->stem() || main->tremolo() || main->up() != anchor->up()) continue;
                        if ((startEnd && main->tick() > anchor->tick()) || (!startEnd && main->tick() < anchor->tick())) {
                              other = main;
                              break;
                              }
                        }
                  if (!other) continue;
                  Chord* start = startEnd ? anchor : other;
                  Chord* end = startEnd ? other : anchor;
                  Slur probe(score.get());
                  SlurProbeRegistration4 registration(probe);
                  probe.setTrack(start->track());
                  probe.setTrack2(end->track());
                  probe.setTick(start->tick());
                  probe.setTicks(end->tick() - start->tick());
                  probe.setStartElement(start);
                  probe.setEndElement(end);
                  const Direction direction = anchor->up() ? Direction::UP : Direction::DOWN;
                  probe.setSlurDirection(direction);
                  score->setStyleValue(Sid::spacingLayout4, false);
                  probe.layout();
                  SlurPos legacy;
                  probe.slurPos(&legacy);
                  score->setStyleValue(Sid::spacingLayout4, true);
                  probe.layout();
                  SlurPos modern;
                  probe.slurPos(&modern);
                  QVERIFY(probe.slurDirection() == direction);
                  QCOMPARE(probe.up(), anchor->up());
                  // Read the actual local stem tip; the source inset is on
                  // its note side, with distinct start/end grace scaling.
                  const QPointF stemTip = anchor->stemPos() - anchor->pagePos() + anchor->stem()->p2();
                  const qreal sign = anchor->up() ? -1 : 1;
                  qreal originY = anchor->y() + anchor->segment()->y() + anchor->measure()->y();
                  if (anchor->isGrace()) originY += anchor->parent()->y();
                  const qreal inset = -.5 * (startEnd ? anchor->chordMag() : qreal(1)) * score->spatium();
                  const QPointF actual = startEnd ? modern.p1 : modern.p2;
                  QVERIFY2(close(sign * (actual.y() - originY - stemTip.y()), inset),
                           "Beam boundary anchor does not use its own stem inset");
                  qreal originX = anchor->x() + anchor->segment()->x() + anchor->measure()->x()
                        + anchor->notes().front()->x();
                  if (anchor->isGrace()) originX += anchor->parent()->x();
                  const qreal offset = qMax(qreal(.35) * anchor->chordMag(), qreal(.2)) * score->spatium();
                  QVERIFY(close(actual.x(), originX + stemTip.x() + (startEnd ? offset : -offset)));
                  // A normal relayout must preserve endpoint user offsets.
                  SlurSegment* segment = startEnd ? probe.frontSegment() : probe.backSegment();
                  const Grip grip = startEnd ? Grip::START : Grip::END;
                  const QPointF userOffset(.2 * score->spatium(), -.25 * score->spatium());
                  segment->ups(grip).off = userOffset;
                  probe.layout();
                  segment = startEnd ? probe.frontSegment() : probe.backSegment();
                  QCOMPARE(segment->ups(grip).off, userOffset);
                  QVERIFY(probe.slurDirection() == direction);
                  score->setStyleValue(Sid::spacingLayout4, false);
                  SlurPos restored;
                  probe.slurPos(&restored);
                  QCOMPARE(restored.p1, legacy.p1);
                  QCOMPARE(restored.p2, legacy.p2);
                  if (startEnd) { if (anchor->up()) ++startsUp; else ++startsDown; }
                  else { if (anchor->up()) ++endsUp; else ++endsDown; }
                  }
            }
      QVERIFY(startsUp > 0 && startsDown > 0 && endsUp > 0 && endsDown > 0);
      score->setStyleValue(Sid::spacingLayout4, true);
      QVERIFY(graceMergedSnapshot4(score.get()) == notation);
      }

void TestSpacing::slurProbeLifetime4()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "v0.167-grace-policy-systems.mscx"));
      QVERIFY(score);
      score->doLayout();
      Chord* start = nullptr;
      Chord* end = nullptr;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
            for (const Segment& segment : m->segments()) {
                  Element* item = segment.element(0);
                  if (!segment.isChordRestType() || !item || !item->isChord()) continue;
                  if (!start) start = toChord(item);
                  end = toChord(item);
                  }
      QVERIFY(start && end && start->measure()->system() != end->measure()->system());
      std::vector<QList<SpannerSegment*>> before;
      for (System* system : score->systems()) before.push_back(system->spannerSegments());
      for (int repeat = 0; repeat < 8; ++repeat) {
            {
            Slur probe(score.get());
            SlurProbeRegistration4 registration(probe);
            probe.setTrack(0);
            probe.setTrack2(0);
            probe.setTick(start->tick());
            probe.setTicks(end->tick() - start->tick());
            probe.setStartElement(start);
            probe.setEndElement(end);
            probe.setSlurDirection(Direction::UP);
            probe.layout();
            QVERIFY(probe.nsegments() >= 2);
            for (SpannerSegment* segment : probe.spannerSegments()) {
                  QVERIFY(segment->system());
                  QVERIFY(segment->system()->spannerSegments().contains(segment));
                  }
            }
            size_t index = 0;
            for (System* system : score->systems())
                  QVERIFY(system->spannerSegments() == before[index++]);
            }
      // Exercise native cleanup after the probes have released their segments.
      score->doLayout();
      }

void TestSpacing::beamSlurAnchorHeights4_data()
      {
      QTest::addColumn<QString>("file");
      QTest::newRow("ordinary") << QString("studio-spacing-baseline.mscx");
      QTest::newRow("graces-up-down") << QString("v0.113-spacing-slurred-merged-graces-natural.mscx");
      }

void TestSpacing::beamSlurAnchorHeights4()
      {
      QFETCH(QString, file);
      std::unique_ptr<MasterScore> score(readScore(DIR + file));
      QVERIFY(score);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      const auto notation = graceMergedSnapshot4(score.get());
      std::vector<Beam*> beams;
      const auto include = [&beams](Chord* chord) {
            if (chord->beam() && std::find(beams.begin(), beams.end(), chord->beam()) == beams.end())
                  beams.push_back(chord->beam());
            };
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (const Segment& segment : m->segments()) {
                  for (Element* item : segment.elist()) {
                        if (!segment.isChordRestType() || !item || !item->isChord()) continue;
                        Chord* chord = toChord(item);
                        include(chord);
                        for (Chord* grace : chord->graceNotes()) include(grace);
                        }
                  }
            }
      int checked = 0;
      bool upSeen = false, downSeen = false, interiorSeen = false;
      for (Beam* beam : beams) {
            if (beam->cross() || beam->elements().size() < 2) continue;
            std::vector<Chord*> members;
            bool usable = true;
            for (ChordRest* member : beam->elements()) {
                  if (!member->isChord() || !toChord(member)->stem() || toChord(member)->tremolo()
                      || !member->staff()->isPitchedStaff(member->tick())
                      || (!members.empty() && member->up() != members.front()->up())) { usable = false; break; }
                  members.push_back(toChord(member));
                  }
            if (!usable) continue;
            // Full-size ordinary and grace members: the source's intrinsic
            // beam magnitude is 1, separately from grace draw magnification.
            for (const Chord* member : members) QVERIFY(!member->isSmall());
            Chord* end = members.back();
            const qreal sign = end->up() ? -1 : 1;
            const auto tip = [](Chord* chord) {
                  qreal y = chord->y() + chord->segment()->y() + chord->measure()->y();
                  if (chord->isGrace()) y += chord->parent()->y();
                  return y + chord->stemPos().y() - chord->pagePos().y() + chord->stem()->p2().y();
                  };
            for (size_t i = 0; i + 1 < members.size(); ++i) {
                  Chord* start = members[i];
                  Slur probe(score.get());
                  SlurProbeRegistration4 registration(probe);
                  probe.setTrack(start->track());
                  probe.setTrack2(end->track());
                  probe.setTick(start->tick());
                  probe.setTicks(end->tick() - start->tick());
                  probe.setStartElement(start);
                  probe.setEndElement(end);
                  probe.setSlurDirection(start->up() ? Direction::UP : Direction::DOWN);
                  // Both anchors lie within one beam, so the merged first-
                  // after-grace/main adapter cannot mask this general branch.
                  score->setStyleValue(Sid::spacingLayout4, false);
                  probe.layout();
                  SlurPos legacy;
                  probe.slurPos(&legacy);
                  score->setStyleValue(Sid::spacingLayout4, true);
                  SlurPos modern;
                  probe.slurPos(&modern);
                  const qreal nominalHalfBeam = beam->point(score->styleS(Sid::beamWidth)) / 2;
                  const qreal startClearance = nominalHalfBeam + score->styleS(Sid::beamWidth).val() * (score->spatium() / SPATIUM20) / 2
                        + qMax(qreal(.35) * start->chordMag(), qreal(.2)) * score->spatium();
                  const qreal endClearance = nominalHalfBeam + score->styleS(Sid::beamWidth).val() / 2
                        + qMax(qreal(.35) * end->chordMag(), qreal(.2)) * score->spatium();
                  QVERIFY(qAbs(sign * (modern.p1.y() - tip(start)) - startClearance) < 1e-6);
                  QVERIFY(qAbs(sign * (modern.p2.y() - tip(end)) - endClearance) < 1e-6);
                  QCOMPARE(modern.p1.x(), legacy.p1.x());
                  QCOMPARE(modern.p2.x(), legacy.p2.x());
                  // The source length includes manual length as well. Read
                  // the physical tip, not only Stem::len(), which omits it.
                  const qreal savedLength = start->stem()->userLen();
                  const qreal extension = .25 * score->spatium();
                  start->stem()->setUserLen(savedLength + extension);
                  start->stem()->layout();
                  SlurPos extended;
                  probe.slurPos(&extended);
                  start->stem()->setUserLen(savedLength);
                  start->stem()->layout();
                  QVERIFY(qAbs(sign * (extended.p1.y() - modern.p1.y()) - extension) < 1e-6);
                  QCOMPARE(extended.p1.x(), modern.p1.x());
                  QCOMPARE(extended.p2, modern.p2);
                  score->setStyleValue(Sid::spacingLayout4, false);
                  SlurPos restored;
                  probe.slurPos(&restored);
                  QCOMPARE(restored.p1, legacy.p1);
                  QCOMPARE(restored.p2, legacy.p2);
                  upSeen = upSeen || start->up();
                  downSeen = downSeen || !start->up();
                  interiorSeen = interiorSeen || i > 0;
                  ++checked;
                  }
            }
      QVERIFY(checked > 0 && interiorSeen);
      if (file.startsWith("v0.113")) QVERIFY(upSeen && downSeen);
      score->setStyleValue(Sid::spacingLayout4, true);
      QVERIFY(graceMergedSnapshot4(score.get()) == notation);
      }

void TestSpacing::continuedSlurStemAnchors4()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "v0.167-grace-policy-systems.mscx"));
      QVERIFY(score);
      score->doLayout();
      QVERIFY(score->firstMeasure()->system() != score->lastMeasure()->system());
      std::vector<Chord*> first, last;
      const auto collect = [](Measure* measure, std::vector<Chord*>& chords) {
            for (const Segment& segment : measure->segments()) {
                  Element* item = segment.element(0);
                  if (!segment.isChordRestType() || !item || !item->isChord()) continue;
                  Chord* main = toChord(item);
                  for (Chord* grace : main->graceNotesBefore()) chords.push_back(grace);
                  chords.push_back(main);
                  for (Chord* grace : main->graceNotesAfter()) chords.push_back(grace);
                  }
            };
      collect(score->firstMeasure(), first);
      collect(score->lastMeasure(), last);
      const auto same = [](const QPointF& a, const QPointF& b) {
            return qIsFinite(a.x()) && qIsFinite(a.y()) && qIsFinite(b.x()) && qIsFinite(b.y())
                  && qAbs(a.x() - b.x()) < 1e-6 && qAbs(a.y() - b.y()) < 1e-6;
            };
      const auto initialNotes = graceMergedSnapshot4(score.get());
      for (bool up : {false, true}) {
            Chord* fixedStart = nullptr;
            Chord* fixedEnd = nullptr;
            for (Chord* chord : first)
                  if (!fixedStart && !chord->isGrace() && !chord->beam() && chord->stem() && chord->up() == up)
                        fixedStart = chord;
            for (Chord* chord : last)
                  if (!chord->isGrace() && !chord->beam() && chord->stem() && chord->up() == up)
                        fixedEnd = chord;
            QVERIFY(fixedStart && fixedEnd);
            Slur continued(score.get());
            SlurProbeRegistration4 registration(continued);
            continued.setTrack(0);
            continued.setTrack2(0);
            continued.setSlurDirection(up ? Direction::UP : Direction::DOWN);
            const auto positions = [&](Chord* start, Chord* end) {
                  continued.setTick(start->tick());
                  continued.setTicks(end->tick() - start->tick());
                  continued.setStartElement(start);
                  continued.setEndElement(end);
                  continued.layout(); // Resolve the explicit side through the native entry point.
                  SlurPos pos;
                  continued.slurPos(&pos);
                  return pos;
                  };
            // Retain legacy coordinates for every probe, then verify that
            // toggling off restores them without changing published notation.
            score->setStyleValue(Sid::spacingLayout4, false);
            std::vector<SlurPos> legacyStarts, legacyEnds;
            for (Chord* start : first) legacyStarts.push_back(positions(start, fixedEnd));
            for (Chord* end : last) legacyEnds.push_back(positions(fixedStart, end));
            const SlurPos legacyLocal = positions(first.front(), first.back());
            score->setStyleValue(Sid::spacingLayout4, true);
            const SlurPos baseline = positions(fixedStart, fixedEnd);
            const QPointF visibleStart = continued.frontSegment()->pos() + continued.frontSegment()->ups(Grip::START).pos();
            const QPointF visibleEnd = continued.backSegment()->pos() + continued.backSegment()->ups(Grip::END).pos();
            for (Chord* start : first) {
                  const SlurPos current = positions(start, fixedEnd);
                  QVERIFY2(same(current.p2, baseline.p2), "Remote start changed the fixed end anchor");
                  QVERIFY2(same(continued.backSegment()->pos() + continued.backSegment()->ups(Grip::END).pos(), visibleEnd),
                           "Remote start moved the fixed rendered end");
                  }
            for (Chord* end : last) {
                  const SlurPos current = positions(fixedStart, end);
                  QVERIFY2(same(current.p1, baseline.p1), "Remote end changed the fixed start anchor");
                  QVERIFY2(same(continued.frontSegment()->pos() + continued.frontSegment()->ups(Grip::START).pos(), visibleStart),
                           "Remote end moved the fixed rendered start");
                  }
            const SlurPos modernLocal = positions(first.front(), first.back());
            QVERIFY(same(modernLocal.p1, legacyLocal.p1) && same(modernLocal.p2, legacyLocal.p2));
            score->setStyleValue(Sid::spacingLayout4, false);
            for (size_t i = 0; i < first.size(); ++i) {
                  const SlurPos current = positions(first[i], fixedEnd);
                  QVERIFY(same(current.p1, legacyStarts[i].p1) && same(current.p2, legacyStarts[i].p2));
                  }
            for (size_t i = 0; i < last.size(); ++i) {
                  const SlurPos current = positions(fixedStart, last[i]);
                  QVERIFY(same(current.p1, legacyEnds[i].p1) && same(current.p2, legacyEnds[i].p2));
                  }
            }
      score->setStyleValue(Sid::spacingLayout4, true);
      QVERIFY(graceMergedSnapshot4(score.get()) == initialNotes);
      }

void TestSpacing::graceSlurSiblingAnchors4()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "v0.167-grace-policy-systems.mscx"));
      QVERIFY(score);
      score->doLayout();
      Slur* upper = nullptr;
      Slur* lower = nullptr;
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            Slur* root = toSlur(entry.second);
            const Element* start = root->startElement();
            if (root->staffIdx() || !start || !start->isChord()
                || !toChord(start)->isGraceAfter() || toChord(start)->measure() != score->lastMeasure()) continue;
            if (root->slurDirection() == Direction::UP) upper = root;
            if (root->slurDirection() == Direction::DOWN) lower = root;
            }
      QVERIFY(upper && lower && upper->startElement() == lower->startElement());
      Chord* originalStart = toChord(upper->startElement());
      Element* originalEnd = upper->endElement();
      Chord* owner = toChord(originalStart->parent());
      Chord* target = toChord(lower->endElement());
      QCOMPARE(originalEnd, static_cast<Element*>(target));
      const Fraction originalTick = upper->tick();
      const Fraction originalEndTick = upper->tick2();
      QVERIFY(owner->graceNotesAfter().size() >= 4 && target->graceNotesBefore().size() >= 2);
      Chord* previousSystem = nullptr;
      for (const Segment& segment : score->firstMeasure()->segments()) {
            Element* item = segment.element(owner->track());
            if (segment.isChordRestType() && item && item->isChord()) previousSystem = toChord(item);
            }
      QVERIFY(previousSystem && previousSystem->measure()->system() != owner->measure()->system());
      const auto curve = [lower]() {
            std::vector<qreal> values;
            values.push_back(qreal(lower->nsegments()));
            for (const SpannerSegment* item : lower->spannerSegments()) {
                  const SlurSegment* segment = toSlurSegment(item);
                  values.insert(values.end(), {segment->x(), segment->y()});
                  for (Grip grip : {Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END}) {
                        const QPointF point = segment->ups(grip).pos();
                        values.insert(values.end(), {point.x(), point.y()});
                        }
                  for (const QRectF& rect : segment->shape())
                        values.insert(values.end(), {rect.x(), rect.y(), rect.width(), rect.height()});
                  }
            return values;
            };
      const auto same = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                  if (!qIsFinite(a[i]) || !qIsFinite(b[i]) || qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      QString reason;
      SpacingShape4 initialObstacles;
      QVERIFY2(buildGraceMergedSlurObstacles4(lower, initialObstacles, &reason), qPrintable(reason));
      const auto initialNotes = graceLayoutSnapshot4(score.get());
      const auto initialCurve = curve();
      // Hold published notation fixed to isolate curve admission from any
      // legitimate horizontal response to an endpoint-side constraint change.
      // Include a stale cross-system render cache and both beam boundaries.
      const std::vector<Chord*> anchors {previousSystem, owner, owner->graceNotesAfter()[1],
            owner->graceNotesAfter().back(), target->graceNotesBefore().front(),
            target->graceNotesBefore().back(), originalStart};
      for (Chord* start : anchors) {
            upper->setStartElement(start);
            upper->setTick(start->tick());
            upper->setTicks(originalEndTick - start->tick());
            {
                  GraceMergedSlurQuery4 query;
                  const GraceMergedGeometry4* geometry = query.geometry(owner, &reason);
                  QVERIFY2(geometry, qPrintable(reason));
                  QVERIFY(geometry->plan.slurPolicy == SpacingSlurPolicy4::PREPARED);
                  SpacingShape4 obstacles;
                  QVERIFY2(buildGraceMergedSlurObstacles4(lower, obstacles, &reason, false, &query), qPrintable(reason));
                  QCOMPARE(int(obstacles.elements.size()), int(initialObstacles.elements.size()));
                  for (size_t i = 0; i < obstacles.elements.size(); ++i) {
                        QCOMPARE(obstacles.elements[i].item, initialObstacles.elements[i].item);
                        const QRectF a = obstacles.elements[i].rect;
                        const QRectF b = initialObstacles.elements[i].rect;
                        QVERIFY(qAbs(a.x() - b.x()) < 1e-6 && qAbs(a.y() - b.y()) < 1e-6
                              && qAbs(a.width() - b.width()) < 1e-6 && qAbs(a.height() - b.height()) < 1e-6);
                        }
                  }
            lower->layout();
            QVERIFY2(same(curve(), initialCurve), "Unedited lower slur changed at fixed published note positions");
            QVERIFY(same(graceLayoutSnapshot4(score.get()), initialNotes));
            }
      upper->setTick(originalTick);
      upper->setTicks(originalEndTick - originalTick);
      // Exercise native command completion, fresh layout and history as well.
      // Reanchoring across systems can change spacing legitimately; require
      // incremental and fresh results to agree rather than freezing the plan.
      score->startCmd();
      score->undoStack()->push1(new ChangeStartEndSpanner(upper, upper->startElement(), upper->endElement()));
      upper->undoChangeProperty(Pid::SPANNER_TICK, previousSystem->tick());
      upper->undoChangeProperty(Pid::SPANNER_TICKS, originalEndTick - previousSystem->tick());
      // Both property setters invalidate endElement(). Preserve its identity
      // from before those mutations, as native changeAnchor does with ecr.
      score->undo(new ChangeSpannerElements(upper, previousSystem, originalEnd));
      QCOMPARE(upper->startElement(), static_cast<Element*>(previousSystem));
      QCOMPARE(upper->endElement(), originalEnd);
      QVERIFY(upper->tick2() == originalEndTick);
      score->endCmd();
      QCOMPARE(upper->startElement(), static_cast<Element*>(previousSystem));
      QCOMPARE(upper->endElement(), originalEnd);
      QVERIFY(upper->tick2() == originalEndTick);
      const auto changedNotes = graceLayoutSnapshot4(score.get());
      const auto changedCurve = curve();
      score->doLayout();
      QVERIFY2(same(graceLayoutSnapshot4(score.get()), changedNotes), "Cross-system command versus fresh note layout differs");
      QVERIFY2(same(curve(), changedCurve), "Cross-system command versus fresh lower curve differs");
      score->undoRedo(true, nullptr);
      QCOMPARE(upper->startElement(), static_cast<Element*>(originalStart));
      QVERIFY(same(graceLayoutSnapshot4(score.get()), initialNotes));
      QVERIFY(same(curve(), initialCurve));
      score->undoRedo(false, nullptr);
      QCOMPARE(upper->startElement(), static_cast<Element*>(previousSystem));
      QVERIFY2(same(graceLayoutSnapshot4(score.get()), changedNotes), "Cross-system redo note layout differs");
      QVERIFY2(same(curve(), changedCurve), "Cross-system redo lower curve differs");
      }

void TestSpacing::slurAnchorSystems4_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<int>("editedStaff");
      QTest::addColumn<bool>("upSlur");
      // Pairwise coverage of all three axes. The single-step test retains
      // the full eight-row matrix; repeating every traversal is expensive.
      for (bool beams : {false, true})
            for (int staff : {0, 1}) {
                  const bool up = beams != bool(staff);
                  QTest::newRow(qPrintable(QString("beams-%1-staff-%2-slur-%3").arg(beams).arg(staff).arg(up)))
                        << beams << staff << up;
                  }
      }

void TestSpacing::slurAnchorSystems4()
      {
      QFETCH(bool, modernBeams);
      QFETCH(int, editedStaff);
      QFETCH(bool, upSlur);
      MasterScore* score = readScore(DIR + "v0.145-spacing-slur-anchor-systems.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      QVERIFY(score->firstMeasure()->system() != score->lastMeasure()->system());
      Slur* selected = nullptr;
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            Slur* slur = toSlur(entry.second);
            if (slur->staffIdx() == editedStaff && slur->startElement() && slur->startElement()->isChord()
                && toChord(slur->startElement())->measure() == score->firstMeasure()
                && slur->slurDirection() == (upSlur ? Direction::UP : Direction::DOWN)) selected = slur;
            }
      QVERIFY(selected);
      QCOMPARE(int(selected->nsegments()), 1);
      Element* originalEnd = selected->endElement();
      std::vector<Chord*> targets;
      for (const Segment& s : score->lastMeasure()->segments()) {
            Element* item = s.element(editedStaff * VOICES);
            if (!s.isChordRestType() || !item || !item->isChord()) continue;
            Chord* main = toChord(item);
            for (Chord* grace : main->graceNotesBefore()) targets.push_back(grace);
            targets.push_back(main);
            for (Chord* grace : main->graceNotesAfter()) targets.push_back(grace);
            }
      QVERIFY(targets.size() > 3);
      const auto same = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                  if (!qIsFinite(a[i]) || !qIsFinite(b[i]) || qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      QString reason;
      const auto published = [score, &reason]() {
            reason.clear();
            const auto equal = [&reason](qreal actual, qreal expected, const QString& component) {
                  if (qIsFinite(actual) && qIsFinite(expected) && qAbs(actual - expected) < 1e-6) return true;
                  reason = QString("Cross-system %1: published %2, modern plan %3").arg(component)
                        .arg(actual, 0, 'g', 17).arg(expected, 0, 'g', 17);
                  return false;
                  };
            for (System* system : score->systems()) {
                  if (system->vbox()) continue;
                  SpacingPlan4 plan;
                  if (!buildSpacingPlan4(system, plan, &reason)) return false;
                  if (system->lastMeasure() != score->lastMeasure()
                      && !justifySpacingPlan4(plan, system->width())) return false;
                  if (!equal(system->width(), plan.width, "system width")) return false;
                  for (const auto& m : plan.measures)
                        if (!equal(m.measure->x(), m.x, "measure x")
                            || !equal(m.measure->width(), m.width, "measure width")) return false;
                  for (const auto& s : plan.segments) {
                        if (s.ignored) continue;
                        if (!equal(s.segment->measure()->x() + s.segment->x(), s.x, "segment x")
                            || !equal(s.segment->width(), s.width, "segment width")) return false;
                        }
                  for (const auto& group : plan.mergedGraces) {
                        auto members = group.owner->graceNotesAfter();
                        for (Chord* grace : group.target->graceNotesBefore()) members.push_back(grace);
                        if (members.size() != int(group.positions.size())) return false;
                        for (int i = 0; i < members.size(); ++i) {
                              const Chord* parent = toChord(members[i]->parent());
                              const qreal actual = members[i]->x() + parent->segment()->x() + parent->x()
                                    - group.target->segment()->x();
                              if (!equal(actual, group.positions[i].x, "merged grace x")) return false;
                              }
                        }
                  }
            return true;
            };
      QVERIFY2(published(), qPrintable(reason));
      const auto curve = [selected]() {
            std::vector<qreal> values;
            values.push_back(qreal(selected->nsegments()));
            for (const SpannerSegment* item : selected->spannerSegments()) {
                  const SlurSegment* segment = toSlurSegment(item);
                  values.insert(values.end(), {segment->x(), segment->y()});
                  for (Grip grip : {Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END}) {
                        const QPointF point = segment->ups(grip).pos();
                        values.insert(values.end(), {point.x(), point.y()});
                        }
                  for (const QRectF& rect : segment->shape())
                        values.insert(values.end(), {rect.x(), rect.y(), rect.width(), rect.height()});
                  }
            return values;
            };
      const auto initial = graceLayoutSnapshot4(score);
      const auto initialCurve = curve();
      for (Chord* target : targets) {
            Element* previous = selected->endElement();
            const auto before = graceLayoutSnapshot4(score);
            // Apply the same undoable anchor mutations as changeAnchor.
            // Cross-system grip reselection needs a GUI view, absent here.
            score->startCmd();
            // changeAnchor records this BEFORE properties invalidate endpoints.
            // It restores exact grace anchors after the tick command is undone.
            score->undoStack()->push1(new ChangeStartEndSpanner(selected,
                  selected->startElement(), selected->endElement()));
            selected->undoChangeProperty(Pid::SPANNER_TICKS, target->tick() - selected->tick());
            score->undo(new ChangeSpannerElements(selected, selected->startElement(), target));
            score->endCmd();
            QCOMPARE(selected->endElement(), static_cast<Element*>(target));
            QCOMPARE(int(selected->nsegments()), 2);
            QVERIFY(!score->undoStack()->slurOffset4(true));
            QVERIFY2(published(), qPrintable(reason));
            const auto changed = graceLayoutSnapshot4(score);
            score->doLayout();
            QVERIFY(same(graceLayoutSnapshot4(score), changed));
            QVERIFY2(published(), qPrintable(reason));
            score->undoRedo(true, nullptr);
            QCOMPARE(selected->endElement(), previous);
            QVERIFY2(same(graceLayoutSnapshot4(score), before), "Cross-system undo geometry differs from the preceding state");
            QVERIFY2(published(), qPrintable(reason));
            score->undoRedo(false, nullptr);
            QCOMPARE(selected->endElement(), static_cast<Element*>(target));
            QVERIFY(same(graceLayoutSnapshot4(score), changed));
            QVERIFY2(published(), qPrintable(reason));
            }
      // A return to the original logical pair must work before the old
      // two-segment render cache is cleaned up by the layout pass.
      score->startCmd();
      score->undoStack()->push1(new ChangeStartEndSpanner(selected,
            selected->startElement(), selected->endElement()));
      selected->undoChangeProperty(Pid::SPANNER_TICKS, originalEnd->tick() - selected->tick());
      score->undo(new ChangeSpannerElements(selected, selected->startElement(), originalEnd));
      QCOMPARE(int(selected->nsegments()), 2);
      const Chord* owner = toChord(selected->startElement()->parent());
      QVERIFY(spacingMergedGraceSlur4(selected, owner, toChord(originalEnd), true));
      score->endCmd();
      QCOMPARE(int(selected->nsegments()), 1);
      QVERIFY2(published(), qPrintable(reason));
      QVERIFY(same(graceLayoutSnapshot4(score), initial));
      QVERIFY(same(curve(), initialCurve));
      score->undoRedo(true, nullptr);
      QCOMPARE(int(selected->nsegments()), 2);
      QVERIFY2(published(), qPrintable(reason));
      for (size_t i = 0; i < targets.size(); ++i) score->undoRedo(true, nullptr);
      QCOMPARE(selected->endElement(), originalEnd);
      QCOMPARE(int(selected->nsegments()), 1);
      QVERIFY(same(graceLayoutSnapshot4(score), initial));
      QVERIFY(same(curve(), initialCurve));
      QVERIFY2(published(), qPrintable(reason));
      delete score;
      }

void TestSpacing::slurAnchorTraversal4_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<int>("editedStaff");
      QTest::addColumn<bool>("upSlur");
      // Visit every musical stop with both beam styles, staves and slur
      // directions represented. slurAnchorSpacing4 tests all combinations;
      // slurAnchorSystems4 additionally covers every pair of those axes.
      QTest::newRow("legacy-beams-upper-down") << false << 0 << false;
      QTest::newRow("modern-beams-lower-up") << true << 1 << true;
      }

void TestSpacing::slurAnchorTraversal4()
      {
      QFETCH(bool, modernBeams);
      QFETCH(int, editedStaff);
      QFETCH(bool, upSlur);
      MasterScore* score = readScore(DIR + "v0.139-spacing-two-staff-slur-offsets.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      Slur* selected = nullptr;
      Slur* companion = nullptr;
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            Slur* slur = toSlur(entry.second);
            if (slur->staffIdx() != editedStaff || !slur->startElement() || !slur->startElement()->isChord()
                || toChord(slur->startElement())->measure() != score->firstMeasure()) continue;
            if (slur->slurDirection() == (upSlur ? Direction::UP : Direction::DOWN)) selected = slur;
            else companion = slur;
            }
      QVERIFY(selected && companion && selected->frontSegment());
      Element* originalEnd = selected->endElement();
      // Musical traversal order, including the grace stops omitted in v0.143.
      // Build it independently from nextChordRest, the navigation under test.
      std::vector<Chord*> order;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
            for (const Segment& s : m->segments()) {
                  Element* item = s.element(editedStaff * VOICES);
                  if (!s.isChordRestType() || !item || !item->isChord()) continue;
                  Chord* main = toChord(item);
                  for (Chord* grace : main->graceNotesBefore()) order.push_back(grace);
                  order.push_back(main);
                  for (Chord* grace : main->graceNotesAfter()) order.push_back(grace);
                  }
      const auto originalIt = std::find(order.begin(), order.end(), originalEnd);
      const auto startIt = std::find(order.begin(), order.end(), selected->startElement());
      QVERIFY(originalIt != order.end() && startIt != order.end());
      const int originalIndex = int(originalIt - order.begin());
      const int startIndex = int(startIt - order.begin());
      const std::vector<Chord*> targets(originalIt + 1, order.end());
      QVERIFY(targets.size() > 3 && originalIndex > startIndex + 1);
      const auto same = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                  if (!qIsFinite(a[i]) || !qIsFinite(b[i]) || qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      QString planReason;
      const auto acceptedPlan = [score, &planReason]() {
            planReason.clear();
            SpacingPlan4 plan;
            System* system = score->firstMeasure()->system();
            if (!buildSpacingPlan4(system, plan, &planReason)) return false;
            if (plan.mergedGraces.empty()) { planReason = "Missing merged grace plan"; return false; }
            const auto equal = [&planReason](qreal actual, qreal expected, const QString& component) {
                  if (qIsFinite(actual) && qIsFinite(expected) && qAbs(actual - expected) < 1e-6) return true;
                  planReason = QString("Published %1: %2, modern plan: %3").arg(component)
                        .arg(actual, 0, 'g', 17).arg(expected, 0, 'g', 17);
                  return false;
                  };
            // This fixture is an unstretched terminal system (fill limit 1).
            // Plan availability alone would not prove its positions were used.
            if (!equal(system->width(), plan.width, "system width")) return false;
            for (const auto& m : plan.measures)
                  if (!equal(m.measure->x(), m.x, "measure x")
                      || !equal(m.measure->width(), m.width, "measure width")) return false;
            for (const auto& s : plan.segments) {
                  if (s.ignored) continue;
                  if (!equal(s.segment->measure()->x() + s.segment->x(), s.x, "segment x")
                      || !equal(s.segment->width(), s.width, "segment width")) return false;
                  }
            for (const auto& group : plan.mergedGraces) {
                  auto members = group.owner->graceNotesAfter();
                  for (Chord* chord : group.target->graceNotesBefore()) members.push_back(chord);
                  if (members.size() != int(group.positions.size())) return false;
                  for (int i = 0; i < members.size(); ++i) {
                        const Chord* parent = toChord(members[i]->parent());
                        const qreal actual = members[i]->x() + parent->segment()->x() + parent->x()
                              - group.target->segment()->x();
                        if (!equal(actual, group.positions[i].x, "merged grace x")) return false;
                        }
                  }
            return true;
            };
      const auto snapshot = [score, selected]() {
            auto values = graceLayoutSnapshot4(score);
            values.push_back(qreal(selected->nsegments()));
            for (const SpannerSegment* item : selected->spannerSegments()) {
                  const SlurSegment* segment = toSlurSegment(item);
                  values.insert(values.end(), {segment->x(), segment->y()});
                  for (Grip grip : {Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END}) {
                        const QPointF point = segment->ups(grip).pos();
                        values.insert(values.end(), {point.x(), point.y()});
                        }
                  for (const QRectF& rect : segment->shape())
                        values.insert(values.end(), {rect.x(), rect.y(), rect.width(), rect.height()});
                  }
            return values;
            };
      const auto initial = snapshot();
      for (Chord* target : targets) {
            Element* previous = selected->endElement();
            const auto before = snapshot();
            EditData ed;
            ed.key = Qt::Key_Right;
            ed.modifiers = Qt::ShiftModifier;
            ed.curGrip = Grip::END;
            score->startCmd();
            QVERIFY(selected->frontSegment()->edit(ed));
            QVERIFY(score->cmdState().slurAnchor4());
            score->endCmd();
            QVERIFY(!score->cmdState().slurAnchor4());
            QCOMPARE(selected->endElement(), static_cast<Element*>(target));
            QVERIFY(!score->undoStack()->slurOffset4(true));
            QVERIFY2(acceptedPlan(), qPrintable(planReason));
            const auto changed = snapshot();
            score->doLayout();
            QVERIFY(same(snapshot(), changed));
            score->undoRedo(true, nullptr);
            QCOMPARE(selected->endElement(), previous);
            QVERIFY(same(snapshot(), before));
            score->undoRedo(false, nullptr);
            QCOMPARE(selected->endElement(), static_cast<Element*>(target));
            QVERIFY(same(snapshot(), changed));
            }
      const auto final = snapshot();
      // Both explicit slur sides can be extended: their endpoint-side flags
      // combine by OR and do not impose an artificial one-span count limit.
      EditData ed;
      ed.key = Qt::Key_Right;
      ed.modifiers = Qt::ShiftModifier;
      ed.curGrip = Grip::END;
      score->startCmd();
      QVERIFY(companion->frontSegment()->edit(ed));
      score->endCmd();
      QCOMPARE(companion->endElement(), static_cast<Element*>(targets.front()));
      QVERIFY2(acceptedPlan(), qPrintable(planReason));
      score->undoRedo(true, nullptr);
      QVERIFY(same(snapshot(), final));
      for (size_t i = 0; i < targets.size(); ++i) score->undoRedo(true, nullptr);
      QCOMPARE(selected->endElement(), originalEnd);
      QVERIFY2(acceptedPlan(), qPrintable(planReason));
      QVERIFY(same(snapshot(), initial));
      // Pull back through the original before-graces and the owner's later
      // after-graces. These endpoints can have the same tick as the start.
      int backwardSteps = 0;
      for (int i = originalIndex - 1; i > startIndex; --i) {
            EditData back;
            back.key = Qt::Key_Left;
            back.modifiers = Qt::ShiftModifier;
            back.curGrip = Grip::END;
            score->startCmd();
            QVERIFY(selected->frontSegment()->edit(back));
            score->endCmd();
            ++backwardSteps;
            QCOMPARE(selected->endElement(), static_cast<Element*>(order[i]));
            QVERIFY2(acceptedPlan(), qPrintable(planReason));
            const auto moved = snapshot();
            score->doLayout();
            QVERIFY(same(snapshot(), moved));
            }
      QVERIFY(backwardSteps > 0);
      while (backwardSteps--) score->undoRedo(true, nullptr);
      QCOMPARE(selected->endElement(), originalEnd);
      QVERIFY2(acceptedPlan(), qPrintable(planReason));
      QVERIFY(same(snapshot(), initial));
      delete score;
      }

void TestSpacing::slurAnchorSpacing4_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<int>("editedStaff");
      QTest::addColumn<bool>("upSlur");
      for (bool beams : {false, true})
            for (int staff : {0, 1})
                  for (bool up : {false, true})
                        QTest::newRow(qPrintable(QString("beams-%1-staff-%2-slur-%3").arg(beams).arg(staff).arg(up)))
                              << beams << staff << up;
      }

void TestSpacing::slurAnchorSpacing4()
      {
      QFETCH(bool, modernBeams);
      QFETCH(int, editedStaff);
      QFETCH(bool, upSlur);
      MasterScore* score = readScore(DIR + "v0.139-spacing-two-staff-slur-offsets.mscx");
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->doLayout();
      Slur* selected = nullptr;
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            Slur* slur = toSlur(entry.second);
            if (slur->staffIdx() == editedStaff && slur->startElement()
                && slur->startElement()->isChord() && toChord(slur->startElement())->measure() == score->firstMeasure()
                && slur->slurDirection() == (upSlur ? Direction::UP : Direction::DOWN)) {
                  selected = slur;
                  break;
                  }
            }
      QVERIFY(selected && selected->frontSegment());
      Element* originalEnd = selected->endElement();
      const auto same = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                  if (!qIsFinite(a[i]) || !qIsFinite(b[i]) || qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      const auto original = graceLayoutSnapshot4(score);
      EditData ed;
      ed.key = Qt::Key_Right;
      ed.modifiers = Qt::ShiftModifier;
      ed.curGrip = Grip::END;
      score->startCmd();
      QVERIFY(selected->frontSegment()->edit(ed)); // Actual Shift+Right anchor command.
      QVERIFY(!score->cmdState().slurOffset4()); // Never classify an anchor edit as a visual offset.
      score->endCmd();
      QVERIFY(!score->undoStack()->slurOffset4(true));
      QVERIFY(selected->endElement() && selected->endElement() != originalEnd);
      QVERIFY(selected->endElement()->isChord());
      Chord* end = toChord(selected->endElement());
      QCOMPARE(end->measure(), score->firstMeasure()->nextMeasure());
      const auto extended = graceLayoutSnapshot4(score);
      SpacingPlan4 plan;
      QString reason;
      QVERIFY2(buildSpacingPlan4(score->firstMeasure()->system(), plan, &reason), qPrintable(reason));
      QVERIFY(!plan.mergedGraces.empty()); // The complete system still uses modern spacing.
      // Longer spans retain native curve layout rather than borrowing the old pair's anchors/obstacles.
      const Chord* owner = toChord(selected->startElement()->parent());
      const Chord* oldTarget = toChord(originalEnd);
      QVERIFY(!spacingMergedGraceSlur4(selected, owner, oldTarget, true));
      score->doLayout();
      QVERIFY2(same(graceLayoutSnapshot4(score), extended), "Anchor-change range layout differs from fresh layout");
      score->undoRedo(true, nullptr);
      QCOMPARE(selected->endElement(), originalEnd);
      QVERIFY(same(graceLayoutSnapshot4(score), original));
      score->undoRedo(false, nullptr);
      QCOMPARE(selected->endElement(), static_cast<Element*>(end));
      QVERIFY(same(graceLayoutSnapshot4(score), extended));

      // Exercise Studio's endpoint-side kerning rule with an actual note and
      // an unattached accidental probe in that note's coordinate system.
      {
      const SpacingPaddingTable4 table(score);
      Note* note = end->notes().front();
      const bool stemUp = end->up();
      Accidental probe(score);
      probe.setParent(note);
      probe.setTrack(note->track());
      for (bool up : {false, true}) {
            end->setUp(up);
            SpacingContextCache4 cache;
            // Include all already-supported merged pairs in this read-only scope.
            SpacingGraceContext4 context;
            for (const auto& group : plan.mergedGraces) {
                  context.mains.push_back(group.target);
                  context.afterMains.push_back(group.owner);
                  context.mergedPairs.push_back({group.owner, group.target});
                  }
            for (int side : {-1, 1}) {
                  probe.setPos(0, side * score->spatium());
                  SpacingPairPolicy4 fresh, cached;
                  QVERIFY(spacingPairPolicy4(table, note, &probe, score->spatium(), fresh, &context));
                  QVERIFY(spacingPairPolicy4(table, note, &probe, score->spatium(), cached, &context, &cache));
                  const bool blocked = (upSlur && !up && side < 0) || (!upSlur && up && side > 0);
                  QCOMPARE(int(fresh.kerning), int(blocked ? SpacingKerning4::NON_KERNING : SpacingKerning4::KERNING));
                  QCOMPARE(int(cached.kerning), int(fresh.kerning));
                  }
            }
      end->setUp(!upSlur); // This endpoint has an active opposite-stem restriction.
      Measure* endMeasure = end->measure();
      Element* parent = endMeasure->parent();
      endMeasure->setParent(nullptr);
      SpacingGraceContext4 context;
      for (const auto& group : plan.mergedGraces) {
            context.mains.push_back(group.target);
            context.afterMains.push_back(group.owner);
            context.mergedPairs.push_back({group.owner, group.target});
            }
      SpacingPairPolicy4 rejectedFrame;
      rejectedFrame.padding = -123;
      const bool ready = spacingPairPolicy4(table, note, &probe, score->spatium(), rejectedFrame, &context);
      endMeasure->setParent(parent);
      QVERIFY(!ready);
      QCOMPARE(rejectedFrame.padding, qreal(-123));
      end->setUp(stemUp);
      probe.setParent(nullptr);
      }
      // Unsupported automatic direction retains fallback, without touching output.
      const Direction direction = selected->slurDirection();
      selected->setSlurDirection(Direction::AUTO);
      SpacingPlan4 rejected;
      rejected.width = -123;
      QVERIFY(!buildSpacingPlan4(score->firstMeasure()->system(), rejected));
      QCOMPARE(rejected.width, qreal(-123));
      selected->setSlurDirection(direction);
      score->doLayout();
      QVERIFY(same(graceLayoutSnapshot4(score), extended));
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = graceLayoutSnapshot4(score);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      QVERIFY(same(graceLayoutSnapshot4(score), extended));
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      QVERIFY(same(graceLayoutSnapshot4(score), legacy));
      delete score;
      }

void TestSpacing::slurOffsetLayout4_data()
      {
      QTest::addColumn<bool>("modernBeams");
      QTest::addColumn<QString>("file");
      QTest::addColumn<int>("editedStaff");
      for (bool beams : { false, true }) {
            const QString suffix = beams ? "-studio-beams" : "-legacy-beams";
            QTest::newRow(qPrintable("one-staff" + suffix)) << beams
                  << QString("v0.113-spacing-slurred-merged-graces-natural.mscx") << 0;
            for (int staff : { 0, 1 })
                  QTest::newRow(qPrintable(QString("two-staff-edit-%1").arg(staff + 1) + suffix)) << beams
                        << QString("v0.139-spacing-two-staff-slur-offsets.mscx") << staff;
            }
      }

void TestSpacing::slurOffsetLayout4()
      {
      QFETCH(bool, modernBeams);
      QFETCH(QString, file);
      QFETCH(int, editedStaff);
      MasterScore* score = readScore(DIR + file);
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      Slur* selected = nullptr;
      for (const auto& entry : score->spannerMap().map()) {
            if (!entry.second->isSlur()) continue;
            Slur* candidate = toSlur(entry.second);
            if (candidate->staffIdx() == editedStaff && candidate->startElement() && candidate->startElement()->isChord()
                && toChord(candidate->startElement())->isGraceAfter()) { selected = candidate; break; }
            }
      QVERIFY(selected && selected->frontSegment());
      const auto same = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i) if (!qIsFinite(a[i]) || !qIsFinite(b[i]) || qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      const auto horizontalSnapshot = [score]() {
            auto values = graceLayoutSnapshot4(score);
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  for (const Segment& s : m->segments()) {
                        for (const Element* item : s.elist()) {
                              if (!item || !item->isChord()) continue;
                              const Chord* main = toChord(item);
                              QVector<const Chord*> chords { main };
                              for (const Chord* grace : main->graceNotes()) chords.push_back(grace);
                              for (const Chord* chord : chords) {
                                    values.insert(values.end(), {chord->x(), chord->y(), qreal(chord->up()),
                                          chord->stem() ? chord->stem()->len() : 0});
                                    if (chord->beam() && chord->beam()->primarySegment4()) {
                                          const QLineF& beam = *chord->beam()->primarySegment4();
                                          values.insert(values.end(), {beam.x1(), beam.y1(), beam.x2(), beam.y2()});
                                          }
                                    }
                              }
                        }
                  }
            return values;
            };
      const auto snapshot = [score, &horizontalSnapshot]() {
            auto values = horizontalSnapshot();
            for (const System* system : score->systems())
                  values.insert(values.end(), {system->x(), system->y(), system->width(), system->height(),
                        system->minTop(), system->minBottom()});
            for (const System* system : score->systems()) {
                  if (system->vbox()) continue;
                  for (int i = 0; i < score->nstaves(); ++i) {
                        const QRectF& box = system->staff(i)->bbox();
                        values.insert(values.end(), {box.x(), box.y(), box.width(), box.height()});
                        }
                  }
            for (const auto& entry : score->spannerMap().map()) {
                  if (!entry.second->isSlur()) continue;
                  Slur* slur = toSlur(entry.second);
                  values.push_back(qreal(slur->spannerSegments().size()));
                  for (const SpannerSegment* item : slur->spannerSegments()) {
                        const SlurSegment* segment = toSlurSegment(item);
                        values.insert(values.end(), {segment->x(), segment->y()});
                        for (Grip grip : {Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END}) {
                              const QPointF point = segment->ups(grip).pos();
                              values.insert(values.end(), {point.x(), point.y()});
                              }
                        for (const QRectF& rect : segment->shape())
                              values.insert(values.end(), {rect.x(), rect.y(), rect.width(), rect.height()});
                        }
                  }
            return values;
            };
      const auto horizontal = horizontalSnapshot();
      // The anchor path may retain horizontal layout only after a fresh plan
      // matches every field, including target-relative grace coordinates.
      SpacingPlan4 unchanged;
      QString proofReason;
      QVERIFY2(buildSpacingPlan4(score->firstMeasure()->system(), unchanged, &proofReason), qPrintable(proofReason));
      QVERIFY2(spacingPlanMatches4(unchanged, &proofReason), qPrintable(proofReason));
      QVERIFY(!unchanged.segments.empty() && !unchanged.measures.empty() && !unchanged.mergedGraces.empty());
      const auto active = std::find_if(unchanged.segments.begin(), unchanged.segments.end(),
            [](const SpacingSegmentPlan4& s) { return !s.ignored; });
      QVERIFY(active != unchanged.segments.end());
      const size_t activeIndex = size_t(active - unchanged.segments.begin());
      for (int field = 0; field < 8; ++field) {
            SpacingPlan4 changed = unchanged;
            switch (field) {
                  case 0: changed.width += .01; break;
                  case 1: changed.measures.front().x += .01; break;
                  case 2: changed.measures.front().width += .01; break;
                  case 3: changed.segments[activeIndex].x += .01; break;
                  case 4: changed.segments[activeIndex].width += .01; break;
                  case 5: changed.segments[activeIndex].stretch += .01; break;
                  case 6: changed.segments[activeIndex].widthOffset += .01; break;
                  case 7: changed.mergedGraces.front().positions.front().x += .01; break;
                  }
            QVERIFY(!spacingPlanMatches4(changed));
            }
      SpacingPlan4 nonfinite = unchanged;
      nonfinite.width = std::numeric_limits<qreal>::quiet_NaN();
      QVERIFY(!spacingPlanMatches4(nonfinite));
      QVERIFY(same(horizontalSnapshot(), horizontal)); // All proof queries are read-only.
      score->cmdState().reset();
      score->cmdState().requestSlurAnchor4(selected->frontSegment());
      QVERIFY(score->doLayoutSlurAnchor4(selected->frontSegment()));
      score->cmdState().reset();
      const auto anchored = snapshot();
      QVERIFY(same(horizontalSnapshot(), horizontal));
      const qreal oldWidth = score->firstMeasure()->width();
      score->firstMeasure()->setWidth(oldWidth + .01);
      const auto mismatched = snapshot();
      score->cmdState().requestSlurAnchor4(selected->frontSegment());
      QVERIFY(!score->doLayoutSlurAnchor4(selected->frontSegment()));
      QVERIFY(same(snapshot(), mismatched));
      score->firstMeasure()->setWidth(oldWidth);
      score->cmdState().reset();
      score->doLayout();
      QVERIFY2(same(snapshot(), anchored), "Proven anchor update differs from complete native layout");
      score->cmdState().reset();
      score->cmdState().requestSlurOffset4(selected->frontSegment());
      QVERIFY(score->doLayoutSlurOffset4(selected->frontSegment()));
      score->cmdState().reset();
      QVERIFY(same(horizontalSnapshot(), horizontal));
      for (Grip grip : {Grip::START, Grip::BEZIER1, Grip::BEZIER2, Grip::END, Grip::DRAG}) {
            SlurSegment* segment = selected->frontSegment();
            const QPointF old = grip == Grip::DRAG ? segment->offset() : segment->ups(grip).off;
            EditData ed;
            auto* data = new ElementEditData;
            data->e = segment;
            ed.addData(data);
            ed.curGrip = grip;
            score->startCmd();
            segment->startEditDrag(ed);
            if (grip == Grip::DRAG) segment->setOffset(old + QPointF(0, -.25 * score->spatium()));
            else segment->ups(grip).off = old + QPointF(0, -.25 * score->spatium());
            segment->computeBezier();
            segment->endEditDrag(ed);
            QCOMPARE(score->cmdState().slurOffset4(), static_cast<const SlurSegment*>(segment));
            score->endCmd();
            QVERIFY(!score->cmdState().slurOffset4());
            QCOMPARE(score->undoStack()->slurOffset4(true), static_cast<const SlurSegment*>(segment));
            const auto fast = snapshot();
            QVERIFY(same(horizontalSnapshot(), horizontal));
            score->doLayout();
            QVERIFY2(same(snapshot(), fast), "Offset layout differs from complete native layout");
            score->undoRedo(true, nullptr);
            QVERIFY(!score->cmdState().slurOffset4());
            segment = selected->frontSegment();
            QCOMPARE(score->undoStack()->slurOffset4(false), static_cast<const SlurSegment*>(segment));
            QCOMPARE(grip == Grip::DRAG ? segment->offset() : segment->ups(grip).off, old);
            score->undoRedo(false, nullptr);
            QVERIFY(!score->cmdState().slurOffset4());
            QVERIFY(same(snapshot(), fast));
            ed.clearData();
            }
      // Anchor and offset candidates are mutually exclusive and must not
      // survive a later generic request or command reset.
      score->cmdState().requestSlurAnchor4(selected->frontSegment());
      score->setLayoutAll();
      QVERIFY(!score->cmdState().slurAnchor4());
      score->cmdState().requestSlurAnchor4(selected->frontSegment());
      score->cmdState().reset();
      QVERIFY(!score->cmdState().slurAnchor4());
      score->cmdState().requestSlurAnchor4(selected->frontSegment());
      score->cmdState().requestSlurOffset4(selected->frontSegment());
      QVERIFY(!score->cmdState().slurAnchor4());
      score->cmdState().requestSlurAnchor4(selected->frontSegment());
      QVERIFY(!score->cmdState().slurOffset4());
      score->cmdState().reset();
      // A later generic request cancels an already classified edit.
      score->cmdState().requestSlurOffset4(selected->frontSegment());
      score->setLayoutAll();
      QVERIFY(!score->cmdState().slurOffset4());
      score->cmdState().reset();
      // Pending work before the commit must also retain the general path.
      SlurSegment* segment = selected->frontSegment();
      EditData ed;
      auto* data = new ElementEditData;
      data->e = segment;
      ed.addData(data);
      score->startCmd();
      segment->startEditDrag(ed);
      score->setLayoutAll();
      segment->endEditDrag(ed);
      QVERIFY(!score->cmdState().slurOffset4());
      score->endCmd();
      ed.clearData();
      // A command reset between capture and commit invalidates the proof,
      // even when its current dirty mode has become clean again.
      segment = selected->frontSegment();
      EditData resetEdit;
      auto* resetData = new ElementEditData;
      resetData->e = segment;
      resetEdit.addData(resetData);
      score->startCmd();
      segment->startEditDrag(resetEdit);
      score->cmdState().reset();
      segment->endEditDrag(resetEdit);
      QVERIFY(!score->cmdState().slurOffset4());
      score->endCmd();
      resetEdit.clearData();
      if (score->nstaves() == 2) {
            // Cross-staff notes/graces and spanners must reject before any layout write.
            Chord* main = nullptr;
            for (const Segment& s : score->firstMeasure()->segments()) {
                  Element* item = s.element(0);
                  if (item && item->isChord() && !toChord(item)->graceNotes().isEmpty()) {
                        main = toChord(item);
                        break;
                        }
                  }
            QVERIFY(main);
            for (Chord* chord : {main, main->graceNotes().front()}) {
                  const auto before = snapshot();
                  chord->setStaffMove(1);
                  score->cmdState().requestSlurOffset4(selected->frontSegment());
                  QVERIFY(!score->doLayoutSlurOffset4(selected->frontSegment()));
                  score->cmdState().requestSlurAnchor4(selected->frontSegment());
                  QVERIFY(!score->doLayoutSlurAnchor4(selected->frontSegment()));
                  chord->setStaffMove(0);
                  QVERIFY(same(snapshot(), before));
                  score->cmdState().reset();
                  }
            const int track2 = selected->track2();
            selected->setTrack2((1 - editedStaff) * VOICES);
            score->cmdState().requestSlurOffset4(selected->frontSegment());
            QVERIFY(!score->doLayoutSlurOffset4(selected->frontSegment()));
            selected->setTrack2(track2);
            score->cmdState().reset();
            }
      score->setStyleValue(Sid::spacingLayout4, false);
      score->cmdState().requestSlurOffset4(selected->frontSegment());
      QVERIFY(!score->doLayoutSlurOffset4(selected->frontSegment()));
      delete score;
      }

void TestSpacing::spacingDecisionScopes4()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "studio-spacing-natural-h1.mscx"));
      QVERIFY(score);
      score->doLayout();
      const System* system = score->firstMeasure()->system();
      QVERIFY(system);
      const auto notation = graceMergedSnapshot4(score.get());
      std::vector<SpacingDecision4> outer, inner;
      QVERIFY(!spacingDecisionObserved4(score.get()));
      QVERIFY(!spacingDecisionObserved4(nullptr));
      {
      SpacingDecisionScope4 scope(score.get(), [&outer](const SpacingDecision4& e) { outer.push_back(e); });
      QVERIFY(spacingDecisionObserved4(score.get()));
      recordSpacingDecision4(system, SpacingPass4::CANDIDATE, SpacingPath4::FULL_PLAN, 12, 20);
      {
      SpacingDecisionScope4 nested(score.get(), [&inner](const SpacingDecision4& e) { inner.push_back(e); });
      recordSpacingDecision4(system, SpacingPass4::FINAL, SpacingPath4::NATIVE_WIDTHS, 15, 20, "probe");
      }
      recordSpacingDecision4(system, SpacingPass4::FINAL, SpacingPath4::FULL_PLAN, 20, 20);
      }
      QVERIFY(!spacingDecisionObserved4(score.get()));
      recordSpacingDecision4(system, SpacingPass4::FINAL, SpacingPath4::STYLE_OFF, 7, 20);
      QCOMPARE(int(outer.size()), 2);
      QCOMPARE(int(inner.size()), 1);
      QVERIFY(outer.front().pass == SpacingPass4::CANDIDATE && outer.back().pass == SpacingPass4::FINAL);
      QVERIFY(inner.front().path == SpacingPath4::NATIVE_WIDTHS);
      QCOMPARE(inner.front().reason, QString("probe"));
      QCOMPARE(inner.front().width, qreal(15));
      QCOMPARE(inner.front().available, qreal(20));
      QCOMPARE(inner.front().measures, 1);
      QVERIFY(inner.front().firstTick == score->firstMeasure()->tick());
      QVERIFY(inner.front().endTick == score->lastMeasure()->endTick());
      QVERIFY(graceMergedSnapshot4(score.get()) == notation);
      }

void TestSpacing::spacingNativeDecisions4()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "studio-spacing-natural-h1.mscx"));
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, true);
      const auto same = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i) if (qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      for (int mode = 0; mode < 3; ++mode) {
            score->setStyleValue(Sid::spacingLayout4, mode != 0);
            QScopedValueRollback<bool> stretch(MScore::noHorizontalStretch, mode == 2);
            score->doLayout();
            const auto baseline = graceMergedSnapshot4(score.get());
            std::vector<SpacingDecision4> events;
            {
            SpacingDecisionScope4 scope(score.get(), [&events](const SpacingDecision4& e) { events.push_back(e); });
            score->doLayout();
            }
            QVERIFY(same(graceMergedSnapshot4(score.get()), baseline));
            bool candidate = false, final = false;
            for (const auto& e : events) {
                  QVERIFY(qIsFinite(e.width) && qIsFinite(e.available));
                  QCOMPARE(e.measures, 1);
                  QVERIFY(e.firstTick == score->firstMeasure()->tick());
                  QVERIFY(e.endTick == score->lastMeasure()->endTick());
                  candidate = candidate || e.pass == SpacingPass4::CANDIDATE;
                  final = final || e.pass == SpacingPass4::FINAL;
                  const SpacingPath4 expected = mode == 0 ? SpacingPath4::STYLE_OFF
                        : mode == 2 ? SpacingPath4::NO_STRETCH : SpacingPath4::FULL_PLAN;
                  QVERIFY2(e.path == expected, qPrintable(QString("mode %1: %2").arg(mode).arg(e.reason)));
                  if (e.pass == SpacingPass4::FINAL) QVERIFY(qAbs(e.width - score->firstMeasure()->system()->width()) < 1e-6);
                  }
            QVERIFY(candidate && final);
            QVERIFY(!spacingDecisionObserved4(score.get()));
            }
      // A real chord symbol exposes the explicitly unsupported annotation
      // path. It must be reported as mixed fallback, never as style-off legacy.
      score->setStyleValue(Sid::spacingLayout4, true);
      Segment* segment = nullptr;
      for (Segment& s : score->firstMeasure()->segments()) if (s.isChordRestType()) { segment = &s; break; }
      QVERIFY(segment);
      Harmony* harmony = new Harmony(score.get());
      harmony->setTrack(0);
      segment->add(harmony);
      harmony->setHarmony("C7");
      score->doLayout();
      const auto decorated = graceMergedSnapshot4(score.get());
      std::vector<SpacingDecision4> events;
      {
      SpacingDecisionScope4 scope(score.get(), [&events](const SpacingDecision4& e) { events.push_back(e); });
      score->doLayout();
      }
      const bool unchanged = same(graceMergedSnapshot4(score.get()), decorated);
      // A spacing query must not invoke the native lazy harmony layout when
      // unsupported decoration geometry is missing. Reject the snapshot.
      const QRectF harmonyBox = harmony->bbox();
      harmony->setbbox(QRectF());
      const SpacingShape4 missingAnnotation = spacingStaffShape4(segment, 0);
      const bool noLazyLayout = harmony->bbox().isEmpty();
      harmony->setbbox(harmonyBox);
      segment->remove(harmony);
      delete harmony;
      QVERIFY(unchanged);
      QVERIFY(noLazyLayout);
      QVERIFY(!missingAnnotation.elements.empty());
      for (const auto& e : missingAnnotation.elements) {
            QVERIFY(e.identity == SpacingIdentity4::UNCLASSIFIED);
            QVERIFY(!e.item && e.aggregateOwners.empty() && e.sharedItems.empty());
            }
      bool fallback = false;
      for (const auto& e : events) {
            if (e.pass != SpacingPass4::FINAL) continue;
            QVERIFY(e.path == SpacingPath4::NATIVE_WIDTHS || e.path == SpacingPath4::POOLED_WIDTHS);
            QVERIFY(!e.reason.isEmpty());
            fallback = true;
            }
      QVERIFY(fallback);
      }

void TestSpacing::spacingDecisionCorpus4_data()
      {
      QTest::addColumn<QString>("file");
      QTest::newRow("measure-rest-fitting") << QString("v0.201-measure-rests-fitting.mscx");
      QTest::newRow("voices") << QString("v0.38-spacing-voices.mscx");
      QTest::newRow("cross-staff-fitting") << QString("v0.181-spacing-cross-staff-fitting.mscx");
      QTest::newRow("grace-fitting") << QString("v0.187-acciaccatura-fitting.mscx");
      QTest::newRow("grace-squeeze") << QString("v0.187-acciaccatura-squeeze.mscx");
      QTest::newRow("merged-system-boundary") << QString("v0.167-grace-policy-systems.mscx");
      }

void TestSpacing::spacingDecisionCorpus4()
      {
      QFETCH(QString, file);
      std::unique_ptr<MasterScore> score(readScore(DIR + file));
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, true);
      score->setStyleValue(Sid::spacingLayout4, true);
      QScopedValueRollback<bool> stretch(MScore::noHorizontalStretch, false);
      std::vector<SpacingDecision4> events;
      {
      SpacingDecisionScope4 scope(score.get(), [&events](const SpacingDecision4& e) { events.push_back(e); });
      score->doLayout();
      }
      QVERIFY(!spacingDecisionObserved4(score.get()));
      const auto observed = graceMergedSnapshot4(score.get());
      score->doLayout();
      const auto unobserved = graceMergedSnapshot4(score.get());
      QCOMPARE(int(observed.size()), int(unobserved.size()));
      for (size_t i = 0; i < observed.size(); ++i)
            QVERIFY2(qAbs(observed[i] - unobserved[i]) < 1e-6, "Observation changed published geometry");
      std::vector<System*> systems;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            QVERIFY(m->system());
            if (systems.empty() || systems.back() != m->system()) systems.push_back(m->system());
            }
      int finals = 0, candidates = 0;
      Fraction nextTick = score->firstMeasure()->tick();
      for (size_t i = 0; i < events.size(); ++i) {
            const SpacingDecision4& e = events[i];
            QVERIFY(e.measures > 0 && e.firstTick < e.endTick);
            QVERIFY(qIsFinite(e.width) && qIsFinite(e.available));
            QVERIFY(e.width > 0 && e.available > 0);
            QVERIFY(e.path == SpacingPath4::FULL_PLAN || e.path == SpacingPath4::NATIVE_WIDTHS
                  || e.path == SpacingPath4::POOLED_WIDTHS);
            if (e.path == SpacingPath4::FULL_PLAN) QVERIFY(e.reason.isEmpty());
            else QVERIFY2(!e.reason.isEmpty(), "Mixed fallback must identify its rejection stage");
            if (e.pass == SpacingPass4::CANDIDATE) {
                  ++candidates;
                  continue;
                  }
            QVERIFY(finals < int(systems.size()));
            const System* system = systems[finals++];
            QVERIFY(e.firstTick == nextTick);
            QVERIFY(e.firstTick == system->firstMeasure()->tick());
            QVERIFY(e.endTick == system->lastMeasure()->endTick());
            int measures = 0;
            for (const MeasureBase* mb : system->measures()) measures += mb->isMeasure();
            QCOMPARE(e.measures, measures);
            QVERIFY(qAbs(e.width - system->width()) < 1e-6);
            // Final headers/squeezing can change width or eligibility. Match
            // the retained prefix by identity/range, never equate its two widths
            // or require a rejected overfull candidate to be the final prefix.
            bool retainedPrefix = false;
            for (size_t j = 0; j < i; ++j) {
                  const auto& candidate = events[j];
                  retainedPrefix = retainedPrefix || (candidate.pass == SpacingPass4::CANDIDATE
                        && candidate.firstTick == e.firstTick && candidate.endTick == e.endTick
                        && candidate.measures == e.measures);
                  }
            QVERIFY2(retainedPrefix, "Final system has no corresponding collected prefix");
            nextTick = e.endTick;
            }
      QVERIFY(candidates >= finals);
      QCOMPARE(finals, int(systems.size()));
      QVERIFY(nextTick == score->lastMeasure()->endTick());
      }

void TestSpacing::pendingCourtesyTrial4()
      {
      QScopedValueRollback<bool> stretch(MScore::noHorizontalStretch, false);
      std::unique_ptr<MasterScore> score(new MasterScore(mscore->baseStyle()));
      {
      ScoreLoad loading;
      QVERIFY(score->loadMsc(root + "/" + DIR + "v0.241-pending-courtesy.mscx", false)
            == Score::FileError::FILE_NO_ERROR);
      }
      QVERIFY(score->styleB(Sid::spacingLayout4));
      Measure* first = score->firstMeasure();
      QVERIFY(first && first->nextMeasure());
      Measure* second = first->nextMeasure();
      QVERIFY(!second->nextMeasure());
      std::vector<SpacingDecision4> events;
      const auto collect = [&]() {
            events.clear();
            SpacingDecisionScope4 observer(score.get(), [&](const SpacingDecision4& e) { events.push_back(e); });
            score->doLayout();
            };
      const auto candidate = [&](int count) -> const SpacingDecision4* {
            for (const auto& e : events)
                  if (e.pass == SpacingPass4::CANDIDATE && e.firstTick == first->tick() && e.measures == count)
                        return &e;
            return nullptr;
            };
      collect();
      const auto* single = candidate(1);
      const auto* pair = candidate(2);
      QVERIFY(single && pair);
      QVERIFY2(single->path == SpacingPath4::FULL_PLAN, qPrintable(single->reason));
      QVERIFY2(pair->path == SpacingPath4::FULL_PLAN, qPrintable(pair->reason));
      QVERIFY(pair->width > single->width && pair->width <= pair->available);
      QVERIFY(first->system() == second->system());
      QVERIFY(!first->trailer());

      // Exercise the same candidate at an available width between its two
      // natural estimates. Rejecting C2 must retain C1's terminal courtesy.
      const qreal originalWidth = score->styleD(Sid::pagePrintableWidth);
      const qreal narrow = originalWidth * (single->width + pair->width) / (2 * pair->available);
      score->setStyleValue(Sid::pagePrintableWidth, narrow);
      collect();
      single = candidate(1);
      pair = candidate(2);
      QVERIFY(single && pair);
      QVERIFY2(pair->path == SpacingPath4::FULL_PLAN, qPrintable(pair->reason));
      QVERIFY(single->width <= single->available && pair->width > pair->available);
      QVERIFY(first->system() != second->system());
      QVERIFY(first->trailer());
      for (SegmentType type : {SegmentType::KeySigAnnounce, SegmentType::TimeSigAnnounce}) {
            const Segment* courtesy = first->findSegmentR(type, first->ticks());
            QVERIFY(courtesy && courtesy->enabled() && courtesy->element(0));
            }

      score->setStyleValue(Sid::pagePrintableWidth, originalWidth);
      collect();
      pair = candidate(2);
      QVERIFY(pair && pair->path == SpacingPath4::FULL_PLAN);
      QVERIFY(first->system() == second->system());
      QVERIFY(!first->trailer());
      }

void TestSpacing::signatureCollection4_data()
      {
      QTest::addColumn<QString>("mode");
      QTest::addColumn<bool>("beams");
      for (const QString& mode : {QString("natural"), QString("fitting")})
            for (bool beams : {false, true})
                  QTest::newRow(qPrintable(mode + (beams ? "-studio" : "-legacy"))) << mode << beams;
      }

void TestSpacing::signatureCollection4()
      {
      QFETCH(QString, mode);
      QFETCH(bool, beams);
      QScopedValueRollback<bool> stretch(MScore::noHorizontalStretch, false);
      std::unique_ptr<MasterScore> score(new MasterScore(mscore->baseStyle()));
      {
      ScoreLoad loading;
      QVERIFY(score->loadMsc(root + "/" + DIR + "v0.239-signature-collection-" + mode + ".mscx", false)
            == Score::FileError::FILE_NO_ERROR);
      }
      score->setStyleValue(Sid::beamLayout4, beams);
      QVERIFY(score->styleB(Sid::spacingLayout4));
      std::vector<Measure*> measures;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) measures.push_back(m);
      QCOMPARE(measures.size(), size_t(8));
      const auto snapshot = [&]() {
            auto values = graceMergedSnapshot4(score.get());
            for (Measure* m : measures) {
                  values.insert(values.end(), {qreal(m->header()), qreal(m->trailer()), qreal(m->hasCourtesyKeySig())});
                  for (const Segment& s : m->segments()) {
                        if (!s.enabled() && (s.header() || s.trailer())) continue;
                        for (const Element* item : s.elist()) {
                              if (!item || !(item->isKeySig() || item->isTimeSig() || item->isBarLine())) continue;
                              const QRectF box = item->bbox();
                              values.insert(values.end(), {qreal(item->generated()), item->ipos().x(), item->ipos().y(),
                                    item->offset().x(), item->offset().y(), box.x(), box.y(), box.width(), box.height()});
                              if (item->isKeySig()) values.push_back(qreal(static_cast<int>(toKeySig(item)->key())));
                              if (item->isTimeSig()) values.insert(values.end(), {qreal(toTimeSig(item)->numerator()),
                                    qreal(toTimeSig(item)->denominator())});
                              if (item->isBarLine()) values.push_back(qreal(static_cast<int>(toBarLine(item)->barLineType())));
                              }
                        }
                  }
            return values;
            };
      const auto near = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                  if (!qIsFinite(a[i]) || !qIsFinite(b[i]) || qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      std::vector<SpacingDecision4> events;
      {
      SpacingDecisionScope4 observer(score.get(), [&events](const SpacingDecision4& e) { events.push_back(e); });
      score->doLayout();
      }
      const auto modern = snapshot();
      int finals = 0, planned = 0, courtesyCandidates = 0, interiorCandidates = 0, pending = 0, rollbacks = 0;
      int retainedMeasures = 0;
      Fraction endTick(0, 1);
      for (size_t i = 0; i < events.size(); ++i) {
            const auto& event = events[i];
            if (event.pass == SpacingPass4::FINAL) {
                  QVERIFY2(event.path == SpacingPath4::FULL_PLAN, qPrintable(event.reason));
                  ++finals;
                  QCOMPARE(event.firstTick, endTick);
                  endTick = event.endTick;
                  retainedMeasures += event.measures;
                  continue;
                  }
            if (event.path == SpacingPath4::FULL_PLAN) {
                  ++planned;
                  for (int n : {1, 3, 5}) if (event.endTick == measures[n]->endTick()) ++courtesyCandidates;
                  bool interior = false;
                  for (int n : {2, 4, 6}) interior = interior
                        || (measures[n]->tick() > event.firstTick && measures[n]->tick() < event.endTick);
                  if (interior) ++interiorCandidates;
                  }
            else {
                  QVERIFY(event.path == SpacingPath4::NATIVE_WIDTHS);
                  QVERIFY2(event.reason.contains("pending penultimate courtesy"), qPrintable(event.reason));
                  ++pending;
                  }
            if (event.measures <= 1 || event.width <= event.available) continue;
            ++rollbacks;
            QVERIFY(i + 2 < events.size());
            const auto& retained = events[i + 1];
            const auto& retried = events[i + 2];
            QVERIFY(retained.pass == SpacingPass4::FINAL && retried.pass == SpacingPass4::CANDIDATE);
            Measure* removed = nullptr;
            for (Measure* m : measures) if (m->endTick() == event.endTick) removed = m;
            QVERIFY(removed);
            QCOMPARE(retained.firstTick, event.firstTick);
            QCOMPARE(retained.endTick, removed->tick());
            QCOMPARE(retained.measures, event.measures - 1);
            QCOMPARE(retried.firstTick, retained.endTick);
            QCOMPARE(retried.measures, 1);
            }
      QVERIFY(planned > 0 && courtesyCandidates > 0 && pending > 0);
      QCOMPARE(retainedMeasures, 8);
      QCOMPARE(endTick, measures.back()->endTick());
      if (mode == "natural") {
            QCOMPARE(finals, 1);
            QVERIFY(interiorCandidates > 0);
            }
      else QVERIFY(finals > 1 && rollbacks > 0);
      QString reason;
      for (System* system : score->systems()) {
            if (system->vbox()) continue;
            QVERIFY2(spacingCandidateSignaturesReady4(system, &reason), qPrintable(reason));
            SpacingAttachmentPreparation4 owner;
            QVERIFY2(snapshotLaidOutSystemSpacing4(system, owner, &reason), qPrintable(reason));
            SpacingSystemRequest4 request;
            request.pass = SpacingPass4::FINAL;
            request.availableWidth = system->width();
            request.next = system->lastMeasure()->next();
            request.fillLimit = score->styleD(Sid::lastSystemFillLimit);
            SpacingPlan4 plan;
            QVERIFY2(selectSystemSpacingPlan4(system, request, owner, plan, &reason), qPrintable(reason));
            QVERIFY2(spacingPlanPublicationValid4(system, plan, &reason), qPrintable(reason));
            QVERIFY2(spacingPlanMatches4(plan, &reason), qPrintable(reason));
            }
      QVERIFY(near(snapshot(), modern));
      if (mode == "natural") {
            Measure* m = measures[1];
            System* original = m->system();
            m->createEndBarLines(true);
            m->addSystemTrailer(m->nextMeasure());
            Segment* timeSeg = m->findSegmentR(SegmentType::TimeSigAnnounce, m->ticks());
            Segment* keySeg = m->findSegmentR(SegmentType::KeySigAnnounce, m->ticks());
            QVERIFY(timeSeg && timeSeg->element(0) && timeSeg->element(0)->isTimeSig());
            QVERIFY(keySeg && keySeg->element(0) && keySeg->element(0)->isKeySig());
            TimeSig* time = toTimeSig(timeSeg->element(0));
            KeySig* key = toKeySig(keySeg->element(0));
            bool terminalReady, wrongTimeRejected, malformedRejected, unchanged;
            {
            System trial(score.get());
            trial.appendMeasure(m);
            auto before = snapshot();
            terminalReady = spacingCandidateSignaturesReady4(&trial, &reason);
            unchanged = near(snapshot(), before);
            const Fraction sig = time->sig();
            const TimeSigType type = time->timeSigType();
            time->setSig(Fraction(7, 8));
            before = snapshot();
            wrongTimeRejected = !spacingCandidateSignaturesReady4(&trial, &reason)
                  && reason == "Candidate courtesy time does not match following signature";
            unchanged = unchanged && near(snapshot(), before);
            time->setSig(sig, type);
            const QRectF box = key->bbox();
            key->setbbox(QRectF(box.x(), box.y(), box.width(), 0));
            before = snapshot();
            const bool active = keySeg->enabled() && keySeg->visible() && !keySeg->allElementsInvisible();
            malformedRejected = active && !spacingCandidateSignaturesReady4(&trial, &reason)
                  && reason == "Candidate signature glyph, ownership or dimensions not prepared";
            unchanged = unchanged && near(snapshot(), before);
            key->setbbox(box);
            m->setSystem(original); // Restore before destroying the borrowed System.
            }
            const auto invalid = snapshot();
            const bool pendingRejected = !spacingCandidateSignaturesReady4(original, &reason);
            const QString pendingReason = reason;
            SpacingPlan4 rejected;
            rejected.width = 123;
            bool prepared = false, accepted = true;
            {
            SpacingAttachmentPreparation4 owner;
            prepared = snapshotLaidOutSystemSpacing4(original, owner, &reason);
            if (prepared) accepted = buildSpacingPlan4(original, rejected, owner, &reason);
            }
            unchanged = unchanged && near(snapshot(), invalid);
            m->createEndBarLines(false);
            m->removeSystemTrailer();
            score->doLayout();
            QVERIFY(terminalReady && wrongTimeRejected && malformedRejected && unchanged);
            QVERIFY2(pendingRejected && pendingReason.contains("pending penultimate courtesy"), qPrintable(pendingReason));
            QVERIFY2(prepared && !accepted, qPrintable(reason));
            QCOMPARE(rejected.width, qreal(123));
            QVERIFY(rejected.segments.empty() && rejected.measures.empty());
            QVERIFY(near(snapshot(), modern));
            }
      score->doLayout();
      QVERIFY(near(snapshot(), modern));
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      QVERIFY2(near(snapshot(), modern), "Cold signature collection differs after OFF/ON warming");
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      QVERIFY(near(snapshot(), legacy));
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      QVERIFY(near(snapshot(), modern));
      QVERIFY(!score->graceQueryFactory4());
      }

void TestSpacing::repeatCollection4_data()
      {
      QTest::addColumn<bool>("beams");
      QTest::newRow("legacy-beams") << false;
      QTest::newRow("studio-beams") << true;
      }

void TestSpacing::repeatCollection4()
      {
      QFETCH(bool, beams);
      QScopedValueRollback<bool> stretch(MScore::noHorizontalStretch, false);
      std::unique_ptr<MasterScore> score(new MasterScore(mscore->baseStyle()));
      {
      ScoreLoad loading;
      QVERIFY(score->loadMsc(root + "/" + DIR + "v0.235-repeat-collection-fitting.mscx", false)
            == Score::FileError::FILE_NO_ERROR);
      }
      score->setStyleValue(Sid::beamLayout4, beams);
      QVERIFY(score->styleB(Sid::spacingLayout4));
      const auto snapshot = [&]() {
            auto values = graceMergedSnapshot4(score.get());
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  values.insert(values.end(), {qreal(m->repeatStart()), qreal(m->repeatEnd()), qreal(m->repeatCount())});
                  for (const Segment& segment : m->segments())
                        for (const Element* item : segment.elist()) if (item && item->isBarLine()) {
                              const BarLine* barline = toBarLine(item);
                              const QRectF r = barline->layoutRect();
                              values.insert(values.end(), {qreal(static_cast<int>(barline->barLineType())),
                                    qreal(barline->generated()), barline->ipos().x(), barline->ipos().y(),
                                    barline->offset().x(), barline->offset().y(), r.x(), r.y(), r.width(), r.height()});
                              }
                  }
            return values;
            };
      const auto near = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                  if (!qIsFinite(a[i]) || !qIsFinite(b[i]) || qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      std::vector<SpacingDecision4> events;
      {
      SpacingDecisionScope4 observer(score.get(), [&events](const SpacingDecision4& e) { events.push_back(e); });
      score->doLayout();
      }
      const auto modern = snapshot();
      int candidates = 0, rollbacks = 0, finals = 0, retainedMeasures = 0;
      Fraction endTick(0, 1);
      for (size_t i = 0; i < events.size(); ++i) {
            const SpacingDecision4& event = events[i];
            QVERIFY2(event.path == SpacingPath4::FULL_PLAN, qPrintable(event.reason));
            if (event.pass == SpacingPass4::CANDIDATE) {
                  ++candidates;
                  if (event.measures <= 1 || event.width <= event.available) continue;
                  ++rollbacks;
                  QVERIFY(i + 1 < events.size());
                  const SpacingDecision4& retained = events[i + 1];
                  QVERIFY(retained.pass == SpacingPass4::FINAL);
                  QCOMPARE(retained.firstTick, event.firstTick);
                  QCOMPARE(retained.endTick, event.endTick - Fraction(1, 1));
                  QCOMPARE(retained.measures, event.measures - 1);
                  QVERIFY(i + 2 < events.size());
                  const SpacingDecision4& retried = events[i + 2];
                  QVERIFY(retried.pass == SpacingPass4::CANDIDATE);
                  QCOMPARE(retried.firstTick, retained.endTick);
                  QCOMPARE(retried.measures, 1);
                  }
            else {
                  ++finals;
                  QCOMPARE(event.firstTick, endTick);
                  endTick = event.endTick;
                  retainedMeasures += event.measures;
                  }
            }
      QVERIFY(candidates > 8 && rollbacks > 0 && finals > 1);
      QCOMPARE(retainedMeasures, 8);
      QCOMPARE(endTick, Fraction(8, 1));
      QString reason;
      int systems = 0;
      for (System* system : score->systems()) {
            if (system->vbox()) continue;
            ++systems;
            QVERIFY2(spacingRepeatBarlinesReady4(system, SpacingPass4::FINAL, &reason), qPrintable(reason));
            SpacingAttachmentPreparation4 owner;
            QVERIFY2(snapshotLaidOutSystemSpacing4(system, owner, &reason), qPrintable(reason));
            SpacingSystemRequest4 request;
            request.pass = SpacingPass4::FINAL;
            request.availableWidth = system->width();
            request.fillLimit = score->styleD(Sid::lastSystemFillLimit);
            request.next = system->lastMeasure()->next();
            SpacingPlan4 plan;
            QVERIFY2(selectSystemSpacingPlan4(system, request, owner, plan, &reason), qPrintable(reason));
            QVERIFY2(spacingPlanPublicationValid4(system, plan, &reason), qPrintable(reason));
            QVERIFY2(spacingPlanMatches4(plan, &reason), qPrintable(reason));
            }
      QCOMPARE(systems, finals);
      QVERIFY(near(snapshot(), modern)); // Readiness/replanning must not publish.

      // Exercise real native barline identities in both collection phases.
      // C3/C4/C5 contain an ordinary end immediately before an interior start.
      Measure* c3 = score->firstMeasure()->nextMeasure()->nextMeasure();
      std::array<Measure*, 3> prefix = {{c3, c3->nextMeasure(), c3->nextMeasure()->nextMeasure()}};
      std::array<System*, 3> parents;
      std::array<Segment*, 3> ends;
      std::array<bool, 3> enabled;
      for (size_t i = 0; i < prefix.size(); ++i) {
            parents[i] = prefix[i]->system();
            ends[i] = prefix[i]->findSegmentR(SegmentType::EndBarLine, prefix[i]->ticks());
            QVERIFY(ends[i]);
            enabled[i] = ends[i]->enabled();
            }
      bool trialReady, finalRejectsTrial, finalReady, trialRejectsFinal, staleOutsideReady, unchanged;
      {
      System trial(score.get());
      for (size_t i = 0; i < prefix.size(); ++i) {
            trial.appendMeasure(prefix[i]);
            ends[i]->setEnabled(true);
            }
      auto before = snapshot();
      trialReady = spacingRepeatBarlinesReady4(&trial, SpacingPass4::CANDIDATE, &reason);
      finalRejectsTrial = !spacingRepeatBarlinesReady4(&trial, SpacingPass4::FINAL, &reason);
      unchanged = near(snapshot(), before);
      ends[1]->setEnabled(false);
      before = snapshot();
      finalReady = spacingRepeatBarlinesReady4(&trial, SpacingPass4::FINAL, &reason);
      trialRejectsFinal = !spacingRepeatBarlinesReady4(&trial, SpacingPass4::CANDIDATE, &reason);
      unchanged = unchanged && near(snapshot(), before);
      trial.removeLastMeasure();
      prefix[2]->setSystem(&trial); // Deliberately stale parent, outside the collected vector.
      ends[1]->setEnabled(true);
      before = snapshot();
      staleOutsideReady = spacingRepeatBarlinesReady4(&trial, SpacingPass4::CANDIDATE, &reason);
      unchanged = unchanged && near(snapshot(), before);
      // Restore before assertions or destruction of the borrowed test system.
      for (size_t i = 0; i < prefix.size(); ++i) {
            prefix[i]->setSystem(parents[i]);
            ends[i]->setEnabled(enabled[i]);
            }
      }
      QVERIFY(trialReady && finalRejectsTrial && finalReady && trialRejectsFinal && staleOutsideReady && unchanged);
      QVERIFY(near(snapshot(), modern));
      Segment* end = score->firstMeasure()->findSegmentR(SegmentType::EndBarLine, score->firstMeasure()->ticks());
      QVERIFY(end && end->enabled());
      end->setEnabled(false);
      const auto invalid = snapshot();
      const bool rejected = !spacingRepeatBarlinesReady4(score->firstMeasure()->system(), SpacingPass4::FINAL, &reason);
      unchanged = near(snapshot(), invalid);
      end->setEnabled(true);
      QVERIFY(rejected && unchanged);
      QVERIFY(near(snapshot(), modern));
      score->doLayout();
      QVERIFY(near(snapshot(), modern));
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      QVERIFY2(near(snapshot(), modern), "Cold repeat collection differs after OFF/ON warming");
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      QVERIFY(near(snapshot(), legacy));
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      QVERIFY(near(snapshot(), modern));
      QVERIFY(!score->graceQueryFactory4());
      }

void TestSpacing::repeatBarlinePlanning4_data()
      {
      QTest::addColumn<QString>("mode");
      QTest::addColumn<bool>("beams");
      for (const QString& mode : {QString("natural"), QString("justified"), QString("fitting"), QString("squeeze")})
            for (bool beams : {false, true})
                  QTest::newRow(qPrintable(mode + (beams ? "-studio" : "-legacy"))) << mode << beams;
      }

void TestSpacing::repeatBarlinePlanning4()
      {
      QFETCH(QString, mode);
      QFETCH(bool, beams);
      QScopedValueRollback<bool> stretch(MScore::noHorizontalStretch, false);
      std::unique_ptr<MasterScore> score(new MasterScore(mscore->baseStyle()));
      {
      ScoreLoad loading;
      QVERIFY(score->loadMsc(root + "/" + DIR + "v0.230-repeat-barlines-" + mode + ".mscx", false)
            == Score::FileError::FILE_NO_ERROR);
      }
      score->setStyleValue(Sid::beamLayout4, beams);
      QVERIFY(score->styleB(Sid::spacingLayout4));
      if (mode == "squeeze") QCOMPARE(score->firstMeasure()->userStretch(), qreal(3));
      const auto snapshot = [&]() {
            auto values = graceMergedSnapshot4(score.get());
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  values.insert(values.end(), {qreal(m->repeatStart()), qreal(m->repeatEnd()), qreal(m->repeatCount())});
                  for (const Segment& s : m->segments()) {
                        if (!s.enabled() && (s.header() || s.trailer())) continue;
                        for (const Element* item : s.elist()) if (item && item->isBarLine()) {
                              const BarLine* barline = toBarLine(item);
                              const QRectF r = barline->layoutRect();
                              values.insert(values.end(), {qreal(static_cast<int>(barline->barLineType())),
                                    barline->ipos().x(), barline->ipos().y(), barline->offset().x(), barline->offset().y(),
                                    barline->mag(), r.x(), r.y(), r.width(), r.height()});
                              }
                        }
                  }
            return values;
            };
      const auto near = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                  if (!qIsFinite(a[i]) || !qIsFinite(b[i]) || qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      std::vector<SpacingDecision4> events;
      {
      SpacingDecisionScope4 observer(score.get(), [&events](const SpacingDecision4& e) { events.push_back(e); });
      score->doLayout();
      }
      int finals = 0, candidates = 0, plannedCandidates = 0;
      for (const auto& event : events) {
            if (event.pass == SpacingPass4::FINAL) {
                  ++finals;
                  QVERIFY2(event.path == SpacingPath4::FULL_PLAN, qPrintable(event.reason));
                  }
            else {
                  ++candidates;
                  if (event.path == SpacingPath4::FULL_PLAN) ++plannedCandidates;
                  else {
                        QVERIFY(event.path == SpacingPath4::NATIVE_WIDTHS);
                        QVERIFY(!event.reason.isEmpty());
                        }
                  }
            }
      QVERIFY(finals > 0 && candidates > 0 && plannedCandidates > 0);
      if (mode == "fitting") QVERIFY(finals > 1);
      const auto modern = snapshot();
      const SpacingPaddingTable4 table(score.get());
      BarLine* edit = nullptr;
      int starts = 0, joins = 0, headerChecks = 0;
      QString reason;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            if (!m->repeatStart()) continue;
            const Segment* start = m->findSegmentR(SegmentType::StartRepeatBarLine, Fraction(0, 1));
            QVERIFY(start && start->enabled() && start->visible() && start->ticks().isZero());
            QVERIFY(start->element(0) && start->element(0)->isBarLine());
            BarLine* barline = toBarLine(start->element(0));
            QCOMPARE(barline->barLineType(), BarLineType::START_REPEAT);
            if (!edit) edit = barline;
            ++starts;
            SpacingShape4 projected;
            QVERIFY(spacingCollisionProjection4(table, spacingStaffShape4(start, 0), score->spatium(), projected));
            QVERIFY(projected.complete() && !projected.elements.empty());
            for (const auto& part : projected.elements) QCOMPARE(part.item, static_cast<const Element*>(barline));
            Measure* previous = m->prevMeasure();
            if (previous && previous->system() == m->system() && previous->repeatEnd()) {
                  const Segment* end = previous->findSegmentR(SegmentType::EndBarLine, previous->ticks());
                  QVERIFY(end && end->enabled() && end->element(0) && end->element(0)->isBarLine());
                  const BarLine* before = toBarLine(end->element(0));
                  const qreal extent = before->layoutRect().right() + before->pos().x();
                  const qreal expected = std::max(extent - score->styleP(Sid::endBarWidth), qreal(0));
                  const qreal actual = m->x() + start->x() - previous->x() - end->x();
                  QVERIFY2(qAbs(actual - expected) < 1e-6,
                        qPrintable(QString("shared repeat stroke: actual=%1 expected=%2").arg(actual).arg(expected)));
                  // The preceding measure ends at the start-repeat origin,
                  // so its physical thick stroke may extend beyond its bbox.
                  QVERIFY(qAbs(end->width() - actual) < 1e-6);
                  ++joins;
                  }
            if (mode == "natural" && m == m->system()->firstMeasure()) {
                  qreal expectedX = m->system()->leftMargin();
                  for (const Segment& header : m->segments()) {
                        if (&header == start) break;
                        if (!header.enabled() || !header.visible() || header.allElementsInvisible()) continue;
                        if (!(header.isHeaderClefType() || header.isKeySigType() || header.isTimeSigType())) continue;
                        SpacingShape4 shape;
                        QVERIFY(spacingCollisionProjection4(table, spacingStaffShape4(&header, 0), score->spatium(), shape));
                        qreal right = 0, minLeft = -std::numeric_limits<qreal>::max(), distance = 0;
                        for (const auto& part : shape.elements) right = std::max(right, part.rect.right());
                        for (const auto& part : projected.elements) if (part.rect.height() != 0)
                              minLeft = std::max(minLeft, -part.rect.left());
                        QVERIFY(qIsFinite(minLeft));
                        QVERIFY(spacingShapeDistance4(table, shape, projected, score->spatium(), 1, distance));
                        const Sid gap = header.isTimeSigType() ? Sid::systemHeaderTimeSigDistance : Sid::systemHeaderDistance;
                        distance = std::max(distance, std::max(right, right + score->styleP(gap)));
                        distance -= score->styleP(Sid::endBarWidth) + .5 * score->styleP(Sid::endBarDistance);
                        distance += std::max(qreal(0), 1.5 * score->spatium() - (distance - right - minLeft));
                        expectedX = std::max(expectedX, m->x() + header.x() + distance);
                        ++headerChecks;
                        }
                  QVERIFY(qAbs(m->x() + start->x() - expectedX) < 1e-6);
                  }
            }
      QCOMPARE(starts, mode == "squeeze" ? 1 : 5);
      QVERIFY(edit);
      if (mode == "natural" || mode == "justified") QVERIFY(joins > 0);
      if (mode == "natural") QVERIFY(headerChecks > 0);
      for (System* system : score->systems()) {
            if (system->vbox()) continue;
            SpacingAttachmentPreparation4 owner;
            QVERIFY2(snapshotLaidOutSystemSpacing4(system, owner, &reason), qPrintable(reason));
            SpacingSystemRequest4 request;
            request.pass = SpacingPass4::FINAL;
            request.availableWidth = system->width();
            request.fillLimit = score->styleD(Sid::lastSystemFillLimit);
            request.next = system->lastMeasure()->next();
            if (mode == "squeeze") {
                  SpacingPlan4 natural;
                  QVERIFY2(buildSpacingPlan4(system, natural, owner, &reason), qPrintable(reason));
                  QVERIFY(natural.width > request.availableWidth);
                  }
            SpacingPlan4 plan;
            QVERIFY2(selectSystemSpacingPlan4(system, request, owner, plan, &reason), qPrintable(reason));
            QVERIFY2(spacingPlanPublicationValid4(system, plan, &reason), qPrintable(reason));
            QVERIFY2(spacingPlanMatches4(plan, &reason), qPrintable(reason));
            }
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      QVERIFY2(near(snapshot(), modern), "First repeat layout differs after OFF/ON warming");
      if (mode == "natural" && beams) {
            const QPointF offset = edit->offset();
            score->startCmd();
            edit->undoChangeProperty(Pid::OFFSET, offset + QPointF(.15 * score->spatium(), 0));
            score->endCmd();
            score->doLayout();
            const auto edited = snapshot();
            QVERIFY(!near(edited, modern));
            score->undoRedo(true, nullptr);
            score->doLayout();
            QVERIFY(near(snapshot(), modern));
            score->undoRedo(false, nullptr);
            score->doLayout();
            QVERIFY(near(snapshot(), edited));
            score->undoRedo(true, nullptr);
            score->doLayout();
            const QRectF box = edit->bbox();
            edit->setbbox(QRectF(box.x(), box.y(), 0, box.height()));
            const auto invalid = snapshot();
            const bool active = edit->segment()->enabled() && edit->segment()->visible()
                  && !edit->segment()->allElementsInvisible(); // Barline types stay active even at zero width.
            SpacingPlan4 rejected;
            rejected.width = 123;
            bool prepared = false, accepted = true;
            {
            SpacingAttachmentPreparation4 owner;
            prepared = snapshotLaidOutSystemSpacing4(edit->measure()->system(), owner, &reason);
            if (prepared) accepted = buildSpacingPlan4(edit->measure()->system(), rejected, owner, &reason);
            }
            const bool unchanged = near(snapshot(), invalid);
            edit->setbbox(box);
            QVERIFY2(active && prepared && !accepted,
                  qPrintable(QString("active=%1 prepared=%2 accepted=%3 reason=%4").arg(active).arg(prepared).arg(accepted).arg(reason)));
            QCOMPARE(reason, QString("Unadapted start-repeat glyph, staff, decoration or dimensions"));
            QCOMPARE(rejected.width, qreal(123));
            QVERIFY(rejected.segments.empty() && rejected.measures.empty() && unchanged);
            }
      score->doLayout();
      QVERIFY(near(snapshot(), modern));
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      QVERIFY(near(snapshot(), legacy));
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      QVERIFY(near(snapshot(), modern));
      QVERIFY(!score->graceQueryFactory4());
      }

void TestSpacing::breathCaesuraPlanning4_data()
      {
      QTest::addColumn<QString>("mode");
      QTest::addColumn<bool>("beams");
      for (const QString& mode : {QString("natural"), QString("justified"), QString("fitting"), QString("squeeze")})
            for (bool beams : {false, true})
                  QTest::newRow(qPrintable(mode + (beams ? "-studio" : "-legacy"))) << mode << beams;
      }

void TestSpacing::breathCaesuraPlanning4()
      {
      QFETCH(QString, mode);
      QFETCH(bool, beams);
      std::unique_ptr<MasterScore> score(readScore(DIR + "v0.217-breath-caesura-" + mode + ".mscx"));
      QVERIFY(score);
      QScopedValueRollback<bool> stretch(MScore::noHorizontalStretch, false);
      score->setStyleValue(Sid::beamLayout4, beams);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto snapshot = [&]() {
            auto values = graceMergedSnapshot4(score.get());
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
                  for (const Segment& s : m->segments())
                        for (const Element* e : s.elist()) if (e && e->isBreath()) {
                              const QRectF r = e->bbox();
                              values.insert(values.end(), {e->ipos().x(), e->ipos().y(), e->offset().x(), e->offset().y(),
                                    r.x(), r.y(), r.width(), r.height()});
                              }
            return values;
            };
      const auto near = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                  if (!qIsFinite(a[i]) || !qIsFinite(b[i]) || qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      std::vector<std::pair<Breath*, qreal>> pauses;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
            for (const Segment& s : m->segments())
                  for (Element* e : s.elist()) if (e && e->isBreath()) {
                        QCOMPARE(e->ipos().x(), qreal(0));
                        pauses.push_back({toBreath(e), toBreath(e)->pause()});
                        }
      QVERIFY(!pauses.empty());
      const auto legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      std::vector<SpacingDecision4> events;
      {
      SpacingDecisionScope4 observer(score.get(), [&events](const SpacingDecision4& e) { events.push_back(e); });
      score->doLayout();
      }
      int finals = 0, candidates = 0, marks = 0, crossVoice = 0;
      for (const auto& e : events) {
            if (e.pass == SpacingPass4::FINAL) {
                  ++finals;
                  QVERIFY2(e.path == SpacingPath4::FULL_PLAN, qPrintable(e.reason));
                  }
            else if (e.path == SpacingPath4::FULL_PLAN) ++candidates;
            else QVERIFY(!e.reason.isEmpty());
            }
      QVERIFY(finals > 0 && candidates > 0);
      if (mode == "fitting") QVERIFY(finals > 1);
      const auto modern = snapshot();
      const SpacingPaddingTable4 table(score.get());
      for (const auto& saved : pauses) {
            Breath* breath = saved.first;
            QCOMPARE(breath->pause(), saved.second);
            QVERIFY(qAbs(breath->ipos().x() + breath->bbox().right()) < 1e-6);
            const Segment* s = breath->segment();
            QVERIFY(s->isBreathType() && s->ticks().isZero());
            const SpacingShape4 shape = spacingStaffShape4(s, breath->staffIdx());
            QVERIFY(shape.complete() && !shape.elements.empty());
            bool owned = false;
            for (const auto& part : shape.elements) {
                  QVERIFY(part.item && part.item->isBreath() && part.item->parent() == s);
                  owned = owned || part.item == breath;
                  }
            QVERIFY(owned);
            const Segment* prev = s->prev(SegmentType::ChordRest);
            QVERIFY(prev && prev->element(breath->track()));
            // The mark's reserved gap is fixed space, not extra rhythmic
            // stretch charged to the quarter/eighth-note interval.
            QVERIFY(prev->spacingWidthOffset4() < 0);
            const Element* other = prev->element(breath->track());
            if (other->isChord()) other = toChord(other)->upNote();
            QVERIFY(other->isNote() || other->isRest());
            const qreal expected = score->spatium() * (breath->mag() + other->mag()) / 2;
            SpacingPairPolicy4 policy;
            QVERIFY(spacingPairPolicy4(table, other, breath, breath->spatium(), policy));
            QVERIFY(policy.kerning == SpacingKerning4::NON_KERNING);
            QVERIFY(qAbs(policy.padding - expected) < 1e-6);
            QVERIFY(spacingPairPolicy4(table, breath, other, breath->spatium(), policy));
            QVERIFY(policy.kerning == SpacingKerning4::NON_KERNING);
            QVERIFY(qAbs(policy.padding - expected) < 1e-6);
            if (breath->track() == 0 && prev->element(1) && prev->element(1)->isChord()) {
                  const Note* second = toChord(prev->element(1))->upNote();
                  QVERIFY(spacingPairPolicy4(table, breath, second, breath->spatium(), policy));
                  QVERIFY(policy.kerning == SpacingKerning4::KERNING);
                  QVERIFY(spacingPairPolicy4(table, second, breath, breath->spatium(), policy));
                  QVERIFY(policy.kerning == SpacingKerning4::KERNING);
                  ++crossVoice;
                  }
            ++marks;
            }
      QVERIFY(marks == (mode == "squeeze" ? 1 : 7));
      if (mode != "squeeze") QVERIFY(crossVoice > 0);
      int cadenceChecks = 0, measureIndex = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++measureIndex) {
            if (mode != "squeeze" && measureIndex != 0 && measureIndex != 2) continue;
            std::vector<qreal> centers;
            std::vector<const Note*> notes;
            const Segment* mark = nullptr;
            for (const Segment& s : m->segments()) {
                  if (s.isBreathType()) mark = &s;
                  if (!s.isChordRestType() || !s.element(0) || !s.element(0)->isChord()) continue;
                  const Note* note = toChord(s.element(0))->upNote();
                  notes.push_back(note);
                  centers.push_back(note->pagePos().x() + note->bbox().center().x());
                  }
            QCOMPARE(centers.size(), size_t(4));
            QVERIFY(mark);
            const qreal first = centers[1] - centers[0];
            QVERIFY(first > 0 && qIsFinite(first));
            qreal markedInterval = first;
            if (mode != "squeeze") {
                  // A natural-width caesura can genuinely need more room.
                  // Permit only its measured collision floor, not an extra
                  // rhythmic advance plus the mark's fixed reserved gap.
                  SpacingShape4 before, glyph, after;
                  const qreal sp = score->spatium();
                  QVERIFY(spacingCollisionProjection4(table, spacingStaffShape4(notes[1]->chord()->segment(), 0), sp, before));
                  QVERIFY(spacingCollisionProjection4(table, spacingStaffShape4(mark, 0), sp, glyph));
                  QVERIFY(spacingCollisionProjection4(table, spacingStaffShape4(notes[2]->chord()->segment(), 0), sp, after));
                  qreal left = 0, right = 0;
                  QVERIFY(spacingShapeDistance4(table, before, glyph, sp, 1, left));
                  QVERIFY(spacingShapeDistance4(table, glyph, after, sp, 1, right));
                  const qreal a = notes[1]->chord()->x() + notes[1]->x() + notes[1]->bbox().center().x();
                  const qreal b = notes[2]->chord()->x() + notes[2]->x() + notes[2]->bbox().center().x();
                  markedInterval = std::max(first, left + right + b - a);
                  }
            for (size_t i = 2; i < centers.size(); ++i) {
                  const qreal interval = centers[i] - centers[i - 1];
                  const qreal expected = i == 2 ? markedInterval : first;
                  QVERIFY2(qIsFinite(interval) && qAbs(interval - expected) < .03 * score->spatium(),
                        qPrintable(QString("measure=%1 interval=%2 expected=%3 clean=%4 (sp)")
                              .arg(measureIndex + 1).arg(interval / score->spatium())
                              .arg(expected / score->spatium()).arg(first / score->spatium())));
                  }
            ++cadenceChecks;
            }
      QCOMPARE(cadenceChecks, mode == "squeeze" ? 1 : 2);
      QString reason;
      for (System* system : score->systems()) {
            if (system->vbox()) continue;
            SpacingAttachmentPreparation4 owner;
            QVERIFY2(snapshotLaidOutSystemSpacing4(system, owner, &reason), qPrintable(reason));
            SpacingSystemRequest4 request;
            request.pass = SpacingPass4::FINAL;
            request.availableWidth = system->width();
            request.fillLimit = score->styleD(Sid::lastSystemFillLimit);
            request.next = system->lastMeasure()->next();
            if (mode == "squeeze") {
                  SpacingPlan4 natural;
                  QVERIFY2(buildSpacingPlan4(system, natural, owner, &reason), qPrintable(reason));
                  QVERIFY(natural.width > request.availableWidth);
                  }
            SpacingPlan4 selected;
            QVERIFY2(selectSystemSpacingPlan4(system, request, owner, selected, &reason), qPrintable(reason));
            QVERIFY2(spacingPlanPublicationValid4(system, selected, &reason), qPrintable(reason));
            QVERIFY2(spacingPlanMatches4(selected, &reason), qPrintable(reason));
            }
      if (mode == "natural" && beams) {
            Breath* edit = pauses.front().first;
            // Pause is playback data, not a horizontal spacing weight.
            edit->setPause(pauses.front().second + 1.25);
            score->doLayout();
            const bool pauseInvariant = near(snapshot(), modern);
            edit->setPause(pauses.front().second);
            score->doLayout();
            QVERIFY(pauseInvariant);
            const QPointF offset = edit->offset();
            score->startCmd();
            edit->undoChangeProperty(Pid::OFFSET, offset + QPointF(.2 * score->spatium(), 0));
            score->endCmd();
            score->doLayout();
            const auto edited = snapshot();
            QVERIFY(!near(edited, modern));
            score->undoRedo(true, nullptr);
            score->doLayout();
            QVERIFY(near(snapshot(), modern));
            score->undoRedo(false, nullptr);
            score->doLayout();
            QVERIFY(near(snapshot(), edited));
            score->undoRedo(true, nullptr);
            score->doLayout();
            const QRectF box = edit->bbox();
            // A zero-width breath is intentionally ignored by the native
            // allElementsInvisible gate. Keep positive width so this probe
            // reaches the active breath dimension guard instead.
            edit->setbbox(QRectF(box.x(), box.y(), box.width(), 0));
            const Segment* boundary = edit->segment();
            const bool active = boundary->enabled() && boundary->visible() && !boundary->allElementsInvisible();
            const auto invalid = snapshot();
            bool prepared = false, accepted = true;
            SpacingPlan4 rejected;
            rejected.width = 123;
            {
            SpacingAttachmentPreparation4 owner;
            prepared = snapshotLaidOutSystemSpacing4(edit->segment()->measure()->system(), owner, &reason);
            if (prepared) accepted = buildSpacingPlan4(edit->segment()->measure()->system(), rejected, owner, &reason);
            }
            const bool unchanged = near(snapshot(), invalid);
            edit->setbbox(box); // Restore native geometry before any assertion exits.
            QVERIFY2(active && prepared && !accepted,
                  qPrintable(QString("active=%1 prepared=%2 accepted=%3 reason=%4")
                        .arg(active).arg(prepared).arg(accepted).arg(reason)));
            QVERIFY2(reason == "Unadapted breath glyph, staff, visibility or dimensions", qPrintable(reason));
            QCOMPARE(rejected.width, qreal(123));
            QVERIFY(rejected.segments.empty() && rejected.measures.empty() && unchanged);
            }
      score->doLayout();
      QVERIFY(near(snapshot(), modern));
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      QVERIFY(near(snapshot(), legacy));
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      QVERIFY(near(snapshot(), modern));
      for (const auto& saved : pauses) QCOMPARE(saved.first->pause(), saved.second);
      QVERIFY(!score->graceQueryFactory4());
      }

void TestSpacing::clefSignaturePlanning4_data()
      {
      QTest::addColumn<QString>("mode");
      QTest::addColumn<bool>("beams");
      for (const QString& mode : {QString("natural"), QString("justified"), QString("fitting"), QString("squeeze")})
            for (bool beams : {false, true})
                  QTest::newRow(qPrintable(mode + (beams ? "-studio" : "-legacy"))) << mode << beams;
      }

void TestSpacing::clefSignaturePlanning4()
      {
      QFETCH(QString, mode);
      QFETCH(bool, beams);
      QScopedValueRollback<bool> stretch(MScore::noHorizontalStretch, false);
      std::unique_ptr<MasterScore> score(new MasterScore(mscore->baseStyle()));
      {
      ScoreLoad loading;
      const QString version = mode == "fitting" || mode == "squeeze" ? "v0.226" : "v0.213";
      QVERIFY(score->loadMsc(root + "/" + DIR + version + "-clef-signatures-" + mode + ".mscx", false)
            == Score::FileError::FILE_NO_ERROR);
      }
      score->setStyleValue(Sid::beamLayout4, beams);
      QVERIFY(score->styleB(Sid::spacingLayout4));
      if (mode == "squeeze") QCOMPARE(score->firstMeasure()->userStretch(), qreal(3));
      const auto snapshot = [&]() {
            auto values = graceMergedSnapshot4(score.get());
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
                  for (const Segment& s : m->segments()) {
                        if (!s.enabled() && (s.header() || s.trailer())) continue;
                        for (const Element* e : s.elist()) if (e && (e->isClef() || e->isKeySig() || e->isTimeSig())) {
                              values.insert(values.end(), {e->x(), e->y(), e->mag(), e->offset().x(), e->offset().y()});
                              for (const QRectF& r : e->shape())
                                    values.insert(values.end(), {r.x(), r.y(), r.width(), r.height()});
                              }
                        }
            return values;
            };
      const auto near = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                  if (!qIsFinite(a[i]) || !qIsFinite(b[i]) || qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      // Check physical signature order on the first layout too. Warming the
      // header/shape caches with a legacy layout must not be a prerequisite.
      QString signatureFailure;
      const auto orderedSignatures = [&]() {
            const auto fail = [&](const QString& message) { signatureFailure = message; return false; };
            int initialChanges = 0;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  qreal right = -std::numeric_limits<qreal>::max();
                  int rank = -1;
                  for (const Segment& s : m->segments()) {
                        if (!s.enabled() || !s.visible() || !s.rtick().isZero()) continue;
                        const int current = s.isHeaderClefType() ? 0 : s.isKeySigType() ? 1
                              : s.isTimeSigType() ? 2 : s.isClefType() ? 3 : -1;
                        if (current < 0) continue;
                        const Element* item = s.element(0);
                        if (!item || !item->visible() || item->bbox().isEmpty()) continue;
                        if (current <= rank) return fail(QString("measure=%1 signature rank=%2 after %3")
                              .arg(m->no() + 1).arg(current).arg(rank));
                        const qreal left = item->pagePos().x() + item->bbox().left();
                        if (!qIsFinite(left) || left < right - 1e-6)
                              return fail(QString("measure=%1 %2 left=%3 preceding right=%4 (sp)")
                                    .arg(m->no() + 1).arg(item->name()).arg(left / score->spatium()).arg(right / score->spatium()));
                        right = item->pagePos().x() + item->bbox().right();
                        rank = current;
                        initialChanges += s.isClefType();
                        }
                  const Segment* cr = m->first(SegmentType::ChordRest);
                  if (!cr || !cr->element(0) || !cr->element(0)->isChord()) return fail("Missing first chord");
                  const Note* note = toChord(cr->element(0))->upNote();
                  if (note->pagePos().x() + note->bbox().left() < right - 1e-6)
                        return fail(QString("measure=%1 first note overlaps its initial signatures").arg(m->no() + 1));
                  }
            if (initialChanges != (mode == "squeeze" ? 0 : 3))
                  return fail(QString("Unexpected count of initial ordinary clefs: %1").arg(initialChanges));
            return true;
            };
      std::vector<SpacingDecision4> firstEvents;
      {
      SpacingDecisionScope4 observer(score.get(), [&firstEvents](const SpacingDecision4& e) { firstEvents.push_back(e); });
      score->doLayout();
      }
      int firstFinals = 0;
      for (const auto& e : firstEvents) if (e.pass == SpacingPass4::FINAL) {
            ++firstFinals;
            QVERIFY2(e.path == SpacingPath4::FULL_PLAN, qPrintable("first layout: " + e.reason));
            }
      QVERIFY(firstFinals > 0);
      const bool firstOrdered = orderedSignatures();
      QVERIFY2(firstOrdered, qPrintable("first layout: " + signatureFailure));
      const auto firstLayout = snapshot();
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      std::vector<SpacingDecision4> events;
      {
      SpacingDecisionScope4 observer(score.get(), [&events](const SpacingDecision4& e) { events.push_back(e); });
      score->doLayout();
      }
      int finals = 0, midClefs = 0;
      for (const auto& e : events) {
            if (e.pass == SpacingPass4::FINAL) {
                  ++finals;
                  QVERIFY2(e.path == SpacingPath4::FULL_PLAN, qPrintable(e.reason));
                  }
            else if (e.path != SpacingPath4::FULL_PLAN) QVERIFY(!e.reason.isEmpty());
            }
      QVERIFY(finals > 0);
      if (mode == "fitting") {
            QCOMPARE(finals, 3);
            for (const System* system : score->systems()) {
                  if (system->vbox()) continue;
                  int measures = 0;
                  for (const MeasureBase* mb : system->measures()) measures += mb->isMeasure();
                  QCOMPARE(measures, 2); // Pinned S1-S2 / S3-S4 / S5-S6 reference.
                  }
            }
      const auto modern = snapshot();
      const bool modernOrdered = orderedSignatures();
      QVERIFY2(modernOrdered, qPrintable("relayout: " + signatureFailure));
      QVERIFY2(near(firstLayout, modern), "First layout differs from the OFF/ON-warmed layout");
      QString reason;
      Clef* edit = nullptr;
      const SpacingPaddingTable4 table(score.get());
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
            for (const Segment& s : m->segments()) {
                  if (!s.enabled() || !s.isClefType() || s.rtick().isZero() || s.rtick() >= m->ticks()) continue;
                  Element* e = s.element(0);
                  QVERIFY(e && e->isClef());
                  Clef* clef = toClef(e);
                  if (!edit) edit = clef;
                  ++midClefs;
                  QVERIFY(clef->shape().size() > 1); // Leland's G/F/C cutout anchors.
                  QVERIFY(qAbs(clef->ipos().x() + clef->shape().right()) < 1e-6);
                  const SpacingShape4 shape = spacingStaffShape4(&s, 0);
                  QVERIFY(shape.complete() && !shape.elements.empty());
                  for (const auto& item : shape.elements) QCOMPARE(item.item, static_cast<const Element*>(clef));
                  const Segment* next = s.next(SegmentType::ChordRest);
                  QVERIFY(next && next->element(0) && next->element(0)->isChord());
                  const Note* note = toChord(next->element(0))->notes().front();
                  SpacingPairPolicy4 policy;
                  QVERIFY(spacingPairPolicy4(table, clef, note, clef->spatium(), policy));
                  QVERIFY(policy.kerning == SpacingKerning4::RIGHT_EDGE);
                  QVERIFY(spacingPairPolicy4(table, note, clef, clef->spatium(), policy));
                  QVERIFY(policy.kerning == SpacingKerning4::RIGHT_EDGE);
                  }
      QVERIFY(midClefs > 0 && edit);
      // Pin printed cadence independently of plan/publication agreement.
      // The squeeze's four quarters have room for equal intervals in Studio;
      // natural/justified/fitting intervals may instead bind at the clef's
      // measured collision floor, using its native cutout producer geometry.
      int cadenceChecks = 0, measureIndex = 0;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure(), ++measureIndex) {
            if (measureIndex != 0) continue; // S1 is the matched four-quarter control.
            std::vector<const Note*> notes;
            std::vector<qreal> centers;
            const Segment* mark = nullptr;
            for (const Segment& s : m->segments()) {
                  if (s.isClefType() && !s.rtick().isZero()) mark = &s;
                  if (!s.isChordRestType() || !s.element(0) || !s.element(0)->isChord()) continue;
                  const Note* note = toChord(s.element(0))->upNote();
                  notes.push_back(note);
                  centers.push_back(note->pagePos().x() + note->bbox().center().x());
                  }
            QCOMPARE(notes.size(), size_t(4));
            QVERIFY(mark);
            const qreal clean = centers[1] - centers[0];
            QVERIFY(qIsFinite(clean) && clean > 0);
            qreal marked = clean;
            if (mode != "squeeze") {
                  const qreal sp = score->spatium();
                  SpacingShape4 before, glyph, after;
                  QVERIFY(spacingCollisionProjection4(table, spacingStaffShape4(notes[1]->chord()->segment(), 0), sp, before));
                  QVERIFY(spacingCollisionProjection4(table, spacingStaffShape4(mark, 0), sp, glyph));
                  QVERIFY(spacingCollisionProjection4(table, spacingStaffShape4(notes[2]->chord()->segment(), 0), sp, after));
                  qreal left = 0, right = 0;
                  QVERIFY(spacingShapeDistance4(table, before, glyph, sp, 1, left));
                  QVERIFY(spacingShapeDistance4(table, glyph, after, sp, 1, right));
                  const qreal a = notes[1]->chord()->x() + notes[1]->x() + notes[1]->bbox().center().x();
                  const qreal b = notes[2]->chord()->x() + notes[2]->x() + notes[2]->bbox().center().x();
                  marked = std::max(clean, left + right + b - a);
                  }
            for (size_t i = 2; i < centers.size(); ++i) {
                  const qreal interval = centers[i] - centers[i - 1];
                  const qreal expected = i == 2 ? marked : clean;
                  QVERIFY2(qIsFinite(interval) && qAbs(interval - expected) < .03 * score->spatium(),
                        qPrintable(QString("clef measure=%1 interval=%2 expected=%3 clean=%4 (sp)")
                              .arg(measureIndex + 1).arg(interval / score->spatium())
                              .arg(expected / score->spatium()).arg(clean / score->spatium())));
                  }
            ++cadenceChecks;
            }
      QCOMPARE(cadenceChecks, 1);
      for (System* system : score->systems()) {
            if (system->vbox()) continue;
            SpacingAttachmentPreparation4 owner;
            QVERIFY2(snapshotLaidOutSystemSpacing4(system, owner, &reason), qPrintable(reason));
            SpacingSystemRequest4 request;
            request.pass = SpacingPass4::FINAL;
            request.availableWidth = system->width();
            request.fillLimit = score->styleD(Sid::lastSystemFillLimit);
            request.next = system->lastMeasure()->next();
            if (mode == "squeeze") {
                  SpacingPlan4 natural;
                  QVERIFY2(buildSpacingPlan4(system, natural, owner, &reason), qPrintable(reason));
                  QVERIFY(natural.width > request.availableWidth);
                  }
            SpacingPlan4 selected;
            QVERIFY2(selectSystemSpacingPlan4(system, request, owner, selected, &reason), qPrintable(reason));
            QVERIFY2(spacingPlanPublicationValid4(system, selected, &reason), qPrintable(reason));
            QVERIFY2(spacingPlanMatches4(selected, &reason), qPrintable(reason));
            }
      if (mode == "natural" && beams) {
            const QPointF offset = edit->offset();
            score->startCmd();
            edit->undoChangeProperty(Pid::OFFSET, offset + QPointF(.15 * score->spatium(), 0));
            score->endCmd();
            score->doLayout();
            const auto edited = snapshot();
            QVERIFY(!near(edited, modern));
            score->undoRedo(true, nullptr);
            score->doLayout();
            QVERIFY(near(snapshot(), modern));
            score->undoRedo(false, nullptr);
            score->doLayout();
            QVERIFY(near(snapshot(), edited));
            score->undoRedo(true, nullptr);
            }
      score->doLayout();
      QVERIFY(near(snapshot(), modern));
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      QVERIFY(near(snapshot(), legacy));
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      QVERIFY(near(snapshot(), modern));
      QVERIFY(!score->graceQueryFactory4());
      }

void TestSpacing::smallNotation4_data()
      {
      QTest::addColumn<QString>("mode");
      QTest::addColumn<bool>("beams");
      for (const QString& mode : {QString("natural"), QString("justified"), QString("fitting"), QString("squeeze")})
            for (bool beams : {false, true})
                  QTest::newRow(qPrintable(mode + (beams ? "-studio" : "-legacy"))) << mode << beams;
      }

void TestSpacing::smallNotation4()
      {
      QFETCH(QString, mode);
      QFETCH(bool, beams);
      std::unique_ptr<MasterScore> score(readScore(DIR + "v0.206-small-notation-" + mode + ".mscx"));
      QVERIFY(score);
      QScopedValueRollback<bool> stretch(MScore::noHorizontalStretch, false);
      score->setStyleValue(Sid::beamLayout4, beams);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto snapshot = [&]() {
            auto values = graceMergedSnapshot4(score.get());
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
                  for (const Segment& s : m->segments()) for (const Element* e : s.elist()) {
                        if (!e) continue;
                        values.insert(values.end(), {e->x(), e->y(), e->mag(), e->bbox().width(), e->bbox().height()});
                        if (e->isChord()) for (const Note* n : toChord(e)->notes())
                              values.insert(values.end(), {n->x(), n->y(), n->mag(), n->headWidth()});
                        }
            return values;
            };
      const auto near = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                  if (!qIsFinite(a[i]) || !qIsFinite(b[i]) || qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      const auto legacy = snapshot();
      score->setStyleValue(Sid::scaleRythmicSpacingForSmallNotes, false);
      score->doLayout();
      QVERIFY(near(snapshot(), legacy)); // The option has no effect on the legacy engine.
      score->setStyleValue(Sid::scaleRythmicSpacingForSmallNotes, true);
      score->setStyleValue(Sid::spacingLayout4, true);
      std::vector<SpacingDecision4> events;
      {
      SpacingDecisionScope4 observer(score.get(), [&events](const SpacingDecision4& e) { events.push_back(e); });
      score->doLayout();
      }
      int finals = 0;
      for (const auto& e : events) if (e.pass == SpacingPass4::FINAL) {
            ++finals;
            QVERIFY2(e.path == SpacingPath4::FULL_PLAN, qPrintable(e.reason));
            }
      QVERIFY(finals > 0);
      const auto modern = snapshot();
      QString reason;
      for (System* system : score->systems()) {
            if (system->vbox()) continue;
            SpacingAttachmentPreparation4 owner;
            QVERIFY2(snapshotLaidOutSystemSpacing4(system, owner, &reason), qPrintable(reason));
            SpacingSystemRequest4 request;
            request.pass = SpacingPass4::FINAL;
            request.availableWidth = system->width();
            request.fillLimit = score->styleD(Sid::lastSystemFillLimit);
            request.next = system->lastMeasure()->next();
            SpacingPlan4 selected;
            QVERIFY2(selectSystemSpacingPlan4(system, request, owner, selected, &reason), qPrintable(reason));
            QVERIFY2(spacingPlanPublicationValid4(system, selected, &reason), qPrintable(reason));
            QVERIFY2(spacingPlanMatches4(selected, &reason), qPrintable(reason));
            }
      // Independently labeled fixtures pin the source mixed-size decisions.
      if (mode == "natural") {
            int number = 0;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  ++number;
                  for (const Segment& s : m->segments()) {
                        if (!s.isChordRestType() || !s.enabled()) continue;
                        const bool scaled = number == 1 || number == 5 || number == 6 || number == 8
                              || (number == 3 && s.rtick().ticks() % 480 == 0);
                        score->setStyleValue(Sid::scaleRythmicSpacingForSmallNotes, false);
                        const qreal normal = segmentSpacingStretch4(&s);
                        score->setStyleValue(Sid::scaleRythmicSpacingForSmallNotes, true);
                        for (qreal size : {qreal(.1), qreal(.5), qreal(.7), qreal(1), qreal(2)}) {
                              score->setStyleValue(Sid::smallNoteMag, size);
                              QVERIFY(qAbs(segmentSpacingStretch4(&s) - normal * (scaled ? size : 1)) < 1e-9);
                              }
                        for (qreal bad : {qreal(0), std::numeric_limits<qreal>::quiet_NaN()}) {
                              score->setStyleValue(Sid::smallNoteMag, bad);
                              QVERIFY(scaled ? segmentSpacingStretch4(&s) == 0 : segmentSpacingStretch4(&s) == normal);
                              }
                        score->setStyleValue(Sid::smallNoteMag, .7);
                        }
                  }
            QCOMPARE(number, 8);
            }
      if (mode == "natural" && beams) {
            score->startCmd();
            score->undoChangeStyleVal(Sid::scaleRythmicSpacingForSmallNotes, false);
            score->endCmd();
            score->doLayout();
            const auto unscaled = snapshot();
            QVERIFY(!near(unscaled, modern));
            score->undoRedo(true, nullptr);
            score->doLayout();
            QVERIFY(near(snapshot(), modern));
            score->undoRedo(false, nullptr);
            score->doLayout();
            QVERIFY(near(snapshot(), unscaled));
            score->undoRedo(true, nullptr);
            }
      score->doLayout();
      QVERIFY(near(snapshot(), modern));
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      QVERIFY(near(snapshot(), legacy));
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      QVERIFY(near(snapshot(), modern));
      QVERIFY(!score->graceQueryFactory4());
      }

void TestSpacing::measureRestPlanning4_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<bool>("modernBeams");
      for (bool beams : {false, true}) {
            const QString suffix = beams ? "-studio-beams" : "-legacy-beams";
            for (const QString& mode : {QString("natural"), QString("justified"), QString("fitting"), QString("squeeze")})
                  QTest::newRow(qPrintable(mode + suffix)) << QString("v0.201-measure-rests-%1.mscx").arg(mode) << beams;
            }
      }

void TestSpacing::measureRestPlanning4()
      {
      QFETCH(QString, file);
      QFETCH(bool, modernBeams);
      std::unique_ptr<MasterScore> score(readScore(DIR + file));
      QVERIFY(score);
      QScopedValueRollback<bool> stretch(MScore::noHorizontalStretch, false);
      score->setStyleValue(Sid::beamLayout4, modernBeams);
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      const auto snapshot = [&]() {
            auto values = graceMergedSnapshot4(score.get());
            for (const System* system : score->systems()) values.push_back(system->width());
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  for (const Segment& s : m->segments()) {
                        if (!s.enabled() && (s.header() || s.trailer())) continue;
                        for (const Element* item : s.elist()) if (item && item->isRest()) {
                              values.insert(values.end(), {item->ipos().x(), item->ipos().y(), item->offset().x(), item->offset().y()});
                              const QRectF r = item->bbox();
                              values.insert(values.end(), {r.x(), r.y(), r.width(), r.height()});
                              }
                        }
                  }
            return values;
            };
      const auto same = [](const std::vector<qreal>& a, const std::vector<qreal>& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                  if (!qIsFinite(a[i]) || !qIsFinite(b[i]) || qAbs(a[i] - b[i]) > 1e-6) return false;
            return true;
            };
      const auto legacy = snapshot();
      score->setStyleValue(Sid::spacingLayout4, true);
      std::vector<SpacingDecision4> decisions;
      {
      SpacingDecisionScope4 scope(score.get(), [&decisions](const SpacingDecision4& e) { decisions.push_back(e); });
      score->doLayout();
      }
      const auto modern = snapshot();
      int finals = 0, candidates = 0, rests = 0, mixedChecks = 0;
      for (const auto& e : decisions) {
            if (e.pass == SpacingPass4::FINAL) {
                  ++finals;
                  QVERIFY2(e.path == SpacingPath4::FULL_PLAN, qPrintable(e.reason));
                  }
            else if (e.path == SpacingPath4::FULL_PLAN) ++candidates;
            }
      QVERIFY(finals > 0 && candidates > 0);
      if (file.contains("fitting")) QVERIFY(finals > 1);
      QString reason;
      std::vector<Rest*> fullRests;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (const Segment& segment : m->segments()) {
                  for (Element* item : segment.elist()) {
                        if (!item || !item->isRest() || !toRest(item)->isFullMeasureRest()) continue;
                        Rest* rest = toRest(item);
                        fullRests.push_back(rest);
                        ++rests;
                        QVERIFY(rest->rtick().isZero() && rest->actualTicks() == m->ticks());
                        // Check the visible body center against the native free
                        // interval, independently of its local-position formula.
                        qreal left = 0, right = m->width();
                        for (const Segment* s = segment.prevActive(); s; s = s->prevActive())
                              if (!s->allElementsInvisible()) left = std::max(left, s->x() + s->minRight());
                        for (const Segment* s = segment.nextActive(); s; s = s->nextActive()) {
                              if (!s->isChordRestType() && s->element(rest->staffIdx() * VOICES)) {
                                    right = s->x() - s->minLeft();
                                    break;
                                    }
                              }
                        const qreal center = segment.x() + rest->ipos().x() + rest->bbox().center().x();
                        QVERIFY(qIsFinite(center) && qAbs(center - .5 * (left + right)) < 1e-6);
                        // Stored native shapes still contain the rest for skyline
                        // use, even when horizontal spacing omits a moving voice.
                        const SpacingShape4 shape = spacingStaffShape4(&segment, rest->staffIdx());
                        bool owned = false;
                        for (const auto& e : shape.elements) owned = owned || e.item == rest;
                        QVERIFY(owned);
                        }
                  }
            }
      QVERIFY(rests > 0);
      for (System* system : score->systems()) {
            if (system->vbox()) continue;
            SpacingPlan4 natural, selected;
            const auto build = [&](SpacingPlan4& result) {
                  SpacingAttachmentPreparation4 owner;
                  if (!snapshotLaidOutSystemSpacing4(system, owner, &reason)) return false;
                  SpacingPlanOptions4 options;
                  options.slurPolicy = SpacingSlurPolicy4::PREPARED;
                  return buildSpacingPlan4(system, result, owner, &reason, options);
                  };
            QVERIFY2(build(natural), qPrintable(reason));
            if (file.contains("squeeze")) QVERIFY(natural.width > system->width() + 1e-6);
            {
            SpacingAttachmentPreparation4 owner;
            QVERIFY2(snapshotLaidOutSystemSpacing4(system, owner, &reason), qPrintable(reason));
            SpacingSystemRequest4 request;
            request.pass = SpacingPass4::FINAL;
            request.availableWidth = system->width();
            request.fillLimit = score->styleD(Sid::lastSystemFillLimit);
            request.next = system->lastMeasure()->next();
            QVERIFY2(selectSystemSpacingPlan4(system, request, owner, selected, &reason), qPrintable(reason));
            }
            QVERIFY2(spacingPlanMatches4(selected, &reason), qPrintable(reason));
            QVERIFY2(spacingPlanPublicationValid4(system, selected, &reason), qPrintable(reason));
            // Normalize only computed rest x, including all-full-rest onsets.
            // Refresh exact native producer geometry after each synthetic stale
            // center; a fresh query must recover the same natural placement.
            std::vector<std::pair<Rest*, qreal>> saved;
            for (Rest* rest : fullRests) if (rest->measure()->system() == system) {
                  saved.push_back({rest, rest->ipos().x()});
                  rest->rxpos() += 23 * score->spatium();
                  rest->segment()->createShapes();
                  }
            SpacingPlan4 rebuilt;
            const bool built = build(rebuilt);
            for (const auto& r : saved) { r.first->rxpos() = r.second; r.first->segment()->createShapes(); }
            QVERIFY2(built, qPrintable(reason));
            QVERIFY(qAbs(rebuilt.width - natural.width) < 1e-6);
            QCOMPARE(int(rebuilt.segments.size()), int(natural.segments.size()));
            for (size_t i = 0; i < natural.segments.size(); ++i) {
                  QVERIFY(qAbs(rebuilt.segments[i].x - natural.segments[i].x) < 1e-6);
                  QVERIFY(qAbs(rebuilt.segments[i].width - natural.segments[i].width) < 1e-6);
                  }
            for (const auto& r : saved) {
                  Rest* rest = r.first;
                  if (!rest->measure()->hasVoices(rest->staffIdx())) continue;
                  bool moving = false;
                  for (int track = rest->staffIdx() * VOICES; track < (rest->staffIdx() + 1) * VOICES; ++track) {
                        const Element* item = rest->segment()->element(track);
                        moving = moving || (item && !(item->isRest() && toRest(item)->isFullMeasureRest()));
                        }
                  if (!moving) continue;
                  const QPointF offset = rest->offset();
                  rest->Element::setOffset(offset + QPointF(100 * score->spatium(), 0));
                  rest->segment()->createShapes();
                  SpacingPlan4 ignored;
                  const bool ok = build(ignored);
                  rest->Element::setOffset(offset);
                  rest->segment()->createShapes();
                  QVERIFY2(ok, qPrintable(reason));
                  QVERIFY(spacingPlansEqual154(ignored, natural));
                  ++mixedChecks;
                  }
            // Both source identity and the current rest context must still be
            // valid. Rejection cannot replace a previously complete plan.
            Rest* probe = saved.front().first;
            SpacingPlan4 retained = natural;
            const QRectF probeBox = probe->bbox();
            Segment* boundary = probe->measure()->last();
            QVERIFY(boundary && boundary->isEndBarLineType());
            for (int mode = 0; mode < 5; ++mode) {
                  if (mode == 0) probe->setGap(true);
                  if (mode == 1) probe->setbbox(QRectF(probeBox.topLeft(), QSizeF(0, probeBox.height())));
                  if (mode == 2) probe->setVisible(false);
                  if (mode == 3) probe->setStaffMove(1);
                  if (mode == 4) boundary->setSegmentType(SegmentType::KeySig);
                  const bool accepted = build(retained);
                  const bool published = publishSpacingPlanSegments4(system, selected, &reason);
                  probe->setGap(false); probe->setbbox(probeBox); probe->setVisible(true); probe->setStaffMove(0);
                  boundary->setSegmentType(SegmentType::EndBarLine);
                  QVERIFY2(!accepted && !published,
                           qPrintable(QString("Rest rejection mode %1: planner accepted=%2, publication accepted=%3; %4")
                                 .arg(mode).arg(accepted).arg(published).arg(reason)));
                  QVERIFY(spacingPlansEqual154(retained, natural));
                  QVERIFY(same(snapshot(), modern));
                  }
            }
      if (!file.contains("squeeze")) QVERIFY(mixedChecks >= 2);
      if (file.contains("natural") && modernBeams) {
            Rest* rest = fullRests.front();
            const QPointF offset(.25 * score->spatium(), .5 * score->spatium());
            score->startCmd();
            rest->undoChangeProperty(Pid::OFFSET, offset);
            score->endCmd();
            score->doLayout();
            QCOMPARE(rest->offset(), offset);
            const auto edited = snapshot();
            score->undoRedo(true, nullptr);
            score->doLayout();
            QVERIFY(same(snapshot(), modern));
            score->undoRedo(false, nullptr);
            score->doLayout();
            QCOMPARE(rest->offset(), offset);
            QVERIFY(same(snapshot(), edited));
            score->undoRedo(true, nullptr);
            score->doLayout();
            }
      QVERIFY(same(snapshot(), modern));
      score->doLayout();
      QVERIFY(same(snapshot(), modern));
      score->setStyleValue(Sid::spacingLayout4, false);
      score->doLayout();
      QVERIFY(same(snapshot(), legacy));
      score->setStyleValue(Sid::spacingLayout4, true);
      score->doLayout();
      QVERIFY(same(snapshot(), modern));
      QVERIFY(!score->graceQueryFactory4());
      }

void TestSpacing::systemSpacingLifecycle4_data()
      {
      QTest::addColumn<QString>("file");
      QTest::newRow("measure-rest-natural") << QString("v0.201-measure-rests-natural.mscx");
      QTest::newRow("measure-rest-squeeze") << QString("v0.201-measure-rests-squeeze.mscx");
      QTest::newRow("ordinary") << QString("studio-spacing-natural-h1.mscx");
      QTest::newRow("after-graces") << QString("v0.97-spacing-after-grace-system-natural.mscx");
      QTest::newRow("merged-graces") << QString("v0.139-spacing-two-staff-slur-offsets.mscx");
      QTest::newRow("root-system-boundary") << QString("v0.167-grace-policy-systems.mscx");
      QTest::newRow("cross-staff-fitting") << QString("v0.181-spacing-cross-staff-fitting.mscx");
      QTest::newRow("grace-squeeze") << QString("v0.187-acciaccatura-squeeze.mscx");
      }

void TestSpacing::systemSpacingLifecycle4()
      {
      QFETCH(QString, file);
      std::unique_ptr<MasterScore> score(readScore(DIR + file));
      QVERIFY(score);
      score->setStyleValue(Sid::beamLayout4, true);
      score->setStyleValue(Sid::spacingLayout4, true);
      QScopedValueRollback<bool> stretch(MScore::noHorizontalStretch, false);
      score->doLayout();
      const auto snapshot = [&]() {
            auto values = graceMergedSnapshot4(score.get());
            for (const System* system : score->systems()) values.push_back(system->width());
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
                  for (const Segment& s : m->segments())
                        values.insert(values.end(), {s.x(), s.width(), s.stretch(), s.spacingWidthOffset4()});
            return values;
            };
      const auto native = snapshot();
      QVERIFY(!spacingSystemJustified4(nullptr, nullptr, 34, 100, .35));
      QVERIFY(spacingSystemJustified4(nullptr, nullptr, 35, 100, .35));
      QVERIFY(spacingSystemJustified4(nullptr, score->firstMeasure(), 1, 100, 1));
      int checked = 0, graceGroups = 0;
      const qreal nan = std::numeric_limits<qreal>::quiet_NaN();
      for (System* system : score->systems()) {
            if (system->vbox()) continue;
            SpacingPlan4 selected;
            QString reason;
            {
            SpacingAttachmentPreparation4 attachments;
            QVERIFY2(snapshotLaidOutSystemSpacing4(system, attachments, &reason), qPrintable(reason));
            SpacingPlanOptions4 options;
            options.slurPolicy = SpacingSlurPolicy4::PREPARED;
            SpacingPlan4 natural, candidate;
            QVERIFY2(buildSpacingPlan4(system, natural, attachments, &reason, options), qPrintable(reason));
            SpacingSystemRequest4 request;
            request.availableWidth = natural.width / 2;
            QVERIFY2(selectSystemSpacingPlan4(system, request, attachments, candidate, &reason), qPrintable(reason));
            QVERIFY(spacingPlansEqual154(natural, candidate)); // An overfull prefix is still a valid candidate.
            QVERIFY(candidate.width > request.availableWidth);
            request.pass = SpacingPass4::FINAL;
            request.availableWidth = system->width();
            request.fillLimit = score->styleD(Sid::lastSystemFillLimit);
            request.next = system->lastMeasure()->next();
            SpacingPlan4 expected;
            QVERIFY2(squeezeSpacingPlan4(system, request.availableWidth, expected, attachments, &reason, nullptr,
                                         SpacingSlurPolicy4::PREPARED), qPrintable(reason));
            if (spacingSystemJustified4(system->lastMeasure(), request.next, expected.width,
                                        request.availableWidth, request.fillLimit))
                  QVERIFY(justifySpacingPlan4(expected, request.availableWidth));
            QVERIFY2(selectSystemSpacingPlan4(system, request, attachments, selected, &reason), qPrintable(reason));
            QVERIFY(spacingPlansEqual154(selected, expected));
            QVERIFY(reason.isEmpty());
            const SpacingPlan4 retained = selected;
            for (int mode = 0; mode < 5; ++mode) {
                  SpacingSystemRequest4 broken = request;
                  switch (mode) {
                  case 0: broken.pass = static_cast<SpacingPass4>(-1); break;
                  case 1: broken.availableWidth = 0; break;
                  case 2: broken.availableWidth = nan; break;
                  case 3: broken.fillLimit = nan; break;
                  case 4: broken.slurPolicy = static_cast<SpacingSlurPolicy4>(-1); break;
                  }
                  QVERIFY(!selectSystemSpacingPlan4(system, broken, attachments, selected, &reason));
                  QVERIFY(!reason.isEmpty());
                  QVERIFY(spacingPlansEqual154(selected, retained));
                  }
            request.availableWidth = .001 * score->spatium();
            QVERIFY(!selectSystemSpacingPlan4(system, request, attachments, selected, &reason));
            QVERIFY(spacingPlansEqual154(selected, retained));
            }
            // No attachment or physical query view survives into publication.
            QVERIFY2(spacingPlanPublicationValid4(system, selected, &reason), qPrintable(reason));
            QVERIFY2(spacingPlanMatches4(selected, &reason), qPrintable(reason));
            QVERIFY(snapshot() == native);
            QVERIFY(selected.segments.size() > 1);
            for (int mode = 0; mode < 16; ++mode) {
                  SpacingPlan4 broken = selected;
                  broken.segments.front().x += .25 * score->spatium();
                  switch (mode) {
                  case 0: broken.width = nan; break;
                  case 1: broken.measures.pop_back(); break;
                  case 2: broken.measures.push_back(broken.measures.front()); break;
                  case 3: broken.measures.front().measure = nullptr; break;
                  case 4: broken.measures.front().x += 1; break;
                  case 5: broken.measures.back().width += 1; break;
                  case 6: broken.segments.pop_back(); break;
                  case 7: broken.segments.push_back(broken.segments.front()); break;
                  case 8: std::swap(broken.segments.front(), broken.segments.back()); break;
                  case 9: broken.segments.back().width = nan; break;
                  case 10: broken.segments.back().stretch = -1; break;
                  case 11: broken.segments.back().widthOffset = nan; break;
                  case 12: broken.segments.back().ignored = !broken.segments.back().ignored; break;
                  case 13: broken.segments.back().segment = nullptr; break;
                  case 14: broken.segments.front().x = nan; break;
                  case 15: broken.width += 1; break;
                  }
                  QVERIFY(!publishSpacingPlanSegments4(system, broken, &reason));
                  QVERIFY(!reason.isEmpty());
                  QVERIFY(snapshot() == native); // Even a late failure must not write the earlier changed x.
                  }
            for (int kind = 0; kind < 2; ++kind) {
                  if (kind == 0 ? selected.afterGraces.empty() : selected.mergedGraces.empty()) continue;
                  ++graceGroups;
                  QVERIFY(!(kind == 0 ? selected.afterGraces.front().positions.empty()
                                       : selected.mergedGraces.front().positions.empty()));
                  for (int mode = 0; mode < 6; ++mode) {
                        SpacingPlan4 broken = selected;
                        broken.segments.front().x += .25 * score->spatium();
                        if (kind == 0) {
                              auto& group = broken.afterGraces.front();
                              switch (mode) {
                              case 0: group.owner = nullptr; break;
                              case 1: group.target = nullptr; break;
                              case 2: group.positions.front().chord = nullptr; break;
                              case 3: group.positions.front().x = nan; break;
                              case 4: group.positions.pop_back(); break;
                              case 5: broken.afterGraces.push_back(group); break;
                              }
                              }
                        else {
                              auto& group = broken.mergedGraces.front();
                              switch (mode) {
                              case 0: group.owner = nullptr; break;
                              case 1: group.target = nullptr; break;
                              case 2: group.positions.front().chord = nullptr; break;
                              case 3: group.positions.front().x = nan; break;
                              case 4: group.positions.pop_back(); break;
                              case 5: broken.mergedGraces.push_back(group); break;
                              }
                              }
                        QVERIFY(!publishSpacingPlanSegments4(system, broken, &reason));
                        QVERIFY(!reason.isEmpty());
                        QVERIFY(snapshot() == native);
                        }
                  SpacingPlan4 overflow = selected;
                  const Segment* source = kind == 0 ? overflow.afterGraces.front().owner->segment()
                                                    : overflow.mergedGraces.front().owner->segment();
                  const Segment* target = kind == 0 ? overflow.afterGraces.front().target
                                                    : overflow.mergedGraces.front().target->segment();
                  if (source != target) {
                        for (auto& entry : overflow.segments) {
                              if (entry.segment == source) entry.x = -std::numeric_limits<qreal>::max();
                              if (entry.segment == target) entry.x = std::numeric_limits<qreal>::max();
                              }
                        // Finite stored fields can still overflow the target-
                        // to-parent conversion used by late grace publication.
                        QVERIFY(!publishSpacingPlanSegments4(system, overflow, &reason));
                        QVERIFY(!reason.isEmpty());
                        QVERIFY(snapshot() == native);
                        }
                  }
            // Check the actual segment commit, then restore native cache fields
            // before assertions. Ignored headers can legitimately retain cache-
            // only positions different from a newly reconstructed ignored entry.
            std::vector<std::array<qreal, 4>> saved;
            for (const auto& s : selected.segments)
                  saved.push_back({{s.segment->x(), s.segment->width(), s.segment->stretch(), s.segment->spacingWidthOffset4()}});
            const bool committed = publishSpacingPlanSegments4(system, selected, &reason);
            bool fields = committed;
            for (size_t i = 0; i < selected.segments.size(); ++i) {
                  const auto& s = selected.segments[i];
                  qreal origin = 0;
                  for (const auto& m : selected.measures) if (m.measure == s.segment->measure()) origin = m.x;
                  fields = fields && s.segment->x() == s.x - origin && s.segment->width() == s.width
                        && s.segment->stretch() == s.stretch && s.segment->spacingWidthOffset4() == s.widthOffset;
                  s.segment->rxpos() = saved[i][0];
                  s.segment->setWidth(saved[i][1]);
                  s.segment->setStretch(saved[i][2]);
                  s.segment->setSpacingWidthOffset4(saved[i][3]);
                  }
            QVERIFY2(committed && fields, qPrintable(reason));
            QVERIFY(snapshot() == native);
            ++checked;
            }
      QVERIFY(checked > 0);
      if (file.startsWith("v0.97") || file.startsWith("v0.139")) QVERIFY(graceGroups > 0);
      QVERIFY(!score->graceQueryFactory4());
      }

void TestSpacing::acciaccaturaSlash4_data()
      {
      QTest::addColumn<bool>("modernSpacing");
      QTest::newRow("native-spacing") << false;
      QTest::newRow("studio-spacing") << true;
      }

void TestSpacing::acciaccaturaSlash4()
      {
      QFETCH(bool, modernSpacing);
      std::unique_ptr<MasterScore> score(readScore(DIR + "v0.182-spacing-grace-accidental-fitting.mscx"));
      QVERIFY(score);
      score->setStyleValue(Sid::spacingLayout4, modernSpacing);
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      std::vector<Chord*> chords;
      std::vector<QLineF> legacy;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            for (const Segment& segment : m->segments()) {
                  const Element* e = segment.element(0);
                  if (!e || !e->isChord()) continue;
                  for (Chord* grace : toChord(e)->graceNotes()) {
                        if (!grace->stemSlash()) continue;
                        chords.push_back(grace);
                        legacy.push_back(grace->stemSlash()->slashLine());
                        }
                  }
            }
      QCOMPARE(int(chords.size()), 6);
      score->setStyleValue(Sid::beamLayout4, true);
      score->doLayout();
      std::vector<QLineF> modern;
      int beamedUp = 0, beamedDown = 0, singleUp = 0, singleDown = 0;
      const auto close = [](const QPointF& a, const QPointF& b) { return (a - b).manhattanLength() < 1e-6; };
      for (Chord* c : chords) {
            const StemSlash* slash = c->stemSlash();
            QVERIFY(slash);
            const QLineF line = slash->slashLine();
            modern.push_back(line);
            const qreal sign = c->up() ? -1 : 1;
            const qreal actual = std::atan2(sign * line.dy(), line.dx());
            qreal expected = 40 * std::acos(qreal(-1)) / 180;
            const Beam* beam = c->beam();
            if (beam) {
                  const QLineF* primary = beam->primarySegment4();
                  QVERIFY(primary && primary->dx() > 0);
                  const qreal beamAngle = std::atan2(primary->dy(), primary->dx());
                  expected += sign * beamAngle / 2;
                  const bool obtuse = (c->up() && beamAngle < 0) || (!c->up() && beamAngle > 0);
                  QVERIFY(qAbs(line.length() / c->spatium() - (obtuse ? 2.2 : 2.0)) < 1e-6);
                  // Exercise both slope signs, including the longer obtuse
                  // stroke, independently of this fixture's natural contour.
                  QLineF* edited = const_cast<QLineF*>(primary);
                  const QLineF saved = *edited;
                  for (qreal slope : {qreal(-.25), qreal(.25)}) {
                        edited->setP2(QPointF(saved.x2(), saved.y1() + saved.dx() * slope));
                        c->stemSlash()->layout();
                        const QLineF changed = c->stemSlash()->slashLine();
                        *edited = saved;
                        c->stemSlash()->layout();
                        const qreal theta = 40 * std::acos(qreal(-1)) / 180 + sign * std::atan(slope) / 2;
                        const bool longer = (c->up() && slope < 0) || (!c->up() && slope > 0);
                        QVERIFY(qAbs(std::atan2(sign * changed.dy(), changed.dx()) - theta) < 1e-6);
                        QVERIFY(qAbs(changed.length() / c->spatium() - (longer ? 2.2 : 2.0)) < 1e-6);
                        QVERIFY(close(c->stemSlash()->slashLine().p1(), line.p1()));
                        QVERIFY(close(c->stemSlash()->slashLine().p2(), line.p2()));
                        }
                  if (c->up()) ++beamedUp; else ++beamedDown;
                  }
            else {
                  QVERIFY(c->hook());
                  QVERIFY(qAbs(line.x2() - c->hook()->bbox().translated(c->hook()->pos()).right()) < 1e-6);
                  if (c->up()) ++singleUp; else ++singleDown;
                  }
            QVERIFY2(qAbs(actual - expected) < 1e-6, "Acciaccatura slash does not use the source angle/beam adjustment");
            QVERIFY(qAbs(slash->lineWidth() - .125 * c->spatium() * c->mag()) < 1e-6);
            QVERIFY(slash->bbox().contains(line.p1()) && slash->bbox().contains(line.p2()));
            const QPointF offset(.2 * c->spatium(), -.1 * c->spatium());
            c->stemSlash()->setOffset(offset);
            c->stemSlash()->layout();
            QCOMPARE(c->stemSlash()->offset(), offset);
            QVERIFY(close(c->stemSlash()->slashLine().p1(), line.p1()));
            QVERIFY(close(c->stemSlash()->slashLine().p2(), line.p2()));
            c->stemSlash()->setOffset(QPointF());
            }
      QCOMPARE(beamedUp, 2); QCOMPARE(beamedDown, 2);
      QCOMPARE(singleUp, 1); QCOMPARE(singleDown, 1);
      score->doLayout();
      for (size_t i = 0; i < chords.size(); ++i) {
            QVERIFY(close(chords[i]->stemSlash()->slashLine().p1(), modern[i].p1()));
            QVERIFY(close(chords[i]->stemSlash()->slashLine().p2(), modern[i].p2()));
            }
      score->setStyleValue(Sid::beamLayout4, false);
      score->doLayout();
      for (size_t i = 0; i < chords.size(); ++i) {
            QVERIFY(close(chords[i]->stemSlash()->slashLine().p1(), legacy[i].p1()));
            QVERIFY(close(chords[i]->stemSlash()->slashLine().p2(), legacy[i].p2()));
            QCOMPARE(chords[i]->stemSlash()->lineWidth(), score->styleP(Sid::stemWidth));
            }
      }

QTEST_MAIN(TestSpacing)
#include "tst_spacing.moc"
