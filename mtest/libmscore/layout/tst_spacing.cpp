// MuseScore — optional Studio spacing regression tests.
// Licensed under GNU GPL version 2, as published in LICENSE.GPL.
#include <QtTest/QtTest>
#include "mtest/testutils.h"
#include "libmscore/score.h"
#include "libmscore/measure.h"
#include "libmscore/segment.h"
#include "libmscore/spacinglayout4.h"
#include "libmscore/chordrest.h"
#include "libmscore/beam.h"

using namespace Ms;
#define DIR QString("libmscore/layout/")

class TestSpacing : public QObject, public MTest {
      Q_OBJECT
   private slots:
      void initTestCase() { initMTest(); }
      void styleRoundTrip();
      void durationPolicy();
      void rhythmicLayoutSwitch();
      void crossBeamSwitch_data();
      void crossBeamSwitch();
      void crossBeamCorners_data();
      void crossBeamCorners();
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
      const int signs[3][8] = {
            { -1, 1, 0, -1, 1, 0, 0, 0 },
            { 1, -1, 0, 1, -1, 0, 0, 0 },
            { -1, 1, -1, 1, -1, 1, -1, 0 }
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
                        const qreal expected = row < 3 ? signs[row][onset] * displacement : 0;
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

QTEST_MAIN(TestSpacing)
#include "tst_spacing.moc"
