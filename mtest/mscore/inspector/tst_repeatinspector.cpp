// Regression coverage for repeat endpoint controls in the real Inspector.
#include <QtTest/QtTest>
#include <QCheckBox>
#include <QPointer>
#include "mtest/testutils.h"
#include "mscore/inspector/inspector.h"
#include "mscore/icons.h"
#include "mscore/shortcut.h"
#include "libmscore/score.h"
#include "libmscore/measure.h"
#include "libmscore/chord.h"
#include "libmscore/note.h"
#include "libmscore/tie.h"

using namespace Ms;

class TestRepeatInspector : public QObject, public MTest
      {
      Q_OBJECT
   private slots:
      void initTestCase()
            {
            initMTest();
            initMuseScoreResources();
            iconPath = ":/data/icons/";
            genIcons();
            Shortcut::init();
            }
      void toggleDestination_data()
            {
            QTest::addColumn<bool>("outgoing");
            QTest::newRow("normal-source") << false;
            QTest::newRow("outgoing-source") << true;
            }
      void toggleDestination()
            {
            QFETCH(bool, outgoing);
            std::unique_ptr<MasterScore> score(readScore("libmscore/repeat/repeat06.mscx"));
            QVERIFY(score);
            std::vector<Note*> notes;
            for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                  Note* n = toChord(m->first(SegmentType::ChordRest)->element(0))->upNote();
                  n->setPitch(60);
                  notes.push_back(n);
                  }
            Note* source = notes[outgoing ? 2 : 1];
            Note* destination = notes[outgoing ? 1 : 3];
            score->select(source);
            score->cmdToggleTie();
            Tie* tie = outgoing ? source->partialTie(false) : source->tieFor();
            QVERIFY(tie);
            QVERIFY(!tie->spannerSegments().empty());
            score->select(tie->segmentAt(0));
            Inspector inspector;
            inspector.update(score.get());
            inspector.show();
            QCoreApplication::processEvents();
            for (int i = 0; i < 12; ++i) {
                  auto checks = inspector.findChildren<QCheckBox*>("repeatTieDestination");
                  QCOMPARE(checks.size(), 1);
                  QPointer<QCheckBox> check = checks.front();
                  const bool active = (i % 2) != 0;
                  QCOMPARE(check->isChecked(), !active);
                  QTest::mouseClick(check.data(), Qt::LeftButton, Qt::NoModifier,
                                    QPoint(5, check->height() / 2));
                  QCOMPARE(destination->partialTie(true) != nullptr, active);
                  // The emitting widget must survive the callback.
                  QVERIFY(check);
                  QCoreApplication::processEvents();
                  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
                  }
            score->undoRedo(true, nullptr);
            QVERIFY(!destination->partialTie(true));
            inspector.update(score.get());
            score->undoRedo(false, nullptr);
            QVERIFY(destination->partialTie(true));
            inspector.update(score.get());
            inspector.update(nullptr);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            }
      };
QTEST_MAIN(TestRepeatInspector)
#include "tst_repeatinspector.moc"
