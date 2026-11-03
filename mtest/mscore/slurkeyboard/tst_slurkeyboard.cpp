// 37E keyboard-only entry points for repeat slurs.
#include <QtTest/QtTest>
#include "mtest/testutils.h"
#include "mscore/musescore.h"
#include "mscore/scoreview.h"
#include "libmscore/score.h"
#include "libmscore/measure.h"
#include "libmscore/segment.h"
#include "libmscore/barline.h"
#include "libmscore/chord.h"
#include "libmscore/note.h"
#include "libmscore/slur.h"
#include "libmscore/navigate.h"

using namespace Ms;

class TestSlurKeyboard : public QObject, public MTest {
      Q_OBJECT
      QTemporaryDir config;
   private slots:
      void initTestCase()
            {
            qputenv("QML_DISABLE_DISK_CACHE", "true");
            MScore::noGui = true;
            MScore::testMode = true;
            initMuseScoreResources();
            QVERIFY(config.isValid());
            QVERIFY(QDir(config.path()).mkpath("workspaces"));
            for (const QString& name : { QString("Basic"), QString("Advanced") }) {
                  QVERIFY(QFile::copy(QString(SLUR_KEYBOARD_WORKSPACES) + "/" + name + ".workspace",
                        config.path() + "/workspaces/" + name + ".workspace"));
                  }
            QVERIFY(MuseScoreApplication::setCustomConfigFolder(config.path()));
            QStringList arguments;
            MuseScore::init(arguments);
            root = rootPath();
            }
      void singleSelection_data()
            {
            QTest::addColumn<int>("kind");
            QTest::newRow("start-repeat-barline") << 0;
            QTest::newRow("last-note-before-repeat") << 1;
            QTest::newRow("ordinary-note-control") << 2;
            QTest::newRow("last-note-before-skipped-volta") << 3;
            QTest::newRow("existing-note-barline-list") << 4;
            QTest::newRow("second-ending-grand-staff") << 5;
            QTest::newRow("second-ending-single-staff") << 6;
            }
      void singleSelection()
            {
            QFETCH(int, kind);
            std::unique_ptr<MasterScore> score(readScore(kind == 5 ? "libmscore/repeat/partial-lyrics-X01.mscx"
                  : kind == 3 || kind == 6 ? "libmscore/repeat/partial-slur-S02.mscx"
                  : "libmscore/repeat/partial-slur-S04.mscx"));
            QVERIFY(score);
            Measure* measure = score->firstMeasure();
            if (kind == 0 || kind == 4)
                  measure = measure->nextMeasure();
            else if (kind == 1 || kind == 3) {
                  while (measure && (kind == 1 ? !measure->repeatEnd()
                         : !Slur::boundaryAnchor(measure, 0, false) || measure->repeatEnd()))
                        measure = measure->nextMeasure();
                  }
            else if (kind >= 5) {
                  while (measure && (!measure->prevMeasure() || !measure->prevMeasure()->repeatEnd()
                         || !Slur::boundaryAnchor(measure, 0, true)))
                        measure = measure->nextMeasure();
                  }
            QVERIFY(measure);
            ChordRest* anchor = nullptr;
            Element* selected = nullptr;
            if (kind == 0 || kind == 4) {
                  Segment* bar = measure->first(SegmentType::StartRepeatBarLine);
                  QVERIFY(bar);
                  selected = bar->element(0);
                  anchor = Slur::boundaryAnchor(measure, 0, true);
                  }
            else if (kind >= 5) {
                  Segment* bar = measure->prevMeasure()->first(SegmentType::EndBarLine);
                  QVERIFY(bar);
                  selected = bar->element(0);
                  QVERIFY(selected && selected->isBarLine());
                  QCOMPARE(int(toBarLine(selected)->barLineType()), int(BarLineType::END_REPEAT));
                  anchor = Slur::boundaryAnchor(measure, 0, true);
                  }
            else {
                  anchor = kind != 2 ? Slur::boundaryAnchor(measure, 0, false)
                        : toChordRest(measure->first(SegmentType::ChordRest)->element(0));
                  QVERIFY(anchor && anchor->isChord());
                  selected = toChord(anchor)->upNote();
                  }
            QVERIFY(anchor && selected);
            QSet<Spanner*> existing;
            for (const auto& entry : score->spanner())
                  existing.insert(entry.second);
            ScoreView view;
            view.setScore(score.get());
            score->select(selected);
            if (kind == 4)
                  score->select(toChord(anchor)->upNote(), SelectType::ADD);
            view.cmdAddSlur(); // the same command dispatched by S
            Slur* created = nullptr;
            for (const auto& entry : score->spanner()) {
                  if (entry.second->isSlur() && !existing.contains(entry.second))
                        created = toSlur(entry.second);
                  }
            QVERIFY(created);
            QCOMPARE(created->startCR(), anchor);
            QCOMPARE(created->endCR(), kind == 2 ? nextChordRest(anchor) : anchor);
            QCOMPARE(created->partialDirection(), kind == 0 || kind == 4 || kind >= 5 ? 0 : kind != 2 ? 1 : -1);
            QCOMPARE(view.editMode(), kind != 4);
            view.cmd("escape");
            score->undoRedo(true, nullptr);
            QCOMPARE(int(score->spanner().size()), existing.size());
            score->undoRedo(false, nullptr);
            QCOMPARE(int(score->spanner().size()), existing.size() + 1);
            }
      void ineligibleBarline_data()
            {
            QTest::addColumn<bool>("scoreEnd");
            QTest::newRow("ordinary-barline") << false;
            QTest::newRow("end-of-score-repeat-destination") << true;
            }
      void ineligibleBarline()
            {
            QFETCH(bool, scoreEnd);
            std::unique_ptr<MasterScore> score(readScore("libmscore/repeat/partial-slur-S04.mscx"));
            QVERIFY(score);
            Measure* last = score->lastMeasure();
            QVERIFY(last && last->prevMeasure());
            if (scoreEnd) {
                  last->setRepeatStart(true);
                  last->setRepeatEnd(true);
                  score->setPlaylistDirty();
                  score->setLayoutAll();
                  score->doLayout();
                  QVERIFY(Slur::boundaryAnchor(last, 0, true));
                  }
            else
                  QVERIFY(!Slur::boundaryAnchor(last, 0, true));
            Segment* bar = (scoreEnd ? last : last->prevMeasure())->first(SegmentType::EndBarLine);
            QVERIFY(bar && bar->element(0));
            const int before = score->spanner().size();
            ScoreView view;
            view.setScore(score.get());
            score->select(bar->element(0));
            view.cmdAddSlur();
            QCOMPARE(int(score->spanner().size()), before);
            QVERIFY(!view.editMode());
            }
      void cleanupTestCase()
            {
            delete Ms::mscore;
            Ms::mscore = nullptr;
            }
      };
QTEST_MAIN(TestSlurKeyboard)
#include "tst_slurkeyboard.moc"
