// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "audio/midi/fluid2/synthbanks.h"
#include "testfont.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

using Ms::Fluid2SynthBanks;
static void require(bool condition, const char* message)
      { if (!condition) { std::cerr << message << '\n'; std::exit(EXIT_FAILURE); } }

#ifdef FLUID2_TEST_WRAP_LOAD
static unsigned loadCall = 0, failLoad = 0;
static unsigned redirectLoad = 0;
static const char* redirectPath = nullptr;
extern "C" int __real_fluid_synth_sfload(fluid_synth_t*, const char*, int);
extern "C" int __wrap_fluid_synth_sfload(fluid_synth_t* synth, const char* path, int reset)
      {
      if (++loadCall == failLoad) return FLUID_FAILED;
      if (loadCall == redirectLoad) path = redirectPath;
      return __real_fluid_synth_sfload(synth, path, reset);
      }
#endif

static Fluid2Test::Bytes readFile(const char* path)
      {
      std::ifstream file(path, std::ios::binary);
      require(bool(file), "cannot open Vorbis test fixture");
      return Fluid2Test::Bytes(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
      }
static void writeFile(const std::string& path, const Fluid2Test::Bytes& data)
      {
      std::ofstream file(path, std::ios::binary);
      file.write(reinterpret_cast<const char*>(data.data()), std::streamsize(data.size()));
      require(bool(file), "cannot write invalid-font fixture");
      }
static unsigned voices(fluid_synth_t* synth)
      {
      std::array<fluid_voice_t*, 512> list {};
      fluid_synth_get_voicelist(synth, list.data(), int(list.size()), -1);
      return unsigned(std::count_if(list.begin(), list.end(), [](fluid_voice_t* v) { return v != nullptr; }));
      }
static void inspect(const Fluid2SynthBanks& engine)
      {
      for (unsigned bank = 0; bank < engine.bankCount(); ++bank) {
            require(fluid_synth_sfcount(engine.synth(bank)) == 1, "staged font leaked or active font disappeared");
            auto* font = fluid_synth_get_sfont_by_id(engine.synth(bank), engine.fontId(bank));
            require(font != nullptr, "active font ID is invalid");
            fluid_sfont_iteration_start(font);
            auto* melodic = fluid_sfont_iteration_next(font);
            require(melodic && fluid_preset_get_banknum(melodic) == 0
                  && fluid_preset_get_num(melodic) == 0, "melodic preset missing after load/rollback");
            auto* drum = fluid_sfont_iteration_next(font);
            require(drum && fluid_preset_get_banknum(drum) == 128, "drum preset missing after load/rollback");
            require(fluid_sfont_iteration_next(font) == nullptr, "unexpected preset in fixture");
            }
      }
static void setup(Fluid2SynthBanks& engine, const std::string& path, bool isolated)
      {
      require(engine.init(44100, 0, isolated), "engine initialization failed");
      require(engine.loadSoundFont(path.c_str()), "valid SF2/SF3 failed to load (SF3 decoding is required)");
      require(engine.tune(442), "master tuning failed");
      for (int ch : { 0, 128 }) {
            require(engine.select(ch, ch ? 128 : 0, 0), "initial preset selection failed");
            require(engine.setSourceChannel(ch, ch) && engine.setSourceSend(ch, ch ? .75f : .375f), "source setup failed");
            auto target = engine.channel(ch);
            require(fluid_synth_cc(target.synth, target.channel, 7, 91) == FLUID_OK, "volume setup failed");
            require(fluid_synth_pitch_bend(target.synth, target.channel, 8500) == FLUID_OK, "bend setup failed");
            engine.noteOn(ch, 69, 90, ch ? -25.0f : 50.0f);
            }
      }
static void compareAudio(Fluid2SynthBanks& actual, Fluid2SynthBanks& reference)
      {
      double energy = 0;
      for (unsigned frames : { 1u, 63u, 513u, 4097u }) {
            std::vector<float> a(frames * 2, 0), b = a, sendA = a, sendB = a;
            require(actual.process(frames, a.data(), actual.isolated() ? sendA.data() : nullptr)
                  && reference.process(frames, b.data(), reference.isolated() ? sendB.data() : nullptr), "render failed");
            for (unsigned i = 0; i < a.size(); ++i) {
                  require(std::isfinite(a[i]) && std::abs(a[i] - b[i]) < .00001f, "failed load changed held-note PCM");
                  require(std::isfinite(sendA[i]) && std::abs(sendA[i] - sendB[i]) < .00001f,
                        "failed load changed source-send PCM");
                  energy += double(a[i]) * a[i];
                  }
            }
      require(energy > .00001, "held-note audio is silent");
      }

static void failures(const std::string& active, const std::vector<std::string>& invalid, bool isolated)
      {
      Fluid2SynthBanks engine, reference;
      setup(engine, active, isolated); setup(reference, active, isolated);
      compareAudio(engine, reference);
      for (auto* synth : { &engine, &reference }) {
            for (int ch : { 0, 128 }) {
                  auto target = synth->channel(ch);
                  require(fluid_synth_cc(target.synth, target.channel, 66, 127) == FLUID_OK, "LV capture failed");
                  require(fluid_synth_noteoff(target.synth, target.channel, 69) == FLUID_OK, "LV note-off failed");
                  }
            }
      const int low = engine.fontId(0), high = engine.fontId(1);
      const unsigned lowVoices = voices(engine.synth(0));
      const unsigned highVoices = isolated ? voices(engine.synth(1)) : 0;
      const auto check = [&]() {
            require(engine.fontId(0) == low && engine.fontId(1) == high, "failed replacement changed active font IDs");
            require(voices(engine.synth(0)) == lowVoices && (!isolated || voices(engine.synth(1)) == highVoices),
                  "failed replacement killed held voices");
            require(engine.sourceChannel(0) == 0 && engine.sourceChannel(128) == 128
                  && engine.sourceSend(0) == .375f && engine.sourceSend(128) == .75f, "failed load changed ownership/gains");
            for (int ch : { 0, 128 }) {
                  auto target = engine.channel(ch);
                  int value = 0;
                  require(fluid_synth_get_cc(target.synth, target.channel, 66, &value) == FLUID_OK && value == 127,
                        "failed load released LV");
                  require(fluid_synth_get_cc(target.synth, target.channel, 7, &value) == FLUID_OK && value == 91,
                        "failed load changed volume");
                  }
            inspect(engine);
            compareAudio(engine, reference);
            };
      for (const auto& file : invalid) {
            require(!engine.loadSoundFont(file.c_str()), "invalid/no-preset replacement accepted");
            check();
            }
#ifdef FLUID2_TEST_WRAP_LOAD
      for (unsigned bank = 1; bank <= engine.bankCount(); ++bank) {
            loadCall = 0; failLoad = bank;
            const bool accepted = engine.loadSoundFont(active.c_str());
            failLoad = 0;
            require(!accepted, "injected load failure accepted");
            check();
            }
      if (isolated) {
            // Also roll back both new objects when bank 2 loads successfully
            // but has no presets, rather than returning a load error.
            loadCall = 0; redirectLoad = 2; redirectPath = invalid.back().c_str();
            const bool accepted = engine.loadSoundFont(active.c_str());
            redirectLoad = 0; redirectPath = nullptr;
            require(!accepted, "second-bank empty font accepted");
            check();
            }
#endif
      // The retained presets must still start new notes, not only finish old ones.
      for (auto* synth : { &engine, &reference }) synth->noteOn(128, 72, 90, 100.0f);
      compareAudio(engine, reference);
      }

static void replacement(const std::string& sf2, const std::string& sf3, bool isolated)
      {
      Fluid2SynthBanks engine;
      setup(engine, sf2, isolated);
      for (const auto& next : { sf3, sf2, sf3 }) {
            require(engine.loadSoundFont(next.c_str()), "valid replacement failed");
            inspect(engine);
            require(engine.sourceChannel(0) < 0 && engine.sourceChannel(128) < 0, "replacement retained old source ownership");
            require(engine.sourceSend(0) == .375f && engine.sourceSend(128) == .75f, "replacement lost send gains");
            for (unsigned bank = 0; bank < engine.bankCount(); ++bank)
                  require(voices(engine.synth(bank)) == 0, "replacement retained old voices");
            require(engine.select(0, 0, 0) && engine.setSourceChannel(0, 0), "replacement preset unusable");
            auto target = engine.channel(0);
            fluid_synth_cc(target.synth, target.channel, 66, 0);
            fluid_synth_cc(target.synth, target.channel, 64, 0);
            fluid_synth_pitch_bend(target.synth, target.channel, 8192);
            engine.noteOn(0, 69, 90, 50.0f);
            std::vector<float> pcm(44100 * 2, 0), auxiliary(pcm.size(), 0);
            // Short callback-style blocks and an export-style larger request
            // both use the same production renderer. No WAV writer is tested.
            require(engine.process(63, pcm.data(), isolated ? auxiliary.data() : nullptr)
                  && engine.process(44100 - 63, pcm.data() + 126, isolated ? auxiliary.data() + 126 : nullptr),
                  "replacement render failed");
            unsigned crossings = 0;
            double energy = 0;
            for (unsigned frame = 4097; frame < 44100; ++frame) {
                  const unsigned i = frame * 2;
                  require(std::isfinite(pcm[i]), "nonfinite replacement audio");
                  if (pcm[i - 2] <= 0 && pcm[i] > 0) ++crossings;
                  energy += double(pcm[i]) * pcm[i];
                  if (isolated) require(std::abs(auxiliary[i] - pcm[i] * .375f) < .00001f, "replacement send not applied");
                  }
            const double frequency = crossings * 44100.0 / (44100 - 4097);
            require(energy > .00001 && std::abs(frequency - 442 * std::pow(2.0, 50.0 / 1200)) < 2,
                  "SF2/SF3 replacement is silent or lost master/per-note tuning");
            }
      require(engine.loadSoundFont(""), "explicit font unload failed");
      for (unsigned bank = 0; bank < engine.bankCount(); ++bank)
            require(engine.fontId(bank) < 0 && fluid_synth_sfcount(engine.synth(bank)) == 0, "font unload leaked a bank");
      std::vector<float> silent(2048, 0);
      require(engine.process(1024, silent.data()), "unloaded render failed");
      for (float value : silent) require(std::abs(value) < 1e-12f, "unload left old cached audio");
      require(engine.loadSoundFont(sf3.c_str()) && engine.select(128, 128, 0), "reload after empty state failed");
      inspect(engine);
      }

int main(int argc, char** argv)
      {
      require(argc == 3, "usage: tst_fluid2_soundfonts OUTPUT_PREFIX VORBIS_FIXTURE");
      const std::string sf2 = argv[1], sf3 = sf2 + ".sf3", empty = sf2 + ".empty.sf2";
      const std::string corrupt = sf2 + ".corrupt.sf3", truncated = sf2 + ".truncated.sf2", missing = sf2 + ".missing";
      const auto vorbis = readFile(argv[2]);
      require(vorbis.size() > 4 && std::equal(vorbis.begin(), vorbis.begin() + 4, "OggS"), "invalid Vorbis fixture");
      const Fluid2Test::Bytes damaged(64, 0);
      require(Fluid2Test::writeFont(sf2.c_str()) && Fluid2Test::writeFont(sf3.c_str(), &vorbis)
            && Fluid2Test::writeFont(empty.c_str(), nullptr, false) && Fluid2Test::writeFont(corrupt.c_str(), &damaged),
            "cannot write test fonts");
      auto partial = readFile(sf2.c_str()); partial.resize(64); writeFile(truncated, partial);
      std::remove(missing.c_str());
      for (bool isolated : { false, true }) {
            Fluid2SynthBanks initiallyEmpty;
            require(initiallyEmpty.init(44100, 0, isolated), "empty-engine initialization failed");
            for (const auto& file : { empty, corrupt, truncated, missing }) {
                  require(!initiallyEmpty.loadSoundFont(file.c_str()), "initial invalid font accepted");
                  for (unsigned bank = 0; bank < initiallyEmpty.bankCount(); ++bank)
                        require(initiallyEmpty.fontId(bank) < 0 && fluid_synth_sfcount(initiallyEmpty.synth(bank)) == 0,
                              "initial failure retained a staged font");
                  }
            for (const auto& active : { sf2, sf3 }) failures(active, { missing, truncated, corrupt, empty }, isolated);
            replacement(sf2, sf3, isolated);
            }
      for (const auto& path : { sf2, sf3, empty, corrupt, truncated }) std::remove(path.c_str());
      std::cout << "FluidSynth " << fluid_version_str()
                << ": SF2/SF3 loading, rollback PCM/LV/sends, replacement tuning and unload checks passed\n";
      }
