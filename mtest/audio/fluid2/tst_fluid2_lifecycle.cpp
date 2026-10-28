// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "audio/midi/fluid2/synthbanks.h"
#include "testfont.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

using Ms::Fluid2SynthBanks;
static void require(bool ok, const char* why)
      { if (!ok) { std::cerr << why << '\n'; std::exit(EXIT_FAILURE); } }

#ifdef FLUID2_TEST_WRAP_LOAD
static unsigned loadCall = 0, failLoad = 0, redirectLoad = 0;
static const char* redirectPath = nullptr;
static unsigned synthCall = 0, failSynth = 0;
extern "C" int __real_fluid_synth_sfload(fluid_synth_t*, const char*, int);
extern "C" int __wrap_fluid_synth_sfload(fluid_synth_t* synth, const char* path, int reset)
      {
      if (++loadCall == failLoad) return FLUID_FAILED;
      return __real_fluid_synth_sfload(synth, loadCall == redirectLoad ? redirectPath : path, reset);
      }
extern "C" fluid_synth_t* __real_new_fluid_synth(fluid_settings_t*);
extern "C" fluid_synth_t* __wrap_new_fluid_synth(fluid_settings_t* settings)
      { return ++synthCall == failSynth ? nullptr : __real_new_fluid_synth(settings); }
#endif

static Fluid2Test::Bytes readFile(const char* path)
      {
      std::ifstream file(path, std::ios::binary);
      require(bool(file), "cannot read compressed fixture");
      return Fluid2Test::Bytes(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
      }

static int cc(const Fluid2SynthBanks& engine, int channel, int controller)
      {
      const auto target = engine.channel(channel);
      int value = -1;
      require(fluid_synth_get_cc(target.synth, target.channel, controller, &value) == FLUID_OK, "cannot read controller");
      return value;
      }

static std::vector<int> channels(bool isolated)
      { return isolated ? std::vector<int> { 0, 128, 255 } : std::vector<int> { 0, 128 }; }

static void setup(Fluid2SynthBanks& engine, const std::string& path, unsigned groups, bool isolated)
      {
      require(engine.init(44100, groups, isolated) && engine.loadSoundFont(path.c_str()), "initial load failed");
      require(engine.tune(442), "master tuning failed");
      for (int ch : channels(isolated)) {
            require(engine.select(ch, ch ? 128 : 0, 0), "preset selection failed");
            require(engine.setSourceChannel(ch, ch) && engine.setSourceSend(ch, ch ? .75f : .375f), "send setup failed");
            const auto target = engine.channel(ch);
            require(fluid_synth_cc(target.synth, target.channel, 7, 91 + (ch ? 1 : 0)) == FLUID_OK
                  && fluid_synth_cc(target.synth, target.channel, 10, 33 + (ch ? 1 : 0)) == FLUID_OK
                  && fluid_synth_cc(target.synth, target.channel, 11, 103) == FLUID_OK
                  && fluid_synth_pitch_wheel_sens(target.synth, target.channel, 12) == FLUID_OK, "controller setup failed");
            engine.noteOn(ch, 69, 90, 50);
            }
      }

static void compareAudio(Fluid2SynthBanks& engine, Fluid2SynthBanks& reference)
      {
      double energy = 0;
      for (unsigned n : { 1u, 63u, 513u, 4097u }) {
            std::vector<float> dry(n * 2, 0), expected = dry, send = dry, expectedSend = dry;
            require(engine.process(n, dry.data(), engine.isolated() ? send.data() : nullptr)
                  && reference.process(n, expected.data(), reference.isolated() ? expectedSend.data() : nullptr), "render failed");
            for (unsigned i = 0; i < dry.size(); ++i) {
                  require(std::isfinite(dry[i]) && std::abs(dry[i] - expected[i]) < .00001f, "failed rebuild changed dry PCM");
                  require(std::isfinite(send[i]) && std::abs(send[i] - expectedSend[i]) < .00001f, "failed rebuild changed send PCM");
                  energy += dry[i] * dry[i];
                  }
            }
      require(energy > .00001, "held voices disappeared after failure");
      }

static void rollback(const std::string& path, const std::string& empty, unsigned groups, bool isolated)
      {
      Fluid2SynthBanks engine, reference;
      setup(engine, path, groups, isolated); setup(reference, path, groups, isolated);
      compareAudio(engine, reference);
      for (auto* current : { &engine, &reference }) {
            for (int ch : channels(isolated)) {
                  const auto target = current->channel(ch);
                  fluid_synth_cc(target.synth, target.channel, 66, 127);
                  fluid_synth_noteoff(target.synth, target.channel, 69);
                  }
            }
      const auto* low = engine.synth(0);
      const auto* high = engine.synth(1);
      const int font = engine.fontId(0);
      const auto check = [&]() {
            require(engine.synth(0) == low && engine.synth(1) == high && engine.fontId(0) == font
                  && engine.sampleRate() == 44100 && engine.isolated() == isolated, "failed rebuild committed partial engine");
            for (int ch : channels(isolated)) {
                  require(cc(engine, ch, 66) == 127 && engine.sourceChannel(ch) == ch, "failure lost captures/ownership");
                  require(engine.sourceSend(ch) == (ch ? .75f : .375f), "failure lost gain");
                  }
            compareAudio(engine, reference);
            };
      require(!engine.reinitialize(48000, groups, isolated, (path + ".missing").c_str(), 442), "missing font accepted");
      check();
      require(!engine.reinitialize(48000, groups, isolated, empty.c_str(), 442), "empty font accepted");
      check();
      for (float rate : { 0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN(), 100000.0f }) {
            require(!engine.reinitialize(rate, groups, isolated, path.c_str(), 442), "invalid rate accepted");
            check();
            }
#ifdef FLUID2_TEST_WRAP_LOAD
      for (unsigned bank = 1; bank <= (isolated ? 2u : 1u); ++bank) {
            loadCall = 0; failLoad = bank;
            require(!engine.reinitialize(48000, groups, isolated, path.c_str(), 442), "injected bank load accepted");
            failLoad = 0; check();
            loadCall = 0; redirectLoad = bank; redirectPath = empty.c_str();
            require(!engine.reinitialize(48000, groups, isolated, path.c_str(), 442), "injected empty bank accepted");
            redirectLoad = 0; check();
            synthCall = 0; failSynth = bank;
            require(!engine.reinitialize(48000, groups, isolated, path.c_str(), 442), "injected allocation failure accepted");
            failSynth = 0; check();
            }
#endif
      }

static void transitions(const std::string& path, unsigned groups, bool isolated)
      {
      Fluid2SynthBanks engine;
      setup(engine, path, groups, isolated);
      for (float rate : { 48000.0f, 22050.0f, 96000.0f, 44100.0f }) {
            for (int ch : channels(isolated)) {
                  const auto target = engine.channel(ch);
                  fluid_synth_cc(target.synth, target.channel, 64, 127);
                  fluid_synth_cc(target.synth, target.channel, 66, 127);
                  fluid_synth_pitch_bend(target.synth, target.channel, 8700);
                  }
            require(engine.reinitialize(rate, groups, isolated, path.c_str(), 442), "valid rate change failed");
            require(engine.sampleRate() == rate && engine.bankCount() == (isolated ? 2u : 1u)
                  && engine.groupCount() == (isolated ? 128u : groups), "wrong replacement layout/rate");
            for (int ch : channels(isolated)) {
                  const auto target = engine.channel(ch);
                  int sf = -1, bank = -1, program = -1, range = -1, bend = -1;
                  fluid_synth_get_program(target.synth, target.channel, &sf, &bank, &program);
                  fluid_synth_get_pitch_wheel_sens(target.synth, target.channel, &range);
                  fluid_synth_get_pitch_bend(target.synth, target.channel, &bend);
                  require(sf == target.font && bank == (ch ? 128 : 0) && program == 0, "preset lost on rate change");
                  require(cc(engine, ch, 7) == 91 + (ch ? 1 : 0) && cc(engine, ch, 10) == 33 + (ch ? 1 : 0)
                        && cc(engine, ch, 11) == 103 && range == 12, "mix controls/wheel range lost");
                  require(cc(engine, ch, 64) == 0 && cc(engine, ch, 66) == 0 && bend == 8192, "old pedal/bend state resumed");
                  require(engine.sourceSend(ch) == (ch ? .75f : .375f) && engine.sourceChannel(ch) == -1, "gain/ownership incorrect");
                  }
            std::vector<float> dry(8192, 0), send = dry;
            require(engine.process(4096, dry.data(), isolated ? send.data() : nullptr), "silent render failed");
            require(std::all_of(dry.begin(), dry.end(), [](float v) { return v == 0; })
                  && std::all_of(send.begin(), send.end(), [](float v) { return v == 0; }), "old voice/PCM survived successful change");
            engine.setSourceChannel(0, 0);
            engine.noteOn(0, 69, 90, 50);
            const unsigned frames = unsigned(rate / 2);
            dry.assign(frames * 2, 0); send = dry;
            require(engine.process(frames, dry.data(), isolated ? send.data() : nullptr), "new-rate render failed");
            double energy = 0;
            unsigned crossings = 0;
            const unsigned start = unsigned(rate / 10);
            for (unsigned n = 0; n < frames; ++n) {
                  require(std::isfinite(dry[n * 2]) && std::isfinite(send[n * 2]), "nonfinite replacement PCM");
                  energy += dry[n * 2] * dry[n * 2];
                  if (n > start && dry[(n - 1) * 2] <= 0 && dry[n * 2] > 0) ++crossings;
                  if (isolated) require(std::abs(send[n * 2] - .375f * dry[n * 2]) < .00001f, "send gain changed on reload");
                  }
            const double hz = crossings * rate / (frames - start - 1);
            require(energy > .00001 && std::abs(hz - 442 * std::pow(2.0, 50.0 / 1200)) < 5.0,
                  "replacement rate/master/per-note tuning incorrect");
            }
      require(engine.loadSoundFont("") && engine.reinitialize(48000, groups, isolated, "", 442), "empty-state reinit failed");
      require(engine.fontId(0) == -1 && engine.sourceSend(0) == .375f, "empty state lost gain or resurrected font");
      require(engine.loadSoundFont(path.c_str()) && engine.select(0, 0, 0), "reload from empty state failed");
      }

int main(int argc, char** argv)
      {
      require(argc == 3, "usage: tst_fluid2_lifecycle OUTPUT_PREFIX VORBIS_FIXTURE");
      const std::string sf2 = std::string(argv[1]) + ".sf2", sf3 = std::string(argv[1]) + ".sf3", empty = std::string(argv[1]) + ".empty.sf2";
      const auto compressed = readFile(argv[2]);
      require(Fluid2Test::writeFont(sf2.c_str()) && Fluid2Test::writeFont(sf3.c_str(), &compressed)
            && Fluid2Test::writeFont(empty.c_str(), nullptr, false), "fixture creation failed");
      for (const auto& path : { sf2, sf3 }) {
            for (unsigned layout : { 0u, 1u, 16u, 128u }) {
                  const bool isolated = layout == 128;
                  const unsigned groups = isolated ? 0 : layout;
                  rollback(path, empty, groups, isolated);
                  transitions(path, groups, isolated);
                  }
            }
      std::remove(sf2.c_str()); std::remove(sf3.c_str()); std::remove(empty.c_str());
      std::cout << "Sample-rate lifecycle: atomic SF2/SF3 rebuild, bank failures, controls, tuning and dry/send PCM passed\n";
      }
