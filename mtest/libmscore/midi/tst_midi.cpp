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

#include <QBuffer>
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QIODevice>
#include <QScopedValueRollback>
#include <QScopeGuard>
#include <QStringList>
#include <QTextStream>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <set>
#include <tuple>
#include <vector>

#include "audio/exports/exportmidi.h"
#include "audio/midi/midifile.h"
#include "audio/midi/msynthesizer.h"
#include "audio/midi/fluid/fluid.h"
#include "audio/midi/synthesizergui.h"
#include "effects/noeffect/noeffect.h"
#include "mtest/audio/fluid2/testfont.h"
#include "mscore/exportaudio.h"
#include "mscore/preferences.h"
#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
#include "audio/midi/fluid2/fluid2.h"
#include "audio/midi/fluid2/fluid2gui.h"
#include "audio/midi/fluid/soundfontpaths.h"
#include "audio/midi/midipatch.h"
#include <QApplication>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QTimer>
#endif

#include "libmscore/chord.h"
#include "libmscore/durationtype.h"
#include "libmscore/keysig.h"
#include "libmscore/instrument.h"
#include "libmscore/part.h"
#include "libmscore/xml.h"
#include "libmscore/laissezvibplayback.h"
#include "libmscore/mcursor.h"
#include "libmscore/measure.h"
#include "libmscore/mscore.h"
#include "libmscore/note.h"
#include "libmscore/rendermidi.h"
#include "libmscore/repeatlist.h"
#include "libmscore/utils.h"
#include "libmscore/score.h"
#include "libmscore/segment.h"
#include "libmscore/stafftextbase.h"

#include "mtest/testutils.h"
#define DIR QString("libmscore/midi/")

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#define endl Qt::endl
#define dec Qt::dec
#endif

namespace Ms {
      extern Score::FileError importMidi(MasterScore*, const QString&);
      extern QString dataPath;
      }

using namespace Ms;

// Use the real engines with a small, owned GUI for Master's signal connection.
// The export regression does not need the application's icons/main window.
class AudioExportTestFluid : public FluidS::Fluid {
   public:
      SynthesizerGui* gui() override {
            if (!_gui) _gui = new SynthesizerGui(this);
            return _gui;
            }
      };

class AudioExportTestMaster : public MasterSynthesizer {
      std::unique_ptr<SynthesizerGui> panel;
   public:
      void registerTestSynth(Synthesizer* engine) {
            registerSynthesizer(engine);
            panel.reset(engine->gui());
            }
      };

//---------------------------------------------------------
//   TestMidi
//---------------------------------------------------------

class TestMidi : public QObject, public MTest
      {
      Q_OBJECT
      void midiExportTestRef(const QString& file);
      void testMidiExport(MasterScore* score, const QString& writeFile, const QString& refFile);

      void testTimeStretchFermata(MasterScore* score, const QString& file, const QString& testName);
      void testTimeStretchFermataTempoEdit(MasterScore* score, const QString& file, const QString& testName);

   private slots:
      void initTestCase();
      void midi01();
      void midi02();
      void midi03();
      void events_data();
      void events();
      void midiBendsExport1() { midiExportTestRef("testBends1"); }
      void midiBendsExport2() { midiExportTestRef("testBends2"); }      // Play property test
      void midiPortExport()   { midiExportTestRef("testMidiPort"); }
      void midiArpeggio()     { midiExportTestRef("testArpeggio"); }
      void midiMutedUnison()  { midiExportTestRef("testMutedUnison"); }
      void midi184376ExportMidiInitialKeySig()
            {
            midiExportTestRef("testInitialKeySigThenRepeatToMeas2");    // tick 0 has Bb keysig.  Meas 2 has no key sig. Meas 2 repeats back to start of Meas 2.  Result should have initial Bb keysig
            midiExportTestRef("testRepeatsWithKeySigs");                // 5 measures, with a key sig on every measure. Meas 3-4 are repeated.
            midiExportTestRef("testRepeatsWithKeySigsExceptFirstMeas"); // 5 measures, with a key sig on every measure except meas 0.  Meas 3-4 are repeated.
            }
      void midiVolta()
          {
          midiExportTestRef("testVoltaTemp"); // test changing temp in prima and seconda volta
          midiExportTestRef("testVoltaDynamic"); // test changing Dynamic in prima and seconda volta
          midiExportTestRef("testVoltaStaffText"); // test changing StaffText in prima and seconda volta
          }
      void midiTimeStretchFermata();
      void midiTimeStretchFermataContinuousView();
      void midiTimeStretchFermataTempoEdit();
      void midiTimeStretchFermataTempoEditContinuousView();
      void midiSingleNoteDynamics();
      void audioReverbChannels();
      void audioReverbReturnState();
      void audioCompatibilityScores();
      void audioControllerResetScore();
      void audioParameterScore();
      void audioLegacyReleasePitch();
      void audioLegacyDefaultFont();
      void audioExportLifecycle();
#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
      void fluid2SampleRateRecovery_data();
      void fluid2SampleRateRecovery();
      void fluid2SoundFontManagement_data();
      void fluid2SoundFontManagement();
      void fluid2SoundFontDiscovery();
      void fluid2DefaultSoundFont();
      void fluid2ControllerCompatibility_data();
      void fluid2ControllerCompatibility();
      void fluid2ModulationCompatibility_data();
      void fluid2ModulationCompatibility();
      void fluid2ParameterCompatibility_data();
      void fluid2ParameterCompatibility();
      void fluid2PortamentoPlayback_data();
      void fluid2PortamentoPlayback();
      void fluid2OutputLevelCompatibility_data();
      void fluid2OutputLevelCompatibility();
      void fluid2DefaultRouting();
      void fluid2SeekReleasePitch_data();
      void fluid2SeekReleasePitch();
#endif
      void laissezVibIntervals();
      void laissezVibPlayback_data();
      void laissezVibPlayback();
      void laissezVibBends_data();
      void laissezVibBends();
      void laissezVibGrace_data();
      void laissezVibGrace();
      void laissezVibRepeatTies_data();
      void laissezVibRepeatTies();
      void laissezVibControls_data();
      void laissezVibControls();
      void lazyPlaybackChunks();
      };

//---------------------------------------------------------
//   initTestCase
//---------------------------------------------------------

void TestMidi::initTestCase()
      {
      initMTest();
      }

void TestMidi::audioControllerResetScore()
      {
      std::unique_ptr<MasterScore> score(readCreatedScore(QString(TESTROOT)
            + "/doc/fixtures/audio-compatibility-v28/C-reset-and-bend.mscx"));
      QVERIFY(score);
      SynthesizerState state;
      EventMap events;
      score->renderMidi(&events, false, true, state);
      int attacks = 0, highestWheel = 8192;
      std::set<int> resets;
      for (const auto& entry : events) {
            const NPlayEvent& event = entry.second;
            if (event.type() == ME_NOTEON && event.velo() > 0) ++attacks;
            if (event.type() == ME_PITCHBEND)
                  highestWheel = std::max(highestWheel, event.dataA() + 128 * event.dataB());
            if (event.type() == ME_CONTROLLER && event.controller() == 121) resets.insert(entry.first);
            }
      QCOMPARE(attacks, 13);
      QVERIFY(highestWheel > 9000);
      QVERIFY(resets == std::set<int>({9600, 13440}));
      }

void TestMidi::audioParameterScore()
      {
      std::unique_ptr<MasterScore> score(readCreatedScore(QString(TESTROOT)
            + "/doc/fixtures/audio-compatibility-v30/D-parameters-and-portamento.mscx"));
      QVERIFY(score);
      SynthesizerState state;
      EventMap events;
      score->renderMidi(&events, false, true, state);
      int attacks = 0;
      std::set<std::tuple<int, int, int>> controls;
      for (const auto& entry : events) {
            const NPlayEvent& event = entry.second;
            if (event.type() == ME_NOTEON && event.velo() > 0) ++attacks;
            if (event.type() == ME_CONTROLLER) controls.emplace(entry.first, event.controller(), event.value());
            }
      QCOMPARE(attacks, 7);
      for (const auto& control : { std::make_tuple(1920, 99, 120), std::make_tuple(1920, 98, 51),
            std::make_tuple(1920, 38, 12), std::make_tuple(1920, 6, 64),
            std::make_tuple(3840, 38, 0), std::make_tuple(3840, 6, 64),
            std::make_tuple(5760, 38, 7), std::make_tuple(5760, 6, 64),
            std::make_tuple(7680, 121, 0), std::make_tuple(9600, 84, 60),
            std::make_tuple(9600, 5, 4), std::make_tuple(9600, 37, 0) })
            QVERIFY(controls.count(control) == 1);
      // Later data entries intentionally do not reselect the NRPN.
      QVERIFY(controls.count(std::make_tuple(3840, 99, 120)) == 0);
      // Merely finding CC84 at the correct tick misses the attack-order bug:
      // it must precede note-on, with the action's controller order intact.
      bool d6Attack = false;
      std::vector<std::pair<int, int>> setup;
      const auto d6 = events.equal_range(9600);
      for (auto it = d6.first; it != d6.second; ++it) {
            const NPlayEvent& event = it->second;
            if (event.type() == ME_NOTEON && event.velo() > 0) d6Attack = true;
            if (event.type() == ME_CONTROLLER && event.portamento()) {
                  QVERIFY(!d6Attack);
                  setup.emplace_back(event.controller(), event.value());
                  }
            }
      QVERIFY(d6Attack);
      QVERIFY((setup == std::vector<std::pair<int, int>> {{5, 4}, {37, 0}, {65, 0}, {84, 60}}));
      // A second explicit one-shot command with the same key is intentional,
      // not redundant controller state. Exercise the renderer's deduplication
      // using a temporary variation; keep the supplied D7 reference unchanged.
      NamedEventList* glide = score->parts().front()->instrument()->midiAction("glide", 0);
      QVERIFY(glide);
      glide->events = { MidiCoreEvent(ME_CONTROLLER, 0, CTRL_PORTAMENTO_CONTROL, 60) };
      Segment* reference = score->tick2segment(Fraction::fromTicks(11520));
      QVERIFY(reference);
      bool attached = false;
      for (Element* element : reference->annotations()) {
            if (!element->isStaffTextBase()) continue;
            ChannelActions action;
            action.channel = 0;
            action.midiActionNames.append("glide");
            toStaffTextBase(element)->channelActions()->append(action);
            attached = true;
            break;
            }
      QVERIFY(attached);
      events.clear();
      score->renderMidi(&events, false, true, state);
      int oneShots = 0;
      for (const auto& entry : events)
            if (entry.second.type() == ME_CONTROLLER && entry.second.controller() == CTRL_PORTAMENTO_CONTROL) ++oneShots;
      QCOMPARE(oneShots, 2);
      }

void TestMidi::audioCompatibilityScores()
      {
      for (const QString& file : { QString("A-controllers.mscx"), QString("B-instruments-percussion.mscx") }) {
            std::unique_ptr<MasterScore> score(readCreatedScore(QString(TESTROOT)
                  + "/doc/fixtures/audio-compatibility-v27/" + file));
            QVERIFY(score);
            SynthesizerState state;
            EventMap events;
            score->renderMidi(&events, false, true, state);
            std::set<std::tuple<int, int, int>> controls;
            int attacks = 0;
            bool tuned = false;
            std::set<int> pitches;
            for (const auto& entry : events) {
                  const NPlayEvent& event = entry.second;
                  if (event.type() == ME_CONTROLLER)
                        controls.emplace(entry.first, event.controller(), event.value());
                  if (event.type() == ME_NOTEON && event.velo() > 0) {
                        ++attacks;
                        pitches.insert(event.pitch());
                        tuned = tuned || event.tuning() == 50;
                        }
                  }
            if (file.startsWith("A-")) {
                  QCOMPARE(attacks, 44);
                  for (const auto& control : { std::make_tuple(1920, 7, 48), std::make_tuple(3840, 11, 40),
                        std::make_tuple(5760, 10, 0), std::make_tuple(7680, 10, 127),
                        std::make_tuple(11520, 64, 127), std::make_tuple(13440, 64, 0),
                        std::make_tuple(15359, int(CTRL_PROGRAM), 73), std::make_tuple(17279, int(CTRL_PROGRAM), 0) })
                        QVERIFY(controls.count(control) == 1);
                  }
            else {
                  QCOMPARE(attacks, 42);
                  QVERIFY(tuned && pitches.count(36) && pitches.count(38) && pitches.count(42) && pitches.count(46));
                  QCOMPARE(int(score->parts().size()), 3);
                  QVERIFY(score->parts().back()->instrument()->useDrumset());
                  QCOMPARE(score->parts().back()->instrument()->channel(0)->bank(), 128);
                  }
            }
      }

void TestMidi::audioExportLifecycle()
      {
      struct Preference {
            QString key;
            QVariant old;
            Preference(const QString& name, const QVariant& value) : key(name), old(value) {}
            ~Preference() { preferences.setTemporaryPreference(key, old); }
            } ratePreference(PREF_EXPORT_AUDIO_SAMPLERATE, preferences.getInt(PREF_EXPORT_AUDIO_SAMPLERATE)),
              normalizePreference(PREF_EXPORT_AUDIO_NORMALIZE, preferences.getBool(PREF_EXPORT_AUDIO_NORMALIZE));
      const QByteArray oldBackend = qgetenv("MSCORE_USE_FLUIDSYNTH2");
      const QByteArray oldIsolated = qgetenv("MSCORE_FLUID2_ISOLATED_CHANNELS");
      const QByteArray oldFont = qgetenv("MSCORE_FLUID2_SOUNDFONT");
      const auto restoreEnvironment = qScopeGuard([&]() {
            const auto restore = [](const char* key, const QByteArray& value) {
                  if (value.isNull()) qunsetenv(key); else qputenv(key, value);
                  };
            restore("MSCORE_USE_FLUIDSYNTH2", oldBackend);
            restore("MSCORE_FLUID2_ISOLATED_CHANNELS", oldIsolated);
            restore("MSCORE_FLUID2_SOUNDFONT", oldFont);
            });
      qunsetenv("MSCORE_FLUID2_SOUNDFONT");
      qputenv("MSCORE_FLUID2_ISOLATED_CHANNELS", "1");
      QScopedValueRollback<int> hostRate(MScore::sampleRate, 22050);
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString font = directory.filePath("export.sf2");
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(font).constData(), nullptr, true,
                  -1, 69, 0, 0, false, nullptr, 8));
      std::unique_ptr<MasterScore> score(readCreatedScore(QString(TESTROOT) + "/mtest/" + DIR + "audio-export.mscx"));
      QVERIFY(score);
      SynthesizerState state = {
            {"master", {{0, "NoEffect"}, {1, "NoEffect"}, {3, "440"}, {4, "0"}}},
            {"Fluid", {{0, font}}}
            };
      state.setIsDefault(false);
      score->setSynthesizerState(state);
      const auto createSynth = []() -> MasterSynthesizer* {
            auto* master = new AudioExportTestMaster;
#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
            if (qgetenv("MSCORE_USE_FLUIDSYNTH2") == "1")
                  master->registerTestSynth(new Fluid2);
            else
#endif
                  master->registerTestSynth(new AudioExportTestFluid);
            master->registerEffect(0, new NoEffect);
            master->registerEffect(1, new NoEffect);
            return master;
            };
      for (int backend : {
#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
                  1,
#endif
                  0}) {
            qputenv("MSCORE_USE_FLUIDSYNTH2", QByteArray::number(backend));
            for (int rate : {44100, 48000}) {
                  preferences.setTemporaryPreference(PREF_EXPORT_AUDIO_SAMPLERATE, rate);
                  for (bool normalized : {false, true}) {
                        preferences.setTemporaryPreference(PREF_EXPORT_AUDIO_NORMALIZE, normalized);
                        QBuffer output;
                        QVERIFY(renderAudio(score.get(), &output, normalized ? nullptr : &state, createSynth));
                        QVERIFY(!output.isOpen());
                        QCOMPARE(MScore::sampleRate, 22050);
                        const QByteArray pcm = output.data();
                        QVERIFY(pcm.size() > rate * 2 * int(sizeof(float)) / 4);
                        QCOMPARE(pcm.size() % (2 * int(sizeof(float))), 0);
                        float peak = 0;
                        for (int i = 0; i < pcm.size(); i += sizeof(float)) {
                              float value;
                              std::memcpy(&value, pcm.constData() + i, sizeof(value));
                              QVERIFY(std::isfinite(value));
                              peak = std::max(peak, std::abs(value));
                              }
                        QVERIFY(peak > .001f);
                        if (normalized) QVERIFY(std::abs(peak - .99f) < .01f);
                        }
                  }
            // Exercise real renderer failure/cleanup without a GUI window.
            preferences.setTemporaryPreference(PREF_EXPORT_AUDIO_NORMALIZE, false);
            class FailedOutput : public QIODevice {
                  int mode;
                  qint64 readData(char*, qint64) override { return -1; }
                  qint64 writeData(const char*, qint64 bytes) override {
                        ++writes;
                        setErrorString("injected export failure");
                        return mode == 1 ? bytes / 2 : -1;
                        }
               public:
                  int writes = 0, closes = 0;
                  explicit FailedOutput(int value) : mode(value) {}
                  bool open(OpenMode value) override {
                        return mode == 2 ? false : QIODevice::open(value);
                        }
                  void close() override { ++closes; QIODevice::close(); }
                  };
            for (int mode : {0, 1, 2}) {
                  FailedOutput failed(mode);
                  QVERIFY(!renderAudio(score.get(), &failed, &state, createSynth));
                  QVERIFY(!failed.isOpen());
                  QCOMPARE(failed.writes, mode == 2 ? 0 : 1);
                  QCOMPARE(failed.closes, mode == 2 ? 0 : 1);
                  QCOMPARE(MScore::sampleRate, 22050);
                  }
            QBuffer cancelled;
            int callbacks = 0;
            QVERIFY(!renderAudio(score.get(), &cancelled, &state, createSynth,
                        [&](float) { ++callbacks; return false; }));
            QCOMPARE(callbacks, 1);
            QVERIFY(!cancelled.isOpen());
            QCOMPARE(MScore::sampleRate, 22050);
            }
      QBuffer invalid;
      QVERIFY(!renderAudio(nullptr, &invalid, &state, createSynth));
      QVERIFY(!invalid.isOpen());
      QVERIFY(!renderAudio(score.get(), nullptr, &state, createSynth));
      }

void TestMidi::audioLegacyDefaultFont()
      {
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QScopedValueRollback<QString> share(mscoreGlobalShare, directory.path());
      QVERIFY(QDir().mkpath(directory.filePath("sound")));
      const QString fallback = directory.filePath("sound/FluidR3Mono_GM.sf3");
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(fallback).constData(), nullptr, true,
                  -1, 69, 0, 0, false, nullptr, 8));
      AudioExportTestFluid synth;
      synth.init(44100);
      QVERIFY(synth.loadDefaultSoundFonts());
      QCOMPARE(synth.soundFontsInfo().size(), size_t(1));
      QVERIFY(synth.loadDefaultSoundFonts());
      QCOMPARE(synth.soundFontsInfo().size(), size_t(1));
      // A valid saved absolute path need not be in any discovery folder.
      const QString external = directory.filePath("saved-external.sf2");
      QVERIFY(QFile::copy(fallback, external));
      QVERIFY(synth.loadSoundFonts({external}));
      QCOMPARE(synth.soundFontsInfo().size(), size_t(1));
      QVERIFY(synth.loadDefaultSoundFonts());
      QCOMPARE(synth.soundFonts().front(), QFileInfo(external).fileName());
      }

void TestMidi::audioLegacyReleasePitch()
      {
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString file = directory.filePath("release.sf2");
      for (unsigned route = 0; route < 4; ++route) {
            Fluid2Test::Modulation modulation;
            if (route == 1) modulation.pitchSource = 0x020e;
            if (route == 2) { modulation.pitchSource = 2; modulation.pitchAmountSource = 0x020e; }
            QVERIFY(Fluid2Test::writeFont(QFile::encodeName(file).constData(), nullptr, true,
                        -1, 69, 0, 0, false, &modulation, 8, route == 3 ? 1 : 0));
            for (int rate : {44100, 48000}) {
                  for (int channel : {0, 128, 255}) {
                        for (int pedal : {0, 64}) {
                              FluidS::Fluid reference, candidate;
                              for (auto* engine : {&reference, &candidate}) {
                                    engine->init(rate);
                                    QVERIFY(engine->addSoundFont(file));
                                    for (int ch : {channel, 1}) {
                                          engine->play(PlayEvent(ME_CONTROLLER, ch, CTRL_PROGRAM, 0));
                                          engine->play(PlayEvent(ME_CONTROLLER, ch, 91, 0));
                                          engine->play(PlayEvent(ME_CONTROLLER, ch, 93, 0));
                                          }
                                    engine->play(PlayEvent(ME_NOTEON, channel, 60, 90));
                                    }
                              const auto render = [&](FluidS::Fluid& engine, unsigned frames) {
                                    std::vector<float> out(frames * 2, 0);
                                    float reverb[128] = {}, chorus[128] = {};
                                    for (unsigned start = 0; start < frames; start += 64) {
                                          std::fill(reverb, reverb + 128, 0);
                                          std::fill(chorus, chorus + 128, 0);
                                          engine.process(std::min(64u, frames - start),
                                                      out.data() + start * 2, reverb, chorus);
                                          }
                                    return out;
                                    };
                              const auto equal = [&](unsigned frames) {
                                    const auto a = render(reference, frames), b = render(candidate, frames);
                                    double energy = 0;
                                    for (unsigned i = 0; i < a.size(); ++i) {
                                          if (a[i] != b[i] || !std::isfinite(a[i])) return false;
                                          energy += a[i] * a[i];
                                          }
                                    return energy > .000001;
                                    };
                              const auto wheel = [&](FluidS::Fluid& engine, int value) {
                                    engine.play(PlayEvent(ME_PITCHBEND, channel, value & 127, value >> 7));
                                    };
                              QVERIFY(equal(4096));
                              if (pedal) {
                                    candidate.play(PlayEvent(ME_CONTROLLER, channel, pedal, 127));
                                    candidate.play(PlayEvent(ME_NOTEON, channel, 60, 0));
                                    }
                              wheel(reference, 8192 + 8192 * 2 / 12);
                              wheel(candidate, 8192 + 8192 * 2 / 12);
                              // A key-up note under sustain must still bend with the held reference.
                              QVERIFY(equal(4096));
                              if (route == 3) {
                                    for (auto* engine : {&reference, &candidate}) {
                                          engine->play(PlayEvent(ME_NOTEON, channel, 62, 90));
                                          engine->play(PlayEvent(ME_NOTEON, channel, 62, 0));
                                          }
                                    }
                              reference.play(PlayEvent(ME_NOTEON, channel, 60, 0));
                              if (pedal) candidate.play(PlayEvent(ME_CONTROLLER, channel, pedal, 0));
                              else candidate.play(PlayEvent(ME_NOTEON, channel, 60, 0));
                              wheel(candidate, 8192);
                              candidate.pitch_wheel_sens(channel, 2);
                              QVERIFY(equal(512));
                              // Recomputing all modulators must not undo the latched tail pitch.
                              candidate.play(PlayEvent(ME_CONTROLLER, channel, 121, 0));
                              QVERIFY(equal(512));
                              reference.play(PlayEvent(ME_NOTEON, 1, 62, 90));
                              candidate.play(PlayEvent(ME_NOTEON, channel, 62, 90));
                              QVERIFY(equal(2048));
                              // Drain old voices, then reuse the pool with centered attacks.
                              reference.allSoundsOff(-1); candidate.allSoundsOff(-1);
                              render(reference, 256); render(candidate, 256);
                              wheel(reference, 8192);
                              reference.pitch_wheel_sens(channel, 12);
                              for (auto* engine : {&reference, &candidate})
                                    engine->play(PlayEvent(ME_NOTEON, channel, 64, 90));
                              QVERIFY(equal(2048));
                              }
                        }
                  }
            }
      }

#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
void TestMidi::fluid2DefaultRouting()
      {
      const QByteArray oldIsolated = qgetenv("MSCORE_FLUID2_ISOLATED_CHANNELS");
      const QByteArray oldGroups = qgetenv("MSCORE_FLUID2_AUDIO_GROUPS");
      const auto restore = qScopeGuard([&]() {
            if (oldIsolated.isNull()) qunsetenv("MSCORE_FLUID2_ISOLATED_CHANNELS");
            else qputenv("MSCORE_FLUID2_ISOLATED_CHANNELS", oldIsolated);
            if (oldGroups.isNull()) qunsetenv("MSCORE_FLUID2_AUDIO_GROUPS");
            else qputenv("MSCORE_FLUID2_AUDIO_GROUPS", oldGroups);
            });
      qunsetenv("MSCORE_FLUID2_ISOLATED_CHANNELS");
      qunsetenv("MSCORE_FLUID2_AUDIO_GROUPS");
      Fluid2 automatic(true);
      QVERIFY(automatic.supportsSourceSends());
      QVERIFY(automatic.setSourceSend(255, .5f));
      QCOMPARE(automatic.sourceSend(255), .5f);
      Fluid2 unchangedDefault;
      QVERIFY(!unchangedDefault.supportsSourceSends());
      qputenv("MSCORE_FLUID2_ISOLATED_CHANNELS", "0");
      Fluid2 explicitlyStereo(true);
      QVERIFY(!explicitlyStereo.supportsSourceSends());
      qputenv("MSCORE_FLUID2_ISOLATED_CHANNELS", "1");
      Fluid2 explicitlyIsolated;
      QVERIFY(explicitlyIsolated.supportsSourceSends());
      }

void TestMidi::fluid2SeekReleasePitch_data()
      {
      fluid2ParameterCompatibility_data();
      }

void TestMidi::fluid2SeekReleasePitch()
      {
      QFETCH(int, mode);
      struct Environment {
            const char* key;
            QByteArray old;
            Environment(const char* key, const QByteArray& value) : key(key), old(qgetenv(key))
                  { if (value.isNull()) qunsetenv(key); else qputenv(key, value); }
            ~Environment() { if (old.isNull()) qunsetenv(key); else qputenv(key, old); }
            } isolated("MSCORE_FLUID2_ISOLATED_CHANNELS", mode == 2 ? QByteArray("1") : QByteArray("0")),
              groups("MSCORE_FLUID2_AUDIO_GROUPS", mode == 1 ? QByteArray("16") : QByteArray()),
              overrideFont("MSCORE_FLUID2_SOUNDFONT", QByteArray());
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString file = directory.filePath("seek-release.sf2");
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(file).constData()));
      Fluid2 synth;
      QVERIFY(synth.loadSoundFonts({file}));
      const auto render = [&](unsigned frames) {
            std::vector<float> dry(frames * 2, 0);
            synth.process(frames, dry.data(), nullptr, nullptr);
            return dry;
            };
      const auto frequency = [&](const std::vector<float>& dry) {
            unsigned crossings = 0;
            for (unsigned n = 4411; n < dry.size() / 2; ++n)
                  if (dry[(n - 1) * 2] <= 0 && dry[n * 2] > 0) ++crossings;
            return crossings * 44100.0 / (dry.size() / 2 - 4411);
            };
      for (int channel : (mode == 2 ? std::vector<int> {0, 128, 255} : std::vector<int> {0, 128})) {
            const auto cc = [&](int controller, int value) { synth.playWithSource(PlayEvent(ME_CONTROLLER, channel, controller, value), 0); };
            for (int pitch : {69, 72}) { // D5's A4 and D7's C5
                  synth.allSoundsOff(-1); render(512);
                  cc(121, 0);
                  synth.playWithSource(PlayEvent(ME_NOTEON, channel, pitch, 90), 0);
                  render(8820);
                  for (unsigned seek = 0; seek < 4; ++seek) {
                        // Include a voice outside the destination's mapping:
                        // the host-wide hard stop must cover that synth state.
                        synth.playWithSource(PlayEvent(ME_NOTEON, 42, 57, 80), 0);
                        synth.allNotesOff(-1); // stopNotes releases, but does not finish voices
                        // Seq::setPos sends CC120 to MIDI mappings and finishes
                        // every internal synth before controller-state replay.
                        cc(CTRL_ALL_SOUNDS_OFF, 0);
                        synth.allSoundsOff(-1);
                        // Rewind through D4's +7 and D5's reset. CC121 clears
                        // channel offsets; a released voice can retain +7.
                        cc(99, 120); cc(98, 51); cc(38, 7); cc(6, 64); cc(121, 0);
                        const auto silence = render(2205);
                        for (unsigned i = 512; i < silence.size(); ++i)
                              QVERIFY(std::isfinite(silence[i]) && std::abs(silence[i]) < .000001f);
                        synth.playWithSource(PlayEvent(ME_NOTEON, channel, pitch, 90), 0);
                        const double expected = 440 * std::pow(2.0, (pitch - 69) / 12.0);
                        QVERIFY(std::abs(frequency(render(13230)) - expected) < 6);
                        }
                  }
            }
      }

void TestMidi::fluid2OutputLevelCompatibility_data()
      {
      fluid2ParameterCompatibility_data();
      }

void TestMidi::fluid2OutputLevelCompatibility()
      {
      QFETCH(int, mode);
      struct Environment {
            const char* key;
            QByteArray old;
            Environment(const char* key, const QByteArray& value) : key(key), old(qgetenv(key))
                  { if (value.isNull()) qunsetenv(key); else qputenv(key, value); }
            ~Environment() { if (old.isNull()) qunsetenv(key); else qputenv(key, old); }
            } isolated("MSCORE_FLUID2_ISOLATED_CHANNELS", mode == 2 ? QByteArray("1") : QByteArray("0")),
              groups("MSCORE_FLUID2_AUDIO_GROUPS", mode == 1 ? QByteArray("16") : QByteArray()),
              overrideFont("MSCORE_FLUID2_SOUNDFONT", QByteArray());
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString file = directory.filePath("levels.sf2");
      // Legacy's loader expects padding around a loop. Default fixtures retain
      // their original bytes; this comparison uses a separate padded variant.
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(file).constData(), nullptr, true, -1, 69, 0, 0, false, nullptr, 8));
      Fluid2 modern;
      QVERIFY(modern.loadSoundFonts({file}));
      for (int rate : {44100, 48000, 44100}) {
            modern.init(rate);
            FluidS::Fluid legacy;
            legacy.init(rate);
            QVERIFY(legacy.addSoundFont(file));
            const auto send = [&](const PlayEvent& event) { legacy.play(event); modern.playWithSource(event, 0); };
            const auto render = [&](unsigned frames, std::vector<float>& oldDry, std::vector<float>& newDry) {
                  oldDry.assign(frames * 2, 0); newDry.assign(frames * 2, 0);
                  std::vector<float> oldA(128, 0), oldB(128, 0);
                  for (unsigned start = 0; start < frames; start += 64) {
                        const unsigned count = std::min(64u, frames - start);
                        std::fill(oldA.begin(), oldA.end(), 0); std::fill(oldB.begin(), oldB.end(), 0);
                        // Legacy writes these effect buses even for zero sends.
                        legacy.process(count, oldDry.data() + start * 2, oldA.data(), oldB.data());
                        modern.process(count, newDry.data() + start * 2, nullptr, nullptr);
                        }
                  };
            for (int channel : (mode == 2 ? std::vector<int> {0, 128, 255} : std::vector<int> {0, 128})) {
                  for (int velocity : {64, 127}) {
                        for (bool chord : {false, true}) {
                              legacy.allSoundsOff(-1); modern.allSoundsOff(-1);
                              std::vector<float> oldDry, newDry;
                              render(512, oldDry, newDry);
                              for (const auto& cc : { std::make_pair(int(CTRL_PROGRAM), 0), std::make_pair(7, 100),
                                    std::make_pair(10, 64), std::make_pair(11, 127), std::make_pair(91, 0), std::make_pair(93, 0) })
                                    send(PlayEvent(ME_CONTROLLER, channel, cc.first, cc.second));
                              for (int pitch : (chord ? std::vector<int> {60, 64, 67, 72} : std::vector<int> {69}))
                                    send(PlayEvent(ME_NOTEON, channel, pitch, velocity));
                              render(rate / 2, oldDry, newDry);
                              double oldEnergy = 0, newEnergy = 0;
                              for (unsigned i = rate / 5; i < oldDry.size(); ++i) {
                                    QVERIFY(std::isfinite(oldDry[i]) && std::isfinite(newDry[i]));
                                    oldEnergy += oldDry[i] * oldDry[i]; newEnergy += newDry[i] * newDry[i];
                                    }
                              QVERIFY(oldEnergy > .001 && newEnergy > .001);
                              const double ratio = std::sqrt(newEnergy / oldEnergy);
                              // Different DSP/envelopes need not be sample-identical.
                              // Catch a backend gain reduction, not timbral details.
                              QVERIFY2(ratio > .90 && ratio < 1.10, qPrintable(QString("new/legacy RMS ratio %1 at %2 Hz, channel %3").arg(ratio).arg(rate).arg(channel)));
                              }
                        }
                  }
            }
      }

void TestMidi::fluid2PortamentoPlayback_data()
      {
      fluid2ParameterCompatibility_data();
      }

void TestMidi::fluid2PortamentoPlayback()
      {
      QFETCH(int, mode);
      struct Environment {
            const char* key;
            QByteArray old;
            Environment(const char* key, const QByteArray& value) : key(key), old(qgetenv(key))
                  { if (value.isNull()) qunsetenv(key); else qputenv(key, value); }
            ~Environment() { if (old.isNull()) qunsetenv(key); else qputenv(key, old); }
            } isolated("MSCORE_FLUID2_ISOLATED_CHANNELS", mode == 2 ? QByteArray("1") : QByteArray("0")),
              groups("MSCORE_FLUID2_AUDIO_GROUPS", mode == 1 ? QByteArray("16") : QByteArray()),
              overrideFont("MSCORE_FLUID2_SOUNDFONT", QByteArray());
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString file = directory.filePath("portamento.sf2");
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(file).constData()));
      for (int channel : (mode == 2 ? std::vector<int> {0, 128, 255} : std::vector<int> {0, 128})) {
            EventMap events;
            // Match the renderer: notes exist before the named action is added.
            for (const auto& note : { std::make_pair(7680, 69), std::make_pair(9600, 72), std::make_pair(11520, 72) }) {
                  events.emplace(note.first, NPlayEvent(ME_NOTEON, channel, note.second, 90));
                  events.emplace(note.first + 959, NPlayEvent(ME_NOTEON, channel, note.second, 0));
                  }
            events.emplace(7680, NPlayEvent(ME_CONTROLLER, channel, 121, 0));
            for (const auto& cc : { std::make_pair(5, 4), std::make_pair(37, 0), std::make_pair(65, 0), std::make_pair(84, 60) })
                  events.insertBeforeNoteOn(9600, NPlayEvent(ME_CONTROLLER, channel, cc.first, cc.second));
            for (int start : {7680, 9600, 11520}) {
                  Fluid2 synth;
                  QVERIFY(synth.loadSoundFonts({file}));
                  // The same predicate is used by Seq::updateSynthesizerState:
                  // chase persistent settings, never a past one-shot key.
                  for (auto it = events.begin(); it != events.upper_bound(start); ++it)
                        if (it->second.isStatefulController()) synth.playWithSource(it->second, 0);
                  const auto frame = [](int tick) { return int(std::llround(tick * 44100.0 / 768)); }; // quarter=96, PPQ480
                  const int firstFrame = frame(start), endFrame = frame(11520 + 768);
                  std::vector<float> dry((endFrame - firstFrame) * 2, 0);
                  int position = firstFrame;
                  for (auto it = events.lower_bound(start); it != events.end() && it->first <= 11520; ++it) {
                        const int next = frame(it->first);
                        if (next > position) synth.process(next - position, dry.data() + 2 * (position - firstFrame), nullptr, nullptr);
                        synth.playWithSource(it->second, 0);
                        position = next;
                        }
                  synth.process(endFrame - position, dry.data() + 2 * (position - firstFrame), nullptr, nullptr);
                  const auto frequency = [&](int tick, double from, double to) {
                        const int base = frame(tick) - firstFrame;
                        unsigned crossings = 0;
                        for (int n = base + int(from * 44100) + 1; n < base + int(to * 44100); ++n) {
                              if (dry[(n - 1) * 2] <= 0 && dry[n * 2] > 0) ++crossings;
                              }
                        return crossings / (to - from);
                        };
                  if (start <= 9600) {
                        QVERIFY(std::abs(frequency(9600, .04, .12) - 261.626 * std::pow(2.0, .08 / .512)) < 20);
                        QVERIFY(std::abs(frequency(9600, .24, .32) - 261.626 * std::pow(2.0, .28 / .512)) < 20);
                        QVERIFY(std::abs(frequency(9600, .65, .85) - 523.251) < 8);
                        }
                  QVERIFY(std::abs(frequency(11520, .04, .12) - 523.251) < 20);
                  QVERIFY(std::abs(frequency(11520, .65, .85) - 523.251) < 8);
                  }
            }
      }

void TestMidi::fluid2ParameterCompatibility_data()
      {
      QTest::addColumn<int>("mode");
      QTest::newRow("stereo") << 0;
      QTest::newRow("raw-groups") << 1;
      QTest::newRow("isolated-banks") << 2;
      }

void TestMidi::fluid2ParameterCompatibility()
      {
      QFETCH(int, mode);
      struct Environment {
            const char* key;
            QByteArray old;
            Environment(const char* key, const QByteArray& value) : key(key), old(qgetenv(key))
                  { if (value.isNull()) qunsetenv(key); else qputenv(key, value); }
            ~Environment() { if (old.isNull()) qunsetenv(key); else qputenv(key, old); }
            } isolated("MSCORE_FLUID2_ISOLATED_CHANNELS", mode == 2 ? QByteArray("1") : QByteArray("0")),
              groups("MSCORE_FLUID2_AUDIO_GROUPS", mode == 1 ? QByteArray("16") : QByteArray()),
              overrideFont("MSCORE_FLUID2_SOUNDFONT", QByteArray());
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString file = directory.filePath("parameters.sf2");
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(file).constData()));
      Fluid2 synth;
      QVERIFY(synth.loadSoundFonts({file}));
      for (int channel : (mode == 2 ? std::vector<int> {0, 128, 255} : std::vector<int> {0, 128})) {
            const auto play = [&](const PlayEvent& event) { synth.playWithSource(event, 0); };
            const auto cc = [&](int controller, int value) { play(PlayEvent(ME_CONTROLLER, channel, controller, value)); };
            const auto pitch = [&](double expected) {
                  const unsigned frames = unsigned(synth.sampleRate() / 2);
                  std::vector<float> dry(frames * 2, 0), send = dry;
                  synth.processWithAux(frames, dry.data(), nullptr, nullptr, send.data());
                  unsigned crossings = 0;
                  for (unsigned n = frames / 5 + 1; n < frames; ++n) {
                        if (dry[(n - 1) * 2] <= 0 && dry[n * 2] > 0) ++crossings;
                        if (mode == 2 && std::abs(send[n * 2] - .375f * dry[n * 2]) > .00001f) return false;
                        }
                  return std::abs(crossings * synth.sampleRate() / (frames - frames / 5) - expected) < 6;
                  };
            synth.allSoundsOff(-1);
            std::vector<float> drain(512, 0); synth.process(256, drain.data(), nullptr, nullptr);
            if (mode == 2) QVERIFY(synth.setSourceSend(0, .375f));
            cc(121, 0); cc(99, 120); cc(98, 51); cc(38, 12); cc(6, 64);
            play(PlayEvent(ME_NOTEON, channel, 69, 90)); QVERIFY(pitch(880));
            cc(38, 0); cc(6, 64); QVERIFY(pitch(440));
            cc(38, 7); cc(6, 64); QVERIFY(pitch(440 * std::pow(2.0, 7.0 / 12)));
            synth.init(48000);
            play(PlayEvent(ME_NOTEON, channel, 69, 90)); QVERIFY(pitch(440 * std::pow(2.0, 7.0 / 12)));
            cc(38, 0); cc(6, 64); QVERIFY(pitch(440));
            cc(101, 0); cc(100, 2); cc(6, 76); QVERIFY(pitch(880));
            cc(101, 127); cc(100, 127); cc(6, 64); QVERIFY(pitch(880));
            cc(121, 0); cc(6, 96);
            // Like legacy Fluid, CC121 clears channel NRPN offsets for new
            // notes; it need not retune an already sounding NRPN voice.
            synth.allSoundsOff(-1);
            std::fill(drain.begin(), drain.end(), 0); synth.process(256, drain.data(), nullptr, nullptr);
            play(PlayEvent(ME_NOTEON, channel, 69, 90)); QVERIFY(pitch(440));
            synth.init(44100);
            }
      }

void TestMidi::fluid2ModulationCompatibility_data()
      {
      QTest::addColumn<int>("mode");
      QTest::newRow("stereo") << 0;
      QTest::newRow("raw-groups") << 1;
      QTest::newRow("isolated-banks") << 2;
      }

void TestMidi::fluid2ModulationCompatibility()
      {
      QFETCH(int, mode);
      struct Environment {
            const char* key;
            QByteArray old;
            Environment(const char* key, const QByteArray& value) : key(key), old(qgetenv(key))
                  { if (value.isNull()) qunsetenv(key); else qputenv(key, value); }
            ~Environment() { if (old.isNull()) qunsetenv(key); else qputenv(key, old); }
            } isolated("MSCORE_FLUID2_ISOLATED_CHANNELS", mode == 2 ? QByteArray("1") : QByteArray("0")),
              groups("MSCORE_FLUID2_AUDIO_GROUPS", mode == 1 ? QByteArray("16") : QByteArray()),
              overrideFont("MSCORE_FLUID2_SOUNDFONT", QByteArray());
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      Fluid2 synth;
      for (int channel : (mode == 2 ? std::vector<int> {0, 128, 255} : std::vector<int> {0, 128})) {
            const QString file = directory.filePath(QString("pan-%1.sf2").arg(channel));
            const auto play = [&](const PlayEvent& event) { synth.playWithSource(event, 0); };
            const auto cc = [&](int controller, int value) { play(PlayEvent(ME_CONTROLLER, channel, controller, value)); };
            synth.allSoundsOff(-1);
            std::vector<float> drain(512, 0); synth.process(256, drain.data(), nullptr, nullptr);
            Fluid2Test::Modulation spec; spec.pan = 500;
            QVERIFY(Fluid2Test::writeFont(QFile::encodeName(file).constData(), nullptr, true, -1, 69, 0, 0, false, &spec));
            QVERIFY(synth.loadSoundFonts({file}));
            cc(121, 0); cc(10, 0);
            if (mode == 2) QVERIFY(synth.setSourceSend(0, .375f));
            synth.playWithSource(PlayEvent(ME_NOTEON, channel, 69, 90), 0);
            const auto channels = [&](bool left) {
                  std::vector<float> dry(8192, 0), send = dry;
                  synth.processWithAux(4096, dry.data(), nullptr, nullptr, send.data());
                  double wanted = 0, other = 0;
                  for (unsigned i = 0; i < 4096; ++i) {
                        wanted += dry[i * 2 + (left ? 0 : 1)] * dry[i * 2 + (left ? 0 : 1)];
                        other += dry[i * 2 + (left ? 1 : 0)] * dry[i * 2 + (left ? 1 : 0)];
                        if (mode == 2) for (unsigned side = 0; side < 2; ++side)
                              if (std::abs(send[i * 2 + side] - .375f * dry[i * 2 + side]) >= .00001f) return false;
                        }
                  return wanted > .000001 && other < .001 * wanted;
                  };
            QVERIFY(channels(true)); // a sample initially panned right can move fully left
            cc(10, 127);
            std::vector<float> settle(1024, 0); synth.process(512, settle.data(), nullptr, nullptr);
            QVERIFY(channels(false));
            spec.panAmount = 0;
            synth.allSoundsOff(-1);
            const QString authored = directory.filePath(QString("authored-%1.sf2").arg(channel));
            QVERIFY(Fluid2Test::writeFont(QFile::encodeName(authored).constData(), nullptr, true, -1, 69, 0, 0, false, &spec));
            QVERIFY(synth.loadSoundFonts({authored}));
            cc(10, 0); synth.playWithSource(PlayEvent(ME_NOTEON, channel, 69, 90), 0);
            QVERIFY(channels(false)); // authored modulator can intentionally disable default pan
            for (int source : {10, 13, 0x88}) {
                  spec = Fluid2Test::Modulation(); spec.pitchSource = source;
                  synth.allSoundsOff(-1);
                  const QString pressure = directory.filePath(QString("pressure-%1-%2.sf2").arg(channel).arg(source));
                  QVERIFY(Fluid2Test::writeFont(QFile::encodeName(pressure).constData(), nullptr, true, -1, 69, 0, 0, false, &spec));
                  QVERIFY(synth.loadSoundFonts({pressure}));
                  cc(121, 0); play(PlayEvent(ME_NOTEON, channel, 69, 90));
                  const auto frequency = [&]() {
                        std::vector<float> dry(44100, 0);
                        synth.process(22050, dry.data(), nullptr, nullptr);
                        unsigned crossings = 0;
                        for (unsigned n = 4411; n < 22050; ++n)
                              if (dry[(n - 1) * 2] <= 0 && dry[n * 2] > 0) ++crossings;
                        return crossings * 44100.0 / (22050 - 4411);
                        };
                  for (int value : {0, 80, 127, 0}) {
                        if (source == 0x88) cc(8, value);
                        else play(source == 10 ? PlayEvent(ME_POLYAFTER, channel, 69, value) : PlayEvent(ME_AFTERTOUCH, channel, value, 0));
                        QVERIFY(std::abs(frequency() - 440 * std::pow(2.0, value / 127.0)) < 8);
                        }
                  if (source == 10) {
                        play(PlayEvent(ME_POLYAFTER, channel, 72, 127));
                        QVERIFY(std::abs(frequency() - 440) < 5);
                        }
                  else if (source == 13) {
                        cc(CTRL_PRESS, 127);
                        QVERIFY(std::abs(frequency() - 880) < 8);
                        }
                  cc(121, 0);
                  QVERIFY(std::abs(frequency() - 440 * std::pow(2.0, source == 10 ? 80.0 / 127 : 0)) < 8);
                  }
            }
      }

void TestMidi::fluid2ControllerCompatibility_data()
      {
      QTest::addColumn<int>("mode");
      QTest::newRow("stereo") << 0;
      QTest::newRow("raw-groups") << 1;
      QTest::newRow("isolated-banks") << 2;
      }

void TestMidi::fluid2ControllerCompatibility()
      {
      QFETCH(int, mode);
      struct Environment {
            const char* key;
            QByteArray old;
            Environment(const char* key, const QByteArray& value) : key(key), old(qgetenv(key))
                  { if (value.isNull()) qunsetenv(key); else qputenv(key, value); }
            ~Environment() { if (old.isNull()) qunsetenv(key); else qputenv(key, old); }
            } isolated("MSCORE_FLUID2_ISOLATED_CHANNELS", mode == 2 ? QByteArray("1") : QByteArray("0")),
              groups("MSCORE_FLUID2_AUDIO_GROUPS", mode == 1 ? QByteArray("16") : QByteArray()),
              overrideFont("MSCORE_FLUID2_SOUNDFONT", QByteArray());
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString base = directory.filePath("base.sf2"), overlay = directory.filePath("overlay.sf2");
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(base).constData(), nullptr, true, 7, 69, 12, 1));
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(overlay).constData(), nullptr, true, 7, 81));
      Fluid2 synth;
      QVERIFY(synth.loadSoundFonts({base, overlay}));
      for (int channel : (mode == 2 ? std::vector<int> {0, 128, 255} : std::vector<int> {0, 128})) {
            const auto cc = [&](int controller, int value) { synth.play(PlayEvent(ME_CONTROLLER, channel, controller, value)); };
            const auto program = [&](int value) { cc(CTRL_PROGRAM, value); };
            const auto frequency = [&]() {
                  synth.allSoundsOff(-1);
                  std::vector<float> drain(512, 0);
                  synth.process(256, drain.data(), nullptr, nullptr);
                  synth.play(PlayEvent(ME_NOTEON, channel, 69, 90));
                  std::vector<float> dry(44100, 0);
                  synth.process(22050, dry.data(), nullptr, nullptr);
                  unsigned crossings = 0;
                  for (unsigned n = 4411; n < 22050; ++n)
                        if (dry[(n - 1) * 2] <= 0 && dry[n * 2] > 0) ++crossings;
                  return crossings * 44100.0 / (22050 - 4411);
                  };
            cc(CTRL_HBANK, 0); cc(CTRL_LBANK, 0); program(0);
            synth.play(PlayEvent(ME_PITCHBEND, channel, 0, 96)); // half wheel = +6 semitones
            QVERIFY(std::abs(frequency() - 440 * std::sqrt(2.0)) < 5);
            cc(101, 0); cc(100, 0); cc(6, 2);
            QVERIFY(std::abs(frequency() - 440 * std::pow(2.0, 1.0 / 12)) < 5);
            cc(7, 45); cc(10, 127); cc(11, 40);
            cc(121, 0);
            QVERIFY(std::abs(frequency() - 440) < 5); // reset centers bend/pan and restores expression
            synth.play(PlayEvent(ME_PITCHBEND, channel, 0, 96));
            QVERIFY(std::abs(frequency() - 440 * std::sqrt(2.0)) < 5); // range returned to 12
            cc(121, 0);
            cc(CTRL_HBANK, 0); cc(CTRL_LBANK, 1); program(7);
            QVERIFY(std::abs(frequency() - 880) < 5); // real native bank 1 in the top font
            cc(CTRL_HBANK, 1); program(7); // MSB clears LSB: bank 128, missing program -> top piano
            QVERIFY(std::abs(frequency() - 440) < 5); // retaining LSB would select lower font at bank 129
            cc(CTRL_LBANK, 1); program(0);
            QVERIFY(std::abs(frequency() - 220) < 5);
            cc(121, 0); program(0);
            QVERIFY(std::abs(frequency() - 220) < 5); // reset retains selected host bank/program
            cc(CTRL_LBANK, 1); program(7);
            QVERIFY(std::abs(frequency() - 880) < 5); // reset cleared pending MSB, as legacy does
            }
      }

void TestMidi::fluid2SoundFontDiscovery()
      {
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString shared = directory.filePath("shared"), user = directory.filePath("user"),
            extension = directory.filePath("extension"), build = directory.filePath("source/sound");
      const auto file = [&](const QString& name) {
            QDir().mkpath(QFileInfo(name).absolutePath());
            QFile output(name);
            return output.open(QIODevice::WriteOnly);
            };
      const QString general = shared + "/sound/MuseScore_General.sf3", nested = user + "/nested/piano.SF2",
            extra = extension + "/organ.sf", mono = build + "/FluidR3Mono_GM.sf3";
      QVERIFY(file(general) && file(nested) && file(extra) && file(mono));
      QVERIFY(file(user + "/ignored.sfz") && file(user + "/.hidden/hidden.sf2"));
#ifdef Q_OS_UNIX
      QVERIFY(QFile::link(user, user + "/nested/cycle"));
      QVERIFY(QFile::link(nested, user + "/alias.sf2"));
#endif
      const auto found = SoundFontPaths::discover(shared, {user, "", user + "/nested"}, {extension}, build);
      QSet<QString> identities;
      for (const QFileInfo& file : found) identities.insert(file.canonicalFilePath());
      QCOMPARE(identities.size(), 4);
      QVERIFY(SoundFontPaths::resolve("MuseScore_General.sf3", found) == QFileInfo(general).canonicalFilePath());
      QVERIFY(SoundFontPaths::resolve("piano.SF2", found) == QFileInfo(nested).canonicalFilePath());
      QVERIFY(SoundFontPaths::resolve("missing.sf2", found).isEmpty());
      QVERIFY(SoundFontPaths::resolve("MuseScore_General.sf3", {QFileInfo(mono)}) == QFileInfo(mono).canonicalFilePath());
      QCOMPARE(SoundFontPaths::discover(shared + "/", {user}, {extension}, build).size(), found.size());
      const QString cwd = QDir::currentPath();
      struct RestoreDirectory { QString previous; ~RestoreDirectory() { QDir::setCurrent(previous); } } restore { cwd };
      QVERIFY(QDir::setCurrent(user));
      QVERIFY(SoundFontPaths::discover(QString(), {}, {}).isEmpty());
      }

void TestMidi::fluid2DefaultSoundFont()
      {
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      struct Environment {
            const char* key;
            QByteArray old;
            Environment(const char* key) : key(key), old(qgetenv(key)) { qunsetenv(key); }
            ~Environment() { if (old.isNull()) qunsetenv(key); else qputenv(key, old); }
            } overrideFont("MSCORE_FLUID2_SOUNDFONT");
      const QString shared = directory.filePath("shared"), user = directory.filePath("user"),
            general = shared + "/sound/MuseScore_General.sf3", nested = user + "/nested/Extra.sf2";
      QVERIFY(QDir().mkpath(QFileInfo(general).absolutePath()) && QDir().mkpath(QFileInfo(nested).absolutePath()));
      // Naming is independent of the RIFF format; codec tests use true SF3 elsewhere.
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(general).constData()));
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(nested).constData(), nullptr, true, 7, 81));
      QScopedValueRollback<QString> share(mscoreGlobalShare, shared), data(dataPath, directory.path());
      struct Preference {
            QString old = preferences.getString(PREF_APP_PATHS_MYSOUNDFONTS);
            explicit Preference(const QString& value) { preferences.setTemporaryPreference(PREF_APP_PATHS_MYSOUNDFONTS, value); }
            ~Preference() { preferences.setTemporaryPreference(PREF_APP_PATHS_MYSOUNDFONTS, old); }
            } preference(user);
      const auto available = FluidS::Fluid::sfFiles();
      QVERIFY(SoundFontPaths::resolve("MuseScore_General.sf3", available) == QFileInfo(general).canonicalFilePath());
      QVERIFY(SoundFontPaths::resolve("Extra.sf2", available) == QFileInfo(nested).canonicalFilePath());
      {
            MasterSynthesizer master;
            auto* fluid = new Fluid2;
            master.registerSynthesizer(fluid);
            master.init(); // No stored defaults: recover the normal General font.
            QCOMPARE(fluid->soundFontsInfo().size(), size_t(1));
            QCOMPARE(fluid->soundFontsInfo().front().fileName, QFileInfo(general).canonicalFilePath());
            }
      QFile settings(directory.filePath("synthesizer.xml"));
      QVERIFY(settings.open(QIODevice::WriteOnly));
      settings.write("<Synthesizer><Fluid/><AuxiliaryReverb><val id=\"9\">0.25</val></AuxiliaryReverb></Synthesizer>");
      settings.close();
      {
            MasterSynthesizer master;
            auto* fluid = new Fluid2;
            master.registerSynthesizer(fluid);
            master.init(); // Empty saved defaults must not leave every synth silent.
            QCOMPARE(fluid->soundFontsInfo().size(), size_t(1));
            QCOMPARE(master.auxiliaryReverbValues()[AuxiliaryReverbSettings::ReturnLevel], .25f);
            QVERIFY(fluid->loadSoundFonts({}));
            QVERIFY(fluid->soundFontsInfo().empty()); // Explicit removal still works.
            QVERIFY(master.ensureSoundFonts());
            }
      struct LoadedSfz final : Synthesizer {
            const char* name() const override { return "Zerberus"; }
            bool loadSoundFonts(const QStringList&) override { return true; }
            std::vector<SoundFontInfo> soundFontsInfo() const override { return { SoundFontInfo("fixture.sfz") }; }
            void process(unsigned, float*, float*, float*) override {}
            void play(const PlayEvent&) override {}
            const QList<MidiPatch*>& getPatchInfo() const override { static const QList<MidiPatch*> empty; return empty; }
            SynthesizerGroup state() const override { return {}; }
            bool setState(const SynthesizerGroup&) override { return true; }
            };
      {
            MasterSynthesizer master;
            auto* fluid = new Fluid2;
            master.registerSynthesizer(fluid);
            master.registerSynthesizer(new LoadedSfz);
            QVERIFY(master.ensureSoundFonts());
            QVERIFY(fluid->soundFontsInfo().empty()); // Existing SFZ-only configuration wins.
            }
      Fluid2 fluid;
      QVERIFY(fluid.loadDefaultSoundFonts());
      std::unique_ptr<SynthesizerGui> gui(fluid.gui());
      auto* add = gui->findChild<QPushButton*>("fluid2FontAdd");
      QVERIFY(add);
      bool sawGeneral = false, sawNested = false, chose = false;
      bool cancel = false;
      QTimer choose;
      connect(&choose, &QTimer::timeout, [&]() {
            for (QWidget* widget : QApplication::topLevelWidgets()) {
                  if (widget->objectName() != "fluid2SoundFontFilesDialog") continue;
                  auto* dialog = qobject_cast<QDialog*>(widget);
                  auto* list = widget->findChild<QListWidget*>("fluid2AvailableSoundFonts");
                  if (!dialog || !list) continue;
                  if (cancel) { dialog->reject(); return; }
                  for (int row = 0; row < list->count(); ++row) {
                        const QString path = list->item(row)->data(Qt::UserRole).toString();
                        sawGeneral |= path == QFileInfo(general).canonicalFilePath();
                        sawNested |= path == QFileInfo(nested).canonicalFilePath();
                        if (path == QFileInfo(nested).canonicalFilePath()) { list->setCurrentRow(row); chose = true; }
                        }
                  dialog->accept();
                  }
            });
      choose.start(1);
      add->click();
      QVERIFY(sawGeneral && sawNested && chose);
      QCOMPARE(fluid.soundFontsInfo().size(), size_t(2));
      QCOMPARE(fluid.soundFontsInfo().front().fileName, QFileInfo(nested).canonicalFilePath());
      cancel = true;
      add->click();
      choose.stop();
      QCOMPARE(fluid.soundFontsInfo().size(), size_t(2));
      }

void TestMidi::fluid2SoundFontManagement_data()
      {
      QTest::addColumn<int>("mode");
      QTest::newRow("stereo") << 0;
      QTest::newRow("raw-groups") << 1;
      QTest::newRow("isolated-banks") << 2;
      }

void TestMidi::fluid2SoundFontManagement()
      {
      QFETCH(int, mode);
      struct Environment {
            const char* key;
            QByteArray old;
            Environment(const char* key, const QByteArray& value) : key(key), old(qgetenv(key))
                  { if (value.isNull()) qunsetenv(key); else qputenv(key, value); }
            ~Environment() { if (old.isNull()) qunsetenv(key); else qputenv(key, old); }
            } isolated("MSCORE_FLUID2_ISOLATED_CHANNELS", mode == 2 ? QByteArray("1") : QByteArray("0")),
              groups("MSCORE_FLUID2_AUDIO_GROUPS", mode == 1 ? QByteArray("16") : QByteArray()),
              overrideFont("MSCORE_FLUID2_SOUNDFONT", QByteArray());
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString base = directory.filePath("base.sf2"), overlay = directory.filePath("overlay.sf3"),
            moved = directory.filePath("moved.sf3"), empty = directory.filePath("empty.sf2");
      QFile sample(QString(TESTROOT) + "/mtest/audio/fluid2/fixtures/sine.ogg");
      QVERIFY(sample.open(QIODevice::ReadOnly));
      const QByteArray bytes = sample.readAll();
      const Fluid2Test::Bytes compressed(bytes.begin(), bytes.end());
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(base).constData()));
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(overlay).constData(), &compressed, true, 7, 81));
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(empty).constData(), nullptr, false));
      const QString basePath = QFileInfo(base).canonicalFilePath(), overlayPath = QFileInfo(overlay).canonicalFilePath();
      Fluid2 synth;
      QVERIFY(synth.loadSoundFonts({base, overlay, base}));
      const auto paths = [&]() {
            QStringList result;
            for (const SoundFontInfo& info : synth.soundFontsInfo()) result.append(info.fileName);
            return result;
            };
      QVERIFY(paths() == QStringList({basePath, overlayPath}));
      QCOMPARE(synth.getPatchInfo().size(), 5); // Both fonts' duplicate presets stay independently selectable.
      bool lowerPiano = false, lowerDrum = false;
      for (const MidiPatch* patch : synth.getPatchInfo()) {
            if (patch->bank == 129 && patch->prog == 0)
                  lowerPiano = patch->sfid == 1 && patch->name == "Overlay" && !patch->drum;
            if (patch->bank == 257 && patch->prog == 0)
                  lowerDrum = patch->sfid == 1 && patch->drum;
            }
      QVERIFY(lowerPiano && lowerDrum);
#ifdef Q_OS_UNIX
      const QString alias = directory.filePath("alias.sf2");
      QVERIFY(QFile::link(base, alias));
      QVERIFY(synth.loadSoundFonts({base, alias, overlay}));
      QVERIFY(paths() == QStringList({basePath, overlayPath}));
#endif
      const auto piano = [&]() -> const MidiPatch* {
            for (const MidiPatch* patch : synth.getPatchInfo()) if (patch->bank == 0 && patch->prog == 0) return patch;
            return nullptr;
            };
      QVERIFY(piano());
      QCOMPARE(piano()->name, QString("Sine"));
      const MidiPatch* accepted = piano();
      QVERIFY(!synth.loadSoundFonts({base, empty, overlay}));
      QVERIFY(piano() == accepted && paths() == QStringList({basePath, overlayPath}));
      synth.setMasterTuning(442);
      if (mode == 2) QVERIFY(synth.setSourceSend(0, .375f));
      synth.init(48000);
      QVERIFY(paths() == QStringList({basePath, overlayPath}));
      QCOMPARE(synth.masterTuning(), 442.0);
      if (mode == 2) QCOMPARE(synth.sourceSend(0), .375f);
      QVERIFY(synth.addSoundFont(overlay));
      QVERIFY(paths() == QStringList({overlayPath, basePath}));
      QCOMPARE(piano()->name, QString("Overlay"));
      QVERIFY(!synth.removeSoundFont("unknown.sf2"));
      QVERIFY(synth.removeSoundFont(base));
      QVERIFY(paths() == QStringList({overlayPath}));
      QVERIFY(synth.addSoundFont(base));
      QVERIFY(paths() == QStringList({basePath, overlayPath}));
      QVERIFY(QDir(directory.path()).mkdir("first"));
      QVERIFY(QDir(directory.path()).mkdir("second"));
      const QString first = directory.filePath("first/same.sf2"), second = directory.filePath("second/same.sf2");
      QVERIFY(QFile::copy(base, first) && QFile::copy(base, second));
      QVERIFY(synth.loadSoundFonts({first, second}));
      QVERIFY(!synth.removeSoundFont("same.sf2"));
      QCOMPARE(synth.soundFontsInfo().size(), size_t(2));
      QVERIFY(synth.removeSoundFont(first));
      QVERIFY(paths() == QStringList({QFileInfo(second).canonicalFilePath()}));
      QVERIFY(synth.loadSoundFonts({base, overlay}));

      SynthesizerState state;
      state.push_back(synth.state());
      state.setIsDefault(false);
      QByteArray xml;
      QBuffer buffer(&xml);
      QVERIFY(buffer.open(QIODevice::WriteOnly));
      { XmlWriter writer(nullptr, &buffer); state.write(writer, true); }
      buffer.close();
      XmlReader reader(xml);
      QVERIFY(reader.readNextStartElement());
      SynthesizerState restored;
      restored.read(reader);
      QVERIFY(!reader.hasError());
      QVERIFY(synth.loadSoundFonts({}));
      QVERIFY(synth.setState(restored.group("Fluid")));
      QVERIFY(paths() == QStringList({basePath, overlayPath}));
      // Exercise host bank-select events and audible selection, not just the
      // patch list. These bank numbers are written by the legacy Mixer too.
      const auto frequency = [&](int channel, int bank, int program) {
            synth.allSoundsOff(-1);
            std::vector<float> drain(512, 0);
            synth.process(256, drain.data(), nullptr, nullptr);
            synth.play(PlayEvent(ME_CONTROLLER, channel, CTRL_HBANK, bank >> 7));
            synth.play(PlayEvent(ME_CONTROLLER, channel, CTRL_LBANK, bank & 127));
            synth.play(PlayEvent(ME_CONTROLLER, channel, CTRL_PROGRAM, program));
            synth.play(PlayEvent(ME_NOTEON, channel, 69, 90));
            std::vector<float> dry(48000, 0);
            synth.process(24000, dry.data(), nullptr, nullptr);
            unsigned crossings = 0;
            for (unsigned n = 4801; n < 24000; ++n)
                  if (dry[(n - 1) * 2] <= 0 && dry[n * 2] > 0) ++crossings;
            return crossings * 48000.0 / (24000 - 4801);
            };
      for (int channel : (mode == 2 ? std::vector<int> {0, 128, 255} : std::vector<int> {0, 128})) {
            QVERIFY(std::abs(frequency(channel, 0, 0) - 442) < 5);
            QVERIFY(std::abs(frequency(channel, 129, 0) - 221) < 5);
            QVERIFY(std::abs(frequency(channel, 129, 7) - 221) < 5);
            QVERIFY(std::abs(frequency(channel, 999, 99) - 442) < 5);
            }
      synth.allSoundsOff(-1);
      if (mode == 2) {
            std::unique_ptr<MasterScore> score(readScore(DIR + "audio-reverb-sends.mscx"));
            QVERIFY(score);
            score->parts().front()->instrument()->channel(0)->setBank(129);
            score->parts().front()->instrument()->channel(0)->setProgram(7);
            score->setSynthesizerState(state);
            const QString file = directory.filePath("font-stack.mscx");
            { QScopedValueRollback<bool> testMode(MScore::testMode, false); QVERIFY(saveScore(score.get(), file)); }
            std::unique_ptr<MasterScore> reopened(readCreatedScore(file));
            QVERIFY(reopened);
            QCOMPARE(reopened->parts().front()->instrument()->channel(0)->bank(), 129);
            QCOMPARE(reopened->parts().front()->instrument()->channel(0)->program(), 7);
            QVERIFY(synth.loadSoundFonts({overlay}));
            QVERIFY(synth.setState(reopened->synthesizerState().group("Fluid")));
            QVERIFY(paths() == QStringList({basePath, overlayPath}));
            const Channel* channel = reopened->parts().front()->instrument()->channel(0);
            QVERIFY(std::abs(frequency(0, channel->bank(), channel->program()) - 221) < 5);
            }

      std::unique_ptr<SynthesizerGui> gui(synth.gui());
      auto* list = gui->findChild<QListWidget*>("fluid2SoundFonts");
      auto* up = gui->findChild<QPushButton*>("fluid2FontUp");
      auto* down = gui->findChild<QPushButton*>("fluid2FontDown");
      auto* remove = gui->findChild<QPushButton*>("fluid2FontRemove");
      QVERIFY(list && up && down && remove);
      QSignalSpy changes(gui.get(), SIGNAL(sfChanged())), dirty(gui.get(), SIGNAL(valueChanged()));
      QSignalSpy requests(gui.get(), SIGNAL(soundFontChangeRequested()));
      QVERIFY(changes.isValid() && dirty.isValid() && requests.isValid());
      list->setCurrentRow(1);
      QVERIFY(up->isEnabled());
      up->click();
      QVERIFY(paths() == QStringList({overlayPath, basePath}));
      QCOMPARE(piano()->name, QString("Overlay"));
      QCOMPARE(list->currentRow(), 0);
      QCOMPARE(changes.count(), 1);
      QCOMPARE(dirty.count(), 1);
      QVERIFY(!up->isEnabled() && down->isEnabled());
      QVERIFY(QFile::rename(overlay, moved));
      QTimer dismiss;
      int warnings = 0;
      connect(&dismiss, &QTimer::timeout, [&]() {
            for (QWidget* widget : QApplication::topLevelWidgets()) {
                  if (auto* message = qobject_cast<QMessageBox*>(widget)) { ++warnings; message->accept(); }
                  }
            });
      dismiss.start(1);
      down->click();
      dismiss.stop();
      QCOMPARE(warnings, 1);
      QVERIFY(paths() == QStringList({overlayPath, basePath}));
      QCOMPARE(changes.count(), 1);
      QCOMPARE(dirty.count(), 1);
      remove->click(); // A missing font can still be removed from the accepted list.
      QVERIFY(paths() == QStringList({basePath}));
      QVERIFY(QFile::rename(moved, overlay));
      QCOMPARE(changes.count(), 2);
      QCOMPARE(dirty.count(), 2);
      remove->click();
      QVERIFY(paths().isEmpty() && synth.getPatchInfo().isEmpty());
      QVERIFY(!remove->isEnabled() && !up->isEnabled() && !down->isEnabled());
      QCOMPARE(requests.count(), 4);
      qputenv("MSCORE_FLUID2_SOUNDFONT", QFile::encodeName(base));
      QVERIFY(synth.loadSoundFonts({overlay}));
      QVERIFY(paths() == QStringList({basePath}));
      QVERIFY(!synth.addSoundFont(overlay) && !synth.removeSoundFont(base));
      gui->synthesizerChanged();
      auto* add = gui->findChild<QPushButton*>("fluid2FontAdd");
      QVERIFY(add && !add->isEnabled() && !remove->isEnabled());
      }

void TestMidi::fluid2SampleRateRecovery_data()
      {
      QTest::addColumn<int>("mode");
      QTest::newRow("stereo") << 0;
      QTest::newRow("raw-groups") << 1;
      QTest::newRow("isolated-banks") << 2;
      }

void TestMidi::fluid2SampleRateRecovery()
      {
      QFETCH(int, mode);
      struct Environment {
            const char* key;
            QByteArray previous;
            Environment(const char* key, const QByteArray& value) : key(key), previous(qgetenv(key))
                  { if (value.isNull()) qunsetenv(key); else qputenv(key, value); }
            ~Environment() { if (previous.isNull()) qunsetenv(key); else qputenv(key, previous); }
            } isolated("MSCORE_FLUID2_ISOLATED_CHANNELS", mode == 2 ? QByteArray("1") : QByteArray("0")),
              groups("MSCORE_FLUID2_AUDIO_GROUPS", mode == 1 ? QByteArray("16") : QByteArray()),
              overrideFont("MSCORE_FLUID2_SOUNDFONT", QByteArray());
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString font = directory.filePath("lifecycle.sf2"), moved = directory.filePath("moved.sf2"),
            empty = directory.filePath("empty.sf2");
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(font).constData()));
      QVERIFY(Fluid2Test::writeFont(QFile::encodeName(empty).constData(), nullptr, false));
      Fluid2 synth;
      QVERIFY(synth.loadSoundFonts({font}));
      synth.setMasterTuning(442);
      if (mode == 2) QVERIFY(synth.setSourceSend(0, .375f));
      const int channel = mode == 2 ? 255 : 128;
      const auto silent = [&]() {
            std::vector<float> dry(8192, .125f), send(8192, -.25f);
            synth.processWithAux(4096, dry.data(), nullptr, nullptr, send.data());
            return std::all_of(dry.begin(), dry.end(), [](float v) { return v == .125f; })
                  && std::all_of(send.begin(), send.end(), [](float v) { return v == -.25f; });
            };
      const auto audible = [&]() {
            synth.playWithSource(PlayEvent(ME_NOTEON, channel, 69, 90), 0);
            std::vector<float> dry(8192, 0), send = dry;
            synth.processWithAux(4096, dry.data(), nullptr, nullptr, send.data());
            double energy = 0;
            bool valid = true;
            for (size_t i = 0; i < dry.size(); ++i) {
                  energy += dry[i] * dry[i];
                  valid = valid && std::isfinite(dry[i]) && std::isfinite(send[i]);
                  if (mode == 2) valid = valid && std::abs(send[i] - .375f * dry[i]) < .00001f;
                  }
            synth.allSoundsOff(-1);
            std::fill(dry.begin(), dry.end(), 0); std::fill(send.begin(), send.end(), 0);
            synth.processWithAux(4096, dry.data(), nullptr, nullptr, send.data());
            return valid && energy > .00001;
            };
      QVERIFY(audible());
      synth.init(48000);
      QCOMPARE(synth.sampleRate(), 48000.0f);
      QVERIFY(silent());
      QVERIFY(audible());
      QVERIFY(!synth.getPatchInfo().isEmpty());
      const MidiPatch* acceptedPatch = synth.getPatchInfo().front();
      QVERIFY(QFile::rename(font, moved));
      synth.init(48000); // Same-rate initialization does not reopen the file.
      QVERIFY(synth.getPatchInfo().front() == acceptedPatch);
      synth.playWithSource(PlayEvent(ME_NOTEON, channel, 69, 90), 0);
      synth.playWithSource(PlayEvent(ME_CONTROLLER, channel, CTRL_SOSTENUTO, 127), 0);
      synth.playWithSource(PlayEvent(ME_NOTEON, channel, 69, 0), 0);
      synth.init(44100);
      QVERIFY(!synth.state().empty());
      QCOMPARE(synth.state().front().data, font);
      const auto fonts = synth.soundFontsInfo();
      QCOMPARE(fonts.size(), size_t(1));
      QCOMPARE(fonts.front().fileName, font);
      QVERIFY(!synth.getPatchInfo().isEmpty() && synth.getPatchInfo().front() == acceptedPatch);
      synth.playWithSource(PlayEvent(ME_NOTEON, channel, 72, 90), 0);
      QVERIFY(silent()); // No wrong-rate audio, including newly arriving events.
      synth.init(48000); // Recover the retained engine without its disk file.
      QVERIFY(silent()); // The interrupted LV voice must not resume.
      QVERIFY(audible());
      synth.init(44100);
      QVERIFY(silent());
      QVERIFY(QFile::rename(moved, font));
      synth.init(44100); // The same pending rate retries instead of early-returning.
      QVERIFY(silent());
      QVERIFY(audible());
      QCOMPARE(synth.masterTuning(), 442.0);
      if (mode == 2) QCOMPARE(synth.sourceSend(0), .375f);
      QVERIFY(QFile::rename(font, moved));
      synth.init(48000);
      QVERIFY(silent());
      QVERIFY(!synth.loadSoundFonts({empty}));
      QVERIFY(silent());
      QVERIFY(synth.loadSoundFonts({moved})); // Explicit replacement recovers the pending rate.
      QCOMPARE(synth.sampleRate(), 48000.0f);
      QCOMPARE(synth.state().front().data, moved);
      QVERIFY(silent());
      QVERIFY(audible());
      QVERIFY(QFile::rename(moved, font));
      synth.init(22050);
      QVERIFY(silent());
      QVERIFY(synth.loadSoundFonts({})); // An explicit empty list is a valid recovery.
      QVERIFY(synth.state().empty() && synth.getPatchInfo().isEmpty() && synth.soundFontsInfo().empty());
      QVERIFY(silent());
      QVERIFY(synth.loadSoundFonts({font}));
      QVERIFY(audible());
      }
#endif


void TestMidi::events_data()
      {
      QTest::addColumn<QString>("file");
      // Test Metronome
      QTest::newRow("testMetronomeSimple") <<  "testMetronomeSimple";
      QTest::newRow("testMetronomeCompound") <<  "testMetronomeCompound";
      QTest::newRow("testMetronomeAnacrusis") <<  "testMetronomeAnacrusis";
      // Test Eighth Swing
      QTest::newRow("testSwing8thSimple") <<  "testSwing8thSimple";
      QTest::newRow("testSwing8thTies") <<  "testSwing8thTies";
      QTest::newRow("testSwing8thTriplets") <<  "testSwing8thTriplets";
      QTest::newRow("testSwing8thDots") <<  "testSwing8thDots";
      // Test Sixteenth Swing
      QTest::newRow("testSwing16thSimple") <<  "testSwing16thSimple";
      QTest::newRow("testSwing16thTies") <<  "testSwing16thTies";
      QTest::newRow("testSwing16thTriplets") <<  "testSwing16thTriplets";
      QTest::newRow("testSwing16thDots") <<  "testSwing16thDots";
      QTest::newRow("testSwingOdd") <<  "testSwingOdd";
      QTest::newRow("testSwingPickup") <<  "testSwingPickup";
      // Test Text Cominations
      QTest::newRow("testSwingStyleText") <<  "testSwingStyleText";
//TODO::ws      QTest::newRow("testSwingTexts") <<  "testSwingTexts";
      // ornaments
      QTest::newRow("testMordents") <<  "testMordents";
      //QTest::newRow("testBaroqueOrnaments") << "testBaroqueOrnaments"; // fail, at least a problem with the first note and stretch
      QTest::newRow("testOrnamentAccidentals") << "testOrnamentAccidentals";
//TODO      QTest::newRow("testGraceBefore") <<  "testGraceBefore";
      QTest::newRow("testBeforeAfterGraceTrill") <<  "testBeforeAfterGraceTrill";
      QTest::newRow("testBeforeAfterGraceTrillPlay=false") <<  "testBeforeAfterGraceTrillPlay=false";
      QTest::newRow("testKantataBWV140Excerpts") <<  "testKantataBWV140Excerpts";
      QTest::newRow("testTrillTransposingInstrument") <<  "testTrillTransposingInstrument";
      QTest::newRow("testAndanteExcerpts") <<  "testAndanteExcerpts";
      QTest::newRow("testTrillLines") << "testTrillLines";
      QTest::newRow("testTrillTempos") << "testTrillTempos";
//      QTest::newRow("testTrillCrossStaff") << "testTrillCrossStaff";
      QTest::newRow("testOrnaments") << "testOrnaments";
      QTest::newRow("testOrnamentsTrillsOttava") << "testOrnamentsTrillsOttava";
      QTest::newRow("testTieTrill") << "testTieTrill";
      // glissando
      QTest::newRow("testGlissando") << "testGlissando";
      QTest::newRow("testGlissandoAcrossStaffs") << "testGlissandoAcrossStaffs";
      QTest::newRow("testGlissando-71826") << "testGlissando-71826";
      // pedal
//      QTest::newRow("testPedal") <<  "testPedal";
      // multi note tremolo
      QTest::newRow("testMultiNoteTremolo") << "testMultiNoteTremolo";
      QTest::newRow("testMultiNoteTremoloTuplet") << "testMultiNoteTremoloTuplet";
      // Test Pauses
      QTest::newRow("testPauses") <<  "testPauses";
      QTest::newRow("testPausesRepeats") <<  "testPausesRepeats";
      QTest::newRow("testPausesTempoTimesigChange") <<  "testPausesTempoTimesigChange";
      QTest::newRow("testGuitarTrem") <<  "testGuitarTrem";
      QTest::newRow("testPlayArticulation") << "testPlayArticulation";
      QTest::newRow("testTremoloDynamics") << "testTremoloDynamics";
      QTest::newRow("testRepeatsDynamics") << "testRepeatsDynamics";
      QTest::newRow("testArticulationDynamics") << "testArticulationDynamics";
      QTest::newRow("testChannelsDynamics") << "testChannelsDynamics";
      }

//---------------------------------------------------------
//   saveMidi
//---------------------------------------------------------

bool saveMidi(Score* score, const QString& name)
      {
      ExportMidi em(score);
      return em.write(name, true, true);
      }


//---------------------------------------------------------
//   compareElements
//---------------------------------------------------------

bool compareElements(Element* e1, Element* e2)
      {
      if (e1->type() != e2->type())
            return false;
      if (e1->type() == ElementType::TIMESIG) {
            }
      else if (e1->type() == ElementType::KEYSIG) {
            KeySig* ks1 = static_cast<KeySig*>(e1);
            KeySig* ks2 = static_cast<KeySig*>(e2);
            if (ks1->key() != ks2->key()) {
                  qDebug("      key signature %d  !=  %d", int(ks1->key()), int(ks2->key()));
                  return false;
                  }
            }
      else if (e1->type() == ElementType::CLEF) {
            }
      else if (e1->type() == ElementType::REST) {
            }
      else if (e1->type() == ElementType::CHORD) {
            Ms::Chord* c1 = static_cast<Ms::Chord*>(e1);
            Ms::Chord* c2 = static_cast<Ms::Chord*>(e2);
            if (c1->ticks() != c2->ticks()) {
                  Fraction f1 = c1->ticks();
                  Fraction f2 = c2->ticks();
                  qDebug("      chord duration %d/%d  !=  %d/%d",
                     f1.numerator(), f1.denominator(),
                     f2.numerator(), f2.denominator()
                     );
                  return false;
                  }
            if (c1->notes().size() != c2->notes().size()) {
                  qDebug("      != note count");
                  return false;
                  }
            int n = c1->notes().size();
            for (int i = 0; i < n; ++i) {
                  Note* n1 = c1->notes()[i];
                  Note* n2 = c2->notes()[i];
                  if (n1->pitch() != n2->pitch()) {
                        qDebug("      != pitch note %d", i);
                        return false;
                        }
                  if (n1->tpc() != n2->tpc()) {
                        qDebug("      note tcp %d != %d", n1->tpc(), n2->tpc());
                        // return false;
                        }
                  }
            }

      return true;
      }

//---------------------------------------------------------
//   compareScores
//---------------------------------------------------------

bool compareScores(Score* score1, Score* score2)
      {
      int staves = score1->nstaves();
      if (score2->nstaves() != staves) {
            printf("   stave count different %d %d\n", staves, score2->nstaves());
            return false;
            }
      Segment* s1 = score1->firstMeasure()->first();
      Segment* s2 = score2->firstMeasure()->first();

      int tracks = staves * VOICES;
      for (;;) {
            for (int track = 0; track < tracks; ++track) {
                  Element* e1 = s1->element(track);
                  Element* e2 = s2->element(track);
                  if ((e1 && !e2) || (e2 && !e1)) {
                        printf("   elements different\n");
                        return false;
                        }
                  if (e1 == 0)
                        continue;
                  if (!compareElements(e1, e2)) {
                        printf("   %s != %s\n", e1->name(), e2->name());
                        return false;
                        }
                  printf("   ok: %s\n", e1->name());
                  }
            s1 = s1->next1();
            s2 = s2->next1();
            if ((s1 && !s2) || (s2 && !s1)) {
                  printf("   segment count different\n");
                  return false;
                  }
            if (s1 == 0)
                  break;
            }
      return true;
      }

//---------------------------------------------------------
///   midi01
///   write/read midi file with timesig 4/4
//---------------------------------------------------------

void TestMidi::midi01()
      {
      MCursor c;
      c.setTimeSig(Fraction(4,4));
      c.createScore("test1a");
      c.addPart("voice");
      c.move(0, Fraction(0,1));     // move to track 0 tick 0

      c.addKeySig(Key(1));
      c.addTimeSig(Fraction(4,4));
      c.addChord(60, TDuration(TDuration::DurationType::V_QUARTER));
      c.addChord(61, TDuration(TDuration::DurationType::V_QUARTER));
      c.addChord(62, TDuration(TDuration::DurationType::V_QUARTER));
      c.addChord(63, TDuration(TDuration::DurationType::V_QUARTER));
      MasterScore* score = c.score();

      score->doLayout();
      score->rebuildMidiMapping();
      c.saveScore();
      saveMidi(score, "test1.mid");

      MasterScore* score2 = new MasterScore(mscore->baseStyle());
      score2->setName("test1b");
      QCOMPARE(importMidi(score2, "test1.mid"), Score::FileError::FILE_NO_ERROR);

      score2->doLayout();
      score2->rebuildMidiMapping();
      MCursor c2(score2);
      c2.saveScore();

      QVERIFY(compareScores(score, score2));

      delete score;
      delete score2;
      }

//---------------------------------------------------------
///   midi02
///   write/read midi file with timesig 3/4
//---------------------------------------------------------

void TestMidi::midi02()
      {
      MCursor c;
      c.setTimeSig(Fraction(3,4));
      c.createScore("test2a");
      c.addPart("voice");
      c.move(0, Fraction(0,1));     // move to track 0 tick 0

      c.addKeySig(Key(2));
      c.addTimeSig(Fraction(3,4));
      c.addChord(60, TDuration(TDuration::DurationType::V_QUARTER));
      c.addChord(61, TDuration(TDuration::DurationType::V_QUARTER));
      c.addChord(62, TDuration(TDuration::DurationType::V_QUARTER));
      MasterScore* score = c.score();

      score->doLayout();
      score->rebuildMidiMapping();
      c.saveScore();
      saveMidi(score, "test2.mid");

      MasterScore* score2 = new MasterScore(mscore->baseStyle());
      score2->setName("test2b");

      QCOMPARE(importMidi(score2, "test2.mid"), Score::FileError::FILE_NO_ERROR);

      score2->doLayout();
      score2->rebuildMidiMapping();
      MCursor c2(score2);
      c2.saveScore();

      QVERIFY(compareScores(score, score2));

      delete score;
      delete score2;
      }

//---------------------------------------------------------
///   midi03
///   write/read midi file with key sig
//---------------------------------------------------------

void TestMidi::midi03()
      {
      MCursor c;
      c.setTimeSig(Fraction(4,4));
      c.createScore("test3a");
      c.addPart("voice");
      c.move(0, Fraction(0,1));     // move to track 0 tick 0

      c.addKeySig(Key(1));
      c.addTimeSig(Fraction(4,4));
      c.addChord(60, TDuration(TDuration::DurationType::V_QUARTER));
      c.addChord(61, TDuration(TDuration::DurationType::V_QUARTER));
      c.addChord(62, TDuration(TDuration::DurationType::V_QUARTER));
      c.addChord(63, TDuration(TDuration::DurationType::V_QUARTER));
      MasterScore* score = c.score();

      score->doLayout();
      score->rebuildMidiMapping();
      c.saveScore();
      saveMidi(score, "test3.mid");

      MasterScore* score2 = new MasterScore(mscore->baseStyle());
      score2->setName("test3b");
      QCOMPARE(importMidi(score2, "test3.mid"), Score::FileError::FILE_NO_ERROR);

      score2->doLayout();
      score2->rebuildMidiMapping();
      MCursor c2(score2);
      c2.saveScore();

      QVERIFY(compareScores(score, score2));

      delete score;
      delete score2;
      }

//---------------------------------------------------------
//   testTimeStretchFermata
//---------------------------------------------------------

void TestMidi::testTimeStretchFermata(MasterScore* score, const QString& file, const QString& testName)
      {
      const QString writeFile = QString("%1-%2-test-%3.mid").arg(file).arg(testName);
      const QString reference(DIR + file + "-ref.mid");

      testMidiExport(score, writeFile.arg(1), reference);

      const Fraction frac1 = 2 * Fraction(4, 4) + Fraction(2, 4); // 3rd measure, 3rd beat
      score->doLayoutRange(frac1, frac1);
      testMidiExport(score, writeFile.arg(2), reference);

      const Fraction frac2 = 6 * Fraction(4, 4); // 7th measure
      score->doLayoutRange(frac2, frac2);
      testMidiExport(score, writeFile.arg(3), reference);
      }

//---------------------------------------------------------
//   midiTimeStretchFermata
//---------------------------------------------------------

void TestMidi::midiTimeStretchFermata()
      {
      const QString file("testTimeStretchFermata");
      const QString readFile(DIR + file + ".mscx");

      MasterScore* score = readScore(readFile);

      testTimeStretchFermata(score, file, "page");

      delete score;
      }

//---------------------------------------------------------
//   midiTimeStretchFermataContinuousView
///   Checks continuous view tempo issues like #289922.
//---------------------------------------------------------

void TestMidi::midiTimeStretchFermataContinuousView()
      {
      const QString file("testTimeStretchFermata");
      const QString readFile(DIR + file + ".mscx");

      MasterScore* score = readScore(readFile);
      score->setLayoutMode(LayoutMode::LINE);
      score->doLayout();

      testTimeStretchFermata(score, file, "linear");

      delete score;
      }

//---------------------------------------------------------
//   testTimeStretchFermataTempoEdit
///   see the issue #290997
//---------------------------------------------------------

void TestMidi::testTimeStretchFermataTempoEdit(MasterScore* score, const QString& file, const QString& testName)
      {
      const QString writeFile = QString("%1-%2-test-%3.mid").arg(file).arg(testName);
      const QString reference(DIR + file + "-%1-ref.mid");

      Element* tempo = score->firstSegment(SegmentType::ChordRest)->findAnnotation(ElementType::TEMPO_TEXT, -1, 3);
      Q_ASSERT(tempo && tempo->isTempoText());

      const int scoreTempo = 200;
      const int defaultTempo = 120;
      const qreal defaultTempoBps = defaultTempo / 60.0;

      testMidiExport(score, writeFile.arg("init"), reference.arg(scoreTempo));

      score->startCmd();
      tempo->undoChangeProperty(Pid::TEMPO_FOLLOW_TEXT, false, PropertyFlags::UNSTYLED);
      tempo->undoChangeProperty(Pid::TEMPO, defaultTempoBps, PropertyFlags::UNSTYLED);
      score->endCmd();
      testMidiExport(score, writeFile.arg("change-tempo"), reference.arg(defaultTempo));

      // undo the last changes
      score->startCmd();
      score->undoRedo(/* undo */ true, /* EditData */ nullptr);
      score->endCmd();
      testMidiExport(score, writeFile.arg("undo-change-tempo"), reference.arg(scoreTempo));

      score->startCmd();
      score->undoRemoveElement(tempo);
      score->endCmd();
      testMidiExport(score, writeFile.arg("remove-tempo"), reference.arg(defaultTempo));

      // undo the last changes
      score->startCmd();
      score->undoRedo(/* undo */ true, /* EditData */ nullptr);
      score->endCmd();
      testMidiExport(score, writeFile.arg("undo-remove-tempo"), reference.arg(scoreTempo));
      }

//---------------------------------------------------------
//   midiTimeStretchFermataTempoEdit
//---------------------------------------------------------

void TestMidi::midiTimeStretchFermataTempoEdit()
      {
      const QString file("testTimeStretchFermataTempoEdit");
      const QString readFile(DIR + file + ".mscx");

      MasterScore* score = readScore(readFile);

      testTimeStretchFermataTempoEdit(score, file, "page");

      delete score;
      }

//---------------------------------------------------------
//   midiTimeStretchFermataTempoEditContinuousView
//---------------------------------------------------------

void TestMidi::midiTimeStretchFermataTempoEditContinuousView()
      {
      const QString file("testTimeStretchFermataTempoEdit");
      const QString readFile(DIR + file + ".mscx");

      MasterScore* score = readScore(readFile);
      score->setLayoutMode(LayoutMode::LINE);
      score->doLayout();

      testTimeStretchFermataTempoEdit(score, file, "linear");

      delete score;
      }

//---------------------------------------------------------
//   midiSingleNoteDynamics
//---------------------------------------------------------

void TestMidi::midiSingleNoteDynamics()
      {
      const QString file("testSingleNoteDynamics");
      QString readFile(DIR   + file + ".mscx");
      QString writeFile(file + "-test.mid");
      QString reference(DIR + file + "-ref.mid");

      MasterScore* score = readScore(readFile);
      score->doLayout();
      testMidiExport(score, writeFile, reference);

      delete score;
      }

//---------------------------------------------------------
//   events
//---------------------------------------------------------

void TestMidi::events()
      {
      QFETCH(QString, file);

      QString readFile(DIR   + file + ".mscx");
      QString writeFile(file + "-test.txt");
      QString reference(DIR + file + "-ref.txt");

      MasterScore* score = readScore(readFile);
      EventMap events;
      // a temporary, uninitialized synth state so we can render the midi - should fall back correctly
      SynthesizerState ss;
      score->renderMidi(&events, ss);
      qDebug() << "Opened score " << readFile;
      QFile filehandler(writeFile);
      filehandler.open(QIODevice::WriteOnly | QIODevice::Text);
      QTextStream out(&filehandler);

      for (auto iter = events.begin(); iter!= events.end(); ++iter){
            if (iter->second.discard())
                  continue;
            out << qSetFieldWidth(5) << "Tick  =  ";
            out << qSetFieldWidth(5) << iter->first;
            out << qSetFieldWidth(5) << "   Type  = ";
            out << qSetFieldWidth(5) << iter->second.type();
            out << qSetFieldWidth(5) << "   Pitch  = ";
            out << qSetFieldWidth(5) << iter->second.dataA();
            out << qSetFieldWidth(5) << "   Velocity  = ";
            out << qSetFieldWidth(5) << iter->second.dataB();
            out << qSetFieldWidth(5) << "   Channel  = ";
            out << qSetFieldWidth(5) << iter->second.channel();
            out << endl;
            }
      filehandler.close();

      QVERIFY(score);
      QVERIFY(compareFiles(writeFile, reference));
     // QVERIFY(saveCompareScore(score, writeFile, reference));

      delete score;
      }

//---------------------------------------------------------
//   testMidiExport
//---------------------------------------------------------

void TestMidi::testMidiExport(MasterScore* score, const QString& writeFile, const QString& refFile)
      {
      Q_ASSERT(writeFile.endsWith(".mid") && refFile.endsWith(".mid"));
      QVERIFY(saveMidi(score, writeFile));
      QVERIFY(compareFiles(writeFile, refFile));
      }

//---------------------------------------------------------
//   midiExportTest
//   read a MuseScore mscx file, write to a MIDI file and verify against reference
//---------------------------------------------------------

void TestMidi::midiExportTestRef(const QString& file)
      {
      MScore::debugMode = true;
      MasterScore* score = readScore(DIR + file + ".mscx");
      QVERIFY(score);
      score->doLayout();
      score->rebuildMidiMapping();
      testMidiExport(score, QString(file) + ".mid", DIR + QString(file) + "-ref.mid");
      delete score;
      }

void TestMidi::laissezVibIntervals()
      {
      // Nested shorter requests must not truncate the outer hold. Preserve
      // new capture times, coalesce simultaneous chord marks, and close gaps.
      const auto switches = laissezVibSwitches({ { 30, 40 }, { 10, 100 }, { 10, 50 },
                                                { 20, 80 }, { 100, 120 }, { 150, 160 } });
      const std::vector<int> ticks { 10, 20, 30, 100, 120, 150, 160 };
      const std::vector<bool> values { true, true, true, true, false, true, false };
      QCOMPARE(int(switches.size()), int(ticks.size()));
      for (size_t i = 0; i < ticks.size(); ++i) {
            QCOMPARE(switches[i].tick, ticks[i]);
            QCOMPARE(switches[i].on, bool(values[i]));
            }
      QVERIFY(laissezVibSwitches({ { 4, 4 }, { 6, 5 } }).empty());
      }

void TestMidi::laissezVibPlayback_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<int>("captures");
      QTest::addColumn<int>("releases");
      QTest::newRow("chord-and-tie-chain") << "laissez-vibrer-playback" << 2 << 1;
      QTest::newRow("repeat-boundaries") << "laissez-vibrer-playback-repeat" << 4 << 2;
      }

void TestMidi::laissezVibPlayback()
      {
      QFETCH(QString, file);
      QFETCH(int, captures);
      QFETCH(int, releases);
      std::unique_ptr<MasterScore> score(readScore(DIR + file + ".mscx"));
      QVERIFY(score != nullptr);
      SynthesizerState state;
      EventMap events;
      score->renderMidi(&events, false, true, state);
      std::vector<int> on, off;
      using NoteStamp = std::tuple<int, int, int, int>;
      std::vector<NoteStamp> notes;
      for (const auto& entry : events) {
            const NPlayEvent& e = entry.second;
            if (e.type() == ME_NOTEON)
                  notes.emplace_back(entry.first, int(e.channel()), e.pitch(), e.velo());
            if (e.type() != ME_CONTROLLER || e.controller() != CTRL_SOSTENUTO) continue;
            (e.value() >= 64 ? on : off).push_back(entry.first);
            }
      QCOMPARE(int(on.size()), captures);
      QCOMPARE(int(off.size()), releases);
      QVERIFY(on[0] > 0 && on[0] < 480);
      // The second capture belongs to the LV on the tie-chain endpoint; the
      // sounding attack is at tick 960, not a fresh attack at the endpoint.
      QVERIFY(on[1] > 960 && on[1] < 1440);
      if (releases == 1) QVERIFY(off[0] >= 3840 && off[0] < 4320);
      else {
            QCOMPARE(off[0], 3840);
            QVERIFY(off[1] >= 7680 && off[1] < 8160);
            }

      // LV must leave the actual MIDI note attacks and notated gate lengths
      // unchanged. Only the pedal stream extends the sounding release.
      for (Segment* segment = score->firstSegment(SegmentType::ChordRest); segment;
           segment = segment->next1(SegmentType::ChordRest)) {
            for (int track = 0; track < score->ntracks(); ++track) {
                  Element* element = segment->element(track);
                  if (!element || !element->isChord()) continue;
                  for (Note* note : toChord(element)->notes()) {
                        std::vector<Element*> remove;
                        for (Element* attached : note->el())
                              if (attached->isLaissezVib()) remove.push_back(attached);
                        for (Element* attached : remove) { note->remove(attached); delete attached; }
                        }
                  }
            }
      EventMap plain;
      score->renderMidi(&plain, false, true, state);
      std::vector<NoteStamp> plainNotes;
      for (const auto& entry : plain) {
            const NPlayEvent& e = entry.second;
            if (e.type() == ME_NOTEON)
                  plainNotes.emplace_back(entry.first, int(e.channel()), e.pitch(), e.velo());
            QVERIFY(e.type() != ME_CONTROLLER || e.controller() != CTRL_SOSTENUTO);
            }
      QVERIFY(notes == plainNotes);
      }

void TestMidi::laissezVibBends_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<int>("captures");
      QTest::newRow("legacy-overlap-and-reuse") << "laissez-vibrer-bends" << 4;
      QTest::newRow("legacy-repeat") << "laissez-vibrer-bends-repeat" << 7;
      QTest::newRow("legacy-tied-endpoint") << "laissez-vibrer-bend-tie" << 1;
      }

void TestMidi::laissezVibBends()
      {
      QFETCH(QString, file);
      QFETCH(int, captures);
      std::unique_ptr<MasterScore> score(readScore(DIR + file + ".mscx"));
      QVERIFY(score != nullptr);
      EventMap events;
      SynthesizerState state;
      score->renderMidi(&events, false, true, state);
      using PedalStamp = std::tuple<int, int, int>;
      using GateStamp = std::tuple<int, int, int>;
      std::vector<PedalStamp> pedals;
      std::vector<GateStamp> gates;
      std::map<std::pair<int, int>, int> attacks;
      std::map<std::pair<int, int>, int> attackChannels;
      int checked = 0;
      const auto& mappings = score->midiMapping();
      for (const auto& entry : events) {
            const NPlayEvent& event = entry.second;
            const int channel = event.channel();
            if (event.type() == ME_CONTROLLER && event.controller() == CTRL_SOSTENUTO)
                  pedals.emplace_back(entry.first, score->midiChannel(channel), event.value());
            if (event.type() != ME_NOTEON) continue;
            gates.emplace_back(entry.first, event.pitch(), event.velo());
            QVERIFY(channel >= 0 && channel < int(mappings.size()));
            if (mappings[channel].bendSource() < 0) {
                  QCOMPARE(event.pitch(), 67); // unbent G uses the source channel
                  continue;
                  }
            const auto key = std::make_pair(channel, event.pitch());
            if (event.velo() > 0) {
                  attacks[key] = entry.first;
                  attackChannels[{ entry.first, event.pitch() }] = channel;
                  continue;
                  }
            const auto found = attacks.find(key);
            QVERIFY(found != attacks.end());
            const int on = found->second;
            attacks.erase(found);
            int capture = -1, release = -1;
            for (const auto& control : events) {
                  const NPlayEvent& e = control.second;
                  if (e.channel() != channel || control.first < on || e.type() != ME_CONTROLLER
                        || e.controller() != CTRL_SOSTENUTO) continue;
                  if (e.value() >= 64 && capture < 0) capture = control.first;
                  else if (e.value() < 64 && capture >= 0) { release = control.first; break; }
                  }
            QVERIFY(capture > on && capture < entry.first);
            QVERIFY(release > entry.first);
            if (captures == 7 && on < 3840) QCOMPARE(release, 3840);
            bool resetFound = false, rangeFound = false;
            int finalWheel = 8192;
            for (const auto& wheel : events) {
                  const NPlayEvent& e = wheel.second;
                  if (e.channel() != channel) continue;
                  if (wheel.first == qMax(0, on - 1) && e.type() == ME_CONTROLLER
                        && e.controller() == CTRL_HDATA && e.value() == 12) rangeFound = true;
                  if (e.type() != ME_PITCHBEND) continue;
                  const int value = e.dataA() + 128 * e.dataB();
                  if (wheel.first >= on && wheel.first <= entry.first) finalWheel = value;
                  // No reset or other curve can disturb the held final pitch.
                  QVERIFY(wheel.first <= entry.first || wheel.first > release);
                  if (wheel.first == release + 1 && value == 8192) resetFound = true;
                  }
            QVERIFY(rangeFound);
            QVERIFY(resetFound);
            QCOMPARE(finalWheel, event.pitch() == 64 ? 8192 : 9557);
            ++checked;
            }
      QCOMPARE(checked, captures);
      QVERIFY(attacks.empty());
      QCOMPARE(int(pedals.size()), 2 * captures);
      if (captures != 1) {
            // Two simultaneous bends and a later same-key bend must each own
            // a channel until release; the later A can reuse the first lane.
            const int c = attackChannels.at({ 0, 60 });
            const int e = attackChannels.at({ 0, 64 });
            const int nextC = attackChannels.at({ 960, 60 });
            QVERIFY(c != e && c != nextC && e != nextC);
            const int lateTick = captures == 7 ? 8160 : 4320;
            QCOMPARE(attackChannels.at({ lateTick, 69 }), c);
            }

      // Exercise the real exporter and MIDI reader, not just EventMap. These
      // small fixtures stay on one port, so channel numbers identify each lane.
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString path = directory.filePath("lv-bends.mid");
      QVERIFY(saveMidi(score.get(), path));
      QFile output(path);
      QVERIFY(output.open(QIODevice::ReadOnly));
      MidiFile midi;
      QVERIFY(midi.read(&output));
      std::vector<PedalStamp> exported;
      for (const MidiTrack& track : midi.tracks()) {
            for (const auto& entry : track.events()) {
                  const MidiEvent& event = entry.second;
                  if (event.type() == ME_CONTROLLER && event.controller() == CTRL_SOSTENUTO)
                        exported.emplace_back(entry.first, event.channel(), event.value());
                  }
            }
      std::sort(pedals.begin(), pedals.end());
      std::sort(exported.begin(), exported.end());
      QVERIFY(pedals == exported);

      // Removing LV restores legacy routing without changing note gates.
      for (Segment* segment = score->firstSegment(SegmentType::ChordRest); segment;
           segment = segment->next1(SegmentType::ChordRest)) {
            for (int track = 0; track < score->ntracks(); ++track) {
                  Element* element = segment->element(track);
                  if (!element || !element->isChord()) continue;
                  for (Note* note : toChord(element)->notes()) {
                        std::vector<Element*> remove;
                        for (Element* attached : note->el())
                              if (attached->isLaissezVib()) remove.push_back(attached);
                        for (Element* attached : remove) { note->remove(attached); delete attached; }
                        }
                  }
            }
      EventMap plain;
      score->renderMidi(&plain, false, true, state);
      std::vector<GateStamp> plainGates;
      for (const auto& entry : plain) {
            const NPlayEvent& e = entry.second;
            if (e.type() == ME_NOTEON) plainGates.emplace_back(entry.first, e.pitch(), e.velo());
            QVERIFY(e.type() != ME_CONTROLLER || e.controller() != CTRL_SOSTENUTO);
            }
      std::sort(gates.begin(), gates.end());
      std::sort(plainGates.begin(), plainGates.end());
      QVERIFY(gates == plainGates);
      }

void TestMidi::laissezVibGrace_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<bool>("repeat");
      QTest::newRow("before-after-and-group") << "laissez-vibrer-grace" << false;
      QTest::newRow("repeat-and-score-end") << "laissez-vibrer-grace-repeat" << true;
      }

void TestMidi::laissezVibGrace()
      {
      QFETCH(QString, file);
      QFETCH(bool, repeat);
      std::unique_ptr<MasterScore> score(readScore(DIR + file + ".mscx"));
      QVERIFY(score != nullptr);
      SynthesizerState state;
      EventMap events;
      score->renderMidi(&events, false, true, state);
      using Stamp = std::tuple<int, int, int, int>;
      auto noteStamps = [](const EventMap& map) {
            std::vector<Stamp> result;
            for (const auto& entry : map) {
                  const NPlayEvent& event = entry.second;
                  if (event.type() == ME_NOTEON)
                        result.emplace_back(entry.first, event.channel(), event.pitch(), event.velo());
                  }
            std::sort(result.begin(), result.end());
            return result;
            };
      auto pedalStamps = [](const EventMap& map) {
            std::vector<Stamp> result;
            for (const auto& entry : map) {
                  const NPlayEvent& event = entry.second;
                  if (event.type() == ME_CONTROLLER && event.controller() == CTRL_SOSTENUTO)
                        result.emplace_back(entry.first, event.channel(), event.controller(), event.value());
                  }
            std::sort(result.begin(), result.end());
            return result;
            };
      const std::vector<int> expectedOn = repeat ? std::vector<int> { 48, 3888, 8208, 11588, 15888 }
                                                : std::vector<int> { 48, 4368, 7748, 12048 };
      const std::vector<int> expectedOff = repeat ? std::vector<int> { 3840, 7728, 11568, 15366, 17280 }
                                                 : std::vector<int> { 3888, 7728, 11526, 13440 };
      std::vector<int> on, off;
      for (const auto& stamp : pedalStamps(events))
            (std::get<3>(stamp) >= 64 ? on : off).push_back(std::get<0>(stamp));
      QVERIFY(on == expectedOn);
      QVERIFY(off == expectedOff);
      // Every capture belongs to a marked grace's emitted gate, rather than
      // the principal's notated duration. Unmarked graces create no capture.
      std::map<const Note*, int> attacks;
      std::set<int> captureOwners;
      for (const auto& entry : events) {
            const NPlayEvent& event = entry.second;
            if (event.type() != ME_NOTEON || !event.note() || !event.note()->chord()->isGrace()) continue;
            const Note* note = event.note();
            if (event.velo() > 0) { attacks[note] = entry.first; continue; }
            const auto found = attacks.find(note);
            QVERIFY(found != attacks.end());
            bool marked = false;
            for (const Element* attached : note->el()) marked |= attached->isLaissezVib();
            if (marked) {
                  const int capture = found->second + qMax(1, (entry.first - found->second + 1) / 10);
                  QVERIFY(capture > found->second && capture < entry.first);
                  QVERIFY(std::find(on.begin(), on.end(), capture) != on.end());
                  captureOwners.insert(capture);
                  }
            attacks.erase(found);
            }
      QVERIFY(attacks.empty());
      QCOMPARE(captureOwners.size(), expectedOn.size());

      // Playback may request short rendering chunks. A grace-only LV must
      // retain exactly the same release as full-score rendering.
      const bool savedRepeats = score->expandRepeats();
      score->setExpandRepeats(true);
      MidiRenderer renderer(score.get());
      renderer.setMinChunkSize(1);
      MidiRenderer::Context context(state);
      context.metronome = false;
      context.renderHarmony = true;
      EventMap chunked;
      renderer.renderScore(&chunked, context);
      score->setExpandRepeats(savedRepeats);
      QVERIFY(pedalStamps(events) == pedalStamps(chunked));
      QVERIFY(noteStamps(events) == noteStamps(chunked));

      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString path = directory.filePath("lv-grace.mid");
      QVERIFY(saveMidi(score.get(), path));
      QFile output(path);
      QVERIFY(output.open(QIODevice::ReadOnly));
      MidiFile midi;
      QVERIFY(midi.read(&output));
      std::vector<Stamp> exported;
      for (const MidiTrack& track : midi.tracks()) {
            for (const auto& entry : track.events()) {
                  const MidiEvent& event = entry.second;
                  if (event.type() == ME_CONTROLLER && event.controller() == CTRL_SOSTENUTO)
                        exported.emplace_back(entry.first, event.channel(), event.controller(), event.value());
                  }
            }
      std::sort(exported.begin(), exported.end());
      std::vector<Stamp> expectedMidi;
      for (const auto& stamp : pedalStamps(events))
            expectedMidi.emplace_back(std::get<0>(stamp), score->midiChannel(std::get<1>(stamp)),
                  std::get<2>(stamp), std::get<3>(stamp));
      std::sort(expectedMidi.begin(), expectedMidi.end());
      QVERIFY(exported == expectedMidi);

      // Remove grace LV marks and compare every note attack and gate.
      int removed = 0;
      for (Segment* segment = score->firstSegment(SegmentType::ChordRest); segment;
           segment = segment->next1(SegmentType::ChordRest)) {
            for (int track = 0; track < score->ntracks(); ++track) {
                  Element* element = segment->element(track);
                  if (!element || !element->isChord()) continue;
                  for (Chord* grace : toChord(element)->graceNotes()) {
                        for (Note* note : grace->notes()) {
                              std::vector<Element*> marks;
                              for (Element* attached : note->el())
                                    if (attached->isLaissezVib()) marks.push_back(attached);
                              for (Element* mark : marks) { note->remove(mark); delete mark; ++removed; }
                              }
                        }
                  }
            }
      QCOMPARE(removed, 6);
      EventMap plain;
      score->renderMidi(&plain, false, true, state);
      QVERIFY(pedalStamps(plain).empty());
      QVERIFY(noteStamps(events) == noteStamps(plain));
      }

void TestMidi::laissezVibRepeatTies_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<int>("kind");
      QTest::newRow("repeat-landing-and-overlap") << "laissez-vibrer-repeat-tie" << 0;
      QTest::newRow("revisit-written-note") << "laissez-vibrer-repeat-tie-chain" << 1;
      QTest::newRow("forward-jump") << "laissez-vibrer-jump-tie" << 2;
      }

void TestMidi::laissezVibRepeatTies()
      {
      QFETCH(QString, file);
      QFETCH(int, kind);
      std::unique_ptr<MasterScore> score(readScore(DIR + file + ".mscx"));
      QVERIFY(score != nullptr);
      SynthesizerState state;
      EventMap events;
      score->renderMidi(&events, false, true, state);
      using Stamp = std::tuple<int, int, int, int>;
      auto stamps = [](const EventMap& map, bool pedals) {
            std::vector<Stamp> result;
            for (const auto& entry : map) {
                  const NPlayEvent& event = entry.second;
                  if (pedals && event.type() == ME_CONTROLLER && event.controller() == CTRL_SOSTENUTO)
                        result.emplace_back(entry.first, event.channel(), event.controller(), event.value());
                  else if (!pedals && event.type() == ME_NOTEON)
                        result.emplace_back(entry.first, event.channel(), event.pitch(), event.velo());
                  }
            std::sort(result.begin(), result.end());
            return result;
            };
      const std::vector<int> expectedAttack = kind == 0 ? std::vector<int> { 0, 1920, 5760 }
                                                      : std::vector<int> { 0 };
      const std::vector<int> expectedGate = kind == 0 ? std::vector<int> { 1919, 5759, 7679 }
            : kind == 1 ? std::vector<int> { 9599 } : std::vector<int> { 3839 };
      const std::vector<int> expectedCapture = kind == 0 ? std::vector<int> { 192, 2304 }
            : kind == 1 ? std::vector<int> { 960 } : std::vector<int> { 384 };
      const int expectedRelease = kind == 0 ? 8064 : kind == 1 ? 12480 : 6144;
      std::vector<int> attacks, gates, captures, releases;
      for (const auto& stamp : stamps(events, false))
            (std::get<3>(stamp) ? attacks : gates).push_back(std::get<0>(stamp));
      for (const auto& stamp : stamps(events, true))
            (std::get<3>(stamp) >= 64 ? captures : releases).push_back(std::get<0>(stamp));
      // These assert the repeat-tie renderer's behavior independently of LV:
      // there must be no new attack at a tied landing or a skipped measure.
      QCOMPARE(attacks.size(), expectedAttack.size());
      QCOMPARE(gates.size(), expectedGate.size());
      QCOMPARE(captures.size(), expectedCapture.size());
      for (size_t i = 0; i < attacks.size(); ++i) QCOMPARE(attacks[i], expectedAttack[i]);
      for (size_t i = 0; i < gates.size(); ++i) QCOMPARE(gates[i], expectedGate[i]);
      for (size_t i = 0; i < captures.size(); ++i) QCOMPARE(captures[i], expectedCapture[i]);
      QCOMPARE(releases.size(), size_t(1));
      QCOMPARE(releases.front(), expectedRelease);

      const bool savedRepeats = score->expandRepeats();
      score->setExpandRepeats(true);
      if (kind == 1) {
            const Note* note = nullptr;
            for (const auto& entry : events)
                  if (entry.second.type() == ME_NOTEON && entry.second.velo() > 0) { note = entry.second.note(); break; }
            QVERIFY(note != nullptr);
            int utick = 0;
            std::vector<int> written;
            while (note) {
                  written.push_back(note->tick().ticks());
                  QVERIFY(written.size() <= 5);
                  const Note* next = nextTiedNoteInPlayback(note, utick);
                  utick += note->chord()->actualTicks().ticks();
                  note = next;
                  }
            QVERIFY(written == std::vector<int>({ 0, 1920, 0, 1920, 3840 }));
            }

      // Exercise the same collect-all-then-finalize route used by Seq, with
      // one-measure chunks and independent playback segments.
      MidiRenderer renderer(score.get());
      renderer.setMinChunkSize(1);
      MidiRenderer::Context context(state);
      context.metronome = false;
      context.renderHarmony = true;
      EventMap chunked;
      int at = 0, chunks = 0;
      while (const MidiRenderer::Chunk chunk = renderer.getChunkAt(at)) {
            QVERIFY(chunk.utick2() > at);
            renderer.renderChunk(chunk, &chunked, context);
            at = chunk.utick2();
            QVERIFY(++chunks < 20);
            }
      QVERIFY(chunks >= 2);
      QVERIFY(stamps(chunked, true).empty());
      renderer.renderLaissezVib(&chunked);
      QVERIFY(stamps(events, false) == stamps(chunked, false));
      QVERIFY(stamps(events, true) == stamps(chunked, true));
      score->setExpandRepeats(savedRepeats);

      if (kind == 0) {
            EventMap flat;
            score->renderMidi(&flat, false, false, state);
            std::vector<int> flatCaptures, flatReleases;
            for (const auto& stamp : stamps(flat, true))
                  (std::get<3>(stamp) >= 64 ? flatCaptures : flatReleases).push_back(std::get<0>(stamp));
            QVERIFY(flatCaptures == std::vector<int>({ 192 }));
            QVERIFY(flatReleases == std::vector<int>({ 4032 }));
            }

      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString path = directory.filePath("lv-repeat-tie.mid");
      QVERIFY(saveMidi(score.get(), path));
      QFile output(path);
      QVERIFY(output.open(QIODevice::ReadOnly));
      MidiFile midi;
      QVERIFY(midi.read(&output));
      std::vector<Stamp> exported;
      for (const MidiTrack& track : midi.tracks()) {
            for (const auto& entry : track.events()) {
                  const MidiEvent& event = entry.second;
                  if (event.type() == ME_CONTROLLER && event.controller() == CTRL_SOSTENUTO)
                        exported.emplace_back(entry.first, event.channel(), event.controller(), event.value());
                  }
            }
      std::sort(exported.begin(), exported.end());
      std::vector<Stamp> expectedMidi;
      for (const auto& stamp : stamps(events, true))
            expectedMidi.emplace_back(std::get<0>(stamp), score->midiChannel(std::get<1>(stamp)),
                  std::get<2>(stamp), std::get<3>(stamp));
      std::sort(expectedMidi.begin(), expectedMidi.end());
      QVERIFY(exported == expectedMidi);

      for (Segment* segment = score->firstSegment(SegmentType::ChordRest); segment;
           segment = segment->next1(SegmentType::ChordRest)) {
            for (int track = 0; track < score->ntracks(); ++track) {
                  Element* element = segment->element(track);
                  if (!element || !element->isChord()) continue;
                  for (Note* note : toChord(element)->notes()) {
                        std::vector<Element*> marks;
                        for (Element* attached : note->el())
                              if (attached->isLaissezVib()) marks.push_back(attached);
                        for (Element* mark : marks) { note->remove(mark); delete mark; }
                        }
                  }
            }
      EventMap plain;
      score->renderMidi(&plain, false, true, state);
      QVERIFY(stamps(plain, true).empty());
      QVERIFY(stamps(events, false) == stamps(plain, false));
      }

void TestMidi::laissezVibControls_data()
      {
      QTest::addColumn<QString>("file");
      QTest::addColumn<bool>("bendPolicy");
      QTest::newRow("ordinary") << "laissez-vibrer-playback" << false;
      QTest::newRow("grace-repeat") << "laissez-vibrer-grace-repeat" << false;
      QTest::newRow("repeat-landing") << "laissez-vibrer-repeat-tie" << false;
      QTest::newRow("repeat-chain") << "laissez-vibrer-repeat-tie-chain" << false;
      QTest::newRow("forward-jump") << "laissez-vibrer-jump-tie" << false;
      QTest::newRow("unrelated-piano-events") << "laissez-vibrer-lazy-controls" << false;
      QTest::newRow("isolated-bend-fallback") << "laissez-vibrer-bends" << true;
      }

void TestMidi::laissezVibControls()
      {
      QFETCH(QString, file);
      QFETCH(bool, bendPolicy);
      std::unique_ptr<MasterScore> score(readScore(DIR + file + ".mscx"));
      QVERIFY(score != nullptr);
      Note* unrelated = nullptr;
      Note* endpoint = nullptr;
      if (file == "laissez-vibrer-lazy-controls") {
            Chord* last = toChord(score->lastMeasure()->first(SegmentType::ChordRest)->element(0));
            QVERIFY(last != nullptr);
            unrelated = last->upNote();
            QCOMPARE(unrelated->pitch(), 67);
            QCOMPARE(unrelated->playEvents().size(), 1);
            QCOMPARE(unrelated->playEvents().front().len(), 1000);
            const Measure* third = score->firstMeasure()->nextMeasure()->nextMeasure();
            endpoint = toChord(third->first(SegmentType::ChordRest)->element(0))->upNote();
            }
      const bool savedRepeats = score->expandRepeats();
      score->setExpandRepeats(true);
      SynthesizerState state;
      MidiRenderer::Context context(state);
      context.metronome = false;
      context.renderHarmony = false;
      MidiRenderer renderer(score.get());
      EventMap controls;
      const size_t mappings = score->midiMapping().size();
      QCOMPARE(renderer.renderLaissezVibControls(&controls, context), !bendPolicy);
      // The prepass never reserves a bend lane or returns note/audio events.
      QCOMPARE(score->midiMapping().size(), mappings);
      for (const auto& entry : controls) {
            QCOMPARE(int(entry.second.type()), int(ME_CONTROLLER));
            QCOMPARE(entry.second.controller(), int(CTRL_SOSTENUTO));
            }
      if (bendPolicy) {
            QVERIFY(controls.empty());
            return;
            }
      QVERIFY(!controls.empty());
      if (unrelated) {
            QCOMPARE(unrelated->playEvents().front().len(), 1000);
            QCOMPARE(endpoint->playEvents().front().len(), 950);
            }
      score->setExpandRepeats(savedRepeats);
      EventMap full;
      score->renderMidi(&full, false, true, state);
      using Stamp = std::tuple<int, int, int>;
      std::vector<Stamp> prepared, expected;
      for (const auto& entry : controls)
            prepared.emplace_back(entry.first, entry.second.channel(), entry.second.value());
      for (const auto& entry : full) {
            const NPlayEvent& event = entry.second;
            if (event.type() == ME_CONTROLLER && event.controller() == CTRL_SOSTENUTO)
                  expected.emplace_back(entry.first, event.channel(), event.value());
            }
      std::sort(prepared.begin(), prepared.end());
      std::sort(expected.begin(), expected.end());
      QVERIFY(prepared == expected);
      if (unrelated) QCOMPARE(unrelated->playEvents().front().len(), 950);
      }

// A large ordinary score must not prepare the final note while rendering its
// initial playback buffer. Requesting that later chunk must prepare it normally.
void TestMidi::lazyPlaybackChunks()
      {
      MCursor cursor;
      cursor.setTimeSig(Fraction(4, 4));
      cursor.createScore("lazy-playback");
      cursor.addPart("piano");
      cursor.addTimeSig(Fraction(4, 4));
      Chord* firstChord = nullptr;
      Chord* lastChord = nullptr;
      for (unsigned beat = 0; beat < 120 * 4; ++beat) {
            lastChord = cursor.addChord(60 + int(beat % 12), TDuration(TDuration::DurationType::V_QUARTER));
            if (!firstChord) firstChord = lastChord;
            }
      std::unique_ptr<MasterScore> score(cursor.score());
      score->parts().front()->instrument()->setArticulation({MidiArticulation("", "", 100, 95)});
      score->doLayout();
      score->rebuildMidiMapping();
      QCOMPARE(lastChord->upNote()->playEvents().front().len(), 1000);
      const SynthesizerState state;
      MidiRenderer::Context context(state);
      context.metronome = false;
      MidiRenderer renderer(score.get());
      renderer.setMinChunkSize(10);
      const auto first = renderer.getChunkAt(0);
      QVERIFY(first);
      QCOMPARE(first.utick1(), 0);
      QCOMPARE(first.utick2(), Fraction(10, 1).ticks());
      EventMap events;
      renderer.renderChunk(first, &events, context);
      QVERIFY(!events.empty());
      QCOMPARE(firstChord->upNote()->playEvents().front().len(), 950);
      QCOMPARE(lastChord->upNote()->playEvents().front().len(), 1000);
      for (const auto& entry : events)
            if (entry.second.type() == ME_NOTEON && entry.second.velo() > 0)
                  QVERIFY(entry.second.note() != lastChord->upNote());
      const auto last = renderer.getChunkAt(lastChord->tick().ticks());
      QVERIFY(last);
      renderer.renderChunk(last, &events, context);
      QCOMPARE(lastChord->upNote()->playEvents().front().len(), 950);
      QVERIFY(std::any_of(events.begin(), events.end(), [&](const EventMap::value_type& entry) {
            return entry.second.type() == ME_NOTEON && entry.second.velo() > 0
                  && entry.second.note() == lastChord->upNote();
            }));
      }

// Saving audio sends must preserve the normal channel linkage while adding no
// controller, changing no CC91 value, and leaving exported MIDI byte-identical.
void TestMidi::audioReverbChannels()
      {
      Channel channel;
      QCOMPARE(channel.audioReverbSend(), 0);
      channel.setReverb(17);
      const std::vector<MidiCoreEvent> before = channel.initList();
      channel.setAudioReverbSend(37);
      const std::vector<MidiCoreEvent>& after = channel.initList();
      QCOMPARE(int(after.size()), int(before.size()));
      for (unsigned i = 0; i < before.size(); ++i) {
            QCOMPARE(after[i].type(), before[i].type());
            QCOMPARE(after[i].dataA(), before[i].dataA());
            QCOMPARE(after[i].dataB(), before[i].dataB());
            }
      QCOMPARE(int(channel.reverb()), 17);
      Channel copy(channel);
      QCOMPARE(copy.audioReverbSend(), 37);
      copy.setAudioReverbSend(22);
      QCOMPARE(channel.audioReverbSend(), 37);

      QByteArray data;
      QBuffer buffer(&data);
      QVERIFY(buffer.open(QIODevice::WriteOnly));
      { XmlWriter writer(nullptr, &buffer); channel.write(writer, nullptr); }
      buffer.close();
      QVERIFY(data.contains("<audioReverbSend>37</audioReverbSend>"));
      XmlReader reader(data);
      QVERIFY(reader.readNextStartElement());
      Channel restored;
      restored.read(reader, nullptr);
      QVERIFY(!reader.hasError());
      QCOMPARE(restored.audioReverbSend(), 37);
      QCOMPARE(int(restored.reverb()), 17);
      // Reuse the object with an old channel: the absent field must reset to zero.
      XmlReader old(QByteArray("<Channel><controller ctrl=\"91\" value=\"55\"/></Channel>"));
      QVERIFY(old.readNextStartElement());
      restored.read(old, nullptr);
      QCOMPARE(restored.audioReverbSend(), 0);
      QCOMPARE(int(restored.reverb()), 55);
      for (const auto& input : { std::make_pair(-10, 0), std::make_pair(200, 100) }) {
            XmlReader limits(QString("<Channel><audioReverbSend>%1</audioReverbSend></Channel>").arg(input.first).toUtf8());
            QVERIFY(limits.readNextStartElement());
            restored.read(limits, nullptr);
            QCOMPARE(restored.audioReverbSend(), input.second);
            }

      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      std::unique_ptr<MasterScore> score(readScore(DIR + "audio-reverb-sends.mscx"));
      QVERIFY(score);
      QCOMPARE(score->parts().size(), 2);
      auto savedChannel = [&](int part) { return score->parts()[part]->instrument()->channel(0); };
      auto playbackChannel = [&](int part) { return score->playbackChannel(savedChannel(part)); };
      QCOMPARE(playbackChannel(0)->audioReverbSend(), 0);
      QCOMPARE(playbackChannel(1)->audioReverbSend(), 0);
      playbackChannel(0)->setReverb(17);
      const QString originalMidi = directory.filePath("before.mid");
      QVERIFY(saveMidi(score.get(), originalMidi));
      playbackChannel(0)->setAudioReverbSend(37);
      QCOMPARE(savedChannel(0)->audioReverbSend(), 37); // playback -> saved settings link
      QCOMPARE(savedChannel(1)->audioReverbSend(), 0);
      const QString updatedMidi = directory.filePath("after.mid");
      QVERIFY(saveMidi(score.get(), updatedMidi));
      QFile original(originalMidi), updated(updatedMidi);
      QVERIFY(original.open(QIODevice::ReadOnly));
      QVERIFY(updated.open(QIODevice::ReadOnly));
      QCOMPARE(updated.readAll(), original.readAll());
      const QString path = directory.filePath("sends.mscx");
      QVERIFY(saveScore(score.get(), path));
      std::unique_ptr<MasterScore> loaded(readCreatedScore(path));
      QVERIFY(loaded);
      for (int part = 0; part < 2; ++part) {
            const Channel* stored = loaded->parts()[part]->instrument()->channel(0);
            QCOMPARE(stored->audioReverbSend(), part ? 0 : 37);
            QCOMPARE(loaded->playbackChannel(stored)->audioReverbSend(), part ? 0 : 37);
            }
      const int source = playbackChannel(0)->channel();
      QVERIFY(score->midiMapping(source)->scoreChannel() == savedChannel(0));
      const int bend = score->guitarBendChannel(source, 0);
      QVERIFY(bend >= 0);
      QCOMPARE(score->midiMapping(bend)->bendSource(), source);
      playbackChannel(0)->setAudioReverbSend(50);
      QCOMPARE(playbackChannel(0)->audioReverbSend(), 50);
      QCOMPARE(score->midiMapping(bend)->articulation()->audioReverbSend(), 37);
      // Dispatch must read source=50, not this deliberately stale bend copy=37.
      }

// Shared return state is separate from both master-effect Zita groups and
// remains round-trippable even when no isolated backend is registered.
void TestMidi::audioReverbReturnState()
      {
      using Settings = AuxiliaryReverbSettings;
      MasterSynthesizer original;
      QVERIFY(!original.auxiliaryReverbAvailable());
      auto expected = Settings::defaults();
      expected[Settings::Delay] = .075f;
      expected[Settings::LowRT60] = 3.5f;
      expected[Settings::MidRT60] = 5.0f;
      expected[Settings::EQ1Gain] = 3.0f;
      expected[Settings::ReturnLevel] = .25f;
      // Keep exact float equality, but identify the stage and parameter when
      // a particular build cannot reproduce the expected round trip.
      const auto differences = [&](const Settings::Values& actual, const char* stage) {
            QStringList result;
            for (int id = 0; id < Settings::Count; ++id) {
                  if (actual[id] != expected[id])
                        result.append(QString("%1: parameter %2, expected %3, actual %4")
                              .arg(stage).arg(id)
                              .arg(QString::number(expected[id], 'g', 9))
                              .arg(QString::number(actual[id], 'g', 9)));
                  }
            return result.join("; ");
            };
      for (int id = 0; id < Settings::Count; ++id)
            QVERIFY(original.setAuxiliaryReverbParameter(id, expected[id]));
      QString mismatch = differences(original.auxiliaryReverbValues(), "parameter setters");
      QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
      SynthesizerState state;
      state.push_back(original.state().group(Settings::stateGroup()));
      state.setIsDefault(false);
      QCOMPARE(int(state.front().size()), int(Settings::Count));
      QVERIFY(state.group("Zita1").empty());
      for (const IdValue& value : state.front()) {
            QVERIFY(value.id >= 0 && value.id < Settings::Count);
            QCOMPARE(value.data, QString::number(expected[value.id], 'g', 9));
            }
      MasterSynthesizer loaded;
      QVERIFY(loaded.setState(state));
      mismatch = differences(loaded.auxiliaryReverbValues(), "in-memory state");
      QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
      QByteArray data;
      QBuffer buffer(&data);
      QVERIFY(buffer.open(QIODevice::WriteOnly));
      { XmlWriter writer(nullptr, &buffer); state.write(writer, true); }
      buffer.close();
      QVERIFY(data.contains("<AuxiliaryReverb>"));
      QVERIFY(!data.contains("<Zita1>"));
      XmlReader reader(data);
      QVERIFY(reader.readNextStartElement());
      SynthesizerState restored;
      restored.read(reader);
      QVERIFY(!reader.hasError());
      QCOMPARE(int(restored.size()), 1);
      QCOMPARE(restored.front().name(), QString(Settings::stateGroup()));
      QCOMPARE(int(restored.front().size()), int(Settings::Count));
      auto savedValue = state.front().begin();
      for (const IdValue& value : restored.front()) {
            QCOMPARE(value.id, savedValue->id);
            QCOMPARE(value.data, savedValue->data);
            ++savedValue;
            }
      loaded.resetAuxiliaryReverb();
      QVERIFY(loaded.setState(restored));
      mismatch = differences(loaded.auxiliaryReverbValues(), "XML state");
      QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
      loaded.resetAuxiliaryReverb();
      QVERIFY(loaded.auxiliaryReverbValues() == Settings::defaults());
      QVERIFY(loaded.setState(restored));
      QVERIFY(loaded.setState(SynthesizerState())); // older state with no return group
      QVERIFY(loaded.auxiliaryReverbValues() == Settings::defaults());
      SynthesizerGroup malformed;
      malformed.setName(Settings::stateGroup());
      malformed.push_back(IdValue(Settings::MidRT60, "invalid"));
      malformed.push_back(IdValue(Settings::EQ1Gain, "nan"));
      malformed.push_back(IdValue(999, "1"));
      malformed.push_back(IdValue(Settings::ReturnLevel, "20"));
      SynthesizerState malformedState;
      malformedState.push_back(malformed);
      QVERIFY(loaded.setState(malformedState));
      QVERIFY(loaded.auxiliaryReverbValues() == Settings::defaults());
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      std::unique_ptr<MasterScore> score(readScore(DIR + "audio-reverb-sends.mscx"));
      QVERIFY(score);
      score->setSynthesizerState(state);
      const QString path = directory.filePath("return-state.mscx");
      {
            // Score::write normally omits synthesizer state in test mode.
            // Exercise the real save path, restoring the flag even on failure.
            QScopedValueRollback<bool> testMode(MScore::testMode, false);
            QVERIFY(saveScore(score.get(), path));
            }
      QFile savedFile(path);
      QVERIFY(savedFile.open(QIODevice::ReadOnly));
      QVERIFY(savedFile.readAll().contains("<AuxiliaryReverb>"));
      savedFile.close();
      std::unique_ptr<MasterScore> reopened(readCreatedScore(path));
      QVERIFY(reopened);
      QCOMPARE(int(reopened->synthesizerState().group(Settings::stateGroup()).size()), int(Settings::Count));
      QVERIFY(loaded.setState(reopened->synthesizerState()));
      mismatch = differences(loaded.auxiliaryReverbValues(), "saved score");
      QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
      }

QTEST_MAIN(TestMidi)

#include "tst_midi.moc"

