// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "config.h"
#include "exportaudio.h"
#include "libmscore/score.h"
#include "libmscore/part.h"
#include "libmscore/mscore.h"
#include "audio/midi/msynthesizer.h"
#include "preferences.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QIODevice>
#include <QScopeGuard>
#include <QScopedValueRollback>
#include <cstring>
#include <memory>

namespace Ms {
extern MasterSynthesizer* synti;

// Keep this object independent of MuseScore's main-window implementation:
// lightweight score tests supply their own synth factory and test globals.
bool renderAudio(Score* score, QIODevice* device, const SynthesizerState* liveState,
                 AudioSynthesizerFactory createSynth, std::function<bool(float)> updateProgress)
      {
      QElapsedTimer exportTimer;
      exportTimer.start();
      if (!score || !device || !createSynth) {
            qDebug() << "Invalid score or device";
            return false;
            }

      if (!device->open(QIODevice::WriteOnly)) {
            qDebug() << "Could not write to device";
            return false;
            }
      auto closeDevice = qScopeGuard([device]() { device->close(); });

      EventMap events;
      // In non-GUI mode current synthesizer settings won't
      // allow single note dynamics. See issue #289947.
      const bool useCurrentSynthesizerState = liveState != nullptr;

      if (useCurrentSynthesizerState) {
            score->renderMidi(&events, *liveState);
            if (events.empty())
                  return false;
            }

      std::unique_ptr<MasterSynthesizer> synth(createSynth());
      if (!synth) return false;
      synth->init();
      int sampleRate = preferences.getInt(PREF_EXPORT_AUDIO_SAMPLERATE);
      synth->setSampleRate(sampleRate);

      const SynthesizerState state = useCurrentSynthesizerState ? *liveState : score->synthesizerState();
      const bool setStateOk = synth->setState(state);

      if (!setStateOk || !synth->hasSoundFontsLoaded())
            synth->init(); // re-initialize master synthesizer with default settings

      if (!useCurrentSynthesizerState) {
            score->masterScore()->rebuildAndUpdateExpressive(synth->synthesizer("Fluid"));
            score->renderMidi(&events, score->synthesizerState());
            if (synti)
                  score->masterScore()->rebuildAndUpdateExpressive(synti->synthesizer("Fluid"));

            if (events.empty())
                  return false;
            }

      QScopedValueRollback<int> restoreSampleRate(MScore::sampleRate, sampleRate);

      const qint64 setupMilliseconds = exportTimer.elapsed();
      float peak  = 0.0;
      double gain = 1.0;
      EventMap::const_iterator endPos = events.cend();
      --endPos;
      const int et = (score->utick2utime(endPos->first) + 1) * MScore::sampleRate;
      const int maxEndTime = (score->utick2utime(endPos->first) + 3) * MScore::sampleRate;

      bool cancelled = false;
      int passes = preferences.getBool(PREF_EXPORT_AUDIO_NORMALIZE) ? 2 : 1;
      for (int pass = 0; pass < passes; ++pass) {
            EventMap::const_iterator playPos;
            playPos = events.cbegin();
            synth->allSoundsOff(-1);

            //
            // init instruments
            //
            for (Part* part : score->parts()) {
                  const InstrumentList* il = part->instruments();
                  for (auto i = il->begin(); i!= il->end(); i++) {
                        for (const Channel* instrChan : i->second->channel()) {
                              const Channel* a = score->masterScore()->playbackChannel(instrChan);
                              for (MidiCoreEvent e : a->initList()) {
                                    if (e.type() == ME_INVALID)
                                          continue;
                                    e.setChannel(a->channel());
                                    int syntiIdx = synth->index(score->masterScore()->midiMapping(a->channel())->articulation()->synti());
                                    synth->play(e, syntiIdx, a->channel(), a->audioReverbSend() / 100.0f);
                                    }
                              }
                        }
                  }

            static const unsigned FRAMES = 512;
            float buffer[FRAMES * 2];
            int playTime = 0;

            for (;;) {
                  unsigned frames = FRAMES;
                  //
                  // collect events for one segment
                  //
                  float max = 0.0;
                  memset(buffer, 0, sizeof(float) * FRAMES * 2);
                  int endTime = playTime + frames;
                  float* p = buffer;
                  for (; playPos != events.cend(); ++playPos) {
                        int f = score->utick2utime(playPos->first) * MScore::sampleRate;
                        if (f >= endTime)
                              break;
                        int n = f - playTime;
                        if (n) {
                              synth->process(n, p);
                              p += 2 * n;
                              }

                        playTime  += n;
                        frames    -= n;
                        const NPlayEvent& e = playPos->second;
                        if (!(!e.velo() && e.discard()) && e.isChannelEvent()) {
                              int channelIdx = e.channel();
                              const MidiMapping* mapping = score->masterScore()->midiMapping(channelIdx);
                              const int source = mapping->bendSource();
                              const int root = source >= 0 ? source : channelIdx;
                              const Channel* c = score->masterScore()->midiMapping(root)->articulation();
                              if (!c->mute()) {
                                    synth->play(e, synth->index(c->synti()), root, c->audioReverbSend() / 100.0f);
                                    }
                              }
                        }
                  if (frames) {
                        synth->process(frames, p);
                        playTime += frames;
                        }
                  if (pass == 1) {
                        for (unsigned i = 0; i < FRAMES * 2; ++i) {
                              max = qMax(max, qAbs(buffer[i]));
                              buffer[i] *= gain;
                              }
                        }
                  else {
                        for (unsigned i = 0; i < FRAMES * 2; ++i) {
                              max = qMax(max, qAbs(buffer[i]));
                              peak = qMax(peak, qAbs(buffer[i]));
                              }
                        }
                  if (pass == (passes - 1)) {
                        const qint64 bytes = 2 * FRAMES * sizeof(float);
                        if (device->write(reinterpret_cast<const char*>(buffer), bytes) != bytes) {
                              qWarning() << "Could not write audio:" << device->errorString();
                              return false;
                              }
                        }
                  playTime = endTime;
                  if (updateProgress) {
                        // normalize to [0, 1] range
                        if (!updateProgress(float(pass * et + playTime) / passes / et)) {
                              cancelled = true;
                              break;
                              }
                        }
                  if (playTime >= et)
                        synth->allNotesOff(-1);
                  // create sound until the sound decays
                  if (playTime >= et && max*peak < 0.000001)
                        break;
                  // hard limit
                  if (playTime > maxEndTime)
                        break;
                  }
            if (cancelled)
                  break;
            if (pass == 0 && qFuzzyIsNull(peak)) {
                  qDebug("song is empty");
                  break;
                  }
            gain = 0.99 / peak;
            }

      synth.reset();

      device->close();
      closeDevice.dismiss();

      if (!qgetenv("MSCORE_AUDIO_EXPORT_TIMING").isEmpty()) {
            const qint64 totalMilliseconds = exportTimer.elapsed();
            qInfo("Audio export: setup=%lld ms render/write/cleanup=%lld ms total=%lld ms",
                  static_cast<long long>(setupMilliseconds),
                  static_cast<long long>(totalMilliseconds - setupMilliseconds),
                  static_cast<long long>(totalMilliseconds));
            }
      return !cancelled;
      }

}
