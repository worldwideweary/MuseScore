// SPDX-License-Identifier: GPL-2.0-or-later
// Integration tests: actual score parsing, repeat traversal, layout, and round-trip.
#include <QtTest/QtTest>
#include "mtest/testutils.h"
#include "libmscore/clef.h"
#include "libmscore/barline.h"
#include "libmscore/excerpt.h"
#include "libmscore/keysig.h"
#include "libmscore/measure.h"
#include "libmscore/page.h"
#include "libmscore/layoutbreak.h"
#include "libmscore/repeatlist.h"
#include "libmscore/score.h"
#include "libmscore/segment.h"
#include "libmscore/staff.h"
#include "libmscore/timesig.h"
#include "libmscore/undo.h"
#include "libmscore/xml.h"
#include "libmscore/system.h"
#include "libmscore/tempotext.h"
#include "libmscore/stafftext.h"
#include "libmscore/systemtext.h"

using namespace Ms;
#define DIR QString("libmscore/repeatcourtesy/")

class TestRepeatCourtesy : public QObject, public MTest {
      Q_OBJECT
      Element* at(Measure* m, SegmentType type, bool start = false) {
            Segment* s = m->findSegmentR(type, start ? Fraction(0,1) : m->ticks());
            return s && s->enabled() ? s->element(0) : nullptr;
            }
      Element* actualAtTick(Score* score, SegmentType type, const Fraction& tick) {
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  for (Segment& seg : m->segments()) {
                        if (seg.segmentType() != type || seg.tick() != tick) continue;
                        Element* e = seg.element(0);
                        if (e && !e->generated() && !e->isCourtesy()) return e;
                        }
                  }
            return nullptr;
            }
      int parens(Measure* m, bool continuation = false) {
            int n = 0;
            for (Segment& s : m->segments()) {
                  if (s.enabled() && s.isType(continuation ? SegmentType::ContinuationCourtesy : SegmentType::RepeatCourtesy))
                        n += int(s.annotations().size());
                  }
            return n;
            }
   private slots:
      void initTestCase() { initMTest(); }
      void endBarlinePreservesSystemHeaderSpacing() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "header-spacing.mscx")); QVERIFY(s);
            s->style().set(Sid::systemHeaderTimeSigDistance, Spatium(6.0));
            s->doLayout();
            Measure* m = s->firstMeasure(); QVERIFY(m && m->isFirstInSystem());
            Segment* chordRest = m->first(SegmentType::ChordRest); QVERIFY(chordRest);
            QVERIFY(!chordRest->next(SegmentType::ChordRest));
            m->computeMinWidth();
            const qreal expectedX = chordRest->x();
            QVERIFY(expectedX > 0.0);
            for (int pass = 0; pass < 3; ++pass) {
                  m->createEndBarLines(false);
                  QVERIFY(qAbs(chordRest->x() - expectedX) < 0.001);
                  }
            }
      void ordinaryClefPlacement_data() {
            QTest::addColumn<bool>("afterBarline");
            QTest::newRow("before-barline") << false;
            QTest::newRow("after-barline") << true;
            }
      void ordinaryClefPlacement() {
            QFETCH(bool, afterBarline);
            std::unique_ptr<MasterScore> s(readScore(DIR + "repeat.mscx")); QVERIFY(s);
            Measure* next = s->lastMeasure(); QVERIFY(next);
            Measure* previous = next->prevMeasure(); QVERIFY(previous);
            QVERIFY(!previous->repeatEnd());
            Measure* owner = afterBarline ? next : previous;
            const Fraction rtick = afterBarline ? Fraction(0,1) : previous->ticks();
            Segment* segment = owner->getSegmentR(SegmentType::Clef, rtick);
            auto clef = new Clef(s.get());
            clef->setTrack(0); clef->setClefType(ClefType::F); clef->setGenerated(false);
            segment->add(clef);
            const Fraction absoluteTick = clef->tick();
            for (int pass = 0; pass < 3; ++pass) {
                  s->doLayout();
                  QCOMPARE(clef->segment(), segment);
                  QCOMPARE(segment->measure(), owner);
                  QVERIFY(segment->rtick() == rtick);
                  QVERIFY(clef->tick() == absoluteTick);
                  QCOMPARE(segment->segmentType(), SegmentType::Clef);
                  }
            }
      void legacyClefSerialization() {
            std::unique_ptr<MasterScore> s(readScore("libmscore/compat206/clefs.mscx")); QVERIFY(s);
            QVERIFY(saveCompareScore(s.get(), "courtesy-legacy-clefs-test.mscx",
                                     "libmscore/compat206/clefs-ref.mscx"));
            }
      void legacyAnnounceSpacing_data() {
            QTest::addColumn<int>("kind");
            QTest::addColumn<int>("margin");
            QTest::newRow("key") << int(SegmentType::KeySigAnnounce) << int(Sid::keysigLeftMargin);
            QTest::newRow("meter") << int(SegmentType::TimeSigAnnounce) << int(Sid::timesigLeftMargin);
            }
      void legacyAnnounceSpacing() {
            QFETCH(int, kind); QFETCH(int, margin);
            std::unique_ptr<MasterScore> s(readScore(DIR + "repeat.mscx")); QVERIFY(s);
            s->style().set(Sid::timeSigPlacement, int(TimeSigPlacement::NORMAL));
            Measure* m = s->firstMeasure();
            Segment barline(m, SegmentType::EndBarLine, m->ticks());
            Segment announce(m, SegmentType(kind), m->ticks());
            const Sid sid = Sid(margin);
            const qreal value = s->styleV(sid).value<Spatium>().val();
            const qreal before = barline.minHorizontalDistance(&announce, false);
            s->style().set(sid, Spatium(value + 1.0));
            const qreal after = barline.minHorizontalDistance(&announce, false);
            QVERIFY(qAbs(after - before - s->spatium()) < 0.001);
            }
      void parentlessPaletteBarlines() {
            MasterScore score(MScore::baseStyle());
            QImage image(180, 180, QImage::Format_ARGB32_Premultiplied);
            for (int mode = 0; mode < 3; ++mode) {
                  score.style().set(Sid::timeSigPlacement, mode);
                  for (BarLineType type : {BarLineType::NORMAL, BarLineType::DOUBLE, BarLineType::START_REPEAT,
                                          BarLineType::END_REPEAT, BarLineType::END_START_REPEAT}) {
                        BarLine line(&score);
                        line.setBarLineType(type); line.layout();
                        QVERIFY(!line.parent()); QVERIFY(!line.measure());
                        image.fill(Qt::white);
                        QPainter painter(&image); painter.translate(20,20);
                        line.draw(&painter);
                        QVERIFY(!painter.hasClipping());
                        }
                  }
            }
      MasterScore* readStudio(const QString& name) {
            auto s = new MasterScore(MScore::baseStyle());
            if (s->loadMsc(root + "/" + DIR + name, true) != Score::FileError::FILE_NO_ERROR) { delete s; return nullptr; }
            s->doLayout();
            return s;
            }
      void timeSignatureDrawingStyleRoundTrip() {
            QByteArray xml("<Style><timeSigPlacement>acrossStaves</timeSigPlacement>"
                  "<timeSigCenterOnBarline>0</timeSigCenterOnBarline>"
                  "<timeSigVSMarginCentered>createSpace</timeSigVSMarginCentered>"
                  "<timeSigVSMarginNonCentered>hangIntoMargin</timeSigVSMarginNonCentered>"
                  "<timeSigCenterAcrossStaveGroup>1</timeSigCenterAcrossStaveGroup>");
            for (const char* mode : {"Normal", "Above", "Across"}) {
                  QByteArray prefix = QByteArray("timeSig") + mode;
                  xml += "<" + prefix + "Style>large</" + prefix + "Style>";
                  xml += "<" + prefix + "Scale w=\"1.25\" h=\"1.75\"/>";
                  xml += "<" + prefix + "ScaleLock>0</" + prefix + "ScaleLock>";
                  xml += "<" + prefix + "NumDist>0.375</" + prefix + "NumDist>";
                  xml += "<" + prefix + "Y>0.75</" + prefix + "Y>";
                  }
            xml += "</Style>";
            MStyle imported; XmlReader reader(xml); QVERIFY(reader.readNextStartElement());
            imported.load(reader, 460); QVERIFY(!reader.hasError());
            QCOMPARE(imported.value(Sid::timeSigPlacement).toInt(), int(TimeSigPlacement::ACROSS_STAVES));
            QCOMPARE(imported.value(Sid::timeSigVSMarginCentered).toInt(), int(TimeSigVSMargin::CREATE_SPACE));
            QCOMPARE(imported.value(Sid::timeSigVSMarginNonCentered).toInt(), int(TimeSigVSMargin::HANG_INTO_MARGIN));
            for (Sid sid : {Sid::timeSigNormalStyle, Sid::timeSigAboveStyle, Sid::timeSigAcrossStyle})
                  QCOMPARE(imported.value(sid).toInt(), int(TimeSigStyle::LARGE));
            QCOMPARE(imported.value(Sid::timeSigNormalScale).toSizeF(), QSizeF(1.25, 1.75));
            QCOMPARE(imported.value(Sid::timeSigAcrossNumDist).value<Spatium>().val(), 0.375);
            std::unique_ptr<MasterScore> score(readScore(DIR + "repeat.mscx")); QVERIFY(score);
            for (int i = int(Sid::timeSigPlacement); i <= int(Sid::timeSigAcrossY); ++i)
                  score->style().set(Sid(i), imported.value(Sid(i)));
            QTemporaryDir tmp; const QString path = tmp.filePath("drawing-styles.mscx");
            QVERIFY(saveScore(score.get(), path));
            if (qEnvironmentVariableIsSet("TIMESIG_DRAWING_EXPORT"))
                  QVERIFY(saveScore(score.get(), QString::fromLocal8Bit(qgetenv("TIMESIG_DRAWING_EXPORT"))));
            QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly));
            const QByteArray saved = file.readAll();
            QVERIFY(saved.contains("<timeSigPlacement>acrossStaves</timeSigPlacement>"));
            QVERIFY(saved.contains("<timeSigNormalStyle>large</timeSigNormalStyle>"));
            std::unique_ptr<MasterScore> copy(readCreatedScore(path)); QVERIFY(copy);
            for (int i = int(Sid::timeSigPlacement); i <= int(Sid::timeSigAcrossY); ++i) {
                  Sid sid = Sid(i);
                  if (QString::fromLatin1(MStyle::valueType(sid)) == "Ms::Spatium")
                        QCOMPARE(copy->styleV(sid).value<Spatium>().val(), imported.value(sid).value<Spatium>().val());
                  else
                        QCOMPARE(copy->styleV(sid), imported.value(sid));
                  }
            QByteArray bad("<Style><timeSigPlacement>bogus</timeSigPlacement>"
                           "<timeSigAcrossStyle>99</timeSigAcrossStyle></Style>");
            XmlReader badReader(bad); QVERIFY(badReader.readNextStartElement());
            MStyle defaults; defaults.load(badReader, 460);
            QCOMPARE(defaults.value(Sid::timeSigPlacement).toInt(), int(TimeSigPlacement::NORMAL));
            QCOMPARE(defaults.value(Sid::timeSigAcrossStyle).toInt(), int(TimeSigStyle::NARROW));
            }
      void timeSignatureDrawingGeometry() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "repeat.mscx")); QVERIFY(s);
            TimeSig* ts = toTimeSig(at(s->firstMeasure(), SegmentType::TimeSig, true)); QVERIFY(ts);
            ts->setSig(Fraction(4,4)); ts->layout();
            const QRectF normal = ts->bbox(); QVERIFY(normal.width() > 0);
            s->style().set(Sid::timeSigNormalStyle, int(TimeSigStyle::NARROW)); ts->layout();
            QCOMPARE(ts->drawingSymbol(SymId::timeSig4), SymId::timeSig4Narrow);
            const QRectF narrow = ts->bbox(); QVERIFY(narrow.width() > 0);
            QVERIFY(narrow.width() < normal.width());
            s->style().set(Sid::timeSigNormalStyle, int(TimeSigStyle::LARGE)); ts->layout();
            QCOMPARE(ts->drawingSymbol(SymId::timeSig4), SymId::timeSig4Large);
            QVERIFY(ts->bbox().width() > 0);
            s->style().set(Sid::timeSigNormalStyle, int(TimeSigStyle::NORMAL));
            s->style().set(Sid::timeSigNormalScale, QSizeF(1.5,2.0)); ts->layout();
            QCOMPARE(s->styleV(Sid::timesigScale).toSizeF(), QSizeF(1.5,2.0));
            QVERIFY(qAbs(ts->bbox().width() - 1.5 * normal.width()) < 0.001);
            QVERIFY(qAbs(ts->bbox().height() - 2.0 * normal.height()) < 0.001);
            s->style().set(Sid::timesigScale, QSizeF(1.0,1.0)); ts->layout();
            QCOMPARE(s->styleV(Sid::timeSigNormalScale).toSizeF(), QSizeF(1.0,1.0));
            s->style().set(Sid::timeSigNormalNumDist, Spatium(0.5)); ts->layout();
            QVERIFY(qAbs(ts->bbox().height() - normal.height() - 0.5 * ts->spatium()) < 0.001);
            s->style().set(Sid::timeSigNormalY, Spatium(0.75)); ts->layout();
            QVERIFY(qAbs(ts->pos().y() - 0.75 * ts->spatium()) < 0.001);
            // Common-time glyph metrics and painting use the same X/Y scale.
            ts->setSig(Fraction(4,4), TimeSigType::FOUR_FOUR); ts->layout();
            const QRectF common = ts->bbox();
            s->style().set(Sid::timeSigNormalScale, QSizeF(1.5,2.0)); ts->layout();
            QVERIFY(qAbs(ts->bbox().width() - 1.5 * common.width()) < 0.001);
            QVERIFY(qAbs(ts->bbox().height() - 2.0 * common.height()) < 0.001);
            // An explicit per-element scale continues to override the style.
            ts->setPropertyFlags(Pid::SCALE, PropertyFlags::UNSTYLED);
            ts->setScale(QSizeF(0.8,0.9));
            QCOMPARE(ts->drawingScale(), QSizeF(0.8,0.9));
            }
      void timeSignatureLargePlacementAndGroups() {
            std::unique_ptr<MasterScore> s(readStudio("studio-changes-inside-repeats.mscx")); QVERIFY(s);
            QCOMPARE(s->nstaves(), 3);
            s->staff(2)->setTimeSigSystemObject(true);
            s->style().set(Sid::timeSigPlacement, int(TimeSigPlacement::ABOVE_STAVES)); s->doLayout();
            Measure* first = s->firstMeasure(); QVERIFY(first->system());
            Segment* seg = first->findSegmentR(SegmentType::TimeSig, Fraction(0,1)); QVERIFY(seg);
            TimeSig* top = toTimeSig(seg->element(0));
            TimeSig* middle = toTimeSig(seg->element(VOICES));
            TimeSig* bottom = toTimeSig(seg->element(2 * VOICES));
            QVERIFY(top && middle && bottom);
            QVERIFY(!top->bbox().isEmpty()); QVERIFY(middle->bbox().isEmpty()); QVERIFY(!bottom->bbox().isEmpty());
            QVERIFY(qAbs(top->bbox().bottom() + top->pos().y() - top->drawingYOffset()) < 0.001);
            System* sys = first->system();
            sys->staff(0)->setShow(false);
            top->layout(); middle->layout();
            QVERIFY(top->bbox().isEmpty()); QVERIFY(!middle->bbox().isEmpty());
            // The implicit top display moves to the first visible staff.
            QVERIFY(middle->showOnThisStaff());
            sys->staff(0)->setShow(true);
            s->style().set(Sid::timeSigPlacement, int(TimeSigPlacement::ACROSS_STAVES));
            s->style().set(Sid::timeSigCenterAcrossStaveGroup, true); s->doLayout();
            sys = first->system(); top->layout(); middle->layout(); bottom->layout();
            const qreal staffY = sys->staff(1)->y();
            s->doLayout(); sys = first->system();
            QVERIFY(qAbs(sys->staff(1)->y() - staffY) < 0.001);
            QCOMPARE(top->lastStaffInDrawingGroup(), 1);
            QCOMPARE(bottom->lastStaffInDrawingGroup(), 2);
            const qreal center = (sys->staff(1)->y() + s->staff(1)->height()
                                - sys->staff(0)->y() - s->staff(0)->height()) * 0.5;
            QVERIFY(qAbs(top->pos().y() - center) < 0.001);
            QVERIFY(qAbs(bottom->pos().y()) < 0.001);
            Score* part = new Score(s.get());
            auto ex = new Excerpt(s.get()); ex->setPartScore(part); ex->setParts(s->parts());
            ex->setTitle("Drawing display groups"); s->excerpts().append(ex);
            s->startCmd(); Excerpt::createExcerpt(ex); s->endCmd();
            QCOMPARE(part->nstaves(), 3); QVERIFY(part->staff(2)->timeSigSystemObject());
            QCOMPARE(part->styleI(Sid::timeSigPlacement), int(TimeSigPlacement::ACROSS_STAVES));
            QTemporaryDir tmp; const QString path = tmp.filePath("drawing-system-staves.mscx");
            QVERIFY(saveScore(s.get(), path));
            QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly));
            const QByteArray saved = file.readAll(); QVERIFY(saved.contains("<SystemObjects>"));
            QVERIFY(saved.contains("staffId=\"3\""));
            if (qEnvironmentVariableIsSet("TIMESIG_GROUP_EXPORT"))
                  QVERIFY(saveScore(s.get(), QString::fromLocal8Bit(qgetenv("TIMESIG_GROUP_EXPORT"))));
            std::unique_ptr<MasterScore> copy(readCreatedScore(path)); QVERIFY(copy);
            QVERIFY(copy->staff(2)->timeSigSystemObject()); QVERIFY(!copy->staff(1)->timeSigSystemObject());
            QCOMPARE(copy->styleI(Sid::timeSigPlacement), int(TimeSigPlacement::ACROSS_STAVES));
            QCOMPARE(copy->excerpts().size(), 1);
            Score* loadedPart = copy->excerpts().front()->partScore(); QVERIFY(loadedPart);
            QVERIFY(loadedPart->staff(2)->timeSigSystemObject());
            QCOMPARE(loadedPart->styleI(Sid::timeSigPlacement), int(TimeSigPlacement::ACROSS_STAVES));
            }
      void timeSignatureAboveMargins_data() {
            QTest::addColumn<bool>("center"); QTest::addColumn<int>("margin");
            QTest::newRow("center-hang") << true << int(TimeSigVSMargin::HANG_INTO_MARGIN);
            QTest::newRow("center-inset-signature") << true << int(TimeSigVSMargin::RIGHT_ALIGN_TO_BARLINE);
            QTest::newRow("center-inset-barline") << true << int(TimeSigVSMargin::CREATE_SPACE);
            QTest::newRow("left-inset-signature") << false << int(TimeSigVSMargin::RIGHT_ALIGN_TO_BARLINE);
            QTest::newRow("left-inset-barline") << false << int(TimeSigVSMargin::CREATE_SPACE);
            }
      void timeSignatureDrawingLineCutouts_data() {
            QTest::addColumn<int>("placement");
            QTest::newRow("above-barline") << int(TimeSigPlacement::ABOVE_STAVES);
            QTest::newRow("across-staff-lines") << int(TimeSigPlacement::ACROSS_STAVES);
            }
      void timeSignatureDrawingLineCutouts() {
            QFETCH(int, placement);
            std::unique_ptr<MasterScore> s(readStudio("studio-changes-inside-repeats.mscx")); QVERIFY(s);
            s->style().set(Sid::timeSigPlacement, placement);
            if (placement == int(TimeSigPlacement::ABOVE_STAVES)) s->staff(2)->setTimeSigSystemObject(true);
            else s->style().set(Sid::timeSigCenterAcrossStaveGroup, true);
            s->doLayout();
            Segment* seg = s->firstMeasure()->findSegmentR(SegmentType::TimeSig, Fraction(0,1)); QVERIFY(seg);
            // The internal above-staff change intersects the spanning repeat barline;
            // an across-staff meter intersects the middle staff's horizontal lines.
            if (placement == int(TimeSigPlacement::ABOVE_STAVES)) {
                  Element* change = actualAtTick(s.get(), SegmentType::TimeSig, s->firstMeasure()->ticks());
                  QVERIFY(change);
                  seg = toTimeSig(change)->segment();
                  }
            TimeSig* meter = toTimeSig(seg->element(placement == int(TimeSigPlacement::ABOVE_STAVES) ? 2 * VOICES : 0));
            QVERIFY(meter && !meter->bbox().isEmpty());
            const QRectF box = meter->bbox().translated(meter->pagePos());
            const QColor paper(105, 170, 215);
            auto pixelsUnderMeter = [&]() {
                  QImage image(240, 240, QImage::Format_ARGB32_Premultiplied); image.fill(paper);
                  QPainter painter(&image);
                  const qreal scale = 200.0 / qMax(box.width(), box.height());
                  painter.translate(120, 120); painter.scale(scale, scale); painter.translate(-box.center());
                  const QTransform transform = painter.transform();
                  for (Element* e : s->pages().front()->elements()) {
                        if (!e->visible() || (!e->isBarLine() && e->type() != ElementType::STAFF_LINES)) continue;
                        painter.save(); painter.translate(e->pagePos()); e->draw(&painter); painter.restore();
                        }
                  painter.end();
                  const QRect inside = transform.mapRect(box).adjusted(2,2,-2,-2).toAlignedRect().intersected(image.rect());
                  int painted = 0;
                  for (int y = inside.top(); y <= inside.bottom(); ++y)
                        for (int x = inside.left(); x <= inside.right(); ++x)
                              painted += image.pixelColor(x,y) != paper;
                  return painted;
                  };
            const int drawn = pixelsUnderMeter();
            if (placement == int(TimeSigPlacement::ABOVE_STAVES)) QCOMPARE(drawn, 0);
            else QVERIFY(drawn > 0); // horizontal staff lines are never erased
            // Disable only the mask policy, retaining identical line geometry.
            s->style().set(Sid::timeSigPlacement, int(TimeSigPlacement::NORMAL));
            QVERIFY(pixelsUnderMeter() > 0);
            }
      void timeSignatureAboveMargins() {
            QFETCH(bool, center); QFETCH(int, margin);
            std::unique_ptr<MasterScore> s(readScore(DIR + "matching.mscx")); QVERIFY(s);
            s->style().set(Sid::timeSigPlacement, int(TimeSigPlacement::ABOVE_STAVES));
            s->style().set(Sid::timeSigCenterOnBarline, center);
            s->style().set(Sid::genCourtesyKeysig, false); // isolate the meter's margin policy
            s->style().set(center ? Sid::timeSigVSMarginCentered : Sid::timeSigVSMarginNonCentered, margin);
            Measure* end = s->firstMeasure()->nextMeasure()->nextMeasure(); end->setRepeatEnd(false);
            auto br = new LayoutBreak(s.get()); br->setLayoutBreakType(LayoutBreak::LINE); br->setParent(end); end->add(br);
            s->doLayout(); QVERIFY(end->system() != end->nextMeasure()->system());
            Segment* ann = end->findSegmentR(SegmentType::TimeSigAnnounce, end->ticks()); QVERIFY(ann && ann->enabled());
            TimeSig* ts = toTimeSig(ann->element(0)); QVERIFY(ts && !ts->bbox().isEmpty());
            Segment* bar = end->findSegmentR(SegmentType::EndBarLine, end->ticks()); QVERIFY(bar);
            const qreal anchor = end->pagePos().x() + bar->x() + bar->minRight() - 0.5 * s->styleP(Sid::barWidth);
            const QRectF box = ts->bbox().translated(ts->pagePos());
            const qreal right = end->system()->pagePos().x() + end->system()->width();
            if (margin == int(TimeSigVSMargin::RIGHT_ALIGN_TO_BARLINE)) {
                  QVERIFY(qAbs(box.right() - anchor) < 0.01);
                  QVERIFY(box.right() <= right + 0.1 * ts->spatium());
                  }
            else if (center) QVERIFY(qAbs(box.center().x() - anchor) < 0.01);
            else QVERIFY(qAbs(box.left() - anchor) < 0.01);
            if (margin == int(TimeSigVSMargin::HANG_INTO_MARGIN)) QVERIFY(box.right() > right);
            if (margin == int(TimeSigVSMargin::CREATE_SPACE)) {
                  QVERIFY(box.right() <= right + 0.1 * ts->spatium());
                  QVERIFY(anchor < right - 0.5 * ts->spatium());
                  }
            const QRectF firstBox = box;
            s->doLayout();
            QVERIFY(qAbs(ts->pagePos().x() + ts->bbox().left() - firstBox.left()) < 0.01);
            }
      void timeSignatureAboveClearsTempo() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "repeat.mscx")); QVERIFY(s);
            Segment* beat = s->firstMeasure()->first(SegmentType::ChordRest); QVERIFY(beat);
            auto tempo = new TempoText(s.get()); tempo->setXmlText("Allegro"); tempo->setTempo(2.0);
            tempo->setTrack(0); tempo->setParent(beat); beat->add(tempo);
            s->style().set(Sid::timeSigPlacement, int(TimeSigPlacement::ABOVE_STAVES)); s->doLayout();
            TimeSig* ts = toTimeSig(at(s->firstMeasure(), SegmentType::TimeSig, true)); QVERIFY(ts);
            const QRectF meter = ts->bbox().translated(ts->pagePos());
            const QRectF text = tempo->bbox().translated(tempo->pagePos());
            QVERIFY(!meter.intersects(text));
            const qreal y = ts->pagePos().y(); s->doLayout();
            QVERIFY(qAbs(ts->pagePos().y() - y) < 0.01);
            }
      void aboveMeterReservesSpaceBeforeText_data() {
            QTest::addColumn<bool>("systemText");
            QTest::newRow("staff-text") << false;
            QTest::newRow("system-text") << true;
            }
      void aboveMeterReservesSpaceBeforeText() {
            QFETCH(bool, systemText);
            std::unique_ptr<MasterScore> s(readScore(DIR + "repeat.mscx")); QVERIFY(s);
            s->style().set(Sid::timeSigPlacement, int(TimeSigPlacement::ABOVE_STAVES)); s->doLayout();
            TimeSig* meter = toTimeSig(at(s->firstMeasure(), SegmentType::TimeSig, true)); QVERIFY(meter);
            Segment* beat = s->firstMeasure()->first(SegmentType::ChordRest); QVERIFY(beat);
            TextBase* text = systemText ? static_cast<TextBase*>(new SystemText(s.get()))
                                        : static_cast<TextBase*>(new StaffText(s.get()));
            text->setXmlText("Text must remain above the large meter");
            text->setTrack(0); text->setParent(beat);
            text->setOffset(QPointF(meter->pagePos().x() - beat->pagePos().x(), 0));
            text->setPropertyFlags(Pid::OFFSET, PropertyFlags::UNSTYLED);
            beat->add(text);
            QRectF previous;
            for (int pass = 0; pass < 4; ++pass) {
                  s->doLayout();
                  QRectF meterBox = meter->pageBoundingRect();
                  QRectF textBox = text->pageBoundingRect();
                  QVERIFY(textBox.left() < meterBox.right() && textBox.right() > meterBox.left());
                  QVERIFY2(textBox.bottom() < meterBox.top(), "Text must autoplace above the meter, not underneath it");
                  if (pass) QVERIFY(qAbs(textBox.y() - previous.y()) < 0.01);
                  previous = textBox;
                  }
            }
      void timeSignatureAboveRepeatAtSystemBreak_data() {
            QTest::addColumn<int>("margin");
            QTest::newRow("hang") << int(TimeSigVSMargin::HANG_INTO_MARGIN);
            QTest::newRow("inset-signature") << int(TimeSigVSMargin::RIGHT_ALIGN_TO_BARLINE);
            QTest::newRow("inset-barline") << int(TimeSigVSMargin::CREATE_SPACE);
            }
      void timeSignatureAboveRepeatAtSystemBreak() {
            QFETCH(int, margin);
            std::unique_ptr<MasterScore> s(readScore(DIR + "repeat.mscx")); QVERIFY(s);
            s->style().set(Sid::timeSigPlacement, int(TimeSigPlacement::ABOVE_STAVES));
            s->style().set(Sid::timeSigVSMarginCentered, margin);
            Measure* end = s->firstMeasure()->nextMeasure()->nextMeasure();
            auto meter = new TimeSig(s.get()); meter->setSig(Fraction(4,4)); meter->setTrack(0);
            s->startCmd(); s->cmdAddTimeSig(end->nextMeasure(), 0, meter, false); s->endCmd();
            auto br = new LayoutBreak(s.get()); br->setLayoutBreakType(LayoutBreak::LINE); br->setParent(end); end->add(br);
            s->doLayout();
            Segment* ann = end->findSegmentR(SegmentType::TimeSigAnnounce, end->ticks());
            Segment* rep = end->findSegmentR(SegmentType::TimeSigRepeatAnnounce, end->ticks());
            QVERIFY(ann && ann->enabled()); QVERIFY(rep && rep->enabled());
            TimeSig* next = toTimeSig(ann->element(0)); TimeSig* repeat = toTimeSig(rep->element(0));
            QVERIFY(next && repeat); QVERIFY(!next->sig().identical(repeat->sig()));
            QVERIFY(!next->bbox().translated(next->pagePos()).intersects(repeat->bbox().translated(repeat->pagePos())));
            QVERIFY(parens(end) >= 4); // separate meter and clef/key pairs
            }
      void destinationAndCancellation() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "repeat.mscx"));
            QVERIFY(s);
            Measure* end = s->firstMeasure()->nextMeasure()->nextMeasure();
            Measure* cont = end->nextMeasure();
            auto c = toClef(at(end, SegmentType::ClefRepeatAnnounce));
            auto k = toKeySig(at(end, SegmentType::KeySigRepeatAnnounce));
            auto t = toTimeSig(at(end, SegmentType::TimeSigRepeatAnnounce));
            QVERIFY(c && k && t);
            QCOMPARE(c->clefType(), ClefType::G);
            QCOMPARE(k->key(), Key::F);
            QVERIFY(t->sig().identical(Fraction(3,4)));
            QCOMPARE(parens(end), 2); // one pair surrounds all three signatures
            QVERIFY(at(cont, SegmentType::ClefStartRepeatAnnounce, true));
            QVERIFY(at(cont, SegmentType::KeySigStartRepeatAnnounce, true));
            QVERIFY(at(cont, SegmentType::TimeSigStartRepeatAnnounce, true));
            QCOMPARE(parens(cont, true), 2);
            // Generated courtesies must not change any musical map.
            QCOMPARE(s->staff(0)->clef(end->tick()), ClefType::F);
            QCOMPARE(s->staff(0)->key(end->tick()), Key::G);
            QVERIFY(s->staff(0)->timeSig(end->tick())->sig().identical(Fraction(2,4)));
            s->setExpandRepeats(false);
            s->doLayout();
            QVERIFY(at(end, SegmentType::TimeSigRepeatAnnounce));
            s->style().set(Sid::useParensRepeatCourtesies, false);
            s->doLayout();
            QCOMPARE(parens(end), 0);
            s->style().set(Sid::showCourtesiesRepeats, false);
            s->doLayout();
            QVERIFY(!at(end, SegmentType::TimeSigRepeatAnnounce));
            QVERIFY(!at(cont, SegmentType::TimeSigStartRepeatAnnounce, true));
            }
      void matchingChangesAndRoundTrip() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "matching.mscx"));
            QVERIFY(s);
            Measure* end = s->firstMeasure()->nextMeasure()->nextMeasure();
            Measure* cont = end->nextMeasure();
            s->startCmd();
            s->undo(new ChangeStyleVal(s.get(), Sid::changesBeforeBarlineRepeats, true));
            s->doLayout();
            s->endCmd();
            QVERIFY(at(end, SegmentType::TimeSig));
            QVERIFY(at(end, SegmentType::KeySig));
            QVERIFY(at(end, SegmentType::Clef));
            QVERIFY(!at(end, SegmentType::TimeSigRepeatAnnounce));
            QVERIFY(!at(cont, SegmentType::TimeSig, true));
            QTemporaryDir tmp;
            QString path = tmp.filePath("courtesy-roundtrip.mscx");
            QVERIFY(saveScore(s.get(), path));
            if (qEnvironmentVariableIsSet("COURTESY_EXPORT"))
                  QVERIFY(saveScore(s.get(), QString::fromLocal8Bit(qgetenv("COURTESY_EXPORT"))));
            std::unique_ptr<MasterScore> copy(readCreatedScore(path));
            QVERIFY(copy);
            Measure* loadedEnd = copy->firstMeasure()->nextMeasure()->nextMeasure();
            QVERIFY(loadedEnd->ticks().identical(Fraction(2,4)));
            QVERIFY(loadedEnd->nextMeasure()->ticks().identical(Fraction(3,4)));
            QVERIFY(at(loadedEnd, SegmentType::TimeSig));
            QVERIFY(copy->styleB(Sid::changesBeforeBarlineRepeats));
            s->startCmd();
            s->undo(new ChangeStyleVal(s.get(), Sid::changesBeforeBarlineRepeats, false));
            s->doLayout();
            s->endCmd();
            QVERIFY(at(cont, SegmentType::TimeSig, true));
            QVERIFY(at(end, SegmentType::TimeSigRepeatAnnounce));
            }
      void otherJumps() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "jump.mscx"));
            QVERIFY(s);
            Measure* end = s->firstMeasure()->nextMeasure()->nextMeasure();
            QVERIFY(at(end, SegmentType::ClefRepeatAnnounce));
            QVERIFY(at(end, SegmentType::KeySigRepeatAnnounce));
            QVERIFY(at(end, SegmentType::TimeSigRepeatAnnounce));
            s->style().set(Sid::showCourtesiesOtherJumps, false);
            s->doLayout();
            QVERIFY(!at(end, SegmentType::TimeSigRepeatAnnounce));
            }
      void multiMeasureRestsAndRelayout() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "repeat.mscx"));
            QVERIFY(s);
            s->style().set(Sid::createMultiMeasureRests, true);
            s->style().set(Sid::minEmptyMeasures, 2);
            for (int i = 0; i < 3; ++i) {
                  s->doLayout();
                  bool foundRepeat = false;
                  for (Measure* m = s->firstMeasureMM(); m; m = m->nextMeasureMM()) {
                        if (m->repeatEnd()) {
                              foundRepeat = true;
                              QVERIFY(m->isMMRest());
                              QVERIFY(at(m, SegmentType::TimeSigRepeatAnnounce));
                              }
                        }
                  QVERIFY(foundRepeat);
                  }
            }
      void modernDestinations() {
            std::unique_ptr<MasterScore> s(readStudio("studio-courtesy-changes.mscx"));
            QVERIFY(s);
            for (Sid sid : {Sid::showCourtesiesRepeats, Sid::showCourtesiesOtherJumps,
                             Sid::showCourtesiesAfterCancellingRepeats, Sid::showCourtesiesAfterCancellingOtherJumps})
                  QVERIFY(s->styleB(sid));
            auto measure = [&](int index) { Measure* m = s->firstMeasure(); while (index-- && m) m = m->nextMeasure(); return m; };
            for (int index : {2, 4, 5}) {
                  Measure* m = measure(index); QVERIFY(m);
                  QVERIFY(at(m, SegmentType::KeySigRepeatAnnounce));
                  QVERIFY(at(m, SegmentType::TimeSigRepeatAnnounce));
                  }
            for (int index : {3, 5, 6}) {
                  Measure* m = measure(index); QVERIFY(m);
                  QVERIFY(at(m, SegmentType::TimeSigStartRepeatAnnounce, true));
                  }
            }
      void combinedRepeatsAndClefOverride() {
            std::unique_ptr<MasterScore> s(readStudio("studio-changes-inside-repeats.mscx"));
            QVERIFY(s); QCOMPARE(s->nstaves(), 3);
            Measure* end = s->firstMeasure();
            Measure* next = end->nextMeasure(); QVERIFY(next);
            auto ordered = [&](bool split) {
                  int last = -1;
                  const SegmentType order[] = {split ? SegmentType::Clef : SegmentType::StartRepeatBarLine,
                        split ? SegmentType::KeySig : SegmentType::Clef,
                        split ? SegmentType::TimeSig : SegmentType::KeySig,
                        split ? SegmentType::StartRepeatBarLine : SegmentType::TimeSig};
                  for (SegmentType t : order) {
                        int idx = 0; bool found = false;
                        for (Segment* seg = next->first(); seg; seg = seg->next(), ++idx) {
                              if (seg->segmentType() == t && seg->enabled() && seg->rtick().isZero()) {
                                    if (idx <= last) return false;
                                    last = idx; found = true; break;
                                    }
                              }
                        if (!found) return false;
                        }
                  return true;
                  };
            QVERIFY(ordered(true));
            s->style().set(Sid::changesBetweenEndStartRepeat, false); s->doLayout();
            QVERIFY(ordered(false));
            s->style().set(Sid::placeClefsBeforeRepeats, true); s->doLayout();
            Segment* clefSeg = end->findSegmentR(SegmentType::Clef, end->ticks());
            QVERIFY(clefSeg && clefSeg->element(VOICES));
            auto clef = toClef(clefSeg->element(VOICES));
            clef->setProperty(Pid::CLEF_TO_BARLINE_POS, 2); s->doLayout();
            clefSeg = next->findSegmentR(SegmentType::Clef, Fraction(0,1));
            QVERIFY(clefSeg && clefSeg->element(VOICES));
            }
      void meterEditingBeforeRepeat() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "matching.mscx")); QVERIFY(s);
            s->style().set(Sid::changesBeforeBarlineRepeats, true); s->doLayout();
            Measure* end = s->firstMeasure()->nextMeasure()->nextMeasure();
            TimeSig* ts = toTimeSig(at(end, SegmentType::TimeSig)); QVERIFY(ts);
            Fraction boundary = ts->tick();
            QCOMPARE(ts->changeMeasure()->tick(), boundary);
            s->startCmd(); s->cmdRemoveTimeSig(ts); s->endCmd(); s->doLayout();
            QVERIFY(s->tick2measure(boundary)->timesig().identical(Fraction(2,4)));
            s->undoStack()->undo(&ed); s->doLayout();
            QVERIFY(s->tick2measure(boundary)->timesig().identical(Fraction(3,4)));
            }
      void courtesyOptOutAndBreaks() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "repeat.mscx")); QVERIFY(s);
            Measure* end = s->firstMeasure()->nextMeasure()->nextMeasure();
            auto ts = s->staff(0)->timeSig(Fraction(0,1)); QVERIFY(ts);
            ts->setShowCourtesySig(false); s->doLayout();
            QVERIFY(!at(end, SegmentType::TimeSigRepeatAnnounce));
            QVERIFY(at(end, SegmentType::KeySigRepeatAnnounce));
            ts->setShowCourtesySig(true);
            end->setLineBreak(true); s->doLayout();
            QVERIFY(at(end, SegmentType::TimeSigRepeatAnnounce));
            s->setLayoutMode(LayoutMode::LINE); s->doLayout();
            QVERIFY(at(end, SegmentType::TimeSigRepeatAnnounce));
            s->style().set(Sid::smallParens, false); s->doLayout();
            QCOMPARE(parens(end), 2);
            }
      void matchingChangesOnMultiMeasureRest() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "matching.mscx")); QVERIFY(s);
            s->style().set(Sid::changesBeforeBarlineRepeats, true);
            s->style().set(Sid::createMultiMeasureRests, true);
            s->style().set(Sid::minEmptyMeasures, 2); s->doLayout();
            Measure* end = s->firstMeasureMM();
            while (end && !end->repeatEnd()) end = end->nextMeasureMM();
            QVERIFY(end);
            QVERIFY(at(end, SegmentType::TimeSig));
            QVERIFY(at(end, SegmentType::KeySig));
            s->style().set(Sid::changesBeforeBarlineRepeats, false); s->doLayout();
            end = s->firstMeasureMM(); while (end && !end->repeatEnd()) end = end->nextMeasureMM();
            QVERIFY(end);
            QVERIFY(!at(end, SegmentType::TimeSig));
            QVERIFY(at(end, SegmentType::TimeSigRepeatAnnounce));
            }
      void insertionAtMovedSignature() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "matching.mscx")); QVERIFY(s);
            s->style().set(Sid::changesBeforeBarlineRepeats, true); s->doLayout();
            Measure* end = s->firstMeasure()->nextMeasure()->nextMeasure();
            Measure* originalNext = end->nextMeasure();
            s->startCmd(); s->insertMeasure(ElementType::MEASURE, originalNext); s->endCmd(); s->doLayout();
            QVERIFY(end->nextMeasure() != originalNext);
            QVERIFY(originalNext->timesig().identical(Fraction(3,4)));
            QVERIFY(s->staff(0)->timeSig(originalNext->tick())->sig().identical(Fraction(3,4)));
            QTemporaryDir tmp; QString path = tmp.filePath("insert-roundtrip.mscx");
            QVERIFY(saveScore(s.get(), path));
            std::unique_ptr<MasterScore> copy(readCreatedScore(path)); QVERIFY(copy);
            QCOMPARE(copy->lastMeasure()->endTick(), s->lastMeasure()->endTick());
            }
      void sectionCourtesyOptOut() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "section.mscx")); QVERIFY(s);
            Measure* beforeStart = s->firstMeasure();
            Measure* end = beforeStart->nextMeasure()->nextMeasure()->nextMeasure();
            QVERIFY(at(end, SegmentType::TimeSigRepeatAnnounce));
            auto section = new LayoutBreak(s.get()); section->setLayoutBreakType(LayoutBreak::SECTION);
            section->setShowCourtesy(false); section->setTrack(0); beforeStart->add(section);
            s->setPlaylistDirty(); s->doLayout();
            QVERIFY(!at(end, SegmentType::TimeSigRepeatAnnounce));
            }
      void styleUndoRestoresPlacement() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "matching.mscx")); QVERIFY(s);
            Measure* end = s->firstMeasure()->nextMeasure()->nextMeasure();
            s->startCmd(); s->undo(new ChangeStyleVal(s.get(), Sid::changesBeforeBarlineRepeats, true)); s->endCmd();
            QVERIFY(at(end, SegmentType::TimeSig));
            s->undoStack()->undo(&ed); s->doLayout();
            QVERIFY(at(end->nextMeasure(), SegmentType::TimeSig, true));
            s->undoStack()->redo(&ed); s->doLayout();
            QVERIFY(at(end, SegmentType::TimeSig));
            }
      void linkedPartRoundTrip() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "matching.mscx")); QVERIFY(s);
            Score* part = new Score(s.get());
            auto ex = new Excerpt(s.get()); ex->setPartScore(part); ex->setParts(s->parts()); ex->setTitle("Courtesy part");
            // Register first, as the Parts dialog does: inserting a missing title
            // frame during creation must update both the master and the new part.
            s->excerpts().append(ex);
            s->startCmd(); Excerpt::createExcerpt(ex); s->endCmd();
            QVERIFY(s->first()->isVBox());
            QVERIFY(part->first()->isVBox());
            QCOMPARE(part->nmeasures(), s->nmeasures());
            for (Score* score : s->scoreList()) {
                  score->style().set(Sid::changesBeforeBarlineRepeats, true); score->doLayout();
                  Measure* end = score->firstMeasure()->nextMeasure()->nextMeasure();
                  QVERIFY(at(end, SegmentType::TimeSig));
                  }
            QTemporaryDir tmp; QString path = tmp.filePath("parts-roundtrip.mscx");
            QVERIFY(saveScore(s.get(), path));
            std::unique_ptr<MasterScore> copy(readCreatedScore(path)); QVERIFY(copy);
            QCOMPARE(copy->excerpts().size(), 1);
            Score* loadedPart = copy->excerpts().front()->partScore(); QVERIFY(loadedPart);
            loadedPart->doLayout();
            QVERIFY(loadedPart->styleB(Sid::changesBeforeBarlineRepeats));
            QVERIFY(at(loadedPart->firstMeasure()->nextMeasure()->nextMeasure(), SegmentType::TimeSig));
            }
      void linkedPartStyleInheritance_data() {
            QTest::addColumn<bool>("value");
            QTest::newRow("all-disabled") << false;
            QTest::newRow("all-enabled") << true;
            }
      void linkedPartStyleInheritance() {
            QFETCH(bool, value);
            std::unique_ptr<MasterScore> s(readScore(DIR + "matching.mscx")); QVERIFY(s);
            for (int sid = int(Sid::changesBeforeBarlineRepeats); sid <= int(Sid::smallParens); ++sid)
                  s->style().set(Sid(sid), value);
            s->doLayout();
            Score* part = new Score(s.get());
            auto ex = new Excerpt(s.get()); ex->setPartScore(part); ex->setParts(s->parts());
            ex->setTitle("Inherited courtesy style"); s->excerpts().append(ex);
            s->startCmd(); Excerpt::createExcerpt(ex); s->endCmd();
            for (int sid = int(Sid::changesBeforeBarlineRepeats); sid <= int(Sid::smallParens); ++sid)
                  QCOMPARE(part->styleB(Sid(sid)), value);
            // Native part styles are optimized against global defaults. Check
            // independent overrides using the normal serializer and reader.
            for (int sid = int(Sid::changesBeforeBarlineRepeats); sid <= int(Sid::smallParens); ++sid)
                  part->style().set(Sid(sid), !value);
            part->doLayout();
            QTemporaryDir tmp; QString path = tmp.filePath("inherited-parts.mscx");
            QVERIFY(saveScore(s.get(), path));
            std::unique_ptr<MasterScore> copy(readCreatedScore(path)); QVERIFY(copy);
            QCOMPARE(copy->excerpts().size(), 1);
            Score* loadedPart = copy->excerpts().front()->partScore(); QVERIFY(loadedPart);
            for (int sid = int(Sid::changesBeforeBarlineRepeats); sid <= int(Sid::smallParens); ++sid) {
                  QCOMPARE(copy->styleB(Sid(sid)), value);
                  QCOMPARE(loadedPart->styleB(Sid(sid)), !value);
                  }
            loadedPart->doLayout();
            Measure* end = loadedPart->firstMeasure()->nextMeasure()->nextMeasure();
            QCOMPARE(at(end, SegmentType::TimeSig) != nullptr, !value);
            QCOMPARE(at(end->nextMeasure(), SegmentType::TimeSig, true) != nullptr, value);
            }
      void linkedPartPlacementUndo() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "matching.mscx")); QVERIFY(s);
            Score* part = new Score(s.get());
            auto ex = new Excerpt(s.get()); ex->setPartScore(part); ex->setParts(s->parts());
            ex->setTitle("Courtesy undo"); s->excerpts().append(ex);
            s->startCmd(); Excerpt::createExcerpt(ex); s->endCmd();
            auto checkPlacement = [&](Score* score, bool before) {
                  score->doLayout();
                  Measure* end = score->firstMeasure()->nextMeasure()->nextMeasure();
                  Measure* cont = end->nextMeasure();
                  for (SegmentType type : {SegmentType::KeySig, SegmentType::TimeSig}) {
                        Element* e = before ? at(end, type) : at(cont, type, true);
                        QVERIFY(e); QVERIFY(!e->generated()); QVERIFY(!e->isCourtesy());
                        QCOMPARE(e->tick(), cont->tick());
                        QVERIFY(!(before ? at(cont, type, true) : at(end, type)));
                        }
                  TimeSig* meter = score->staff(0)->timeSig(cont->tick());
                  QVERIFY(meter); QVERIFY(!meter->measure()->isMMRest());
                  QVERIFY(meter->sig().identical(Fraction(3,4)));
                  QCOMPARE(score->staff(0)->key(cont->tick()), Key::F);
                  };
            checkPlacement(s.get(), false); checkPlacement(part, false);
            s->startCmd();
            for (Score* score : s->scoreList())
                  score->undo(new ChangeStyleVal(score, Sid::changesBeforeBarlineRepeats, true));
            s->endCmd();
            checkPlacement(s.get(), true); checkPlacement(part, true);
            for (int i = 0; i < 3; ++i) {
                  s->undoStack()->undo(&ed);
                  checkPlacement(s.get(), false); checkPlacement(part, false);
                  s->undoStack()->redo(&ed);
                  checkPlacement(s.get(), true); checkPlacement(part, true);
                  }
            // A style edit to the part alone must leave the master's placement alone.
            s->startCmd(); part->undo(new ChangeStyleVal(part, Sid::changesBeforeBarlineRepeats, false)); s->endCmd();
            checkPlacement(s.get(), true); checkPlacement(part, false);
            s->undoStack()->undo(&ed);
            checkPlacement(s.get(), true); checkPlacement(part, true);
            s->undoStack()->redo(&ed);
            checkPlacement(s.get(), true); checkPlacement(part, false);
            }
      void linkedPartMultiMeasureRestLifecycle() {
            std::unique_ptr<MasterScore> s(readScore(DIR + "matching.mscx")); QVERIFY(s);
            s->style().set(Sid::changesBeforeBarlineRepeats, true); s->doLayout();
            Score* part = new Score(s.get());
            auto ex = new Excerpt(s.get()); ex->setPartScore(part); ex->setParts(s->parts());
            ex->setTitle("MM-rest lifecycle"); s->excerpts().append(ex);
            s->startCmd(); Excerpt::createExcerpt(ex); s->endCmd();
            const Fraction boundary = s->firstMeasure()->nextMeasure()->nextMeasure()->endTick();
            auto checkMaps = [&](MasterScore* master, const Fraction& expected) {
                  for (Score* score : master->scoreList()) {
                        score->doLayout();
                        for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                              TimeSig* meter = score->staff(0)->timeSig(m->tick());
                              QVERIFY(meter); QVERIFY(!meter->measure()->isMMRest());
                              QVERIFY(meter->sig().identical(m->timesig()));
                              }
                        TimeSig* meter = score->staff(0)->timeSig(boundary);
                        QVERIFY(meter); QVERIFY(meter->sig().identical(expected));
                        }
                  };
            for (int i = 0; i < 6; ++i) {
                  bool enabled = (i % 2) != 0;
                  for (Score* score : s->scoreList()) {
                        score->style().set(Sid::createMultiMeasureRests, enabled);
                        score->style().set(Sid::minEmptyMeasures, 2);
                        }
                  checkMaps(s.get(), Fraction(3,4));
                  for (Score* score : s->scoreList()) {
                        Measure* end = score->firstMeasureMM();
                        while (end && !end->repeatEnd()) end = end->nextMeasureMM();
                        QVERIFY(end); QVERIFY(at(end, SegmentType::TimeSig));
                        QCOMPARE(at(end, SegmentType::TimeSig)->tick(), boundary);
                        if (enabled) QVERIFY(end->isMMRest());
                        }
                  }
            // Exercise the visible linked copy, as a part's context menu would.
            Measure* end = part->firstMeasureMM();
            while (end && !end->repeatEnd()) end = end->nextMeasureMM();
            QVERIFY(end && end->isMMRest());
            TimeSig* visible = toTimeSig(at(end, SegmentType::TimeSig)); QVERIFY(visible);
            s->startCmd(); part->cmdRemoveTimeSig(visible); s->endCmd();
            checkMaps(s.get(), Fraction(2,4));
            s->undoStack()->undo(&ed); checkMaps(s.get(), Fraction(3,4));
            s->undoStack()->redo(&ed); checkMaps(s.get(), Fraction(2,4));
            s->undoStack()->undo(&ed); checkMaps(s.get(), Fraction(3,4));
            QTemporaryDir tmp; QString path = tmp.filePath("mmrest-lifecycle.mscx");
            QVERIFY(saveScore(s.get(), path));
            if (qEnvironmentVariableIsSet("COURTESY_MMREST_EXPORT"))
                  QFile::copy(path, QString::fromLocal8Bit(qgetenv("COURTESY_MMREST_EXPORT")));
            std::unique_ptr<MasterScore> copy(readCreatedScore(path)); QVERIFY(copy);
            QCOMPARE(copy->excerpts().size(), 1); checkMaps(copy.get(), Fraction(3,4));
            Score* loadedPart = copy->excerpts().front()->partScore(); QVERIFY(loadedPart);
            for (SegmentType type : {SegmentType::Clef, SegmentType::KeySig, SegmentType::TimeSig}) {
                  Element* masterChange = actualAtTick(copy.get(), type, boundary);
                  Element* partChange = actualAtTick(loadedPart, type, boundary);
                  QVERIFY(masterChange && partChange);
                  QVERIFY(masterChange->linkList().contains(partChange));
                  }

            for (Measure* m = copy->firstMeasure(); m; m = m->nextMeasure()) {
                  Element* masterRest = actualAtTick(copy.get(), SegmentType::ChordRest, m->tick());
                  Element* partRest = actualAtTick(loadedPart, SegmentType::ChordRest, m->tick());
                  QVERIFY(masterRest && partRest);
                  QVERIFY2(masterRest->linkList().contains(partRest), qPrintable(m->tick().toString()));
                  }
            for (Score* score : copy->scoreList()) {
                  score->style().set(Sid::createMultiMeasureRests, false);
                  }
            checkMaps(copy.get(), Fraction(3,4));
            }
      void splitStaffChangesAtBoundary_data() {
            QTest::addColumn<int>("type");
            QTest::newRow("clef") << int(SegmentType::Clef);
            QTest::newRow("key") << int(SegmentType::KeySig);
            QTest::newRow("meter") << int(SegmentType::TimeSig);
            }
      void splitStaffChangesAtBoundary() {
            QFETCH(int, type);
            const SegmentType kind = SegmentType(type);
            std::unique_ptr<MasterScore> s(readStudio("studio-changes-inside-repeats.mscx")); QVERIFY(s);
            s->style().set(Sid::changesBeforeBarlineRepeats, false);
            s->style().set(Sid::placeClefsBeforeRepeats, false);
            Measure* end = s->firstMeasure(); Measure* next = end->nextMeasure(); QVERIFY(next);
            Segment* start = next->findSegmentR(kind, Fraction(0,1)); QVERIFY(start);
            if (kind == SegmentType::Clef) {
                  auto clef = new Clef(s.get()); clef->setTrack(0); clef->setClefType(ClefType::F);
                  start->add(clef);
                  }
            Element* moved = start->element(0); QVERIFY(moved);
            QVERIFY(start->element(VOICES));
            // Model a native import with staff 1's change before the repeat,
            // and staff 2's change after it, at the same musical instant.
            start->remove(moved);
            Segment* before = end->getSegmentR(kind, end->ticks()); before->add(moved);
            const Fraction tick = moved->tick();
            s->startCmd(); s->doLayout(); s->endCmd();
            start = next->findSegmentR(kind, Fraction(0,1)); QVERIFY(start);
            QCOMPARE(start->element(0), moved); QVERIFY(start->element(VOICES));
            QCOMPARE(moved->tick(), tick);
            QVERIFY(!at(end, kind));
            s->undoStack()->undo(&ed); s->doLayout();
            start = next->findSegmentR(kind, Fraction(0,1)); QVERIFY(start);
            QCOMPARE(start->element(0), moved); QVERIFY(start->element(VOICES));
            s->undoStack()->redo(&ed); s->doLayout();
            start = next->findSegmentR(kind, Fraction(0,1)); QVERIFY(start);
            QCOMPARE(start->element(0), moved); QVERIFY(start->element(VOICES));
            QTemporaryDir tmp; QString path = tmp.filePath("split-changes.mscx");
            QVERIFY(saveScore(s.get(), path));
            std::unique_ptr<MasterScore> copy(readCreatedScore(path)); QVERIFY(copy);
            Segment* loaded = copy->firstMeasure()->nextMeasure()->findSegmentR(kind, Fraction(0,1));
            QVERIFY(loaded && loaded->element(0) && loaded->element(VOICES));
            QCOMPARE(loaded->tick(), tick);
            }
      void renderPreview_data() {
            QTest::addColumn<QString>("fixture");
            QTest::addColumn<bool>("studio");
            QTest::addColumn<bool>("matching");
            QTest::newRow("repeat") << QString("repeat.mscx") << false << false;
            QTest::newRow("jump") << QString("jump.mscx") << false << false;
            QTest::newRow("matching") << QString("matching.mscx") << false << true;
            QTest::newRow("combined") << QString("studio-changes-inside-repeats.mscx") << true << false;
            QTest::newRow("multi-measure-rest") << QString("matching.mscx") << false << true;
            QTest::newRow("above-staves") << QString("studio-changes-inside-repeats.mscx") << true << false;
            QTest::newRow("above-staff-text") << QString("repeat.mscx") << false << false;
            QTest::newRow("across-staves") << QString("studio-changes-inside-repeats.mscx") << true << false;
            QTest::newRow("margin-hang") << QString("matching.mscx") << false << false;
            QTest::newRow("margin-inset-signature") << QString("matching.mscx") << false << false;
            QTest::newRow("margin-inset-barline") << QString("matching.mscx") << false << false;
            }
      void renderPreview() {
            QFETCH(QString, fixture); QFETCH(bool, studio); QFETCH(bool, matching);
            std::unique_ptr<MasterScore> s(studio ? readStudio(fixture) : readScore(DIR + fixture)); QVERIFY(s);
            if (matching) s->style().set(Sid::changesBeforeBarlineRepeats, true);
            if (QByteArray(QTest::currentDataTag()) == "above-staves") {
                  s->style().set(Sid::timeSigPlacement, int(TimeSigPlacement::ABOVE_STAVES));
                  s->staff(2)->setTimeSigSystemObject(true);
                  }
            if (QByteArray(QTest::currentDataTag()) == "across-staves") {
                  s->style().set(Sid::timeSigPlacement, int(TimeSigPlacement::ACROSS_STAVES));
                  s->style().set(Sid::timeSigCenterAcrossStaveGroup, true);
                  }
            if (QByteArray(QTest::currentDataTag()).startsWith("margin-")) {
                  s->style().set(Sid::timeSigPlacement, int(TimeSigPlacement::ABOVE_STAVES));
                  s->style().set(Sid::genCourtesyKeysig, false);
                  int margin = QByteArray(QTest::currentDataTag()) == "margin-hang" ? int(TimeSigVSMargin::HANG_INTO_MARGIN)
                             : QByteArray(QTest::currentDataTag()) == "margin-inset-signature" ? int(TimeSigVSMargin::RIGHT_ALIGN_TO_BARLINE)
                             : int(TimeSigVSMargin::CREATE_SPACE);
                  s->style().set(Sid::timeSigVSMarginCentered, margin);
                  Measure* end = s->firstMeasure()->nextMeasure()->nextMeasure(); end->setRepeatEnd(false);
                  auto br = new LayoutBreak(s.get()); br->setLayoutBreakType(LayoutBreak::LINE); br->setParent(end); end->add(br);
                  }
            if (QByteArray(QTest::currentDataTag()) == "multi-measure-rest") {
                  s->style().set(Sid::createMultiMeasureRests, true);
                  s->style().set(Sid::minEmptyMeasures, 2);
                  }
            s->doLayout();
            if (QByteArray(QTest::currentDataTag()) == "above-staff-text") {
                  s->style().set(Sid::timeSigPlacement, int(TimeSigPlacement::ABOVE_STAVES)); s->doLayout();
                  TimeSig* meter = toTimeSig(at(s->firstMeasure(), SegmentType::TimeSig, true)); QVERIFY(meter);
                  Segment* beat = s->firstMeasure()->first(SegmentType::ChordRest); QVERIFY(beat);
                  auto text = new StaffText(s.get()); text->setTrack(0); text->setParent(beat);
                  text->setXmlText("Staff text above the meter");
                  text->setOffset(QPointF(meter->pagePos().x() - beat->pagePos().x(), 0));
                  text->setPropertyFlags(Pid::OFFSET, PropertyFlags::UNSTYLED); beat->add(text);
                  s->doLayout();
                  }
            QImage image(1500, 450, QImage::Format_ARGB32_Premultiplied); image.fill(Qt::white);
            QPainter painter(&image); painter.setRenderHint(QPainter::Antialiasing);
            const auto elements = s->pages().front()->elements();
            QRectF bounds;
            for (Element* e : elements) if (e->visible() && !e->bbox().isEmpty() && !e->isPage() && !e->isLayoutBreak())
                  bounds |= e->bbox().translated(e->pagePos());
            QVERIFY(!bounds.isEmpty());
            bounds.adjust(-10,-10,10,10);
            qreal scale = qMin(image.width() / bounds.width(), image.height() / bounds.height());
            painter.scale(scale, scale); painter.translate(-bounds.topLeft());
            for (Element* e : elements) {
                  if (!e->visible() || e->isLayoutBreak()) continue;
                  painter.save(); painter.translate(e->pagePos()); e->draw(&painter); painter.restore();
                  }
            painter.end();
            if (qEnvironmentVariableIsSet("COURTESY_PREVIEW") && QByteArray(QTest::currentDataTag()) == "repeat")
                  QVERIFY(image.save(QString::fromLocal8Bit(qgetenv("COURTESY_PREVIEW"))));
            if (qEnvironmentVariableIsSet("COURTESY_PREVIEW_DIR")) {
                  QDir directory(QString::fromLocal8Bit(qgetenv("COURTESY_PREVIEW_DIR")));
                  QVERIFY(directory.mkpath("."));
                  QVERIFY(image.save(directory.filePath(QString::fromLatin1(QTest::currentDataTag()) + ".png")));
                  }
            }

      };
QTEST_MAIN(TestRepeatCourtesy)
#include "tst_repeatcourtesy.moc"
