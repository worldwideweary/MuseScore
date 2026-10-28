// MuseScore sequencer preparation regressions. GPL-2.0-or-later.
#include <QtTest/QtTest>
#include "mtest/testutils.h"
#include "mscore/preferences.h"
#include "mscore/seq.h"
#include "libmscore/score.h"

namespace Ms {

class TestAudioSequencer : public QObject {
      Q_OBJECT
   private slots:
      void initTestCase()
            {
            qputenv("QML_DISABLE_DISK_CACHE", "true");
            MScore::noGui = true;
            MScore::testMode = true;
            initMuseScoreResources();
            preferences.init(true);
            MScore::init();
            }

      void collectDuringPlayback_data()
            {
            QTest::addColumn<int>("target");
            QTest::addColumn<bool>("pendingChunk");
            for (int target : {7680, 9600, 11520}) {
                  QTest::newRow(qPrintable(QString("tick-%1").arg(target))) << target << false;
                  QTest::newRow(qPrintable(QString("tick-%1-merge").arg(target))) << target << true;
                  }
            }

      void collectDuringPlayback()
            {
            QFETCH(int, target);
            QFETCH(bool, pendingChunk);
            MasterScore score(MScore::defaultStyle());
            Seq sequencer;
            sequencer.cs = &score;
            sequencer.midi = MidiRenderer(&score);
            sequencer.state = Transport::PLAY;
            sequencer.playlistChanged = false;
            // A collected D-style roadmap. D4 selects +7 coarse tuning;
            // D5 resets it and D6 has a one-shot portamento command.
            for (auto control : {std::pair<int, int>{99, 120}, {98, 51}, {38, 7}, {6, 64}})
                  sequencer.events.emplace(5760, NPlayEvent(ME_CONTROLLER, 0, control.first, control.second));
            sequencer.events.emplace(5760, NPlayEvent(ME_NOTEON, 0, 57, 90));
            sequencer.events.emplace(7680, NPlayEvent(ME_CONTROLLER, 0, 121, 0));
            sequencer.events.emplace(7680, NPlayEvent(ME_NOTEON, 0, 57, 90));
            sequencer.events.emplace(9600, NPlayEvent(ME_CONTROLLER, 0, 84, 60));
            sequencer.events.emplace(9600, NPlayEvent(ME_NOTEON, 0, 72, 90));
            sequencer.events.emplace(11520, NPlayEvent(ME_NOTEON, 0, 72, 90));
            sequencer.events.emplace(12479, NPlayEvent(ME_NOTEON, 0, 72, 0));
            sequencer.renderEventsStatus.setOccupied(0, 1000000);
            sequencer.playPos = sequencer.events.upper_bound(target);
            sequencer.playFrame = 123456;
            const auto before = sequencer.playPos;
            if (pendingChunk)
                  sequencer.renderEvents.emplace(20000, NPlayEvent(ME_NOTEON, 0, 64, 90));

            // seekCommon() collects before queuing SEEK. Model a callback in
            // that gap: its old clock/cursor must not replay earlier events.
            sequencer.collectEvents(target);
            QVERIFY(sequencer.playPos == before);
            QCOMPARE(sequencer.playFrame, 123456);
            unsigned staleEvents = 0;
            for (auto it = sequencer.playPos; it != sequencer.events.cend() && it->first <= target; ++it)
                  ++staleEvents;
            QCOMPARE(staleEvents, 0u);
            QVERIFY(sequencer.renderEvents.empty());
            QCOMPARE(sequencer.events.count(20000), size_t(pendingChunk ? 1 : 0));

            // An edited playlist is deliberately not rebuilt under a running
            // audio callback. Its iterator remains valid until transport stops.
            sequencer.playlistChanged = true;
            sequencer.collectEvents(target);
            QVERIFY(sequencer.playPos == before);
            }
      };

} // namespace Ms

QTEST_MAIN(Ms::TestAudioSequencer)
#include "tst_audiosequencer.moc"
