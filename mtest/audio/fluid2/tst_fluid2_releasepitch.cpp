// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "audio/midi/fluid2/synthbanks.h"
#include "testfont.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>

using Ms::Fluid2SynthBanks;
static void require(bool value, const char* message)
      { if (!value) { std::cerr << message << '\n'; std::exit(EXIT_FAILURE); } }
static void setup(Fluid2SynthBanks& engine, const char* path, int rate, unsigned mode)
      {
      require(engine.init(rate, mode == 1 ? 16 : 0, mode == 2) && engine.loadSoundFont(path), "setup failed");
      int enabled = 0;
      require(fluid_settings_getint(fluid_synth_get_settings(engine.synth(0)),
                  "synth.mscore-release-pitch-lock", &enabled) == FLUID_OK && enabled == 1,
              "bundled release pitch lock not enabled");
      for (int channel : {0, 127, 128, 255})
            if (mode == 2 || channel != 255) require(engine.select(channel, 0, 0), "preset failed");
      }
static std::vector<float> render(Fluid2SynthBanks& engine, unsigned frames)
      {
      std::vector<float> out(frames * 2, 0);
      require(engine.process(frames, out.data()), "render failed");
      return out;
      }
static void wheel(Fluid2SynthBanks& engine, int channel, int value)
      { auto c = engine.channel(channel); require(fluid_synth_pitch_bend(c.synth, c.channel, value) == FLUID_OK, "wheel failed"); }
static void off(Fluid2SynthBanks& engine, int channel, int key)
      { auto c = engine.channel(channel); require(fluid_synth_noteoff(c.synth, c.channel, key) == FLUID_OK, "note-off failed"); }
static void equal(const std::vector<float>& a, const std::vector<float>& b, bool exact)
      {
      require(a.size() == b.size(), "size differs");
      double energy = 0;
      for (unsigned i = 0; i < a.size(); ++i) {
            require(exact ? a[i] == b[i] : std::abs(a[i] - b[i]) < 0.00001f, "release/held PCM differs");
            energy += a[i] * a[i];
            }
      require(energy > 0.000001, "test tail unexpectedly silent");
      }

static void check(const char* path, int rate, unsigned mode, int channel, int pedal)
      {
      Fluid2SynthBanks reference, candidate;
      setup(reference, path, rate, mode); setup(candidate, path, rate, mode);
      for (auto* e : {&reference, &candidate}) e->noteOn(channel, 60, 90, 0);
      equal(render(reference, 4096), render(candidate, 4096), true);
      if (pedal) {
            // The reference remains key-down. The candidate is pedal-held:
            // both must still follow the live wheel before actual release.
            candidate.controlChange(channel, pedal, 127);
            off(candidate, channel, 60);
            }
      const int bent = 8192 + 8192 * 2 / 12;
      wheel(reference, channel, bent); wheel(candidate, channel, bent);
      equal(render(reference, 4096), render(candidate, 4096), true);
      off(reference, channel, 60);
      if (pedal) candidate.controlChange(channel, pedal, 0);
      else off(candidate, channel, 60);
      wheel(candidate, channel, 8192);
      auto c = candidate.channel(channel);
      // A new range must not retune an old tail either.
      fluid_synth_pitch_wheel_sens(c.synth, c.channel, 2);
      equal(render(reference, 512), render(candidate, 512), true);
      // A full modulator recomputation must also retain the old wheel sources.
      candidate.controlChange(channel, 121, 0);
      equal(render(reference, 512), render(candidate, 512), true);
      int bend = 0;
      fluid_synth_get_pitch_bend(c.synth, c.channel, &bend);
      require(bend == 8192, "channel did not reset for the next attack");
      // D follows on the reset channel while the old C-to-D tail keeps D.
      // The reference uses another centered channel for its independent D.
      const int other = channel == 0 ? 127 : 0;
      reference.noteOn(other, 62, 90, 0);
      candidate.noteOn(channel, 62, 90, 0);
      equal(render(reference, 2048), render(candidate, 2048), false);
      for (auto* e : {&reference, &candidate}) {
            e->controlChange(channel, 120, 0);
            e->controlChange(other, 120, 0);
            render(*e, 256);
            wheel(*e, channel, 8192);
            auto mapped = e->channel(channel);
            fluid_synth_pitch_wheel_sens(mapped.synth, mapped.channel, 12);
            e->noteOn(channel, 64, 90, 0);
            }
      equal(render(reference, 2048), render(candidate, 2048), false);
      }

static void checkExclusive(const char* path)
      {
      Fluid2SynthBanks reference, candidate;
      setup(reference, path, 44100, 2); setup(candidate, path, 44100, 2);
      for (auto* e : {&reference, &candidate}) {
            wheel(*e, 255, 8192 + 8192 * 2 / 12);
            e->noteOn(255, 60, 90, 0); render(*e, 4096);
            e->noteOn(255, 62, 90, 0); // forces the first voice into exclusive-class release
            render(*e, 512);
            off(*e, 255, 62);
            }
      wheel(candidate, 255, 8192);
      equal(render(reference, 1024), render(candidate, 1024), true);
      }

static void checkStandalone(const char* path, bool lock)
      {
      fluid_synth_t* engines[2] = {};
      fluid_settings_t* settings[2] = {};
      const auto pcm = [](fluid_synth_t* synth, unsigned frames) {
            std::vector<float> out(frames * 2);
            require(fluid_synth_write_float(synth, frames, out.data(), 0, 2, out.data(), 1, 2) == FLUID_OK,
                    "standalone render failed");
            return out;
            };
      for (unsigned i = 0; i < 2; ++i) {
            settings[i] = new_fluid_settings();
            fluid_settings_setint(settings[i], "synth.reverb.active", 0);
            fluid_settings_setint(settings[i], "synth.chorus.active", 0);
            fluid_settings_setint(settings[i], "synth.mscore-release-pitch-lock", lock);
            engines[i] = new_fluid_synth(settings[i]);
            require(engines[i] && fluid_synth_sfload(engines[i], path, 1) >= 0, "standalone setup failed");
            fluid_synth_pitch_wheel_sens(engines[i], 0, 12);
            fluid_synth_noteon(engines[i], 0, 60, 90);
            pcm(engines[i], 4096);
            fluid_synth_pitch_bend(engines[i], 0, 8192 + 8192 * 2 / 12);
            pcm(engines[i], 4096);
            fluid_synth_noteoff(engines[i], 0, 60);
            }
      fluid_synth_pitch_bend(engines[1], 0, 8192);
      auto a = pcm(engines[0], 4096), b = pcm(engines[1], 4096);
      if (lock) equal(a, b, true);
      else {
            double difference = 0;
            for (unsigned i = 0; i < a.size(); ++i) difference += std::abs(a[i] - b[i]);
            require(difference > .001, "disabled lock must retain upstream wheel-sensitive releases");
            }
      for (unsigned i = 0; i < 2; ++i) {
            delete_fluid_synth(engines[i]); delete_fluid_settings(settings[i]);
            }
      }

int main(int argc, char** argv)
      {
      require(argc == 2, "supply a generated SF2 path");
      fluid_settings_t* defaults = new_fluid_settings();
      int value = -1;
      require(fluid_settings_getint(defaults, "synth.mscore-release-pitch-lock", &value) == FLUID_OK && value == 0,
              "standalone default behavior changed");
      delete_fluid_settings(defaults);
      for (unsigned route = 0; route < 3; ++route) {
            Fluid2Test::Modulation modulation;
            if (route == 1) modulation.pitchSource = 0x020e;
            if (route == 2) { modulation.pitchSource = 2; modulation.pitchAmountSource = 0x020e; }
            require(Fluid2Test::writeFont(argv[1], nullptr, true, -1, 69, 0, 0, false, &modulation), "font write failed");
            for (int rate : {44100, 48000})
                  for (unsigned mode : {0u, 1u, 2u})
                        for (int pedal : {0, 64, 66})
                              for (int channel : (mode == 2 ? std::vector<int>{0, 255} : std::vector<int>{0}))
                                    check(argv[1], rate, mode, channel, pedal);
            checkStandalone(argv[1], false); checkStandalone(argv[1], true);
            }
      require(Fluid2Test::writeFont(argv[1], nullptr, true, -1, 69, 0, 0, false, nullptr, 0, 1), "exclusive font failed");
      checkExclusive(argv[1]);
      require(std::remove(argv[1]) == 0, "font cleanup failed");
      std::cout << "Release pitch: tail lock, held/pedal bends, next centered attack and rates/modes passed\n";
      }
