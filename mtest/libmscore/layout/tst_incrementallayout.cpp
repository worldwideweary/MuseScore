//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2026 MuseScore contributors
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2 as
//  published by the Free Software Foundation.
//=============================================================================

#include <QtTest/QtTest>
#include <QLineF>
#include <QMap>
#include <QVector>
#include <cmath>
#include <memory>

#include "mtest/testutils.h"
#include "libmscore/score.h"
#include "libmscore/measure.h"
#include "libmscore/segment.h"
#include "libmscore/system.h"
#include "libmscore/chord.h"
#include "libmscore/chordrest.h"
#include "libmscore/spanner.h"
#include "libmscore/fret.h"
#include "libmscore/lyrics.h"
#include "libmscore/articulation.h"
#include "libmscore/note.h"
#include "libmscore/rest.h"
#include "libmscore/beam.h"
#include "libmscore/tie.h"
#include "libmscore/shape.h"

using namespace Ms;

#define DIR QString("libmscore/layout/")

namespace {

// Stable score-relative IDs, not Element* addresses: a complete relayout may
// recreate systems, beams and other temporary objects.
using Geometry = QMap<QString, QVector<qreal>>;

void addPoint(QVector<qreal>& out, const QPointF& p)
      {
      out << p.x() << p.y();
      }

void addRect(QVector<qreal>& out, const QRectF& r)
      {
      out << r.x() << r.y() << r.width() << r.height();
      }

void addElement(Geometry& result, const QString& id, const Element* e)
      {
      if (!e)
            return;
      QVector<qreal> values;
      values << qreal(int(e->type()));
      addPoint(values, e->ipos());
      addPoint(values, e->pos());
      addPoint(values, e->pagePos());
      addRect(values, e->bbox());
      result.insert(id, values);
      }

Geometry snapshot(Score* score)
      {
      Geometry result;
      const auto& systems = score->systems();
      for (int i = 0; i < systems.size(); ++i) {
            const System* system = systems[i];
            addElement(result, QString("system/%1").arg(i), system);
            }

      int measureNo = 0;
      for (Measure* measure = score->firstMeasure(); measure;
           measure = measure->nextMeasure(), ++measureNo) {
            const QString mKey = QString("measure/%1").arg(measureNo);
            addElement(result, mKey, measure);
            result[mKey] << qreal(systems.indexOf(measure->system()))
                         << measure->width();

            int segmentNo = 0;
            for (Segment& segment : measure->segments()) {
                  const QString sKey = mKey + QString("/segment/%1").arg(segmentNo++);
                  addElement(result, sKey, &segment);
                  result[sKey] << segment.width() << segment.stretch()
                               << qreal(segment.enabled());

                  // Record the collision shapes as well as the rhythmic
                  // positions; a stale skyline may not move a visible note.
                  const auto& shapes = segment.shapes();
                  for (size_t staff = 0; staff < shapes.size(); ++staff) {
                        const QString shapeKey = sKey + QString("/shape/%1").arg(int(staff));
                        QVector<qreal> shapeValues;
                        shapeValues << qreal(shapes[staff].size());
                        for (const auto& rect : shapes[staff])
                              addRect(shapeValues, rect);
                        result.insert(shapeKey, shapeValues);
                        }

                  const auto& elements = segment.elist();
                  for (size_t track = 0; track < elements.size(); ++track) {
                        const Element* element = elements[track];
                        if (!element)
                              continue;
                        const QString eKey = sKey + QString("/track/%1").arg(int(track));
                        addElement(result, eKey, element);
                        if (!element->isChordRest())
                              continue;
                        const ChordRest* cr = static_cast<const ChordRest*>(element);
                        const auto& lyrics = cr->lyrics();
                        for (size_t verse = 0; verse < lyrics.size(); ++verse)
                              addElement(result, eKey + QString("/lyric/%1").arg(int(verse)), lyrics[verse]);
                        if (!element->isChord())
                              continue;
                        const Chord* chord = toChord(element);
                        const auto& notes = chord->notes();
                        for (size_t n = 0; n < notes.size(); ++n)
                              addElement(result, eKey + QString("/note/%1").arg(int(n)), notes[n]);
                        const auto& articulations = chord->articulations();
                        for (int a = 0; a < articulations.size(); ++a)
                              addElement(result, eKey + QString("/articulation/%1").arg(a), articulations[a]);
                        if (const Beam* beam = chord->beam()) {
                              const QString beamKey = eKey + "/beam";
                              QVector<qreal> beamValues;
                              addPoint(beamValues, beam->beamPos());
                              addRect(beamValues, beam->bbox());
                              result.insert(beamKey, beamValues);
                              }
                        }
                  const auto& annotations = segment.annotations();
                  for (size_t i = 0; i < annotations.size(); ++i) {
                        const Element* annotation = annotations[i];
                        const QString aKey = sKey + QString("/annotation/%1").arg(int(i));
                        addElement(result, aKey, annotation);
                        if (annotation && annotation->type() == ElementType::FRET_DIAGRAM)
                              addElement(result, aKey + "/harmony", toFretDiagram(annotation)->harmony());
                        }
                  }
            }

      // Text-line spanners (notably pedal lines) live in the score's spanner
      // map, not the segment annotation list. Compare their drawn segments.
      int spannerNo = 0;
      for (const auto& entry : score->spanner()) {
            const Spanner* spanner = entry.second;
            const QString key = QString("spanner/%1").arg(spannerNo++);
            addElement(result, key, spanner);
            result[key] << qreal(spanner->tick().ticks())
                        << qreal(spanner->tick2().ticks())
                        << qreal(spanner->track());
            // The const spannerSegments() overload is protected in 37E;
            // use the public read-only accessor instead.
            for (size_t i = 0; i < spanner->nsegments(); ++i) {
                  const SpannerSegment* ss = spanner->segmentAt(int(i));
                  const QString skey = key + QString("/drawn/%1").arg(int(i));
                  addElement(result, skey, ss);
                  addPoint(result[skey], ss->ipos2());
                  addPoint(result[skey], ss->pos2());
                  }
            }
      return result;
      }

// addElement() records: type, ipos, pos, pagePos, bbox; extra fields
// appended by snapshot() follow these 11 slots.
QString componentName(const QString& key, int component)
      {
      if (key.startsWith("measure/") && key.count('/') == 1) {
            if (component == 11)
                  return "systemIndex";
            if (component == 12)
                  return "measure.width";
            }
      if (key.contains("/shape/") || key.endsWith("/beam"))
            return QString("component %1").arg(component);
      static const char* const fields[] = {
            "type", "ipos.x", "ipos.y", "pos.x", "pos.y",
            "pagePos.x", "pagePos.y", "bbox.x", "bbox.y",
            "bbox.width", "bbox.height"
            };
      if (component >= 0 && component < 11)
            return QString::fromLatin1(fields[component]);
      return QString("component %1").arg(component);
      }

QString firstDifference(const Geometry& before, const Geometry& after, qreal tolerance)
      {
      for (auto it = before.cbegin(); it != before.cend(); ++it) {
            auto other = after.constFind(it.key());
            if (other == after.cend())
                  return QString("Missing geometry after full relayout: %1").arg(it.key());
            if (it.value().size() != other.value().size())
                  return QString("Geometry length differs for %1 (%2 vs %3)")
                        .arg(it.key()).arg(it.value().size()).arg(other.value().size());
            for (int i = 0; i < it.value().size(); ++i) {
                  const qreal a = it.value()[i];
                  const qreal b = other.value()[i];
                  if (!qIsFinite(a) || !qIsFinite(b) || qAbs(a - b) > tolerance)
                        return QString("%1, %2: before=%3, after=%4, delta=%5")
                              .arg(it.key()).arg(componentName(it.key(), i))
                              .arg(a, 0, 'g', 12).arg(b, 0, 'g', 12)
                              .arg(qAbs(a - b), 0, 'g', 12);
                  }
            }
      for (auto it = after.cbegin(); it != after.cend(); ++it)
            if (!before.contains(it.key()))
                  return QString("New geometry after full relayout: %1").arg(it.key());
      return QString();
      }

QString checkAgainstFullLayout(Score* score)
      {
      // The first snapshot must be taken before doLayout(), otherwise a full
      // relayout would silently conceal broken incremental geometry.
      const Geometry incremental = snapshot(score);
      score->doLayout();
      return firstDifference(incremental, snapshot(score), score->spatium() * 1e-4);
      }

Rest* measureRest(Measure* measure, int staffIdx)
      {
      for (Segment& segment : measure->segments()) {
            if (!segment.isChordRestType())
                  continue;
            Element* e = segment.element(staffIdx * VOICES);
            if (e && e->isRest())
                  return toRest(e);
            }
      return nullptr;
      }

// Guard against XML fixtures that parse but silently omit unsupported items.
QString richContentMissing(Measure* measure)
      {
      int chordSymbols = 0, fretDiagrams = 0, staffText = 0, dynamics = 0, lyrics = 0;
      for (Segment& segment : measure->segments()) {
            for (const Element* e : segment.annotations()) {
                  if (!e)
                        continue;
                  switch (e->type()) {
                        case ElementType::HARMONY: ++chordSymbols; break;
                        case ElementType::FRET_DIAGRAM: ++fretDiagrams; break;
                        case ElementType::STAFF_TEXT: ++staffText; break;
                        case ElementType::DYNAMIC: ++dynamics; break;
                        default: break;
                        }
                  }
            for (Element* e : segment.elist())
                  if (e && e->isChordRest())
                        lyrics += int(static_cast<const ChordRest*>(e)->lyrics().size());
            }
      if (chordSymbols < 1 || fretDiagrams < 1 || staffText < 2 || dynamics < 1 || lyrics < 1)
            return QString("Missing rich fixture content (harmony=%1, fret=%2, staffText=%3, "
                           "dynamics=%4, lyrics=%5)")
                  .arg(chordSymbols).arg(fretDiagrams).arg(staffText).arg(dynamics).arg(lyrics);
      return QString();
      }

} // namespace

class TestIncrementalLayout : public QObject, public MTest {
      Q_OBJECT
   private slots:
      void initTestCase() { initMTest(); }
      void untouchedBoundary();
      void fullLayoutIdempotence();
      void annotatedFollowingSystems_data();
      void annotatedFollowingSystems();
      void tieDeletionUndoRedo();
      void lastSystemFillUsesNaturalWidth();
      void pairedWholeRestsUseStaffLines();
      void mixedRestRunsUseCommonHeight();
      void movedChordsDoNotDisplaceOrdinaryRests();
      void halfRestRunsFitSameStaffAndMovedChords();
      void fullMeasureRestUsesLocalClearance();
      void fullMeasureRestClearsAtItsOffset();
      void fullMeasureRestKeepsClearanceOnLookahead();
      void crossStaffAutoStemsUseVoiceDirection();
      void afterGraceOctaveKeepsPreviousGracePosition();
      void afterGraceFitsBeforeEighthRest();
      void followingBeamSlopesSurviveEdit();
      };

void TestIncrementalLayout::untouchedBoundary()
      {
      MasterScore* score = readScore(DIR + "native-lookahead-tie-rests.mscx");
      QVERIFY(score);
      Measure* m2 = score->firstMeasure()->nextMeasure();
      Measure* m3 = m2 ? m2->nextMeasure() : nullptr;
      QVERIFY(m2 && m3 && m2->system() && m3->system());
      QVERIFY(m2->system() != m3->system());
      Rest* rest = measureRest(m3, 1);
      QVERIFY(rest);
      QVERIFY2(qAbs(rest->rxpos()) > score->spatium() * 0.1,
               "Fixture does not contain a horizontally centered following-system rest");

      // Rerender just the earlier system. getNextMeasure() must not damage
      // geometry in the following system when the lookahead is preserved.
      score->doLayoutRange(m2->tick(), m2->tick());
      const QString difference = checkAgainstFullLayout(score);
      QVERIFY2(difference.isEmpty(), qPrintable(difference));
      delete score;
      }

// The rich fixture may itself have non-idempotent full layout (for example,
// from changing font or spanner extents). Establish that baseline independently
// before attributing any discrepancy to speculative lookahead.
void TestIncrementalLayout::fullLayoutIdempotence()
      {
      MasterScore* score = readScore(DIR + "native-lookahead-tie-rests.mscx");
      QVERIFY(score);
      const Geometry first = snapshot(score);  // readScore() already did a full layout
      const qreal tolerance = score->spatium() * 1e-4;
      score->doLayout();
      const Geometry second = snapshot(score);
      const QString firstDifferenceFound = firstDifference(first, second, tolerance);
      QVERIFY2(firstDifferenceFound.isEmpty(),
               qPrintable(QString("Full layout #1 vs #2: ") + firstDifferenceFound));
      score->doLayout();
      const QString secondDifferenceFound = firstDifference(second, snapshot(score), tolerance);
      QVERIFY2(secondDifferenceFound.isEmpty(),
               qPrintable(QString("Full layout #2 vs #3: ") + secondDifferenceFound));
      delete score;
      }

// Run each boundary against a freshly loaded and fully laid-out score.
// Testing both boundaries on a single mutable score hid which transition
// produced the mismatch and could introduce prior full-layout effects.
void TestIncrementalLayout::annotatedFollowingSystems_data()
      {
      QTest::addColumn<int>("priorMeasureIndex");
      QTest::newRow("system-1-to-2") << 1;
      QTest::newRow("system-2-to-3") << 3;
      }

void TestIncrementalLayout::annotatedFollowingSystems()
      {
      MasterScore* score = readScore(DIR + "native-lookahead-tie-rests.mscx");
      QVERIFY(score);
      Measure* m1 = score->firstMeasure();
      Measure* m2 = m1 ? m1->nextMeasure() : nullptr;
      Measure* m3 = m2 ? m2->nextMeasure() : nullptr;
      Measure* m4 = m3 ? m3->nextMeasure() : nullptr;
      Measure* m5 = m4 ? m4->nextMeasure() : nullptr;
      QVERIFY(m2 && m3 && m4 && m5);
      QVERIFY(m2->system() && m3->system() && m4->system() && m5->system());
      QVERIFY(m2->system() != m3->system());
      QVERIFY(m4->system() != m5->system());
      QVERIFY2(richContentMissing(m3).isEmpty(), qPrintable(richContentMissing(m3)));
      QVERIFY2(richContentMissing(m5).isEmpty(), qPrintable(richContentMissing(m5)));
      QVERIFY(measureRest(m3, 1) && measureRest(m5, 1));

      // Ensure both pedal spanners were actually loaded and laid out.
      int pedals = 0, drawnSegments = 0;
      for (const auto& entry : score->spanner()) {
            const Spanner* spanner = entry.second;
            if (spanner->type() == ElementType::PEDAL) {
                  ++pedals;
                  drawnSegments += int(spanner->nsegments());
                  }
            }
      QCOMPARE(pedals, 2);
      QVERIFY(drawnSegments >= 2);

      QFETCH(int, priorMeasureIndex);
      Measure* beforeBreak = priorMeasureIndex == 1 ? m2 : m4;
      const QString boundary = QString("before measure %1 -> following system").arg(priorMeasureIndex + 1);
      const Geometry baseline = snapshot(score);
      const qreal tolerance = score->spatium() * 1e-4;
      score->doLayoutRange(beforeBreak->tick(), beforeBreak->tick());
      const Geometry incremental = snapshot(score);
      score->doLayout();
      const Geometry complete = snapshot(score);
      const QString difference = firstDifference(incremental, complete, tolerance);
      if (!difference.isEmpty()) {
            const QString baselineToIncremental = firstDifference(baseline, incremental, tolerance);
            const QString baselineToFull = firstDifference(baseline, complete, tolerance);
            const QString explanation = QString("%1: incremental vs full: %2; "
                  "baseline vs incremental: %3; baseline vs full: %4")
                  .arg(boundary).arg(difference)
                  .arg(baselineToIncremental.isEmpty() ? QString("same") : baselineToIncremental)
                  .arg(baselineToFull.isEmpty() ? QString("same") : baselineToFull);
            QVERIFY2(difference.isEmpty(), qPrintable(explanation));
            }
      delete score;
      }

void TestIncrementalLayout::tieDeletionUndoRedo()
      {
      MasterScore* score = readScore(DIR + "native-lookahead-tie-rests.mscx");
      QVERIFY(score);
      Measure* m2 = score->firstMeasure()->nextMeasure();
      QVERIFY(m2);
      Segment* first = m2->first(SegmentType::ChordRest);
      QVERIFY(first && first->element(0) && first->element(0)->isChord());
      Chord* chord = toChord(first->element(0));
      QVERIFY(!chord->notes().empty());
      Tie* tie = chord->notes().front()->tieFor();
      QVERIFY2(tie, "Fixture's first note in measure 2 must start a tie");

      score->startCmd();
      score->undoRemoveElement(tie);
      score->endCmd();
      QString difference = checkAgainstFullLayout(score);
      QVERIFY2(difference.isEmpty(), qPrintable(QString("After tie deletion: ") + difference));

      score->undoRedo(true, nullptr);
      difference = checkAgainstFullLayout(score);
      QVERIFY2(difference.isEmpty(), qPrintable(QString("After Undo: ") + difference));

      score->undoRedo(false, nullptr);
      difference = checkAgainstFullLayout(score);
      QVERIFY2(difference.isEmpty(), qPrintable(QString("After Redo: ") + difference));
      delete score;
      }

void TestIncrementalLayout::lastSystemFillUsesNaturalWidth()
      {
      if (qgetenv("MSCORE_NATIVE_NATURAL_SPACING") == "0")
            QSKIP("Native natural spacing explicitly disabled");

      // H18 fills 21.04% at natural width versus 24.54% by the old estimate.
      // The preceding forced line break leaves one measure on the last system.
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-last-system-fill.mscx"));
      QVERIFY(score);
      QCOMPARE(score->styleD(Sid::lastSystemFillLimit), qreal(0.21));
      const Measure* last = score->lastMeasure();
      QVERIFY(last && last->prevMeasure() && last->system() && last->prevMeasure()->system());
      const auto fullWidth = [&]() { return last->prevMeasure()->system()->width(); };
      const auto finalWidth = [&]() { return last->system()->width(); };
      const qreal tolerance = score->spatium() * 0.001;

      QVERIFY(last->system() != last->prevMeasure()->system());
      const qreal justifiedWidth = fullWidth();
      QVERIFY(justifiedWidth > 0.0);
      QVERIFY(qAbs(finalWidth() - justifiedWidth) < tolerance);

      score->setStyleValue(Sid::lastSystemFillLimit, 0.22);
      score->doLayout();
      QVERIFY(last->system() != last->prevMeasure()->system());
      QVERIFY(qAbs(fullWidth() - justifiedWidth) < tolerance);
      const qreal fill = finalWidth() / justifiedWidth;
      QVERIFY2(fill >= 0.21 && fill < 0.22,
               qPrintable(QString("Expected a short system at 22%, got a fill ratio of %1").arg(fill)));

      score->setStyleValue(Sid::lastSystemFillLimit, 0.21);
      score->doLayout();
      QVERIFY(last->system() != last->prevMeasure()->system());
      QVERIFY(qAbs(finalWidth() - justifiedWidth) < tolerance);
      }

void TestIncrementalLayout::afterGraceOctaveKeepsPreviousGracePosition()
      {
      if (qgetenv("MSCORE_NATIVE_NATURAL_SPACING") == "0")
            QSKIP("Native natural spacing explicitly disabled");
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-after-grace-spacing.mscx"));
      QVERIFY(score);
      Measure* measure = score->firstMeasure();
      QVERIFY(measure);
      Segment* first = measure->first(SegmentType::ChordRest);
      QVERIFY(first && first->element(0) && first->element(0)->isChord());
      Segment* second = first->next(SegmentType::ChordRest);
      QVERIFY(second && second->element(0) && second->element(0)->isChord());
      const auto left = toChord(first->element(0))->graceNotesAfter();
      const auto right = toChord(second->element(0))->graceNotesAfter();
      QCOMPARE(left.size(), 1);
      QCOMPARE(right.size(), 1);
      Note* note = right.front()->upNote();
      QCOMPARE(note->pitch(), 72);
      QVERIFY(!right.front()->ledgerLines());
      const qreal graceX = left.front()->x();
      const qreal chordDistance = second->x() - first->x();
      const qreal tolerance = score->spatium() * 1e-4;

      score->startCmd();
      score->undoChangePitch(note, note->pitch() + 12, note->tpc1(), note->tpc2());
      score->endCmd();
      QCOMPARE(note->pitch(), 84);
      QVERIFY(right.front()->ledgerLines());
      for (int pass = 0; pass < 2; ++pass) {
            QVERIFY(qAbs(second->x() - first->x() - chordDistance) < tolerance);
            QVERIFY2(qAbs(left.front()->x() - graceX) < tolerance,
                     qPrintable(QString("Preceding after-grace moved by %1sp")
                                .arg((left.front()->x() - graceX) / score->spatium())));
            if (pass == 0)
                  score->doLayout();
            }
      }

void TestIncrementalLayout::afterGraceFitsBeforeEighthRest()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-after-grace-rest-clearance.mscx"));
      QVERIFY(score && score->firstMeasure());
      Measure* m = score->firstMeasure()->nextMeasure();
      QVERIFY(m);
      Segment* on = m->findSegmentR(SegmentType::ChordRest, Fraction(1, 4));
      Segment* next = m->findSegmentR(SegmentType::ChordRest, Fraction(3, 8));
      QVERIFY(on && on->element(0) && on->element(0)->isChord());
      QVERIFY(next && next->element(0) && next->element(0)->isRest());
      const Chord* owner = toChord(on->element(0));
      const auto after = owner->graceNotesAfter();
      QCOMPARE(after.size(), 1);
      const Chord* grace = after.front();
      QCOMPARE(owner->upNote()->pitch(), 81);
      QCOMPARE(grace->upNote()->pitch(), 53);

      const qreal sp = score->spatium();
      const qreal headRight = owner->x() + owner->upNote()->x() + owner->upNote()->bbox().right();
      const qreal graceHeadLeft = owner->x() + grace->x() + grace->upNote()->x() + grace->upNote()->bbox().left();
      const qreal graceRight = on->x() + owner->x() + grace->x() + grace->shape().right();
      const qreal restLeft = next->x() + next->element(0)->x() + next->element(0)->bbox().left();
      QVERIFY2(graceHeadLeft - headRight >= 0.4 * sp, "The rest squeezed the after-grace into its main note");
      QVERIFY2(restLeft - graceRight >= 0.4 * sp, "The after-grace flag is too close to the rest");
      }

void TestIncrementalLayout::followingBeamSlopesSurviveEdit()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-following-system-beams.mscx"));
      QVERIFY(score);
      QVERIFY(score->styleB(Sid::beamLayout4));
      Measure* first = score->firstMeasure();
      QVERIFY(first && first->nextMeasure());
      Measure* following = first->nextMeasure()->nextMeasure();
      QVERIFY(following && following->system() && following->system() != first->system());
      Segment* start = first->first(SegmentType::ChordRest);
      QVERIFY(start && start->element(0) && start->element(0)->isChord());
      Note* edited = toChord(start->element(0))->upNote();

      const auto beamLines = [following](bool relayout) {
            QVector<QLineF> lines;
            for (Segment* s = following->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
                  const auto* e = s->element(0);
                  Beam* beam = e && e->isChord() ? toChord(e)->beam() : nullptr;
                  if (!beam || beam->elements().empty() || beam->elements().front() != e)
                        continue;
                  if (relayout)
                        beam->layout();
                  if (const QLineF* line = beam->primarySegment4())
                        lines.append(*line);
                  }
            return lines;
            };
      const auto baseline = beamLines(false);
      QCOMPARE(baseline.size(), 2);
      const qreal sp = score->spatium();
      const qreal tolerance = sp * 1e-4;
      for (const QLineF& line : baseline)
            QVERIFY(line.dx() > 4 * sp && line.dy() < -0.2 * sp);
      const auto compare = [&](const QVector<QLineF>& lines) {
            if (lines.size() != baseline.size())
                  return QStringLiteral("Number of beam strokes changed");
            for (int i = 0; i < lines.size(); ++i) {
                  if (qAbs(lines[i].y1() - baseline[i].y1()) >= tolerance
                      || qAbs(lines[i].y2() - baseline[i].y2()) >= tolerance)
                        return QString("Beam %1: rise %2sp became %3sp")
                              .arg(i + 1).arg(baseline[i].dy() / sp).arg(lines[i].dy() / sp);
                  }
            return QString();
            };

      score->startCmd();
      score->undoChangePitch(edited, edited->pitch() + 12, edited->tpc1(), edited->tpc2());
      score->endCmd();
      const QString afterEdit = compare(beamLines(false));
      QVERIFY2(afterEdit.isEmpty(), qPrintable(afterEdit));
      score->doLayout();
      const QString afterFull = compare(beamLines(false));
      QVERIFY2(afterFull.isEmpty(), qPrintable(afterFull));

      // The same beam must also solve identically when only its page origin changes.
      System* system = following->system();
      const QPointF position = system->ipos();
      system->setPos(position + QPointF(12 * sp, 20 * sp));
      const QString afterTranslation = compare(beamLines(true));
      system->setPos(position);
      beamLines(true);
      QVERIFY2(afterTranslation.isEmpty(), qPrintable(afterTranslation));
      }

void TestIncrementalLayout::pairedWholeRestsUseStaffLines()
      {
      if (qEnvironmentVariableIntValue("MSCORE_NATIVE_REST_LAYOUT") != 1)
            QSKIP("The rest layout switch must be enabled.");
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-paired-whole-rests.mscx"));
      QVERIFY(score);
      Measure* measure = score->firstMeasure();
      for (bool fullMeasure : { false, true }) {
            QVERIFY(measure);
            const Segment* segment = measure->first(SegmentType::ChordRest);
            QVERIFY(segment);
            for (int voice : { 0, 1 }) {
                  const Element* element = segment->element(voice);
                  QVERIFY(element && element->isRest());
                  const Rest* rest = toRest(element);
                  QCOMPARE(rest->isFullMeasureRest(), fullMeasure);
                  QVERIFY(rest->sym() == SymId::restWhole);
                  const qreal sp = rest->spatium();
                  QVERIFY(qAbs(rest->ipos().y() - (voice == 0 ? 0.0 : 3.0) * sp) < 1e-5 * sp);
                  if (fullMeasure)
                        QVERIFY(rest->ipos().x() > sp);
                  else
                        QVERIFY(qFuzzyIsNull(rest->ipos().x()));
                  }
            measure = measure->nextMeasure();
            }
      }

void TestIncrementalLayout::mixedRestRunsUseCommonHeight()
      {
      if (qEnvironmentVariableIntValue("MSCORE_NATIVE_REST_LAYOUT") != 1)
            QSKIP("The rest layout switch must be enabled.");
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-quarter-rest-alignment.mscx"));
      QVERIFY(score);
      Measure* measure = score->firstMeasure();
      for (int voice : { 0, 1 }) {
            QVERIFY(measure);
            const Segment* first = measure->first(SegmentType::ChordRest);
            const Segment* beatThree = first ? first->next(SegmentType::ChordRest) : nullptr;
            const Segment* beatFour = beatThree ? beatThree->next(SegmentType::ChordRest) : nullptr;
            QVERIFY(beatThree && beatFour);
            const Element* half = first->element(voice);
            const Element* oppositeHalf = first->element(1 - voice);
            const Element* a = beatThree->element(voice);
            const Element* b = beatFour->element(voice);
            const Element* other = beatThree->element(1 - voice);
            QVERIFY(a && b && a->isRest() && b->isRest() && other && other->isChord());
            QVERIFY(half && oppositeHalf && half->isRest() && oppositeHalf->isRest());
            const qreal sp = half->spatium();
            QCOMPARE(half->ipos().y(), a->ipos().y());
            QCOMPARE(a->ipos().y(), b->ipos().y());
            QVERIFY(voice == 0 ? half->ipos().y() < 0.0 : half->ipos().y() > 4.0 * sp);
            QVERIFY(toRest(half)->sym() == SymId::restHalfLegerLine);
            QVERIFY(toRest(oppositeHalf)->sym() == SymId::restHalf);
            QVERIFY(qAbs(oppositeHalf->ipos().y() - (voice == 0 ? 3.0 : 1.0) * sp) < 1e-5 * sp);
            const Note* note = toChord(other)->notes().front();
            const QRectF noteInk = note->bbox().translated(note->pagePos());
            const QRectF restInk = a->bbox().translated(a->pagePos());
            const qreal gap = voice == 0 ? noteInk.top() - restInk.bottom() : restInk.top() - noteInk.bottom();
            QVERIFY(gap >= 0.35 * a->spatium() - 1e-5 * a->spatium());
            measure = measure->nextMeasure();
            }

      // A chord in the rest's own voice ends the group.
      QVERIFY(measure && measure->first(SegmentType::ChordRest));
      const Segment* first = measure->first(SegmentType::ChordRest);
      const Segment* last = measure->last();
      if (last && !last->isChordRestType())
            last = last->prev(SegmentType::ChordRest);
      QVERIFY(first->element(0) && first->element(0)->isRest());
      QVERIFY(last && last->element(0) && last->element(0)->isRest() && last != first);
      QVERIFY(last->element(0)->ipos().y() > first->element(0)->ipos().y() + 0.5 * score->spatium());
      const Geometry initial = snapshot(score.get());
      score->doLayout();
      const QString diff = firstDifference(initial, snapshot(score.get()), 1e-4 * score->spatium());
      QVERIFY2(diff.isEmpty(), qPrintable(diff));
      }

void TestIncrementalLayout::movedChordsDoNotDisplaceOrdinaryRests()
      {
      if (qEnvironmentVariableIntValue("MSCORE_NATIVE_REST_LAYOUT") != 1)
            QSKIP("The rest layout switch must be enabled.");
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-rest-placement-with-moved-chords.mscx"));
      QVERIFY(score);
      Measure* measure = score->firstMeasure();
      for (bool above : { true, false }) {
            QVERIFY(measure);
            const Segment* s = measure->first(SegmentType::ChordRest);
            QVERIFY(s);
            const Element* r = s->element(above ? 0 : VOICES + 1);
            const Element* c = s->element(above ? VOICES : 0);
            QVERIFY(r && r->isRest() && c && c->isChord());
            const Rest* rest = toRest(r);
            const Chord* chord = toChord(c);
            QCOMPARE(chord->staffMove(), above ? -1 : 1);
            QCOMPARE(chord->vStaffIdx(), rest->staffIdx());
            const qreal sp = rest->spatium();
            QVERIFY(qAbs(rest->ipos().y() - (above ? 2.0 : 3.0) * sp) < 1e-5 * sp);
            if (!above) {
                  // The moved note's line is on the bass staff; it must
                  // not push down the voice-2 rest on the source staff.
                  QVERIFY(chord->notes().front()->line() > 5);
                  const Element* sourceRest = s->element(1);
                  QVERIFY(sourceRest && sourceRest->isRest());
                  QVERIFY(qAbs(sourceRest->ipos().y() - 4.0 * sp) < 1e-5 * sp);
                  }

            if (above) {
                  const Segment* second = s->next(SegmentType::ChordRest);
                  QVERIFY(second && second->element(0) && second->element(0)->isRest());
                  const Element* manual = second->element(0);
                  QVERIFY(!manual->offset().isNull());
                  QVERIFY(qAbs(manual->ipos().y() - 2.0 * sp) < 1e-5 * sp);
                  }
            measure = measure->nextMeasure();
            }

      const Geometry initial = snapshot(score.get());
      score->doLayout();
      const QString diff = firstDifference(initial, snapshot(score.get()), 1e-4 * score->spatium());
      QVERIFY2(diff.isEmpty(), qPrintable(diff));
      }

void TestIncrementalLayout::halfRestRunsFitSameStaffAndMovedChords()
      {
      if (qEnvironmentVariableIntValue("MSCORE_NATIVE_REST_LAYOUT") != 1)
            QSKIP("The rest layout switch must be enabled.");
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-half-rest-runs.mscx"));
      QVERIFY(score);
      const Measure* measure = score->firstMeasure();
      for (int bar = 1; bar <= 6; ++bar, measure = measure->nextMeasure()) {
            QVERIFY(measure);
            if (bar == 1 || bar == 3)
                  continue;
            const Segment* first = measure->findSegmentR(SegmentType::ChordRest, Fraction(0, 1));
            QVERIFY(first);
            const int voice = bar < 5 ? 1 : 0;
            const Element* item = first->element(voice);
            QVERIFY(item && item->isRest());
            const Rest* half = toRest(item);
            const qreal sp = half->spatium();
            const qreal y = half->ipos().y();
            if (bar == 2 || bar == 4) {
                  // Moved chords leave the same half and quarter rest rows
                  // in both bars, regardless of their pitch.
                  QVERIFY(qAbs(y - 4.0 * sp) < 1e-5 * sp);
                  QVERIFY(half->sym() == SymId::restHalf);
                  // Beat 2 has a bass note while the upper-staff half rests are still in progress.
                  for (int beat : { 0, 2, 3 }) {
                        const Segment* segment = measure->findSegmentR(SegmentType::ChordRest, Fraction(beat, 4));
                        QVERIFY(segment);
                        for (int restVoice : { 0, 1 }) {
                              const Element* rest = segment->element(restVoice);
                              QVERIFY(rest && rest->isRest());
                              QVERIFY(qAbs(rest->ipos().y() - (restVoice == 0 ? 0.0 : 4.0) * sp) < 1e-5 * sp);
                              }
                        }
                  }
            else if (bar == 5)
                  QVERIFY(qAbs(y - sp) < 1e-5 * sp);
            else {
                  QVERIFY(y < 0.0 && half->sym() == SymId::restHalfLegerLine);
                  for (int beat : { 2, 3 }) {
                        const Segment* segment = measure->findSegmentR(SegmentType::ChordRest, Fraction(beat, 4));
                        QVERIFY(segment);
                        const Element* quarter = segment->element(0);
                        QVERIFY(quarter && quarter->isRest());
                        QCOMPARE(quarter->ipos().y(), y);
                        }
                  }
            }

      const Geometry initial = snapshot(score.get());
      score->doLayout();
      const QString diff = firstDifference(initial, snapshot(score.get()), 1e-4 * score->spatium());
      QVERIFY2(diff.isEmpty(), qPrintable(diff));
      }

void TestIncrementalLayout::fullMeasureRestUsesLocalClearance()
      {
      if (qEnvironmentVariableIntValue("MSCORE_NATIVE_REST_LAYOUT") != 1)
            QSKIP("Run with MSCORE_NATIVE_REST_LAYOUT=1 to exercise the preview.");
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-full-measure-rest-clearance.mscx"));
      QVERIFY(score);
      const qreal sp = score->spatium();
      Measure* measure = score->firstMeasure();
      qreal edgeRestY = 0.0;
      for (int bar = 0; bar < 4; ++bar) {
            QVERIFY(measure);
            const bool above = bar < 2;
            const int voice = above ? 0 : 1;
            Segment* seg = measure->first(SegmentType::ChordRest);
            QVERIFY(seg && seg->element(voice) && seg->element(voice)->isRest());
            const Rest* rest = toRest(seg->element(voice));
            QVERIFY(rest->isFullMeasureRest());
            if (bar % 2 == 0)
                  edgeRestY = rest->ipos().y();
            else {
                  QVERIFY2(above ? rest->ipos().y() <= edgeRestY - sp : rest->ipos().y() >= edgeRestY + sp,
                           "The centered high or low note should push the rest farther out than the note at the edge");
                  Segment* second = seg->next(SegmentType::ChordRest);
                  QVERIFY(second);
                  Segment* center = second->next(SegmentType::ChordRest);
                  QVERIFY(center);
                  const Element* other = center->element(1 - voice);
                  QVERIFY(other && other->isChord());
                  const Note* note = toChord(other)->notes().front();
                  const QRectF noteInk = note->bbox().translated(note->pagePos());
                  const QRectF restInk = rest->bbox().translated(rest->pagePos());
                  const qreal gap = above ? noteInk.top() - restInk.bottom() : restInk.top() - noteInk.bottom();
                  QVERIFY(gap >= 0.75 * sp - 1e-6 * sp);
                  }
            measure = measure->nextMeasure();
            }
      const Geometry first = snapshot(score.get());
      score->doLayout();
      const QString diff = firstDifference(first, snapshot(score.get()), 1e-4 * sp);
      QVERIFY2(diff.isEmpty(), qPrintable(diff));
      }

void TestIncrementalLayout::fullMeasureRestClearsAtItsOffset()
      {
      if (qEnvironmentVariableIntValue("MSCORE_NATIVE_REST_LAYOUT") != 1)
            QSKIP("The full-measure rest layout switch must be enabled.");
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-full-measure-rest-clearance.mscx"));
      QVERIFY(score && score->firstMeasure());
      Measure* measure = score->firstMeasure()->nextMeasure();
      QVERIFY(measure);
      Rest* rest = measureRest(measure, 0);
      QVERIFY(rest && rest->isFullMeasureRest() && rest->offset().isNull());
      const qreal sp = rest->spatium();
      const qreal initialY = rest->ipos().y();
      QVERIFY(initialY < -sp);

      const Segment* last = measure->last();
      if (last && !last->isChordRestType())
            last = last->prev(SegmentType::ChordRest);
      const Element* chord = last ? last->element(1) : nullptr;
      QVERIFY(chord && chord->isChord());
      const Note* note = toChord(chord)->upNote();
      const qreal dx = note->pagePos().x() + note->bbox().center().x()
            - rest->pagePos().x() - rest->bbox().center().x();
      QVERIFY(dx > 6.0 * sp);

      // Move to the quiet beat on the right. The high note should stop
      // pushing the rest up even though both remain in the same measure.
      const QPointF shift(dx, 0.0);
      QVERIFY(rest->setProperty(Pid::OFFSET, QVariant::fromValue(shift)));
      score->doLayout();
      QCOMPARE(rest->offset(), shift);
      QVERIFY(rest->ipos().y() > initialY + sp);

      // A second layout must not apply the horizontal offset a second time.
      const QPointF moved = rest->pagePos() + rest->bbox().center();
      score->doLayout();
      QVERIFY(QLineF(rest->pagePos() + rest->bbox().center(), moved).length() < 1e-5 * sp);

      QVERIFY(rest->setProperty(Pid::OFFSET, QVariant::fromValue(QPointF())));
      score->doLayout();
      QVERIFY(qAbs(rest->ipos().y() - initialY) < 1e-5 * sp);
      }

void TestIncrementalLayout::fullMeasureRestKeepsClearanceOnLookahead()
      {
      if (qEnvironmentVariableIntValue("MSCORE_NATIVE_REST_LAYOUT") != 1)
            QSKIP("Run with MSCORE_NATIVE_REST_LAYOUT=1 to exercise the preview.");
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-full-measure-rest-clearance.mscx"));
      QVERIFY(score && score->firstMeasure());
      Measure* second = score->firstMeasure()->nextMeasure();
      QVERIFY(second);
      Measure* third = second->nextMeasure();
      QVERIFY(third);
      Measure* fourth = third->nextMeasure();
      QVERIFY(fourth);
      // Make the displaced voice-2 rest the first rest of the next system.
      third->setLineBreak(true);
      score->doLayout();
      QVERIFY(third->system() != fourth->system());
      const Segment* segment = fourth->first(SegmentType::ChordRest);
      QVERIFY(segment && segment->element(1) && segment->element(1)->isRest());
      const Rest* rest = toRest(segment->element(1));
      QVERIFY(rest->isFullMeasureRest());
      const qreal sp = rest->spatium();
      const QPointF pos = rest->ipos();
      const QRectF ink = rest->bbox();
      QVERIFY(pos.y() > 4.0 * sp);
      QVERIFY(rest->sym() == SymId::restWholeLegerLine);

      // Starting at the preceding boundary enters the third measure but
      // allows the system starting at the fourth to be retained.
      score->doLayoutRange(second->tick(), second->tick());
      QVERIFY(QLineF(pos, rest->ipos()).length() < 1e-5 * sp);
      QCOMPARE(rest->bbox(), ink);
      QVERIFY(rest->sym() == SymId::restWholeLegerLine);
      score->doLayout();
      QVERIFY(QLineF(pos, rest->ipos()).length() < 1e-5 * sp);
      QCOMPARE(rest->bbox(), ink);
      }

void TestIncrementalLayout::crossStaffAutoStemsUseVoiceDirection()
      {
      if (qEnvironmentVariableIntValue("MSCORE_NATIVE_STEM_LAYOUT") != 1)
            QSKIP("Run with MSCORE_NATIVE_STEM_LAYOUT=1 to exercise the preview.");
      std::unique_ptr<MasterScore> score(readScore(DIR + "native-cross-staff-auto-stems.mscx"));
      QVERIFY(score);
      Measure* measure = score->firstMeasure();
      for (int bar = 0; bar < 3; ++bar) {
            QVERIFY(measure);
            int beat = 0;
            for (const Segment* seg = measure->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                  const Element* element = seg->element(VOICES);
                  QVERIFY(element && element->isChord());
                  const Chord* chord = toChord(element);
                  QCOMPARE(chord->staffMove(), -1);
                  QVERIFY(!chord->beam());
                  const bool manual = bar == 2 && beat == 3;
                  QCOMPARE(chord->stemDirection() == Direction::DOWN, manual);
                  // Visible full-measure rest, hidden rest, and four visible
                  // notes in voice 2, respectively. Manual Down still wins.
                  QCOMPARE(chord->up(), bar != 1 && !manual);
                  ++beat;
                  }
            QCOMPARE(beat, 4);
            measure = measure->nextMeasure();
            }
      }

QTEST_MAIN(TestIncrementalLayout)
#include "tst_incrementallayout.moc"
