// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "audio/midi/fluid2/synthbanks.h"
#include "testfont.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>

using Ms::Fluid2SynthBanks;
static void require(bool condition, const char* message)
      {
      if (!condition) { std::cerr << message << '\n'; std::exit(EXIT_FAILURE); }
      }

#ifdef FLUID2_TEST_WRAP_LOAD
static unsigned loadCall = 0, failLoad = 0;
extern "C" int __real_fluid_synth_sfload(fluid_synth_t*, const char*, int);
extern "C" int __wrap_fluid_synth_sfload(fluid_synth_t* synth, const char* path, int reset)
      {
      if (++loadCall == failLoad) return FLUID_FAILED;
      return __real_fluid_synth_sfload(synth, path, reset);
      }
#endif

static double energy(const std::vector<float>& samples)
      { double sum = 0; for (float x : samples) sum += x * x; return sum; }
static unsigned voices(fluid_synth_t* synth)
      {
      std::array<fluid_voice_t*, 512> list {};
      fluid_synth_get_voicelist(synth, list.data(), int(list.size()), -1);
      unsigned result = 0; for (auto* voice : list) { if (!voice) break; ++result; }
      return result;
      }
static void setup(Fluid2SynthBanks& engine, const char* path, float rate, bool isolated, unsigned groups = 0)
      {
      require(engine.init(rate, groups, isolated), "engine initialization failed");
      require(engine.loadSoundFont(path), "synthetic SF2 load failed");
      require(engine.tune(440), "master tuning failed");
      for (int ch = 0; ch < 256; ++ch) require(engine.select(ch, 0, 0), "melodic preset selection failed");
      }

static void compare(const char* path, float rate, unsigned groups, bool isolated = false)
      {
      Fluid2SynthBanks reference, candidate;
      setup(reference, path, rate, false);
      setup(candidate, path, rate, isolated, groups);
      // FluidSynth 2.3.x uses 255 as its unused-voice channel marker.
      // Controller modulation there can visit uninitialized voices in a
      // single 256-channel synth. Exercise logical 255 only in two-bank mode.
      for (int ch : { 0, 127, 128, 254 }) {
            for (auto* engine : { &reference, &candidate }) {
                  auto target = engine->channel(ch);
                  fluid_synth_cc(target.synth, target.channel, 10, ch & 1 ? 110 : 17);
                  fluid_synth_pitch_bend(target.synth, target.channel, ch & 1 ? 9000 : 8192);
                  engine->noteOn(ch, 57 + ch % 12, 90, ch & 1 ? 25.0f : -50.0f);
                  }
            }
      for (unsigned frames : { 1u, 63u, 513u, 4097u }) {
            std::vector<float> expected(frames * 2, 0.125f), actual = expected;
            require(reference.process(frames, expected.data()) && candidate.process(frames, actual.data()), "PCM render failed");
            double difference = 0;
            for (unsigned i = 0; i < actual.size(); ++i) difference = std::max(difference, std::abs(double(actual[i] - expected[i])));
            if (difference >= 0.00001)
                  std::cerr << "PCM rate=" << rate << " groups=" << groups << " isolated=" << isolated
                            << " frames=" << frames << " difference=" << difference << '\n';
            require(difference < 0.00001, "grouped/two-bank PCM differs from single-synth reference");
            }
      }

static void initialTuning(const char* path)
      {
      Fluid2SynthBanks reference, candidate;
      setup(reference, path, 44100, false);
      setup(candidate, path, 44100, false);
      require(reference.tune(440.0 * std::pow(2.0, 1.0 / 12.0)), "reference tuning failed");
      reference.noteOn(0, 69, 90, 0);
      candidate.noteOn(0, 69, 90, 100);
      // No inspecting synth API call between this attack and rendering: such
      // a call could accidentally flush and hide a queued fine-tuning update.
      std::vector<float> expected(128, 0), actual = expected;
      require(reference.process(64, expected.data()) && candidate.process(64, actual.data()), "initial tuning render failed");
      for (unsigned i = 0; i < actual.size(); ++i)
            require(std::abs(actual[i] - expected[i]) < 0.00001, "first DSP block missed the note tuning");
      }

static void independentOutputs(const char* path)
      {
      Fluid2SynthBanks engine;
      setup(engine, path, 44100, true);
      engine.noteOn(127, 69, 90, 0);
      Ms::Fluid2AudioBuffer buffer(128);
      for (unsigned pass = 0; pass < 2; ++pass) {
            if (pass) engine.noteOn(255, 72, 90, 0);
            for (unsigned bank = 0; bank < 2; ++bank) {
                  std::vector<float> dry(1024, 0);
                  bool signal = false;
                  require(buffer.process(512, dry.data(), [&](int frames, int outputs, float** planes) {
                        if (fluid_synth_process(engine.synth(bank), frames, 0, nullptr, outputs, planes) != FLUID_OK) return false;
                        for (int plane = 0; plane < outputs; ++plane) {
                              for (int frame = 0; frame < frames; ++frame) {
                                    if (std::abs(planes[plane][frame]) < 0.000001) continue;
                                    require(plane / 2 == 127, "note leaked into another output group");
                                    signal = true;
                                    }
                              }
                        return true;
                        }), "independent output render failed");
                  require(signal == (bank == 0 || pass == 1), "bank audio leaked or expected output silent");
                  }
            }
      }

static void auxiliaryStems(const char* path)
      {
      Fluid2SynthBanks mixed;
      std::array<Fluid2SynthBanks, 3> stems;
      const int channels[] = { 0, 128, 255 };
      setup(mixed, path, 44100, true);
      for (unsigned i = 0; i < stems.size(); ++i) {
            setup(stems[i], path, 44100, true);
            const int ch = channels[i];
            const int source = ch == 128 ? 128 : 0;
            require(mixed.setSourceChannel(ch, source), "auxiliary owner rejected");
            for (auto* engine : { &mixed, &stems[i] }) {
                  auto target = engine->channel(ch);
                  fluid_synth_cc(target.synth, target.channel, 10, 20 + int(i) * 40);
                  fluid_synth_cc(target.synth, target.channel, 7, 80 + int(i) * 10);
                  engine->noteOn(ch, 60 + int(i) * 4, 90, i == 1 ? 25 : -50);
                  }
            }
      auto step = [&](unsigned frames, float first, float second, float lane) {
            std::vector<float> dry(frames * 2, 0.125f), aux(frames * 2, 0.25f);
            std::array<std::vector<float>, 3> reference;
            require(mixed.process(frames, dry.data(), aux.data()), "auxiliary render failed");
            for (unsigned i = 0; i < stems.size(); ++i) {
                  reference[i].assign(frames * 2, 0);
                  require(stems[i].process(frames, reference[i].data()), "stem render failed");
                  }
            for (unsigned i = 0; i < dry.size(); ++i) {
                  const float expectedDry = 0.125f + reference[0][i] + reference[1][i] + reference[2][i];
                  const float expectedAux = 0.25f + first * reference[0][i] + second * reference[1][i] + lane * reference[2][i];
                  require(std::abs(dry[i] - expectedDry) < 0.00001, "send changed the dry mix");
                  require(std::abs(aux[i] - expectedAux) < 0.00001, "auxiliary differs from weighted independent stems");
                  }
            };
      step(64, 0, 0, 0);
      require(mixed.setSourceSend(0, 0.25f) && mixed.setSourceSend(128, 0.75f)
              && mixed.setSourceSend(255, 1), "auxiliary gain rejected");
      for (unsigned frames : { 1u, 63u, 513u, 2048u }) step(frames, 0.25f, 0.75f, 0.25f);
      require(mixed.setSourceSend(0, 0.5f), "live send update rejected");
      step(513, 0.5f, 0.75f, 0.5f);
      require(!mixed.setSourceChannel(255, 128), "ambiguous send owner accepted");
      step(128, 0.5f, 0.75f, 0);

      float alias[] = { 1, 2, 3, 4 };
      require(!mixed.process(1, alias, alias) && !mixed.process(1, alias, alias + 1), "overlapping dry/aux accepted");
      require(alias[0] == 1 && alias[1] == 2 && alias[2] == 3 && alias[3] == 4, "rejected render changed samples");
      step(64, 0.5f, 0.75f, 0); // rejected requests must not advance synth time
      require(mixed.allNotesOff(-1) && mixed.sourceChannel(0) == 0, "note-off lost release ownership");
      require(mixed.allSoundsOff(0) && mixed.sourceChannel(0) < 0 && mixed.sourceChannel(128) == 128,
              "channel reset forgot another owner");
      require(mixed.sourceSend(0) == 0.5f, "stop/reset cleared send gain");
      require(mixed.loadSoundFont(path) && mixed.sourceChannel(128) < 0 && mixed.sourceSend(128) == 0.75f,
              "font replacement did not retain gain/reset ownership");
      require(mixed.init(44100, 0, false) && mixed.sourceSend(128) == 0, "reinitialization did not reset gains");
      float dry[] = { 1, 2 }, aux[] = { 3, 4 };
      require(!mixed.process(1, dry, aux) && dry[0] == 1 && aux[0] == 3, "unsupported auxiliary request rendered");
      }

static void resetAuxiliaryOwner(const char* path)
      {
      Fluid2SynthBanks engine;
      setup(engine, path, 44100, true);
      require(engine.setSourceChannel(0, 0) && engine.setSourceSend(0, 0.25f)
              && engine.setSourceSend(1, 1), "reset source setup failed");
      engine.noteOn(0, 69, 90, 0);
      float first[2] = { 0, 0 };
      require(engine.process(1, first), "partial-block preparation failed");
      require(engine.allSoundsOff(0) && engine.setSourceChannel(0, 1), "hard reset/rebind failed");
      engine.noteOn(0, 72, 90, 0);
      std::vector<float> dry(128 * 2, 0), aux(128 * 2, 0);
      unsigned offset = 0;
      for (unsigned frames : { 1u, 63u, 64u }) {
            require(engine.process(frames, dry.data() + offset * 2, aux.data() + offset * 2), "reset fragment render failed");
            offset += frames;
            require(energy(aux) == 0, "cached audio reached new owner's send");
            }
      std::fill(dry.begin(), dry.end(), 0); std::fill(aux.begin(), aux.end(), 0);
      require(engine.process(128, dry.data(), aux.data()) && energy(aux) > 0.000001, "new owner's send did not resume");
      for (unsigned i = 0; i < dry.size(); ++i)
            require(std::abs(dry[i] - aux[i]) < 0.00001, "resumed source gain differs");
      }

static void compareCompactDry(const char* path, float rate)
      {
      Fluid2SynthBanks reference, candidate;
      setup(reference, path, rate, true);
      setup(candidate, path, rate, true);
      // Silent routed lanes force the original planar path in each bank of
      // the reference without contributing any auxiliary audio.
      reference.setSourceSend(254, 1.0f);
      reference.setSourceChannel(126, 254);
      reference.setSourceChannel(254, 254);
      for (int channel : {0, 127, 128, 255}) {
            reference.setSourceChannel(channel, channel < 128 ? 0 : 128);
            candidate.setSourceChannel(channel, channel < 128 ? 0 : 128);
            reference.noteOn(channel, 60 + channel % 12, 85, 0);
            candidate.noteOn(channel, 60 + channel % 12, 85, 0);
            }
      for (int phase = 0; phase < 6; ++phase) {
            const float lowSend = phase == 1 || phase == 2 ? 0.35f : 0.0f;
            const float highSend = phase == 1 || phase == 3 ? 0.35f : 0.0f;
            for (auto* engine : {&reference, &candidate}) {
                  engine->setSourceSend(0, lowSend);
                  engine->setSourceSend(128, highSend);
                  }
            if (phase == 5) {
                  reference.allSoundsOff(-1);
                  candidate.allSoundsOff(-1);
                  reference.setSourceChannel(126, 254);
                  reference.setSourceChannel(254, 254);
                  }
            for (unsigned frames : {1u, 63u, 64u, 65u, 511u, 513u}) {
                  std::vector<float> expected(frames * 2, 0.125f), actual(expected);
                  std::vector<float> expectedAux(frames * 2, 0.25f), actualAux(expectedAux);
                  require(reference.process(frames, expected.data(), expectedAux.data())
                          && candidate.process(frames, actual.data(), actualAux.data()), "compact render failed");
                  for (unsigned i = 0; i < actual.size(); ++i) {
                        require(expected[i] == actual[i], "compact dry PCM is not bit-exact");
                        require(std::abs(expectedAux[i] - actualAux[i]) < 0.00001f, "compact send transition differs");
                        }
                  }
            }
      }

int main(int argc, char** argv)
      {
      require(argc == 2, "pass a temporary generated SF2 output path");
      const char* path = argv[1];
      require(Fluid2Test::writeFont(path), "cannot write synthetic test font");
      compareCompactDry(path, 44100);
      compareCompactDry(path, 48000);
      Fluid2SynthBanks engine;
      setup(engine, path, 44100, true);
      require(engine.bankCount() == 2 && engine.groupCount() == 128 && engine.isolated(), "wrong isolated layout");
      require(engine.voiceLimitPerBank() == 256, "total voice budget changed");
      require(fluid_synth_get_polyphony(engine.synth(0)) == 256 && fluid_synth_get_polyphony(engine.synth(1)) == 256,
              "native voice limits differ");
      for (int ch = 0; ch < 256; ++ch) {
            auto target = engine.channel(ch);
            require(target.synth == engine.synth(ch / 128) && target.channel == ch % 128, "bank address alias");
            }
      require(!engine.channel(-1).synth && !engine.channel(256).synth, "invalid channel accepted");
      require(fluid_synth_get_sfont_by_id(engine.synth(0), engine.fontId(0))
              != fluid_synth_get_sfont_by_id(engine.synth(1), engine.fontId(1)), "shared owned SoundFont pointer");

      auto low = engine.channel(0), high = engine.channel(128);
      fluid_synth_cc(low.synth, low.channel, 7, 33);
      int value = -1;
      fluid_synth_get_cc(high.synth, high.channel, 7, &value);
      require(value != 33, "volume controller leaked between banks");
      fluid_synth_pitch_bend(low.synth, low.channel, 12345);
      fluid_synth_get_pitch_bend(high.synth, high.channel, &value);
      require(value == 8192, "pitch wheel leaked between banks");
      require(engine.select(255, 128, 0), "high-channel drum selection failed");
      auto drum = engine.channel(255);
      require(fluid_preset_get_banknum(fluid_synth_get_channel_preset(drum.synth, drum.channel)) == 128, "wrong drum bank");
      require(engine.select(127, 0, 0), "low melodic selection failed");
      require(fluid_preset_get_banknum(fluid_synth_get_channel_preset(engine.synth(0), 127)) == 0, "drum mode leaked");
      require(fluid_synth_cc(drum.synth, drum.channel, 7, 55) == FLUID_OK
              && fluid_synth_pitch_bend(drum.synth, drum.channel, 10000) == FLUID_OK, "logical 255 controller failed");
      fluid_synth_get_cc(engine.synth(0), 127, 7, &value);
      require(value != 55, "logical 255 controller leaked into channel 127");

      require(engine.tune(442), "two-bank master tuning failed");
      for (unsigned bank = 0; bank < 2; ++bank) {
            double pitches[128]; char name[64];
            require(fluid_synth_tuning_dump(engine.synth(bank), 0, 0, name, 64, pitches) == FLUID_OK, "missing tuning table");
            require(std::abs(pitches[69] - (6900 + 1200 * std::log2(442.0 / 440.0))) < 0.0001, "master tuning differs between banks");
            }
      engine.noteOn(0, 60, 90, 25);
      engine.noteOn(128, 64, 90, -50);
      for (unsigned bank = 0; bank < 2; ++bank) {
            std::array<fluid_voice_t*, 512> list {};
            fluid_synth_get_voicelist(engine.synth(bank), list.data(), int(list.size()), -1);
            require(list[0] && std::abs(fluid_voice_gen_get(list[0], GEN_FINETUNE) - (bank ? -50 : 25)) < 0.001,
                    "per-note tuning lost across banks");
            }
      for (auto target : { low, high }) {
            fluid_synth_cc(target.synth, target.channel, 64, 127);
            fluid_synth_cc(target.synth, target.channel, 66, 127);
            }
      require(engine.allSoundsOff(0), "channel-specific sound reset failed");
      // FluidSynth finishes killed voices when its queued DSP block runs.
      std::vector<float> settled(256, 0);
      require(engine.process(128, settled.data()), "reset settle render failed");
      require(voices(engine.synth(0)) == 0 && voices(engine.synth(1)) > 0, "channel reset affected other bank");
      fluid_synth_get_cc(high.synth, high.channel, 66, &value);
      require(value == 127, "channel reset cleared other bank's sostenuto");
      fluid_synth_noteoff(high.synth, high.channel, 64);
      fluid_synth_cc(high.synth, high.channel, 64, 0);
      std::vector<float> held(22050 * 2, 0);
      require(engine.process(22050, held.data()) && voices(engine.synth(1)) > 0 && energy(held) > 0.001,
              "sostenuto did not retain the high-bank note after note-off");
      require(engine.allNotesOff(-1), "all-notes-off failed");
      for (auto target : { low, high }) {
            fluid_synth_get_cc(target.synth, target.channel, 64, &value); require(value == 0, "sustain not reset");
            fluid_synth_get_cc(target.synth, target.channel, 66, &value); require(value == 0, "sostenuto not reset");
            }
      std::vector<float> release(44100 * 2, 0);
      require(engine.process(44100, release.data()), "release render failed");
      require(voices(engine.synth(1)) == 0, "released bank stuck on");

      engine.noteOn(0, 67, 90, 0);
      engine.noteOn(128, 72, 90, 0);
      std::fill(settled.begin(), settled.end(), 0);
      require(engine.process(128, settled.data()), "held-note preparation failed");
      const unsigned heldLow = voices(engine.synth(0)), heldHigh = voices(engine.synth(1));
      const int oldLow = engine.fontId(0), oldHigh = engine.fontId(1);
#ifdef FLUID2_TEST_WRAP_LOAD
      loadCall = 0; failLoad = 2;
      require(!engine.loadSoundFont(path), "second-bank load failure accepted");
      failLoad = 0;
      require(fluid_synth_sfcount(engine.synth(0)) == 1 && fluid_synth_sfcount(engine.synth(1)) == 1,
              "partial-load rollback leaked a font");
#else
      require(!engine.loadSoundFont("/37e-test-missing-font.sf2"), "missing font accepted");
#endif
      require(engine.fontId(0) == oldLow && engine.fontId(1) == oldHigh, "failed load replaced old fonts");
      require(voices(engine.synth(0)) == heldLow && voices(engine.synth(1)) == heldHigh, "failed load killed held notes");
      engine.noteOn(128, 64, 90, 0);
      std::vector<float> audio(2048, 0);
      require(engine.process(1024, audio.data()) && energy(audio) > 0.001, "old high-bank font unusable after failed load");
      require(engine.loadSoundFont(path), "successful replacement failed");
      require(engine.loadSoundFont(""), "font unload failed");
      require(engine.fontId(0) < 0 && engine.fontId(1) < 0, "one bank retained its unloaded font");
      setup(engine, path, 48000, true);
      engine.noteOn(255, 60, 90, 0);
      std::fill(audio.begin(), audio.end(), 0);
      require(engine.process(1024, audio.data()) && energy(audio) > 0.001, "rate-change/reload high bank silent");
      require(engine.allSoundsOff(-1), "global reset failed");
      std::fill(settled.begin(), settled.end(), 0);
      require(engine.process(128, settled.data()) && voices(engine.synth(0)) == 0 && voices(engine.synth(1)) == 0,
              "global reset left voices");
      for (float rate : { 44100.0f, 48000.0f }) {
            compare(path, rate, 1);
            compare(path, rate, 16);
            compare(path, rate, 128);
            compare(path, rate, 0, true);
            }
      initialTuning(path);
      independentOutputs(path);
      auxiliaryStems(path);
      resetAuxiliaryOwner(path);
      engine.clear();
      require(!engine.init(44100, 129, false) && engine.bankCount() == 0, "invalid layout survived");
      std::remove(path);
      std::cout << "FluidSynth " << fluid_version_str()
                << ": PCM parity, tuning, CC/reset, font rollback and source auxiliary/stem checks passed\n";
      }
