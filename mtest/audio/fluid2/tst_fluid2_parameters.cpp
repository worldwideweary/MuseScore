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
static void cc(Fluid2SynthBanks& engine, int channel, int controller, int value)
      { require(engine.controlChange(channel, controller, value), "controller rejected"); }
static double gen(const Fluid2SynthBanks& engine, int channel, int generator)
      {
      const auto target = engine.channel(channel);
      return fluid_synth_get_gen(target.synth, target.channel, generator);
      }
static std::vector<float> render(Fluid2SynthBanks& engine, double seconds)
      {
      const unsigned frames = unsigned(seconds * engine.sampleRate());
      std::vector<float> dry(frames * 2, 0);
      require(engine.process(frames, dry.data()), "render failed");
      for (float sample : dry) require(std::isfinite(sample), "nonfinite parameter PCM");
      return dry;
      }
static double frequency(const std::vector<float>& dry, double rate, double start, double end)
      {
      unsigned crossings = 0;
      for (unsigned n = unsigned(start * rate) + 1; n < unsigned(end * rate); ++n)
            if (dry[(n - 1) * 2] <= 0 && dry[n * 2] > 0) ++crossings;
      return crossings / (end - start);
      }
static void pitch(Fluid2SynthBanks& engine, double expected)
      {
      const auto dry = render(engine, .5);
      require(std::abs(frequency(dry, engine.sampleRate(), .1, .5) - expected) < 6, "parameter PCM pitch mismatch");
      }
static void stop(Fluid2SynthBanks& engine)
      { require(engine.allSoundsOff(-1), "stop failed"); render(engine, .01); }
static void nrpn(Fluid2SynthBanks& engine, int channel, int generator)
      { cc(engine, channel, 99, 120); cc(engine, channel, 98, generator); }
static void rpn(Fluid2SynthBanks& engine, int channel, int parameter)
      { cc(engine, channel, 101, 0); cc(engine, channel, 100, parameter); }
static void check(const std::string& font, unsigned groups, bool isolated)
      {
      Fluid2SynthBanks engine;
      require(engine.init(44100, groups, isolated) && engine.loadSoundFont(font.c_str()) && engine.tune(440), "setup failed");
      require(!engine.controlChange(-1, 6, 64) && !engine.controlChange(256, 6, 64)
            && !engine.controlChange(0, -1, 64) && !engine.controlChange(0, 128, 64)
            && !engine.controlChange(0, 6, -1) && !engine.controlChange(0, 6, 128), "invalid CC accepted");
      if (!isolated) require(!engine.controlChange(255, 6, 64), "unsafe physical 255 accepted");
      for (int channel : (isolated ? std::vector<int> {0, 127, 128, 255} : std::vector<int> {0, 128, 254})) {
            stop(engine);
            require(engine.select(channel, 0, 0) && engine.resetControllers(channel), "channel setup failed");
            engine.noteOn(channel, 69, 90, 0);
            nrpn(engine, channel, GEN_COARSETUNE);
            cc(engine, channel, 38, 12); cc(engine, channel, 6, 64);
            pitch(engine, 880);
            // Select once, repeatedly update the same held note. Before v0.30
            // the second write went to sample-start offset, leaving +12 here.
            cc(engine, channel, 38, 0); cc(engine, channel, 6, 64);
            pitch(engine, 440);
            cc(engine, channel, 38, 7); cc(engine, channel, 6, 64);
            pitch(engine, 440 * std::pow(2.0, 7.0 / 12));
            require(gen(engine, channel, GEN_STARTADDROFS) == 0 && gen(engine, 42, GEN_COARSETUNE) == 0,
                  "NRPN changed wrong generator/channel");
            cc(engine, channel, 38, 12); cc(engine, channel, 6, 64);
            const auto target = engine.channel(channel);
            require(fluid_synth_set_gen(target.synth, target.channel, GEN_FILTERFC, -1200) == FLUID_OK, "filter setup failed");
            require(engine.reinitialize(48000, groups, isolated, font.c_str(), 440), "48k replacement failed");
            require(gen(engine, channel, GEN_FILTERFC) == -1200, "generator offset lost on replacement");
            engine.noteOn(channel, 69, 90, 0); pitch(engine, 880);
            // Data-entry LSB and selector survive the rebuild, too.
            cc(engine, channel, 6, 64); pitch(engine, 880);
            cc(engine, channel, 38, 0); cc(engine, channel, 6, 64); pitch(engine, 440);
            require(!engine.reinitialize(44100, groups, isolated, "/missing/audio-v30.sf2", 440), "missing font accepted");
            require(engine.sampleRate() == 48000, "failed reload changed active engine");
            cc(engine, channel, 38, 12); cc(engine, channel, 6, 64); pitch(engine, 880);
            // Modern RPN behavior remains available and does not re-enter NRPN.
            rpn(engine, channel, 2); cc(engine, channel, 6, 64); pitch(engine, 440);
            rpn(engine, channel, 1); cc(engine, channel, 38, 0); cc(engine, channel, 6, 96);
            pitch(engine, 440 * std::pow(2.0, 50.0 / 1200));
            require(engine.reinitialize(44100, groups, isolated, font.c_str(), 440), "44.1k replacement failed");
            engine.noteOn(channel, 69, 90, 0); pitch(engine, 440 * std::pow(2.0, 50.0 / 1200));
            cc(engine, channel, 6, 64); pitch(engine, 440); // retained RPN1
            rpn(engine, channel, 0); cc(engine, channel, 6, 2);
            auto current = engine.channel(channel);
            fluid_synth_pitch_bend(current.synth, current.channel, 12288);
            pitch(engine, 440 * std::pow(2.0, 1.0 / 12));
            cc(engine, channel, 101, 127); cc(engine, channel, 100, 127); cc(engine, channel, 6, 12);
            pitch(engine, 440 * std::pow(2.0, 1.0 / 12)); // null RPN is inert
            cc(engine, channel, 121, 0); cc(engine, channel, 6, 96); pitch(engine, 440);
            require(gen(engine, channel, GEN_FILTERFC) == 0, "CC121 retained generator offset");
            cc(engine, channel, 99, 1); cc(engine, channel, 98, 51); cc(engine, channel, 6, 76);
            pitch(engine, 440); // unrelated NRPN must not execute stale RPN
            cc(engine, channel, 99, 120); cc(engine, channel, 98, 100); cc(engine, channel, 6, 96);
            pitch(engine, 440); // incomplete selector must not target generator 0
            // Accumulated indices, malformed sequences, and reset recovery.
            cc(engine, channel, 99, 120); cc(engine, channel, 98, 20); cc(engine, channel, 98, 31);
            cc(engine, channel, 38, 12); cc(engine, channel, 6, 64); pitch(engine, 880);
            cc(engine, channel, 99, 120);
            for (int n = 0; n < 10000; ++n) cc(engine, channel, 98, 102);
            cc(engine, channel, 98, 0); cc(engine, channel, 6, 96);
            pitch(engine, 880);
            require(gen(engine, channel, GEN_STARTADDROFS) == 0, "invalid index fell through to generator zero");
            nrpn(engine, channel, GEN_COARSETUNE); cc(engine, channel, 38, 0); cc(engine, channel, 6, 64); pitch(engine, 440);
            cc(engine, channel, 121, 0);
            }
      // Legacy explicit CC84 glides over CC5/37 milliseconds, with CC65 either
      // off or on. Check real PCM at the beginning, middle and end of the glide.
      for (int enabled : {0, 127}) {
            stop(engine); require(engine.resetControllers(0) && engine.select(0, 0, 0), "portamento setup failed");
            cc(engine, 0, 5, 4); cc(engine, 0, 37, 0); cc(engine, 0, 65, enabled); cc(engine, 0, 84, 60);
            engine.noteOn(0, 72, 90, 0);
            const auto dry = render(engine, .9);
            require(std::abs(frequency(dry, 44100, .04, .12) - 261.626 * std::pow(2.0, .08 / .512)) < 20
                  && std::abs(frequency(dry, 44100, .24, .32) - 261.626 * std::pow(2.0, .28 / .512)) < 20
                  && std::abs(frequency(dry, 44100, .65, .85) - 523.251) < 8, "explicit portamento timing/pitch changed");
            // CC84 is consumed by one attack. Disable automatic portamento
            // before the next attack so the check isolates the one-shot key.
            stop(engine); cc(engine, 0, 65, 0); engine.noteOn(0, 69, 90, 0);
            pitch(engine, 440);
            }
      }
int main(int argc, char** argv)
      {
      require(argc == 2, "usage: tst_fluid2_parameters OUTPUT_SF2");
      const std::string font = argv[1];
      require(Fluid2Test::writeFont(font.c_str()), "fixture write failed");
      for (unsigned mode : {0u, 16u, 128u}) check(font, mode == 128 ? 0 : mode, mode == 128);
      std::remove(font.c_str());
      std::cout << "NRPN/RPN held PCM, reset, rate/reload retention and explicit portamento passed\n";
      }
