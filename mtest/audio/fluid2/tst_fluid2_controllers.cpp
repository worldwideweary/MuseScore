// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "audio/midi/fluid2/synthbanks.h"
#include "testfont.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

using Ms::Fluid2SynthBanks;
static void require(bool ok, const char* why)
      { if (!ok) { std::cerr << why << '\n'; std::exit(EXIT_FAILURE); } }
static int cc(const Fluid2SynthBanks& engine, int logical, int controller)
      {
      const auto target = engine.channel(logical);
      int value = -1;
      require(fluid_synth_get_cc(target.synth, target.channel, controller, &value) == FLUID_OK, "cannot read CC");
      return value;
      }
static void defaults(const Fluid2SynthBanks& engine, int logical)
      {
      const auto target = engine.channel(logical);
      int range = -1, bend = -1;
      fluid_synth_get_pitch_wheel_sens(target.synth, target.channel, &range);
      fluid_synth_get_pitch_bend(target.synth, target.channel, &bend);
      require(range == 12 && bend == 8192, "legacy bend defaults missing");
      for (int controller : { 1, 7, 8, 10, 11, 39, 42, 43, 64, 66, 91, 93 }) {
            const int expected = controller == 7 || controller == 11 || controller == 43 ? 127
                  : controller == 10 || controller == 42 ? 64 : 0;
            require(cc(engine, logical, controller) == expected, "legacy volume/pan/expression/pedal defaults missing");
            }
      }
static double renderFrequency(Fluid2SynthBanks& engine)
      {
      std::vector<float> dry(44100, 0);
      require(engine.process(22050, dry.data()), "render failed");
      unsigned crossings = 0;
      for (unsigned n = 4411; n < 22050; ++n) {
            require(std::isfinite(dry[n * 2]), "nonfinite controller PCM");
            if (dry[(n - 1) * 2] <= 0 && dry[n * 2] > 0) ++crossings;
            }
      return crossings * 44100.0 / (22050 - 4411);
      }
static double frequency(Fluid2SynthBanks& engine, int logical)
      {
      require(engine.allSoundsOff(-1), "stop failed");
      std::vector<float> drain(512, 0);
      require(engine.process(256, drain.data()), "drain failed");
      engine.noteOn(logical, 69, 90, 0);
      return renderFrequency(engine);
      }
static void check(const std::string& font, const std::string& pressureFont, unsigned groups, bool isolated)
      {
      Fluid2SynthBanks engine;
      require(engine.init(44100, groups, isolated) && engine.loadSoundFont(font.c_str()) && engine.tune(440), "setup failed");
      require(!engine.resetControllers(-1) && !engine.resetControllers(256), "invalid reset channel accepted");
      if (!isolated) require(!engine.resetControllers(255), "unsafe physical channel 255 reset accepted");
      for (int logical : (isolated ? std::vector<int> {0, 127, 128, 255} : std::vector<int> {0, 127, 128, 254})) {
            defaults(engine, logical);
            require(engine.select(logical, 0, 0), "preset failed");
            const auto target = engine.channel(logical);
            fluid_synth_pitch_bend(target.synth, target.channel, 12288);
            require(std::abs(frequency(engine, logical) - 440 * std::sqrt(2.0)) < 5, "default bend is not six semitones at half wheel");
            // Explicit modern RPN changes still work; CC121 restores 37E's range.
            fluid_synth_cc(target.synth, target.channel, 101, 0);
            fluid_synth_cc(target.synth, target.channel, 100, 0);
            fluid_synth_cc(target.synth, target.channel, 6, 2);
            require(std::abs(frequency(engine, logical) - 440 * std::pow(2.0, 1.0 / 12)) < 5, "explicit RPN range was overridden");
            fluid_synth_cc(target.synth, target.channel, 7, 43);
            fluid_synth_cc(target.synth, target.channel, 10, 0);
            fluid_synth_cc(target.synth, target.channel, 11, 60);
            fluid_synth_set_gen(target.synth, target.channel, GEN_COARSETUNE, 3);
            const auto other = engine.channel(42);
            fluid_synth_cc(other.synth, other.channel, 7, 55);
            require(engine.select(logical, 1, 7) && engine.resetControllers(logical), "reset failed");
            defaults(engine, logical);
            require(cc(engine, 42, 7) == 55, "controller reset changed another channel");
            int sf = -1, bank = -1, program = -1;
            fluid_synth_get_program(target.synth, target.channel, &sf, &bank, &program);
            require(bank == 1 && program == 7 && fluid_synth_get_gen(target.synth, target.channel, GEN_COARSETUNE) == 0,
                  "reset lost preset or retained generator offset");
            require(std::abs(frequency(engine, logical) - 880) < 5, "reset did not restore actual pitch/preset");
            }
      require(engine.tune(442) && engine.select(0, 0, 0) && engine.resetControllers(0), "master tuning reset setup failed");
      require(std::abs(frequency(engine, 0) - 442) < 5, "controller reset lost master tuning");
      engine.allSoundsOff(-1);
      std::vector<float> drain(512, 0);
      engine.process(256, drain.data());
      engine.noteOn(0, 69, 90, 50);
      require(engine.resetControllers(0) && std::abs(renderFrequency(engine) - 442 * std::pow(2.0, 50.0 / 1200)) < 5,
            "controller reset killed a held key or lost per-note tuning");
      require(engine.tune(440) && engine.loadSoundFont(pressureFont.c_str()) && engine.select(0, 0, 0), "pressure font failed");
      const auto target = engine.channel(0);
      const double expected = 440 * std::pow(2.0, 80.0 / 127);
      require(engine.resetControllers(0) && std::abs(frequency(engine, 0) - expected) < 5, "default poly pressure is not 80 in actual PCM");
      fluid_synth_key_pressure(target.synth, target.channel, 69, 0);
      require(std::abs(frequency(engine, 0) - 440) < 5, "poly pressure zero failed");
      fluid_synth_key_pressure(target.synth, target.channel, 69, 127);
      require(std::abs(frequency(engine, 0) - 880) < 5, "poly pressure 127 failed");
      require(engine.resetControllers(0) && std::abs(frequency(engine, 0) - expected) < 5, "CC reset did not restore poly pressure");
      if (isolated) {
            require(engine.setSourceChannel(0, 0) && engine.setSourceSend(0, .375f), "send setup failed");
            fluid_synth_cc(target.synth, target.channel, 66, 127);
            fluid_synth_noteoff(target.synth, target.channel, 69);
            require(engine.resetControllers(0) && engine.sourceChannel(0) == 0 && engine.sourceSend(0) == .375f,
                  "controller reset lost source ownership/gain");
            std::vector<float> dry(88200, 0), send = dry;
            require(engine.process(44100, dry.data(), send.data()), "release render failed");
            for (unsigned i = 0; i < dry.size(); ++i)
                  require(std::abs(send[i] - .375f * dry[i]) < .00001f, "reset release tails changed send");
            require(std::all_of(dry.end() - 2048, dry.end(), [](float value) { return value == 0; }), "LV capture survived CC121");
            }
      }
int main(int argc, char** argv)
      {
      require(argc == 2, "usage: tst_fluid2_controllers OUTPUT_PREFIX");
      const std::string font = argv[1], pressureFont = font + ".pressure.sf2";
      require(Fluid2Test::writeFont(font.c_str(), nullptr, true, 7, 69, 12, 1)
            && Fluid2Test::writeFont(pressureFont.c_str(), nullptr, true, -1, 69, 0, 0, true), "fixture write failed");
      for (unsigned mode : { 0u, 16u, 128u }) check(font, pressureFont, mode == 128 ? 0 : mode, mode == 128);
      std::remove(font.c_str()); std::remove(pressureFont.c_str());
      std::cout << "Legacy controller defaults, RPN/bend PCM, poly pressure, reset/preset and LV sends passed\n";
      }
