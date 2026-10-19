#include <algorithm>
#include <QtTest/QtTest>
#include "mtest/testutils.h"
#include "libmscore/box.h"
#include "libmscore/fret.h"
#include "libmscore/harmony.h"
#include "libmscore/textframe.h"
#include "libmscore/xml.h"
#include "libmscore/measure.h"
#include "libmscore/segment.h"
#include "libmscore/chordlist.h"
#include "libmscore/score.h"
#include "libmscore/system.h"

using namespace Ms;
class TestFretLegend : public QObject, public MTest {
      Q_OBJECT
      // Foreign-format fixtures must exercise the compatibility reader;
      // MTest::readScore intentionally rejects newer versions by default.
      MasterScore* readScore(const QString& name) {
            auto* loaded = new MasterScore(score->style());
            if (loaded->loadMsc(root + "/" + name, true) != Score::FileError::FILE_NO_ERROR) {
                  delete loaded;
                  return nullptr;
                  }
            for (Score* part : loaded->scoreList())
                  part->doLayout();
            return loaded;
            }
   private slots:
      void initTestCase() { initMTest(); }
      void importAndLayout();
      void roundTrip();
      void collectAndUndo();
      void preserveCustomOrder();
      void removeSourceAndUndo();
      void emptyAndContinuous();
      void chordDefinitionsWithoutStyle();
      void legendHarmonyAnchorsAndSave();
      void generatedChordsAndMarkerClearance();
      void frameNotationClearance();
      void horizontalLegendRelayout();
      void mixedOrientationLabelClearance();
      void sourcePatternEdits();
      };

void TestFretLegend::importAndLayout()
      {
      auto* s = readScore("libmscore/fretlegend/legend460.mscx");
      QVERIFY(s);
      QVERIFY(s->first()->isFBox());
      auto* frame = toFBox(s->first());
      QCOMPARE(frame->getProperty(Pid::FRET_FRAME_TEXT_SCALE).toDouble(), 1.25);
      s->doLayout();
      QVERIFY(frame->system());
      QVERIFY(frame->system()->vbox());
      QCOMPARE(int(frame->el().size()), 2);
      auto* first = toFretDiagram(frame->el().front());
      auto* last = toFretDiagram(frame->el().back());
      QVERIFY(first->harmony());
      QVERIFY(last->y() > first->y());
      const qreal twoRows = frame->height();
      frame->setProperty(Pid::FRET_FRAME_CHORDS_PER_ROW, 2);
      s->doLayout();
      QVERIFY(frame->height() < twoRows);
      QCOMPARE(first->y(), last->y());
      QVERIFY(last->x() > first->x() + first->width());
      const qreal singleRowHeight = frame->height();
      last->setVisible(false);
      s->doLayout();
      QVERIFY(frame->height() <= singleRowHeight);
      last->setVisible(true);
      const qreal diagramWidth = first->width();
      frame->setProperty(Pid::FRET_FRAME_DIAGRAM_SCALE, 2.0);
      s->doLayout();
      QVERIFY(first->width() > diagramWidth * 1.8);
      delete s;
      }

void TestFretLegend::roundTrip()
      {
      auto* s = readScore("libmscore/fretlegend/legend460.mscx");
      QVERIFY(s);
      auto* frame = toFBox(s->first());
      frame->setProperty(Pid::FRET_FRAME_ROW_GAP, QVariant::fromValue(Spatium(4.5)));
      frame->setProperty(Pid::FRET_FRAME_H_ALIGN, 1);
      frame->setProperty(Pid::SIZE_SPATIUM_DEPENDENT, false);
      QVERIFY(saveScore(s, "fretlegend-roundtrip.mscx"));
      auto* copy = readCreatedScore("fretlegend-roundtrip.mscx");
      QVERIFY(copy);
      auto* restored = toFBox(copy->first());
      QCOMPARE(restored->getProperty(Pid::FRET_FRAME_ROW_GAP).value<Spatium>().val(), 4.5);
      QCOMPARE(restored->getProperty(Pid::FRET_FRAME_H_ALIGN).toInt(), 1);
      QCOMPARE(restored->getProperty(Pid::SIZE_SPATIUM_DEPENDENT).toBool(), false);
      QCOMPARE(int(restored->el().size()), 2);
      delete copy;
      delete s;
      }
void TestFretLegend::collectAndUndo()
      {
      auto* s = readScore("libmscore/fretlegend/legend460.mscx");
      QVERIFY(s);
      auto* frame = toFBox(s->first());
      auto* segment = s->firstMeasure()->first(SegmentType::ChordRest);
      QVERIFY(segment);
      auto* source = new Harmony(s);
      source->setParent(segment);
      source->setTrack(0);
      source->setHarmony("D");
      s->startCmd();
      s->undoAddElement(source);
      s->doLayout();
      s->endCmd();
      QCOMPARE(int(frame->el().size()), 3);
      auto* added = toFretDiagram(frame->el().back());
      QVERIFY(added->harmony());
      QCOMPARE(added->harmony()->harmonyName(), QString("D"));
      // Studio's database has a real D shape, not an empty rectangle.
      QVERIFY(added->dot(3).front().exists());
      s->undoRedo(true, nullptr);
      s->doLayout();
      QCOMPARE(int(frame->el().size()), 2);
      s->undoRedo(false, nullptr);
      s->doLayout();
      QCOMPARE(int(frame->el().size()), 3);
      QVERIFY(frame->el().back() == added);
      delete s;
      }

void TestFretLegend::preserveCustomOrder()
      {
      auto* s = readScore("libmscore/fretlegend/legend460.mscx");
      QVERIFY(s);
      auto* frame = toFBox(s->first());
      auto* first = toFretDiagram(frame->el().front());
      auto* second = toFretDiagram(frame->el().back());
      s->startCmd();
      frame->undoChangeProperty(Pid::FRET_FRAME_DIAGRAMS_ORDER,
            second->harmony()->harmonyName() + QChar(0x1f) + first->harmony()->harmonyName());
      first->undoChangeProperty(Pid::VISIBLE, false);
      s->doLayout();
      s->endCmd();
      QVERIFY(frame->el().front() == second);
      QVERIFY(!first->harmony()->visible());
      QVERIFY(saveScore(s, "fretlegend-order.mscx"));
      auto* copy = readCreatedScore("fretlegend-order.mscx");
      QVERIFY(copy);
      auto* restored = toFBox(copy->first());
      QCOMPARE(restored->getProperty(Pid::FRET_FRAME_DIAGRAMS_ORDER), frame->getProperty(Pid::FRET_FRAME_DIAGRAMS_ORDER));
      QVERIFY(!restored->el().back()->visible());
      s->undoRedo(true, nullptr);
      s->doLayout();
      QVERIFY(frame->el().front() == first);
      QVERIFY(first->visible());
      QVERIFY(first->harmony()->visible());
      delete copy;
      delete s;
      }

void TestFretLegend::removeSourceAndUndo()
      {
      auto* s = readScore("libmscore/fretlegend/legend460.mscx");
      QVERIFY(s);
      auto* frame = toFBox(s->first());
      auto* custom = toFretDiagram(frame->el().front());
      custom->setDot(0, 2);
      Harmony* source = nullptr;
      for (const Segment& segment : s->firstMeasure()->segments()) {
            for (auto* e : segment.annotations()) {
                  if (e->isHarmony())
                        source = toHarmony(e);
                  }
            }
      QVERIFY(source);
      s->startCmd();
      s->undoRemoveElement(source);
      s->endCmd();
      QCOMPARE(int(frame->el().size()), 1);
      s->undoRedo(true, nullptr);
      QCOMPARE(int(frame->el().size()), 2);
      QVERIFY(frame->el().front() == custom);
      QCOMPARE(custom->dot(0).front().fret, 2);
      s->undoRedo(false, nullptr);
      QCOMPARE(int(frame->el().size()), 1);
      delete s;
      }

void TestFretLegend::emptyAndContinuous()
      {
      auto* s = readScore("libmscore/fretlegend/legend460.mscx");
      QVERIFY(s);
      auto* frame = toFBox(s->first());
      for (auto* e : frame->el())
            e->setVisible(false);
      s->doLayout();
      QVERIFY(qIsFinite(frame->height()));
      QVERIFY(frame->height() > 0);
      s->setLayoutMode(LayoutMode::LINE);
      s->doLayout();
      QVERIFY(!frame->system());
      s->startCmd();
      s->undoRemoveElement(frame);
      s->endCmd();
      QVERIFY(s->first()->isMeasure());
      s->undoRedo(true, nullptr);
      QVERIFY(s->first() == frame);
      delete s;
      }

void TestFretLegend::chordDefinitionsWithoutStyle()
      {
      auto* s = readScore("libmscore/fretlegend/legend-demo.mscx");
      QVERIFY(s);
      auto* list = s->style().chordList();
      QVERIFY(list->loaded());
      QVERIFY(!list->renderListRoot.empty());
      list->unload();
      auto* h = new Harmony(s);
      h->setHarmony("Cm");
      QVERIFY(list->loaded());
      QVERIFY(!list->renderListRoot.empty());
      QCOMPARE(h->harmonyName(), QString("Cm"));
      delete h;
      delete s;
      }

void TestFretLegend::legendHarmonyAnchorsAndSave()
      {
      auto* s = readScore("libmscore/fretlegend/legend-demo.mscx");
      QVERIFY(s);
      QVERIFY(s->first()->isFBox());
      auto* frame = toFBox(s->first());
      QStringList names;
      for (Element* e : frame->el()) {
            if (!e->isFretDiagram())
                  continue;
            auto* h = toFretDiagram(e)->harmony();
            QVERIFY(h);
            QVERIFY(!h->getParentSeg());
            QVERIFY(!h->findNext());
            QVERIFY(!h->findPrev());
            QVERIFY(!h->isRealizable());
            QVERIFY(h->ticksTillNext(0).isZero());
            names.append(h->harmonyName());
            }
      QCOMPARE(names, QStringList({"C", "Cm", "C7"}));
      bool staffDiagramChecked = false;
      for (const Segment& segment : s->firstMeasure()->segments()) {
            for (Element* e : segment.annotations()) {
                  if (!e->isFretDiagram())
                        continue;
                  auto* h = toFretDiagram(e)->harmony();
                  QVERIFY(h);
                  QVERIFY(h->getParentSeg() == &segment);
                  staffDiagramChecked = true;
                  }
            }
      QVERIFY(staffDiagramChecked);
      {
            Harmony detached(s);
            QVERIFY(!detached.getParentSeg());
            }
      QVERIFY(saveScore(s, "fretlegend-save-anchors.mscx"));
      auto* copy = readCreatedScore("fretlegend-save-anchors.mscx");
      QVERIFY(copy);
      QCOMPARE(toFBox(copy->first())->getProperty(Pid::FRET_FRAME_DIAGRAMS_ORDER),
            frame->getProperty(Pid::FRET_FRAME_DIAGRAMS_ORDER));
      delete copy;
      delete s;
      }

void TestFretLegend::generatedChordsAndMarkerClearance()
      {
      auto* s = readScore("libmscore/fretlegend/legend-demo.mscx");
      QVERIFY(s);
      auto* frame = toFBox(s->first());
      {
            FBox defaults(s);
            QCOMPARE(defaults.getProperty(Pid::FRET_FRAME_CHORDS_PER_ROW).toInt(), 8);
            QCOMPARE(defaults.getProperty(Pid::FRET_FRAME_TEXT_SCALE).toDouble(), 1.0);
            }
      s->startCmd();
      // Exercise generation from standalone symbols, not relabeling of the
      // explicit diagrams already present in the demonstration score.
      QList<Element*> sourceDiagrams;
      for (Measure* m = s->firstMeasure(); m; m = m->nextMeasure()) {
            for (const Segment& segment : m->segments()) {
                  for (Element* e : segment.annotations())
                        if (e->isFretDiagram())
                              sourceDiagrams.append(e);
                  }
            }
      for (Element* e : sourceDiagrams)
            s->undoRemoveElement(e);
      Measure* measure = s->firstMeasure();
      for (const QString& name : QStringList({"G", "D"})) {
            QVERIFY(measure);
            auto* segment = measure->first(SegmentType::ChordRest);
            QVERIFY(segment);
            auto* h = new Harmony(s);
            h->setParent(segment);
            h->setTrack(0);
            h->setHarmony(name);
            s->undoAddElement(h);
            QVERIFY2(h->parent() == segment, "Generated-chord test symbol must remain a standalone segment annotation");
            QVERIFY(std::find(segment->annotations().begin(), segment->annotations().end(), h)
                  != segment->annotations().end());
            measure = measure->nextMeasure();
            }
      s->endCmd();
      s->doLayout();
      int generated = 0;
      for (Element* e : frame->el()) {
            if (!e->isFretDiagram())
                  continue;
            auto* fd = toFretDiagram(e);
            auto* h = fd->harmony();
            QVERIFY(h);
            QVERIFY(h->bbox().translated(h->pos()).bottom()
                  <= fd->bbox().top() - h->minDistance().val() * fd->spatium() + 0.01);
            const QString name = h->harmonyName();
            if (name != "G" && name != "D")
                  continue;
            ++generated;
            int dots = 0;
            int markers = 0;
            for (int string = 0; string < fd->strings(); ++string) {
                  for (const auto& dot : fd->dot(string))
                        dots += dot.exists() ? 1 : 0;
                  markers += fd->marker(string).exists() ? 1 : 0;
                  }
            // Voicings can use different numbers of X/O markers (barres
            // cover unmarked strings). Verify the musical chord instead of
            // assuming both templates use the same open-position shape.
            QVERIFY(dots > 0);
            QVERIFY(markers > 0);
            QCOMPARE(fd->strings(), 6);
            const int tuning[] = {40, 45, 50, 55, 59, 64};
            const int root = name == "G" ? 7 : 2;
            const QSet<int> expected {root, (root + 4) % 12, (root + 7) % 12};
            QSet<int> sounding;
            for (int string = 0; string < fd->strings(); ++string) {
                  const auto marker = fd->marker(string).mtype;
                  if (marker == FretMarkerType::CROSS)
                        continue;
                  int fret = marker == FretMarkerType::CIRCLE ? 0 : -1;
                  for (const auto& dot : fd->dot(string)) {
                        if (dot.exists())
                              fret = qMax(fret, dot.fret);
                        }
                  for (const auto& entry : fd->barres()) {
                        const auto& barre = entry.second;
                        if (barre.exists() && string >= barre.startString
                              && (barre.endString < 0 || string <= barre.endString))
                              fret = qMax(fret, entry.first);
                        }
                  QVERIFY2(fret >= 0, qPrintable(QString("%1 has an unspecified string %2").arg(name).arg(string)));
                  const int pitchClass = (tuning[string] + fret + (fret > 0 ? fd->fretOffset() : 0)) % 12;
                  QVERIFY2(expected.contains(pitchClass), qPrintable(QString("%1 string %2 has pitch class %3 (fret %4, offset %5)")
                        .arg(name).arg(string).arg(pitchClass).arg(fret).arg(fd->fretOffset())));
                  sounding.insert(pitchClass);
                  }
            QVERIFY(sounding == expected);
            }
      QCOMPARE(generated, 2);
      delete s;
      }

void TestFretLegend::frameNotationClearance()
      {
      auto* s = readScore("libmscore/fretlegend/legend-demo.mscx");
      QVERIFY(s);
      auto* frame = toFBox(s->first());
      {
            VBox vertical(s);
            TBox text(s);
            for (Box* box : QList<Box*>({frame, &vertical, &text})) {
                  QCOMPARE(box->paddingToNotationAbove().val(), 2.0);
                  QCOMPARE(box->paddingToNotationBelow().val(), 2.0);
                  }
            }
      {
            XmlReader xml(QByteArray("<VBox><paddingToNotationAbove>3.5</paddingToNotationAbove>"
                  "<paddingToNotationBelow>4.5</paddingToNotationBelow></VBox>"));
            QVERIFY(xml.readNextStartElement());
            VBox imported(s);
            imported.read(xml);
            QCOMPARE(imported.paddingToNotationAbove().val(), 3.5);
            QCOMPARE(imported.paddingToNotationBelow().val(), 4.5);
            }
      frame->setProperty(Pid::BOTTOM_GAP, 0.0);
      frame->setProperty(Pid::PADDING_TO_NOTATION_BELOW, QVariant::fromValue(Spatium(0.0)));
      s->doLayout();
      qreal before = frame->system()->minDistance(s->firstMeasure()->system());
      frame->setProperty(Pid::PADDING_TO_NOTATION_BELOW, QVariant::fromValue(Spatium(3.0)));
      frame->setPropertyFlags(Pid::PADDING_TO_NOTATION_BELOW, PropertyFlags::UNSTYLED);
      s->doLayout();
      qreal after = frame->system()->minDistance(s->firstMeasure()->system());
      QVERIFY(qAbs(after - before - 3.0 * frame->spatium()) < 0.01);
      // Above-clearance applies to a frame following notation, too.
      s->startCmd();
      s->insertMeasure(ElementType::VBOX, nullptr);
      s->endCmd();
      auto* following = toVBox(s->last());
      following->setProperty(Pid::TOP_GAP, 0.0);
      following->setProperty(Pid::PADDING_TO_NOTATION_ABOVE, QVariant::fromValue(Spatium(0.0)));
      s->doLayout();
      before = s->lastMeasure()->system()->minDistance(following->system());
      following->setProperty(Pid::PADDING_TO_NOTATION_ABOVE, QVariant::fromValue(Spatium(4.0)));
      following->setPropertyFlags(Pid::PADDING_TO_NOTATION_ABOVE, PropertyFlags::UNSTYLED);
      s->doLayout();
      after = s->lastMeasure()->system()->minDistance(following->system());
      QVERIFY(qAbs(after - before - 4.0 * following->spatium()) < 0.01);
      QVERIFY(saveScore(s, "fretlegend-notation-clearance.mscx"));
      auto* copy = readCreatedScore("fretlegend-notation-clearance.mscx");
      QVERIFY(copy);
      QCOMPARE(toFBox(copy->first())->paddingToNotationBelow().val(), 3.0);
      QCOMPARE(toVBox(copy->last())->paddingToNotationAbove().val(), 4.0);
      delete copy;
      delete s;
      }

void TestFretLegend::horizontalLegendRelayout()
      {
      auto* s = readScore("libmscore/fretlegend/legend-demo.mscx");
      QVERIFY(s);
      auto* frame = toFBox(s->first());
      FretDiagram* cm = nullptr;
      s->startCmd();
      for (Element* e : frame->el()) {
            if (!e->isFretDiagram())
                  continue;
            auto* fd = toFretDiagram(e);
            const bool keep = fd->harmony() && fd->harmony()->harmonyName() == "Cm";
            fd->undoChangeProperty(Pid::VISIBLE, keep);
            if (keep)
                  cm = fd;
            }
      s->endCmd();
      QVERIFY(cm);
      s->startCmd();
      cm->undoChangeProperty(Pid::ORIENTATION, int(Orientation::HORIZONTAL), PropertyFlags::UNSTYLED);
      s->endCmd();
      // Compare the immediate edit result with a fresh complete layout, not
      // merely the diagram's own changed bounding rectangle.
      const qreal immediateHeight = frame->height();
      const QPointF immediatePos = cm->pos();
      s->doLayout();
      QVERIFY(qAbs(frame->height() - immediateHeight) < 0.01);
      QVERIFY(QLineF(cm->pos(), immediatePos).length() < 0.01);
      QRectF contents = cm->bbox();
      contents |= cm->harmony()->bbox().translated(cm->harmony()->pos());
      const qreal contentBottom = contents.translated(cm->pos()).bottom();
      QVERIFY(qAbs(frame->height() - contentBottom - frame->bottomMargin() * DPMM) < 0.01);
      s->undoRedo(true, nullptr);
      const qreal undoHeight = frame->height();
      s->doLayout();
      QVERIFY(qAbs(frame->height() - undoHeight) < 0.01);
      s->undoRedo(false, nullptr);
      QVERIFY(qAbs(frame->height() - immediateHeight) < 0.01);
      QVERIFY(saveScore(s, "fretlegend-horizontal.mscx"));
      auto* copy = readCreatedScore("fretlegend-horizontal.mscx");
      QVERIFY(copy);
      QVERIFY(qAbs(toFBox(copy->first())->height() - immediateHeight) < 0.01);
      delete copy;
      delete s;
      }

// A mixed row must retain the same diagram-local label clearance as a
// horizontal diagram alone, while still aligning labels and enclosing cells.
void TestFretLegend::mixedOrientationLabelClearance()
      {
      auto* s = readScore("libmscore/fretlegend/legend-demo.mscx");
      QVERIFY(s);
      auto* frame = toFBox(s->first());
      frame->setProperty(Pid::FRET_FRAME_CHORDS_PER_ROW, 8);
      FretDiagram* cm = nullptr;
      FretDiagram* neighbor = nullptr;
      for (Element* e : frame->el()) {
            if (!e->isFretDiagram())
                  continue;
            auto* fd = toFretDiagram(e);
            if (fd->harmony() && fd->harmony()->harmonyName() == "Cm")
                  cm = fd;
            else if (fd->harmony() && fd->harmony()->harmonyName() == "C")
                  neighbor = fd;
            }
      QVERIFY(cm);
      QVERIFY(neighbor);
      cm->setProperty(Pid::ORIENTATION, int(Orientation::HORIZONTAL));
      cm->setPropertyFlags(Pid::ORIENTATION, PropertyFlags::UNSTYLED);
      neighbor->setProperty(Pid::ORIENTATION, int(Orientation::VERTICAL));
      s->doLayout();
      auto checkRow = [](FBox* f, FretDiagram* a, FretDiagram* b) {
            const qreal aBaseline = a->y() + a->harmony()->y();
            const qreal bBaseline = b->y() + b->harmony()->y();
            QVERIFY(qAbs(aBaseline - bBaseline) < 0.01);
            QVERIFY(a->y() < b->y());
            for (const auto* fd : {a, b}) {
                  QRectF contents = fd->bbox();
                  contents |= fd->harmony()->bbox().translated(fd->harmony()->pos());
                  contents.translate(fd->pos());
                  QVERIFY(contents.top() >= -0.01);
                  QVERIFY(contents.bottom() <= f->height() + 0.01);
                  }
            };
      checkRow(frame, cm, neighbor);
      const qreal mixedLabelY = cm->harmony()->y();
      for (Element* e : frame->el()) {
            if (e->isFretDiagram())
                  e->setVisible(e == cm);
            }
      s->doLayout();
      QVERIFY(qAbs(cm->harmony()->y() - mixedLabelY) < 0.01);
      for (Element* e : frame->el())
            e->setVisible(true);
      s->doLayout();
      checkRow(frame, cm, neighbor);
      QVERIFY(saveScore(s, "fretlegend-mixed-orientation.mscx"));
      auto* copy = readCreatedScore("fretlegend-mixed-orientation.mscx");
      QVERIFY(copy);
      auto* restored = toFBox(copy->first());
      FretDiagram* restoredCm = nullptr;
      FretDiagram* restoredNeighbor = nullptr;
      for (Element* e : restored->el()) {
            if (!e->isFretDiagram())
                  continue;
            auto* fd = toFretDiagram(e);
            if (fd->harmony() && fd->harmony()->harmonyName() == "Cm")
                  restoredCm = fd;
            else if (fd->harmony() && fd->harmony()->harmonyName() == "C")
                  restoredNeighbor = fd;
            }
      QVERIFY(restoredCm);
      QVERIFY(restoredNeighbor);
      checkRow(restored, restoredCm, restoredNeighbor);
      QVERIFY(qAbs(restoredCm->harmony()->y() - mixedLabelY) < 0.01);
      delete copy;
      delete s;
      }

void TestFretLegend::sourcePatternEdits()
      {
      auto* s = readScore("libmscore/fretlegend/legend-demo.mscx");
      QVERIFY(s);
      s->doLayout();
      auto* frame = toFBox(s->first());
      FretDiagram* source = nullptr;
      FretDiagram* legend = nullptr;
      for (Measure* m = s->firstMeasure(); m; m = m->nextMeasure()) {
            for (const Segment& seg : m->segments()) {
                  for (Element* e : seg.annotations()) {
                        if (e->isFretDiagram() && toFretDiagram(e)->harmony()
                              && toFretDiagram(e)->harmony()->harmonyName() == "Cm")
                              source = toFretDiagram(e);
                        }
                  }
            }
      for (Element* e : frame->el()) {
            if (e->isFretDiagram() && toFretDiagram(e)->harmony()
                  && toFretDiagram(e)->harmony()->harmonyName() == "Cm")
                  legend = toFretDiagram(e);
            }
      QVERIFY(source);
      QVERIFY(legend);
      legend->setProperty(Pid::ORIENTATION, int(Orientation::HORIZONTAL));
      legend->setPropertyFlags(Pid::ORIENTATION, PropertyFlags::UNSTYLED);
      // A source edit replaces local schematic customization; undo restores it.
      legend->setDot(0, 4, false, FretDotType::TRIANGLE);
      s->startCmd();
      source->undoSetFretDot(0, 2, false, FretDotType::SQUARE);
      s->endCmd();
      QVERIFY(legend->dot(0, 2).front().exists());
      QCOMPARE(int(legend->dot(0, 2).front().dtype), int(FretDotType::SQUARE));
      QCOMPARE(int(legend->getProperty(Pid::ORIENTATION).toInt()), int(Orientation::HORIZONTAL));
      s->undoRedo(true, nullptr);
      QVERIFY(legend->dot(0, 4).front().exists());
      QCOMPARE(int(legend->dot(0, 4).front().dtype), int(FretDotType::TRIANGLE));
      s->undoRedo(false, nullptr);
      QVERIFY(legend->dot(0, 2).front().exists());
      s->startCmd();
      source->undoSetFretMarker(0, FretMarkerType::CIRCLE);
      s->endCmd();
      QCOMPARE(int(legend->marker(0).mtype), int(FretMarkerType::CIRCLE));
      s->startCmd();
      source->undoSetFretBarre(0, 3, false);
      s->endCmd();
      QVERIFY(legend->barre(3).exists());
      QCOMPARE(legend->barre(3).startString, source->barre(3).startString);
      s->startCmd();
      source->undoChangeProperty(Pid::FRET_OFFSET, 4);
      source->undoChangeProperty(Pid::FRET_FRETS, 6, PropertyFlags::UNSTYLED);
      source->undoChangeProperty(Pid::FRET_STRINGS, 7, PropertyFlags::UNSTYLED);
      source->undoChangeProperty(Pid::FRET_NUT, false, PropertyFlags::UNSTYLED);
      s->endCmd();
      QCOMPARE(legend->fretOffset(), 4);
      QCOMPARE(legend->frets(), 6);
      QCOMPARE(legend->strings(), 7);
      QCOMPARE(int(legend->propertyFlags(Pid::FRET_STRINGS)), int(PropertyFlags::UNSTYLED));
      QVERIFY(!legend->showNut());
      s->undoRedo(true, nullptr);
      QCOMPARE(legend->strings(), source->strings());
      QCOMPARE(legend->fretOffset(), source->fretOffset());
      s->undoRedo(false, nullptr);
      QCOMPARE(legend->strings(), 7);
      s->startCmd();
      source->undoFretClear();
      s->endCmd();
      QVERIFY(legend->dots().empty());
      QVERIFY(legend->markers().empty());
      QVERIFY(legend->barres().empty());
      s->undoRedo(true, nullptr);
      QVERIFY(!legend->barres().empty());
      QVERIFY(saveScore(s, "fretlegend-pattern-edits.mscx"));
      auto* copy = readCreatedScore("fretlegend-pattern-edits.mscx");
      QVERIFY(copy);
      FretDiagram* restored = nullptr;
      for (Element* e : toFBox(copy->first())->el()) {
            if (e->isFretDiagram() && toFretDiagram(e)->harmony()
                  && toFretDiagram(e)->harmony()->harmonyName() == "Cm")
                  restored = toFretDiagram(e);
            }
      QVERIFY(restored);
      QCOMPARE(restored->strings(), 7);
      QVERIFY(restored->barre(3).exists());
      QCOMPARE(restored->getProperty(Pid::ORIENTATION).toInt(), int(Orientation::HORIZONTAL));
      delete copy;
      delete s;
      }

QTEST_MAIN(TestFretLegend)
#include "tst_fretlegend.moc"
