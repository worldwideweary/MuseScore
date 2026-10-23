// MuseScore — optional Studio spacing regression tests.
// Licensed under GNU GPL version 2, as published in LICENSE.GPL.
#include <QtTest/QtTest>
#include "mtest/testutils.h"
#include "libmscore/score.h"
#include "libmscore/measure.h"
#include "libmscore/segment.h"
#include "libmscore/spacinglayout4.h"
#include "libmscore/spacinggeometry4.h"
#include "libmscore/shape.h"
#include "libmscore/notedot.h"
#include "libmscore/chordrest.h"
#include "libmscore/beam.h"
#include "libmscore/chord.h"
#include "libmscore/note.h"
#include "libmscore/accidental.h"

using namespace Ms;
#define DIR QString("libmscore/layout/")

class TestSpacing : public QObject, public MTest {
      Q_OBJECT
   private slots:
      void initTestCase() { initMTest(); }
      void styleRoundTrip();
      void durationPolicy();
      void springForce();
      void geometryResolution();
      void geometryInputs_data();
      void geometryInputs();
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

void TestSpacing::geometryResolution()
      {
      MasterScore* score = readScore(DIR + "studio-spacing-natural-h1.mscx");
      QVERIFY(score);
      {
      Chord context(score);
      context.setTrack(0);
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
                        }
                  QCOMPARE(seg.x(), x);
                  QCOMPARE(seg.width(), width);
                  QCOMPARE(seg.spacingWidthOffset4(), offset);
                  }
            QCOMPARE(m->width(), measureWidth);
            }
      QVERIFY(heads > 0);
      QVERIFY(stems > 0);
      if (file.contains("accidentals")) {
            QVERIFY(signs > 0);
            QVERIFY(crossStaffHeads > 0);
            }
      QVERIFY(spacingStaffShape4(nullptr, 0).elements.empty());
      QVERIFY(spacingStaffShape4(score->firstMeasure()->first(), -1).elements.empty());
      QVERIFY(spacingStaffShape4(score->firstMeasure()->first(), score->nstaves()).elements.empty());
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

QTEST_MAIN(TestSpacing)
#include "tst_spacing.moc"
