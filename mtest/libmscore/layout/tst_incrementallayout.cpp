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

QTEST_MAIN(TestIncrementalLayout)
#include "tst_incrementallayout.moc"
