// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "audio/midi/auxiliarybus.h"
#include "audio/midi/fluid2/synthbanks.h"
#include "effects/zita1/zita.h"
#include "testfont.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace Ms { EffectGui* ZitaReverb::gui() { return nullptr; } }
static void require(bool test, const char* message)
      {
      if (!test) { std::cerr << message << '\n'; std::exit(EXIT_FAILURE); }
      }
static void setup(Ms::Fluid2SynthBanks& engine, const char* path)
      {
      require(engine.init(44100, 0, true) && engine.loadSoundFont(path), "engine/font setup failed");
      for (int ch : { 0, 127, 129 }) require(engine.select(ch, 0, 0), "preset selection failed");
      }
static void notes(Ms::Fluid2SynthBanks& engine, bool otherSource)
      {
      require(engine.setSourceChannel(0, 0) && engine.setSourceChannel(129, 0), "source ownership failed");
      engine.noteOn(0, 60, 90, 25.0f);
      engine.noteOn(129, 67, 90, -25.0f);  // separate bend lane in bank 1
      if (otherSource) {
            require(engine.setSourceChannel(127, 127), "second source ownership failed");
            engine.noteOn(127, 72, 90, 0.0f);
            }
      }

static void check(const char* path, float send)
      {
      Ms::Fluid2SynthBanks engine, dryReference, sourceReference;
      for (auto* synth : { &engine, &dryReference, &sourceReference }) setup(*synth, path);
      require(engine.setSourceSend(0, send), "send setting failed");
      require(engine.setSourceSend(129, 1.0f), "bend lane diagnostic gain failed");
      notes(engine, true); notes(dryReference, true); notes(sourceReference, false);
      Ms::ZitaReverb actualReverb, expectedReverb;
      for (auto* effect : { &actualReverb, &expectedReverb }) { effect->init(44100); effect->set_opmix(1.0f); }
      Ms::AuxiliaryBus<4096> bus;
      double differenceEnergy = 0.0;
      unsigned position = 0;
      for (unsigned step = 0; step < 180; ++step) {
            const unsigned n = step % 3 == 0 ? 1 : step % 3 == 1 ? 63 : 2048;
            if (step == 30) {
                  // Note releases stop generating notes but the return must decay.
                  for (auto* synth : { &engine, &dryReference, &sourceReference }) synth->allNotesOff(-1);
                  }
            std::vector<float> dry(n * 2, 0.125f), actual = dry, stem(n * 2, 0), expectedWet(n * 2, 0);
            require(dryReference.process(n, dry.data()), "dry reference render failed");
            require(sourceReference.process(n, stem.data()), "source stem render failed");
            for (float& value : stem) value *= send;
            bus.process(n, actual.data(), [&](float* aux) {
                        require(engine.process(n, actual.data(), aux), "combined dry/aux render failed");
                        }, [&]() { actualReverb.clear(); },
                  [&](unsigned count, float* input, float* wet) { actualReverb.process(int(count), input, wet); });
            if (send > 0) expectedReverb.process(int(n), stem.data(), expectedWet.data());
            for (unsigned i = 0; i < actual.size(); ++i) {
                  const float expected = dry[i] + expectedWet[i];
                  require(std::fabs(actual[i] - expected) < 0.00001f, "source return differs from independent weighted-stem reference");
                  const double difference = actual[i] - dry[i];
                  differenceEnergy += difference * difference;
                  if (send == 0) require(actual[i] == dry[i], "zero send was not bit-exact");
                  }
            position += n;
            }
      require(position > 44100, "insufficient tail duration");
      if (send > 0) require(differenceEnergy > 0.00001, "shared return was silent");
      // Export normalization starts a new pass with allSoundsOff. Compare its
      // return to a fresh effect fed the very same auxiliary PCM: the synth's
      // cached dry block/voice teardown is a separate existing lifecycle.
      for (unsigned pass = 0; pass < 2; ++pass) {
            require(engine.allSoundsOff(-1), "pass hard reset failed");
            bus.requestReset();
            notes(engine, true);
            std::vector<float> result(4096 * 2, 0.0f);
            std::vector<float> input(result.size(), 0), dry, expectedWet(result.size(), 0);
            Ms::ZitaReverb fresh;
            fresh.init(44100);
            fresh.set_opmix(1.0f);
            bus.process(4096, result.data(), [&](float* aux) {
                        require(engine.process(4096, result.data(), aux), "pass render failed");
                        std::copy(aux, aux + input.size(), input.begin());
                        dry = result;
                        }, [&]() { actualReverb.clear(); },
                  [&](unsigned count, float* in, float* wet) { actualReverb.process(int(count), in, wet); });
            if (send > 0) fresh.process(4096, input.data(), expectedWet.data());
            for (unsigned i = 0; i < result.size(); ++i)
                  require(std::fabs(result[i] - (dry[i] + expectedWet[i])) < 0.00001f,
                        "restarted render retained old wet history");
            }
      }
int main(int argc, char** argv)
      {
      require(argc == 2, "supply a private temporary SF2 fixture path");
      require(Fluid2Test::writeFont(argv[1]), "test font generation failed");
      check(argv[1], 0.0f);
      check(argv[1], 0.25f);
      require(std::remove(argv[1]) == 0, "test font cleanup failed");
      std::cout << "FluidSynth + Zita: zero sends, source/bend stems, release tails and repeat-pass reset checks passed\n";
      }
