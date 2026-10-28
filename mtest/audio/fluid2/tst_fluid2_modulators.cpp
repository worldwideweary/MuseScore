// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "audio/midi/fluid2/synthbanks.h"
#include "testfont.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

using Ms::Fluid2SynthBanks;
static void require(bool ok, const char* why)
      { if (!ok) { std::cerr << why << '\n'; std::exit(EXIT_FAILURE); } }
struct Energy {
      double left = 0, right = 0;
      };
static Energy render(Fluid2SynthBanks& engine, unsigned frames, bool auxiliary = false)
      {
      std::vector<float> dry(frames * 2, 0), wet = dry;
      require(engine.process(frames, dry.data(), auxiliary ? wet.data() : nullptr), "pan render failed");
      Energy result;
      for (unsigned i = 0; i < frames; ++i) {
            require(std::isfinite(dry[i * 2]) && std::isfinite(dry[i * 2 + 1]), "nonfinite pan PCM");
            result.left += dry[i * 2] * dry[i * 2];
            result.right += dry[i * 2 + 1] * dry[i * 2 + 1];
            if (auxiliary) for (unsigned side = 0; side < 2; ++side)
                  require(std::abs(wet[i * 2 + side] - .375f * dry[i * 2 + side]) < .00001f, "pan changed source send ratio");
            }
      require(result.left + result.right > 1e-9, "pan test is silent");
      return result;
      }
static void side(const Energy& value, bool left)
      {
      const double wanted = left ? value.left : value.right, other = left ? value.right : value.left;
      if (!(wanted > 0 && other < wanted * .001))
            std::cerr << "wanted left=" << left << " L=" << value.left << " R=" << value.right << '\n';
      require(wanted > 0 && other < wanted * .001, "intrinsically panned sample did not reach the opposite side");
      }
static double frequency(Fluid2SynthBanks& engine)
      {
      std::vector<float> dry(44100, 0);
      require(engine.process(22050, dry.data()), "pressure render failed");
      unsigned crossings = 0;
      for (unsigned n = 4411; n < 22050; ++n)
            if (dry[(n - 1) * 2] <= 0 && dry[n * 2] > 0) ++crossings;
      return crossings * 44100.0 / (22050 - 4411);
      }
static void panning(const char* path, const Fluid2Test::Modulation& spec, unsigned groups, bool isolated)
      {
      require(Fluid2Test::writeFont(path, nullptr, true, -1, 69, 0, 0, false, &spec), "pan fixture failed");
      Fluid2SynthBanks engine;
      require(engine.init(44100, groups, isolated) && engine.loadSoundFont(path), "pan setup failed");
      for (int logical : (isolated ? std::vector<int> {0, 127, 128, 255} : std::vector<int> {0, 127, 128, 254})) {
            require(engine.allSoundsOff(-1) && engine.resetControllers(logical) && engine.select(logical, 0, 0), "pan reset failed");
            std::vector<float> drain(512, 0); engine.process(256, drain.data());
            require(engine.setSourceChannel(logical, 0) && engine.setSourceSend(0, .375f), "pan send setup failed");
            const auto target = engine.channel(logical);
            const int opposite = spec.pan < 0 ? 127 : 0;
            fluid_synth_cc(target.synth, target.channel, 10, opposite);
            engine.noteOn(logical, 69, 90, 0);
            // Include the font's initial envelope delay; no inspecting API call
            // between attack and the first output chunk.
            const Energy first = render(engine, 256, isolated);
            const Energy held = render(engine, 4096, isolated);
            if (spec.panAmount < 0) {
                  side(first, opposite == 0); side(held, opposite == 0);
                  }
            else if (spec.panAmount == 0) {
                  // An authored zero-amount instrument modulator disables CC10.
                  side(first, false); side(held, false);
                  }
            else {
                  // An authored amount 500 supersedes the host's default 1000.
                  require(std::abs(held.left - held.right) < .01 * (held.left + held.right), "authored pan replacement was forced to 1000");
                  }
            fluid_synth_cc(target.synth, target.channel, 10, spec.pan < 0 ? 0 : 127);
            render(engine, 512, isolated); // settle live pan ramp
            side(render(engine, 4096, isolated), spec.pan < 0);
            require(engine.resetControllers(logical), "held pan reset failed");
            render(engine, 512, isolated);
            side(render(engine, 4096, isolated), spec.pan < 0); // centered CC10 retains intrinsic sample pan
            require(engine.reinitialize(48000, groups, isolated, path, 440), "pan rate rebuild failed");
            const auto changed = engine.channel(logical);
            fluid_synth_cc(changed.synth, changed.channel, 10, opposite);
            engine.setSourceChannel(logical, 0);
            engine.noteOn(logical, 69, 90, 0);
            const Energy rebuilt = render(engine, 4096, isolated);
            if (spec.panAmount < 0) side(rebuilt, opposite == 0);
            else if (spec.panAmount == 0) side(rebuilt, false);
            // Return to the test's reference rate before another channel.
            require(engine.reinitialize(44100, groups, isolated, path, 440), "pan return rate failed");
            }
      }
static void pressure(const char* path, int source, unsigned groups, bool isolated)
      {
      Fluid2Test::Modulation spec; spec.pitchSource = source;
      require(Fluid2Test::writeFont(path, nullptr, true, -1, 69, 0, 0, false, &spec), "pressure fixture failed");
      Fluid2SynthBanks engine;
      require(engine.init(44100, groups, isolated) && engine.loadSoundFont(path), "pressure setup failed");
      for (int logical : (isolated ? std::vector<int> {0, 128, 255} : std::vector<int> {0, 128})) {
            require(engine.allSoundsOff(-1) && engine.resetControllers(logical) && engine.select(logical, 0, 0), "pressure reset failed");
            std::vector<float> drain(512, 0); engine.process(256, drain.data());
            const auto target = engine.channel(logical);
            engine.noteOn(logical, 69, 90, 0);
            const auto set = [&](int value) {
                  if (source == 0x88) fluid_synth_cc(target.synth, target.channel, 8, value);
                  else if (source == 13) fluid_synth_channel_pressure(target.synth, target.channel, value);
                  else fluid_synth_key_pressure(target.synth, target.channel, 69, value);
                  };
            for (int value : {0, 80, 127, 0}) {
                  set(value);
                  require(std::abs(frequency(engine) - 440 * std::pow(2.0, value / 127.0)) < 8, "authored pressure modulation or held update failed");
                  // No implicit balance route may mute a side; a CC8 authored
                  // pitch route must remain fully functional and centered.
                  if (source == 0x88) {
                        const Energy centered = render(engine, 4096);
                        require(std::abs(centered.left - centered.right) < .00001 * (centered.left + centered.right),
                              "implicit CC8 balance changed stereo output");
                        }
                  }
            if (source == 10) {
                  fluid_synth_key_pressure(target.synth, target.channel, 72, 127);
                  require(std::abs(frequency(engine) - 440) < 5, "poly pressure changed another key");
                  }
            require(engine.resetControllers(logical), "pressure CC121 failed");
            require(std::abs(frequency(engine) - 440 * std::pow(2.0, source == 10 ? 80.0 / 127 : 0)) < 8,
                  "reset lost held voice or authored pressure route");
            }
      }
static void velocityFilter(const char* path, unsigned groups, bool isolated)
      {
      require(Fluid2Test::writeFont(path), "filter fixture failed");
      Fluid2SynthBanks reference, candidate;
      for (auto* engine : {&reference, &candidate})
            require(engine->init(44100, groups, isolated) && engine->loadSoundFont(path) && engine->select(0, 0, 0), "filter setup failed");
      // Legacy disables this ambiguous default. Check upstream does likewise,
      // instead of changing timbre through an unnecessary host workaround.
      fluid_mod_t* mod = new_fluid_mod(); require(mod != nullptr, "filter mod allocation failed");
      fluid_mod_set_source1(mod, FLUID_MOD_VELOCITY, FLUID_MOD_NEGATIVE | FLUID_MOD_LINEAR);
      fluid_mod_set_source2(mod, FLUID_MOD_VELOCITY, FLUID_MOD_POSITIVE | FLUID_MOD_SWITCH);
      fluid_mod_set_dest(mod, GEN_FILTERFC); fluid_mod_set_amount(mod, 0);
      require(fluid_synth_add_default_mod(reference.synth(0), mod, FLUID_SYNTH_OVERWRITE) == FLUID_OK, "filter override failed");
      delete_fluid_mod(mod);
      for (int velocity : {40, 80, 127}) {
            for (auto* engine : {&reference, &candidate}) {
                  engine->allSoundsOff(-1);
                  std::vector<float> drain(512, 0); engine->process(256, drain.data());
                  const auto target = engine->channel(0);
                  fluid_synth_set_gen(target.synth, target.channel, GEN_FILTERFC, -2400);
                  engine->noteOn(0, 105, velocity, 0);
                  }
            std::vector<float> expected(8192, 0), actual = expected;
            require(reference.process(4096, expected.data()) && candidate.process(4096, actual.data()), "filter render failed");
            for (unsigned i = 0; i < actual.size(); ++i)
                  require(std::abs(actual[i] - expected[i]) < .000001f, "default velocity/filter behavior differs from legacy-disabled case");
            }
      }
int main(int argc, char** argv)
      {
      require(argc == 2, "usage: tst_fluid2_modulators OUTPUT_PREFIX");
      const std::string path = argv[1];
      for (unsigned mode : {0u, 16u, 128u}) {
            const unsigned groups = mode == 128 ? 0 : mode; const bool isolated = mode == 128;
            Fluid2Test::Modulation spec;
            spec.pan = 500; panning(path.c_str(), spec, groups, isolated);
            spec.pan = -500; panning(path.c_str(), spec, groups, isolated);
            spec.pan = 500; spec.panAmount = 0; panning(path.c_str(), spec, groups, isolated);
            spec.panAmount = 500; panning(path.c_str(), spec, groups, isolated);
            pressure(path.c_str(), 10, groups, isolated); pressure(path.c_str(), 13, groups, isolated);
            pressure(path.c_str(), 0x88, groups, isolated);
            velocityFilter(path.c_str(), groups, isolated);
            }
      std::remove(path.c_str());
      std::cout << "Legacy pan range/first block/live reset, authored pan/pressure and velocity-filter PCM passed\n";
      }
