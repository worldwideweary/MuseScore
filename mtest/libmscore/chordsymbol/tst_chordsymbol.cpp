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
#include <QPaintEngine>
#include <QRawFont>
#include "mtest/testutils.h"
#include "libmscore/score.h"
#include "libmscore/undo.h"
#include "libmscore/excerpt.h"
#include "libmscore/part.h"
#include "libmscore/measure.h"
#include "libmscore/segment.h"
#include "libmscore/chordrest.h"
#include "libmscore/harmony.h"
#include "libmscore/duration.h"
#include "libmscore/xml.h"
#include "libmscore/chordlist.h"
#include "libmscore/durationtype.h"
#include "libmscore/sym.h"

#define DIR QString("libmscore/chordsymbol/")

using namespace Ms;

// Record actual painter output so tests catch wrong glyphs and wrong fonts,
// including glyphs selected through an OS fallback font.
class ChordTextRecorder : public QPaintEngine {
   public:
      struct Run { QString text; QFont font; };
      QList<Run> runs;
      ChordTextRecorder() : QPaintEngine(QPaintEngine::AllFeatures) {}
      bool begin(QPaintDevice*) override { return true; }
      bool end() override { return true; }
      Type type() const override { return QPaintEngine::User; }
      void updateState(const QPaintEngineState&) override {}
      void drawPixmap(const QRectF&, const QPixmap&, const QRectF&) override {}
      void drawTextItem(const QPointF&, const QTextItem& item) override
            { runs.append(Run { item.text(), item.font() }); }
      };

class ChordTextDevice : public QPaintDevice {
   public:
      mutable ChordTextRecorder recorder;
      QPaintEngine* paintEngine() const override { return &recorder; }
   protected:
      int metric(PaintDeviceMetric metric) const override {
            switch (metric) {
                  case PdmWidth: case PdmHeight: return 4096;
                  case PdmWidthMM: case PdmHeightMM: return 290;
                  case PdmDpiX: case PdmPhysicalDpiX: return MScore::paintDevice()->logicalDpiX();
                  case PdmDpiY: case PdmPhysicalDpiY: return MScore::paintDevice()->logicalDpiY();
                  case PdmDepth: return 32;
                  case PdmNumColors: return 16777216;
                  case PdmDevicePixelRatio: return 1;
                  case PdmDevicePixelRatioScaled: return QPaintDevice::devicePixelRatioFScale();
                  default: return 0;
                  }
            }
      };

//---------------------------------------------------------
//   TestChordSymbol
//---------------------------------------------------------

class TestChordSymbol : public QObject, public MTest {
      Q_OBJECT

      MasterScore* test_pre(const char* p);
      void test_post(MasterScore* score, const char* p);

      void selectAllChordSymbols(MasterScore* score);
      void realizeSelectionVoiced(MasterScore* score, Voicing voicing);

   private slots:
      void initTestCase();
      void presentationStyleRoundTrip();
      void presentationStudioImport();
      void presentationScoreRoundTrip();
      void presentationVerticalAlignment();
      void presentationProperties();
      void presentationGeometry();
      void presentationGlyphs();
      void presentationFontChanges();
      void presentationStyleRollback();
      void polychordRoundTrip();
      void polychordInvalidInput();
      void testExtend();
      void testClear();
      void testAddLink();
      void testAddPart();
      void testNoSystem();
      void testTranspose();
      void testTransposePart();
      void testRealizeClose();
      void testRealizeDrop2();
      void testRealize3Note();
      void testRealize4Note();
      void testRealize6Note();
      void testRealizeConcertPitch();
      void testRealizeTransposed();
      void testRealizeOverride();
      void testRealizeTriplet();
      void testRealizeDuration();
      void testRealizeJazz();
      };

//---------------------------------------------------------
//   initTestCase
//---------------------------------------------------------

void TestChordSymbol::initTestCase()
      {
      initMTest();
      }

//---------------------------------------------------------
//   chordsymbol
//---------------------------------------------------------

MasterScore* TestChordSymbol::test_pre(const char* p)
      {
      QString p1 = DIR + p + ".mscx";
      MasterScore* score = readScore(p1);
      score->doLayout();
      return score;
      }

void TestChordSymbol::test_post(MasterScore* score, const char* p)
      {
      QString p1 = p;
      p1 += "-test.mscx";
      QString p2 = DIR + p + "-ref.mscx";
      QVERIFY(saveCompareScore(score, p1, p2));
      delete score;
      }

//---------------------------------------------------------
//   TestChordSymbol
///   select all chord symbols within the specified score
//---------------------------------------------------------
void TestChordSymbol::selectAllChordSymbols(MasterScore* score)
      {
      //find a chord symbol
      Segment* seg = score->firstSegment(SegmentType::ChordRest);
      Element* e = 0;
      while (seg) {
            e = seg->findAnnotation(ElementType::HARMONY,
                                              0, score->ntracks());
            if (e)
                  break;
            seg = seg->next1();
            }
      score->selectSimilar(e, false);
      }

//---------------------------------------------------------
//   realizeSelectionVoiced
///   realize the current selection of the score using
///   the specified voicing
//---------------------------------------------------------
void TestChordSymbol::realizeSelectionVoiced(MasterScore* score, Voicing voicing)
      {
      for (Element* e : score->selection().elements()) {
            if (e->isHarmony())
                  e->setProperty(Pid::HARMONY_VOICING, int(voicing));
            }
      score->startCmd();
      score->cmdRealizeChordSymbols();
      score->endCmd();
      }

// Bass scale must survive cloning and XML, including a per-symbol override.
void TestChordSymbol::presentationStyleRoundTrip()
      {
      MasterScore score(MScore::defaultStyle());
      MStyle style = score.style();
      style.set(Sid::chordSymbolLayoutVersion, 1);
      style.set(Sid::verticallyStackModifiers, true);
      style.set(Sid::chordModifierAdjust, -0.35);
      style.set(Sid::chordBassNoteScale, 0.7);
      style.set(Sid::chordBassNoteGap, 0.125);
      style.set(Sid::chordAlignmentToFretboard, QString("right"));
      style.set(Sid::polychordDividerSpacing, QVariant::fromValue(Spatium(0.65)));
      QBuffer buffer;
      QVERIFY(buffer.open(QIODevice::WriteOnly));
      XmlWriter xml(&score, &buffer);
      style.save(xml, true);
      buffer.close();
      QVERIFY(buffer.data().indexOf("chordSymbolLayoutVersion") < buffer.data().indexOf("chordModifierAdjust"));
      MStyle loaded = score.style();
      XmlReader reader(buffer.data());
      QVERIFY(reader.readNextStartElement());
      loaded.load(reader, 302);
      for (Sid sid : { Sid::chordSymbolLayoutVersion, Sid::verticallyStackModifiers,
                      Sid::chordExtensionMag, Sid::chordModifierAdjust,
                      Sid::chordBassNoteScale, Sid::chordBassNoteGap, Sid::polychordDividerSpacing,
                      Sid::chordAlignmentToFretboard, Sid::chordSymPosition })
            QCOMPARE(loaded.value(sid), style.value(sid));
      }

void TestChordSymbol::presentationProperties()
      {
      MasterScore score(MScore::defaultStyle());
      score.style().set(Sid::chordBassNoteScale, 0.8);
      Harmony h(&score);
      h.setHarmony("C7/G");
      QCOMPARE(h.getProperty(Pid::HARMONY_BASS_SCALE).toDouble(), 0.8);
      QVERIFY(h.setProperty(Pid::HARMONY_BASS_SCALE, 0.65));
      h.setPropertyFlags(Pid::HARMONY_BASS_SCALE, PropertyFlags::UNSTYLED);
      QVERIFY(!h.setProperty(Pid::HARMONY_BASS_SCALE, 0.0));
      std::unique_ptr<Harmony> copy(h.clone());
      QCOMPARE(copy->getProperty(Pid::HARMONY_BASS_SCALE).toDouble(), 0.65);
      std::unique_ptr<Harmony> loaded(toHarmony(writeReadElement(&h)));
      QCOMPARE(loaded->getProperty(Pid::HARMONY_BASS_SCALE).toDouble(), 0.65);
      QCOMPARE(loaded->harmonyName(), h.harmonyName());
      }

void TestChordSymbol::polychordRoundTrip()
      {
      MasterScore score(MScore::defaultStyle());
      score.style().set(Sid::chordSymbolLayoutVersion, 1);
      Harmony h(&score);
      h.setHarmony("Db7|G|Am");
      QVERIFY(h.isPolychord());
      QCOMPARE(h.chordCount(), 3);
      QVERIFY(!h.isRealizable());
      QCOMPARE(h.harmonyName(), QString("Db7|G|Am"));
      h.layout();
      const QRectF before = h.bbox();
      QVERIFY(before.height() > 0.0);
      std::unique_ptr<Harmony> clone(h.clone());
      QCOMPARE(clone->harmonyName(), h.harmonyName());
      std::unique_ptr<Harmony> loaded(toHarmony(writeReadElement(&h)));
      QCOMPARE(loaded->chordCount(), 3);
      QCOMPARE(loaded->harmonyName(), h.harmonyName());
      h.setRootTpc(h.rootTpc() + 2); // transpose all three roots up a whole tone
      QCOMPARE(h.harmonyName(), QString("Eb7|A|Bm"));
      h.setHarmony("C7");
      QVERIFY(!h.isPolychord());
      QCOMPARE(h.chordCount(), 1);
      }

void TestChordSymbol::polychordInvalidInput()
      {
      MasterScore score(MScore::defaultStyle());
      for (const QString& text : { QString("C|"), QString("|G"), QString("C||G"), QString("C|invalid") }) {
            Harmony h(&score);
            h.setHarmony(text);
            QVERIFY(!h.isPolychord());
            QCOMPARE(h.harmonyName(), text);
            }
      }

void TestChordSymbol::presentationStudioImport()
      {
      MasterScore score(MScore::defaultStyle());
      QCOMPARE(score.loadMsc(rootPath() + DIR + "presentation-460.mscx", true), Score::FileError::FILE_NO_ERROR);
      score.doLayout();
      QCOMPARE(score.styleI(Sid::chordSymbolLayoutVersion), 1);
      QVERIFY(score.styleB(Sid::verticallyStackModifiers));
      QCOMPARE(score.styleD(Sid::chordModifierAdjust), 0.25);
      QCOMPARE(score.styleSt(Sid::chordAlignmentToFretboard), QString("right"));
      Segment* segment = score.firstSegment(SegmentType::ChordRest);
      QVERIFY(segment);
      Element* element = segment->findAnnotation(ElementType::HARMONY, 0, score.ntracks());
      QVERIFY(element);
      Harmony* harmony = toHarmony(element);
      QCOMPARE(harmony->chordCount(), 2);
      QCOMPARE(harmony->rootTpc(), 9);
      QCOMPARE(harmony->baseTpc(), 13);
      QCOMPARE(harmony->getProperty(Pid::HARMONY_BASS_SCALE).toDouble(), 0.65);
      std::unique_ptr<Harmony> loaded(toHarmony(writeReadElement(harmony)));
      QCOMPARE(loaded->chordCount(), 2);
      QCOMPARE(loaded->harmonyName(), harmony->harmonyName());
      }

void TestChordSymbol::presentationScoreRoundTrip()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "presentation-demo.mscx"));
      QVERIFY(score);
      auto names = [](MasterScore* s) {
            QStringList result;
            for (Segment* segment = s->firstSegment(SegmentType::ChordRest); segment; segment = segment->next1()) {
                  Element* element = segment->findAnnotation(ElementType::HARMONY, 0, s->ntracks());
                  if (element)
                        result.append(toHarmony(element)->harmonyName());
                  }
            return result;
            };
      const QStringList before = names(score.get());
      QCOMPARE(before.size(), 6);
      QTemporaryDir dir;
      QVERIFY(dir.isValid());
      const QString path = dir.path() + "/presentation.mscx";
      QVERIFY(saveScore(score.get(), path));
      MasterScore loaded(MScore::defaultStyle());
      QCOMPARE(loaded.loadMsc(path, false), Score::FileError::FILE_NO_ERROR);
      loaded.doLayout();
      QCOMPARE(names(&loaded), before);
      QCOMPARE(loaded.styleI(Sid::chordSymbolLayoutVersion), 1);
      QVERIFY(loaded.styleB(Sid::verticallyStackModifiers));
      const QString output = QString::fromLocal8Bit(qgetenv("MSCORE_CHORD_DEMO_SAVE"));
      if (!output.isEmpty())
            QVERIFY(saveScore(&loaded, output));
      }

void TestChordSymbol::presentationVerticalAlignment()
      {
      std::unique_ptr<MasterScore> score(readScore(DIR + "presentation-demo.mscx"));
      QVERIFY(score);
      QMap<System*, qreal> baselines;
      int count = 0;
      for (Segment* segment = score->firstSegment(SegmentType::ChordRest); segment; segment = segment->next1()) {
            Element* element = segment->findAnnotation(ElementType::HARMONY, 0, score->ntracks());
            if (!element)
                  continue;
            System* system = segment->measure()->system();
            QVERIFY(system);
            const qreal y = element->y();
            if (baselines.contains(system))
                  QVERIFY(qAbs(y - baselines.value(system)) < 1e-6);
            else
                  baselines.insert(system, y);
            ++count;
            }
      QCOMPARE(count, 6);
      }

void TestChordSymbol::presentationStyleRollback()
      {
      class RefreshScore : public MasterScore {
         public:
            int refreshes = 0;
            RefreshScore() : MasterScore(MScore::defaultStyle()) {}
            void styleChanged() override { ++refreshes; MasterScore::styleChanged(); }
            };
      RefreshScore score;
      const qreal initial = score.styleD(Sid::chordExtensionMag);
      score.startCmd();
      for (int i = 1; i <= 400; ++i)
            score.undo(new ChangeStyleVal(&score, Sid::chordExtensionMag, initial + i * 0.01));
      score.refreshes = 0;
      score.endCmd(true);
      QCOMPARE(score.styleD(Sid::chordExtensionMag), initial);
      QCOMPARE(score.refreshes, 1);

      bool boundarySawRestoredStyle = true;
      class Boundary : public UndoCommand {
            RefreshScore* score;
            bool* ok;
            qreal expected;
            void flip(EditData*) override {
                  *ok = *ok && score->styleD(Sid::chordExtensionMag) == expected && score->refreshes > 0;
                  }
         public:
            Boundary(RefreshScore* s, bool* result, qreal value) : score(s), ok(result), expected(value) {}
            };
      score.startCmd();
      score.undo(new ChangeStyleVal(&score, Sid::chordExtensionMag, initial + 1.0));
      score.undo(new Boundary(&score, &boundarySawRestoredStyle, initial + 1.0));
      score.undo(new ChangeStyleVal(&score, Sid::chordExtensionMag, initial + 2.0));
      score.refreshes = 0;
      score.endCmd(true);
      QVERIFY(boundarySawRestoredStyle);
      QCOMPARE(score.styleD(Sid::chordExtensionMag), initial);
      QCOMPARE(score.refreshes, 2);
      }

void TestChordSymbol::presentationFontChanges()
      {
      MasterScore score(MScore::defaultStyle());
      // Inspect the cache immediately, without a layout pass that could hide
      // the one-selection lag. Each transition must also undo and redo correctly.
      for (const QString& family : { QString("Petaluma"), QString("MuseJazz"),
                  QString("Gonville"), QString("MuseJazz"), QString("Petaluma"), QString("Gonville") }) {
            const QString before = score.styleSt(Sid::musicalSymbolFont);
            ChangeStyleVal change(&score, Sid::musicalSymbolFont, family);
            change.redo(nullptr);
            QCOMPARE(score.scoreFont()->name(), family);
            QCOMPARE(score.styleSt(Sid::musicalSymbolFont), family);
            change.undo(nullptr);
            QCOMPARE(score.scoreFont()->name(), before);
            QCOMPARE(score.styleSt(Sid::musicalSymbolFont), before);
            change.redo(nullptr);
            QCOMPARE(score.scoreFont()->name(), family);
            }
      }

void TestChordSymbol::presentationGlyphs()
      {
      MasterScore score(MScore::defaultStyle());
      score.style().set(Sid::chordSymbolLayoutVersion, 1);
      score.style().set(Sid::musicalTextFont, QString("Leland Text"));
      score.setScoreFont(ScoreFont::fontFactory("Leland"));
      Harmony harmony(&score);
      auto output = [&]() {
            harmony.render();
            harmony.layout();
            ChordTextDevice device;
            QPainter painter(&device);
            static_cast<const Element&>(harmony).draw(&painter);
            painter.end();
            return device.recorder.runs;
            };
      auto hasGlyph = [](const QList<ChordTextRecorder::Run>& runs, ushort code) {
            for (const auto& run : runs) {
                  if (run.text.contains(QChar(code)))
                        return QFontMetricsF(run.font, MScore::paintDevice()).inFontUcs4(code);
                  }
            return false;
            };
      harmony.setHarmony("Co");
      QVERIFY(hasGlyph(output(), 0xE870));
      harmony.setHarmony("C0");
      QVERIFY(hasGlyph(output(), 0xE871));
      // Missing primary-font symbols must not be accepted through Qt fallback.
      score.style().set(Sid::musicalTextFont, QString("DejaVu Sans"));
      for (const QString& name : { QString("Co"), QString("C0") }) {
            harmony.setHarmony(name);
            const auto runs = output();
            const ushort code = name == "Co" ? 0xE870 : 0xE871;
            QVERIFY(hasGlyph(runs, code));
            for (const auto& run : runs) {
                  if (run.text.contains(QChar(code))) {
                        const QRawFont raw = QRawFont::fromFont(run.font);
                        QVERIFY(raw.supportsCharacter(code));
                        QCOMPARE(raw.familyName(), QString("Bravura Text"));
                        QVERIFY(run.font.styleStrategy() & QFont::NoFontMerging);
                        }
                  }
            }
      score.style().set(Sid::musicalTextFont, QString("Leland Text"));
      // A zero in an extension must remain a digit, not half-diminished.
      harmony.setHarmony("C10");
      QVERIFY(!hasGlyph(output(), 0xE871));
      score.style().set(Sid::chordSymbolLayoutVersion, 0);
      harmony.setHarmony("Co");
      QVERIFY(!hasGlyph(output(), 0xE870));
      score.style().set(Sid::chordSymbolLayoutVersion, 1);
      score.style().set(Sid::verticallyStackModifiers, true);
      score.style().set(Sid::harmonyParenUseSmuflSym, true);
      harmony.setHarmony("C7(b9,#11)");
      const auto smufl = output();
      QVERIFY(hasGlyph(smufl, 0xE875));
      QVERIFY(hasGlyph(smufl, 0xE876));
      // Parentheses follow the score's music font, not its text-font setting.
      // The default Leland music font has no pair; both sides must use Bravura.
      for (const auto& run : smufl) {
            if (run.text.contains(QChar(ushort(0xE875))) || run.text.contains(QChar(ushort(0xE876)))) {
                  const QRawFont raw = QRawFont::fromFont(run.font);
                  QVERIFY(raw.isValid());
                  QVERIFY(raw.supportsCharacter(0xE875));
                  QVERIFY(raw.supportsCharacter(0xE876));
                  QCOMPARE(raw.familyName(), QString("Bravura"));
                  QVERIFY(run.font.styleStrategy() & QFont::NoFontMerging);
                  }
            }
      score.style().set(Sid::musicalTextFont, QString("DejaVu Sans"));
      const auto fallback = output();
      QVERIFY(hasGlyph(fallback, 0xE875));
      QVERIFY(hasGlyph(fallback, 0xE876));
      // Keep a selected music font's own pair when it has both sides.
      // Bundled Finale Broadway lacks the pair and must use Bravura.
      for (const QString& family : { QString("Bravura"), QString("Petaluma"), QString("MuseJazz"), QString("Finale Broadway") }) {
            ChangeStyleVal change(&score, Sid::musicalSymbolFont, family);
            change.redo(nullptr);
            const auto selected = output();
            QVERIFY(hasGlyph(selected, 0xE875));
            QVERIFY(hasGlyph(selected, 0xE876));
            for (const auto& run : selected) {
                  if (run.text.contains(QChar(ushort(0xE875))) || run.text.contains(QChar(ushort(0xE876))))
                        QCOMPARE(QRawFont::fromFont(run.font).familyName(),
                              family == "Finale Broadway" ? QString("Bravura") : family);
                  }
            }
      score.style().set(Sid::harmonyParenUseSmuflSym, false);
      const auto text = output();
      QVERIFY(!hasGlyph(text, 0xE875));
      QVERIFY(!hasGlyph(text, 0xE876));
      }

void TestChordSymbol::presentationGeometry()
      {
      MasterScore score(MScore::defaultStyle());
      score.style().set(Sid::chordSymbolLayoutVersion, 1);
      Harmony h(&score);
      h.setHarmony("C7(b9,#11)");
      h.layout();
      const QRectF inlineBox = h.bbox();
      score.style().set(Sid::verticallyStackModifiers, true);
      h.render();
      h.layout();
      QVERIFY(h.bbox().height() > inlineBox.height());
      QVERIFY(h.bbox().width() < inlineBox.width());
      const QRectF stackedBox = h.bbox();
      h.layout();
      QCOMPARE(h.bbox(), stackedBox); // layout is idempotent

      // Single-modifier chords do not unexpectedly turn into multi-line stacks.
      h.setHarmony("C7b9");
      h.layout();
      QVERIFY(h.bbox().height() < stackedBox.height());

      // Extension sizing works independently of the root.
      h.setHarmony("C13");
      h.layout();
      const qreal fullWidth = h.bbox().width();
      score.style().set(Sid::chordExtensionMag, 0.5);
      score.style().chordList()->unload();
      score.style().checkChordList();
      h.render();
      h.layout();
      QVERIFY2(h.bbox().width() < fullWidth, qPrintable(QString("C13 widths before %1 after %2; name %3, scale %4").arg(fullWidth).arg(h.bbox().width()).arg(h.harmonyName()).arg(score.styleD(Sid::chordExtensionMag))));

      // Staggered inversion glyphs and the bass scale must remain independent
      // of extension/modifier scaling and survive repeated layout.
      score.style().set(Sid::chordBassNoteStagger, true);
      h.setHarmony("C13/G");
      h.layout();
      const QRectF inversionBox = h.bbox();
      h.layout();
      QCOMPARE(h.bbox(), inversionBox);
      h.setProperty(Pid::HARMONY_BASS_SCALE, 0.5);
      h.render();
      h.layout();
      QVERIFY(h.bbox().width() < inversionBox.width());
      h.setHarmony("F#m7/B");
      h.layout();
      QVERIFY(h.bbox().isValid());

      // Produce a rendering sample for visual QA when explicitly requested.
      const QString preview = QString::fromLocal8Bit(qgetenv("MSCORE_CHORD_PREVIEW"));
      if (!preview.isEmpty()) {
            QImage image(1200, 700, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::white);
            const qreal oldRatio = MScore::pixelRatio;
            MScore::pixelRatio = DPI / image.logicalDpiX();
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);
            const QStringList examples { "C7(b9,#11)", "C13b9#11b13", "C7sus4add9",
                  "C7(add9,add11)", "Db|G", "C7|F#m7/B|G", "C13/G", "F#m7/B", "Cm7" };
            score.style().set(Sid::chordExtensionMag, 1.0);
            score.style().chordList()->unload();
            score.style().checkChordList();
            for (int i = 0; i < examples.size(); ++i) {
                  Harmony sample(&score);
                  sample.setHarmony(examples[i]);
                  sample.layout();
                  painter.save();
                  const QPointF pos(35 + (i % 3) * 400, 40 + (i / 3) * 230);
                  painter.setPen(Qt::black);
                  painter.setFont(QFont("DejaVu Sans", 11));
                  painter.drawText(pos, examples[i]);
                  const QRectF box = sample.bbox();
                  const qreal zoom = qMin(1.0, qMin(330.0 / qMax(1.0, box.width()), 155.0 / qMax(1.0, box.height())));
                  painter.translate(pos + QPointF(20, 30) - box.topLeft() * zoom);
                  painter.scale(zoom, zoom);
                  static_cast<const Element&>(sample).draw(&painter);
                  painter.restore();
                  }
            painter.end();
            MScore::pixelRatio = oldRatio;
            QVERIFY(image.save(preview));
            }
      }

void TestChordSymbol::testExtend()
      {
      MasterScore* score = test_pre("extend");
      Measure* m = score->firstMeasure();
      Segment* s = m->first(SegmentType::ChordRest);
      ChordRest* cr = s->cr(0);
      score->changeCRlen(cr, TDuration::DurationType::V_WHOLE);
      score->doLayout();
      test_post(score, "extend");
      }

void TestChordSymbol::testClear()
      {
      MasterScore* score = test_pre("clear");
      Measure* m = score->firstMeasure();
      score->select(m, SelectType::SINGLE, 0);
      score->cmdDeleteSelection();
      score->doLayout();
      test_post(score, "clear");
      }

void TestChordSymbol::testAddLink()
      {
      MasterScore* score = test_pre("add-link");
      Segment* seg = score->firstSegment(SegmentType::ChordRest);
      ChordRest* cr = seg->cr(0);
      Harmony* harmony = new Harmony(score);
      harmony->setHarmony("C7");
      harmony->setTrack(cr->track());
      harmony->setParent(cr->segment());
      score->undoAddElement(harmony);
      score->doLayout();
      test_post(score, "add-link");
      }

void TestChordSymbol::testAddPart()
      {
      MasterScore* score = test_pre("add-part");
      Segment* seg = score->firstSegment(SegmentType::ChordRest);
      ChordRest* cr = seg->cr(0);
      Harmony* harmony = new Harmony(score);
      harmony->setHarmony("C7");
      harmony->setTrack(cr->track());
      harmony->setParent(cr->segment());
      score->undoAddElement(harmony);
      score->doLayout();
      test_post(score, "add-part");
      }

void TestChordSymbol::testNoSystem()
      {
      MasterScore* score = test_pre("no-system");

      //
      // create first part
      //
      QList<Part*> parts;
      parts.append(score->parts().at(0));
      Score* nscore = new Score(score);

      Excerpt* ex = new Excerpt(score);
      ex->setPartScore(nscore);
      nscore->setExcerpt(ex);
      score->excerpts().append(ex);
      ex->setTitle(parts.front()->longName());
      ex->setParts(parts);
      Excerpt::createExcerpt(ex);
      QVERIFY(nscore);

//      nscore->setTitle(parts.front()->partName());
      nscore->style().set(Sid::createMultiMeasureRests, true);

      //
      // create second part
      //
      parts.clear();
      parts.append(score->parts().at(1));
      nscore = new Score(score);

      ex = new Excerpt(score);
      ex->setPartScore(nscore);
      nscore->setExcerpt(ex);
      score->excerpts().append(ex);
      ex->setTitle(parts.front()->longName());
      ex->setParts(parts);
      Excerpt::createExcerpt(ex);
      QVERIFY(nscore);

//      nscore->setTitle(parts.front()->partName());
      nscore->style().set(Sid::createMultiMeasureRests, true);

      score->setExcerptsChanged(true);
      score->doLayout();
      test_post(score, "no-system");
      }

void TestChordSymbol::testTranspose()
      {
      MasterScore* score = test_pre("transpose");
      score->startCmd();
      score->cmdSelectAll();
      score->transpose(TransposeMode::BY_INTERVAL, TransposeDirection::UP, Key::C, 4, false, true, true);
      score->endCmd();
      test_post(score, "transpose");
      }

void TestChordSymbol::testTransposePart()
      {
      MasterScore* score = test_pre("transpose-part");
      score->startCmd();
      score->cmdSelectAll();
      score->transpose(TransposeMode::BY_INTERVAL, TransposeDirection::UP, Key::C, 4, false, true, true);
      score->endCmd();
      test_post(score, "transpose-part");
      }

//---------------------------------------------------------
//   testRealizeClose
///   check close voicing algorithm
//---------------------------------------------------------
void TestChordSymbol::testRealizeClose()
      {
      MasterScore* score = test_pre("realize");
      selectAllChordSymbols(score);
      realizeSelectionVoiced(score, Voicing::CLOSE);
      test_post(score, "realize-close");
      }

//---------------------------------------------------------
//   testRealizeDrop2
///   check Drop 2 voicing algorithm
//---------------------------------------------------------
void TestChordSymbol::testRealizeDrop2()
      {
      MasterScore* score = test_pre("realize");
      selectAllChordSymbols(score);
      realizeSelectionVoiced(score, Voicing::DROP_2);
      test_post(score, "realize-drop2");
      }

//---------------------------------------------------------
//   testRealize3Note
///   check 3 note voicing algorithm
//---------------------------------------------------------
void TestChordSymbol::testRealize3Note()
      {
      MasterScore* score = test_pre("realize");
      selectAllChordSymbols(score);
      realizeSelectionVoiced(score, Voicing::THREE_NOTE);
      test_post(score, "realize-3note");
      }

//---------------------------------------------------------
//   testRealize4Note
///   check 4 note voicing algorithm
//---------------------------------------------------------
void TestChordSymbol::testRealize4Note()
      {
      MasterScore* score = test_pre("realize");
      selectAllChordSymbols(score);
      realizeSelectionVoiced(score, Voicing::FOUR_NOTE);
      test_post(score, "realize-4note");
      }

//---------------------------------------------------------
//   testRealize6Note
///   check 6 note voicing algorithm
//---------------------------------------------------------
void TestChordSymbol::testRealize6Note()
      {
      MasterScore* score = test_pre("realize");
      selectAllChordSymbols(score);
      realizeSelectionVoiced(score, Voicing::SIX_NOTE);
      test_post(score, "realize-6note");
      }

//---------------------------------------------------------
//   testRealizeConcertPitch
///   Check if the note pitches and tpcs are correct after realizing
///   chord symbols on transposed instruments.
//---------------------------------------------------------
void TestChordSymbol::testRealizeConcertPitch()
      {
      MasterScore* score = test_pre("realize-concert-pitch");
      //concert pitch off
      score->startCmd();
      score->cmdConcertPitchChanged(false, true);
      score->endCmd();

      //realize all chord symbols
      selectAllChordSymbols(score);
      score->startCmd();
      score->cmdRealizeChordSymbols();
      score->endCmd();
      test_post(score, "realize-concert-pitch");
      }

//---------------------------------------------------------
//   testRealizeTransposed
///   Check if the note pitches and tpcs are correct after
///   transposing the score
//---------------------------------------------------------
void TestChordSymbol::testRealizeTransposed()
      {
      MasterScore* score = test_pre("transpose");
      //transpose
      score->cmdSelectAll();
      score->transpose(TransposeMode::BY_INTERVAL, TransposeDirection::UP, Key::C, 4, false, true, true);

      //realize all chord symbols
      selectAllChordSymbols(score);
      score->startCmd();
      score->cmdRealizeChordSymbols();
      score->endCmd();
      test_post(score, "realize-transpose");
      }

//---------------------------------------------------------
//   testRealizeOverride
///   Check for correctness when using the override
///   feature for realizing chord symbols
//---------------------------------------------------------
void TestChordSymbol::testRealizeOverride()
      {
      MasterScore* score = test_pre("realize-override");
      //realize all chord symbols
      selectAllChordSymbols(score);
      score->startCmd();
      score->cmdRealizeChordSymbols(true, Voicing::ROOT_ONLY, HDuration::SEGMENT_DURATION);
      score->endCmd();
      test_post(score, "realize-override");
      }

//---------------------------------------------------------
//   testRealizeTriplet
///   Check for correctness when realizing chord symbols on triplets
//---------------------------------------------------------
void TestChordSymbol::testRealizeTriplet()
      {
      MasterScore* score = test_pre("realize-triplet");
      //realize all chord symbols
      selectAllChordSymbols(score);
      score->startCmd();
      score->cmdRealizeChordSymbols();
      score->endCmd();
      test_post(score, "realize-triplet");
      }

//---------------------------------------------------------
//   testRealizeDuration
///   Check for correctness when realizing chord symbols
///   with different durations
//---------------------------------------------------------
void TestChordSymbol::testRealizeDuration()
      {
      MasterScore* score = test_pre("realize-duration");
      //realize all chord symbols
      selectAllChordSymbols(score);
      score->startCmd();
      score->cmdRealizeChordSymbols();
      score->endCmd();
      test_post(score, "realize-duration");
      }

//---------------------------------------------------------
//   testRealizeJazz
///   Check for correctness when realizing chord symbols
///   with jazz mode
//---------------------------------------------------------
void TestChordSymbol::testRealizeJazz()
      {
      MasterScore* score = test_pre("realize-jazz");
      //realize all chord symbols
      selectAllChordSymbols(score);
      score->startCmd();
      score->cmdRealizeChordSymbols();
      score->endCmd();
      test_post(score, "realize-jazz");
      }


QTEST_MAIN(TestChordSymbol)
#include "tst_chordsymbol.moc"
