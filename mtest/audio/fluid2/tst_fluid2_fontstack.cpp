// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "audio/midi/fluid2/synthbanks.h"
#include "testfont.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iterator>

using Ms::Fluid2SynthBanks;
static void require(bool ok, const char* why)
      { if (!ok) { std::cerr << why << '\n'; std::exit(EXIT_FAILURE); } }
#ifdef FLUID2_TEST_WRAP_LOAD
static unsigned calls = 0, fail = 0, redirect = 0;
static const char* emptyPath = nullptr;
extern "C" int __real_fluid_synth_sfload(fluid_synth_t*, const char*, int);
extern "C" int __wrap_fluid_synth_sfload(fluid_synth_t* synth, const char* path, int reset)
      {
      if (++calls == fail) return FLUID_FAILED;
      return __real_fluid_synth_sfload(synth, calls == redirect ? emptyPath : path, reset);
      }
#endif
static void select(Fluid2SynthBanks& engine, int logical, int bank, int program, unsigned priority)
      {
      require(engine.select(logical, bank, program), "stack preset selection failed");
      const auto target = engine.channel(logical);
      int sf = -1, actualBank = -1, actualProgram = -1;
      require(fluid_synth_get_program(target.synth, target.channel, &sf, &actualBank, &actualProgram) == FLUID_OK
            && sf == engine.fontId(engine.isolated() ? unsigned(logical) / 128 : 0, priority)
            && actualBank == bank - engine.fontBankOffset(engine.isolated() ? unsigned(logical) / 128 : 0, priority)
            && actualProgram == program, "wrong font/bank/program");
      }
static void compare(Fluid2SynthBanks& engine, Fluid2SynthBanks& reference)
      {
      double energy = 0;
      for (unsigned frames : { 1u, 63u, 513u, 4097u }) {
            std::vector<float> dry(frames * 2, 0), expected = dry, send = dry, expectedSend = dry;
            require(engine.process(frames, dry.data(), engine.isolated() ? send.data() : nullptr)
                  && reference.process(frames, expected.data(), reference.isolated() ? expectedSend.data() : nullptr), "render failed");
            for (unsigned i = 0; i < dry.size(); ++i) {
                  require(std::isfinite(dry[i]) && std::abs(dry[i] - expected[i]) < .00001f
                        && std::abs(send[i] - expectedSend[i]) < .00001f, "failed list replacement changed held dry/send PCM");
                  energy += dry[i] * dry[i];
                  }
            }
      require(energy > .00001, "held stack audio disappeared");
      }
static void check(const std::string& base, const std::string& overlay, const std::string& empty, unsigned groups, bool isolated)
      {
      const std::vector<std::string> order { base, overlay };
      Fluid2SynthBanks engine, reference;
      for (auto* current : { &engine, &reference }) {
            require(current->init(44100, groups, isolated) && current->loadSoundFonts(order)
                  && current->tune(442), "stack setup failed");
            for (int channel : { 0, 128 }) {
                  select(*current, channel, 129, 7, 1); // Lower-font choice from legacy saved scores.
                  require(current->setSourceChannel(channel, channel) && current->setSourceSend(channel, .375f), "send setup failed");
                  current->noteOn(channel, 69, 90, 50);
                  auto target = current->channel(channel);
                  fluid_synth_cc(target.synth, target.channel, 66, 127);
                  fluid_synth_noteoff(target.synth, target.channel, 69);
                  }
            }
      compare(engine, reference);
      const auto unchanged = [&]() {
            for (unsigned bank = 0; bank < engine.bankCount(); ++bank) {
                  require(engine.fontCount(bank) == 2 && fluid_synth_sfcount(engine.synth(bank)) == 2, "partial stack leaked or unloaded old fonts");
                  require(engine.fontId(bank, 0) == reference.fontId(bank, 0)
                        && engine.fontId(bank, 1) == reference.fontId(bank, 1), "failure changed accepted font IDs");
                  }
            for (int channel : { 0, 128 }) {
                  const auto target = engine.channel(channel);
                  int value = 0;
                  fluid_synth_get_cc(target.synth, target.channel, 66, &value);
                  require(value == 127 && engine.sourceChannel(channel) == channel
                        && engine.sourceSend(channel) == .375f, "failure lost LV/routing/gain");
                  }
            compare(engine, reference);
            };
      for (const auto& bad : { empty, base + ".missing" }) {
            for (unsigned position = 0; position < 3; ++position) {
                  auto requested = order;
                  requested.insert(requested.begin() + position, bad);
                  require(!engine.loadSoundFonts(requested), "bad member accepted");
                  unchanged();
                  }
            }
#ifdef FLUID2_TEST_WRAP_LOAD
      emptyPath = empty.c_str();
      for (unsigned at = 1; at <= engine.bankCount() * 2; ++at) {
            calls = 0; fail = at;
            require(!engine.loadSoundFonts(order), "injected member failure accepted");
            fail = 0; unchanged();
            calls = 0; redirect = at;
            require(!engine.loadSoundFonts(order), "injected empty member accepted");
            redirect = 0; unchanged();
            }
#endif
      for (const auto& requested : { order, std::vector<std::string> { overlay, base }, order }) {
            require(engine.loadSoundFonts(requested), "valid reorder failed");
            const bool overlayFirst = requested.front() == overlay;
            for (int channel : (isolated ? std::vector<int> {0, 128, 255} : std::vector<int> {0, 128})) {
                  select(engine, channel, 0, 0, 0);
                  select(engine, channel, 128, 0, 0);
                  select(engine, channel, 129, 0, 1); // Duplicate melodic choice remains reachable.
                  select(engine, channel, 257, 0, 1); // Lower font's native drum bank remains a drum.
                  select(engine, channel, overlayFirst ? 0 : 129, 7, overlayFirst ? 0 : 1);
                  }
            // Missing bank falls back to bank zero's same program, then its
            // first program. It must never retain a previously selected sound.
            require(engine.select(0, 999, overlayFirst ? 7 : 0), "same-program fallback failed");
            const auto sameProgram = engine.channel(0);
            int fallbackId = -1, fallbackBank = -1, fallbackProgram = -1;
            fluid_synth_get_program(sameProgram.synth, sameProgram.channel, &fallbackId, &fallbackBank, &fallbackProgram);
            require(fallbackId == engine.fontId(0, 0) && fallbackBank == 0 && fallbackProgram == (overlayFirst ? 7 : 0),
                  "missing-bank fallback did not select bank zero's same program");
            require(engine.select(0, 129, 0) && engine.select(0, 999, 99), "piano fallback failed");
            const auto fallback = engine.channel(0);
            int sf = -1, nativeBank = -1, program = -1;
            fluid_synth_get_program(fallback.synth, fallback.channel, &sf, &nativeBank, &program);
            require(sf == engine.fontId(0, 0) && nativeBank == 0 && program == 0, "unavailable preset kept previous instrument");
            for (unsigned priority : { 0u, 1u }) {
                  require(engine.allSoundsOff(-1), "cannot stop previous preset");
                  std::vector<float> drained(256 * 2, 0);
                  require(engine.process(256, drained.data()), "cannot drain previous preset");
                  select(engine, 0, priority ? 129 : 0, 0, priority);
                  require(engine.setSourceChannel(0, 0), "reordered ownership failed");
                  engine.noteOn(0, 69, 90, 50);
                  std::vector<float> dry(44100, 0), send = dry;
                  require(engine.process(22050, dry.data(), isolated ? send.data() : nullptr), "priority render failed");
                  unsigned crossings = 0;
                  for (unsigned n = 4411; n < 22050; ++n) {
                        if (dry[(n - 1) * 2] <= 0 && dry[n * 2] > 0) ++crossings;
                        if (isolated) require(std::abs(send[n * 2] - .375f * dry[n * 2]) < .00001f, "reorder changed source gain");
                        }
                  const double hz = crossings * 44100.0 / (22050 - 4411);
                  const bool playingOverlay = overlayFirst != (priority != 0);
                  const double expected = 442 * std::pow(2.0, 50.0 / 1200) / (playingOverlay ? 2 : 1);
                  require(std::abs(hz - expected) < 5, "selected top/lower font did not change actual tuned PCM");
                  }
            }
      require(engine.reinitialize(48000, groups, isolated, order, 442), "stack rate reload failed");
      select(engine, 0, 129, 7, 1);
      require(engine.reinitialize(44100, groups, isolated, order, 442), "offset-preset rate reload failed");
      const auto restored = engine.channel(0);
      int id = -1, bank = -1, program = -1;
      fluid_synth_get_program(restored.synth, restored.channel, &id, &bank, &program);
      require(id == engine.fontId(0, 1) && bank == 0 && program == 7, "rate reload changed lower-font selection");
      require(engine.reinitialize(48000, groups, isolated, order, 442), "return rate reload failed");
      require(engine.sampleRate() == 48000 && engine.fontCount(0) == 2 && engine.sourceSend(0) == .375f, "rate reload lost stack/gain");
      require(!engine.reinitialize(44100, groups, isolated, { base, empty }, 442)
            && engine.sampleRate() == 48000 && engine.fontCount(0) == 2, "failed stack rate reload committed");
      require(engine.loadSoundFonts({base}) && engine.fontCount(0) == 1 && engine.select(0, 129, 7), "removed-font fallback failed");
      const auto removed = engine.channel(0);
      fluid_synth_get_program(removed.synth, removed.channel, &id, &bank, &program);
      require(id == engine.fontId(0) && bank == 0 && program == 0, "removed font did not fall back to piano");
      select(engine, 0, 0, 0, 0);
      require(engine.loadSoundFonts({}) && engine.fontCount(0) == 0 && engine.fontId(0) == -1
            && engine.sourceSend(0) == .375f, "empty list did not unload/retain gains");
      require(engine.loadSoundFonts(order), "reload from empty list failed");
      }
int main(int argc, char** argv)
      {
      require(argc == 3, "usage: tst_fluid2_fontstack OUTPUT_PREFIX VORBIS_FIXTURE");
      std::ifstream input(argv[2], std::ios::binary);
      require(bool(input), "cannot read Vorbis fixture");
      const Fluid2Test::Bytes compressed(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>{});
      const std::string base = std::string(argv[1]) + ".sf2", overlay = std::string(argv[1]) + ".sf3", empty = std::string(argv[1]) + ".empty.sf2";
      require(Fluid2Test::writeFont(base.c_str()) && Fluid2Test::writeFont(overlay.c_str(), &compressed, true, 7, 81)
            && Fluid2Test::writeFont(empty.c_str(), nullptr, false), "cannot write fixtures");
      for (unsigned groups : { 0u, 16u, 128u }) check(base, overlay, empty, groups == 128 ? 0 : groups, groups == 128);
      std::remove(base.c_str()); std::remove(overlay.c_str()); std::remove(empty.c_str());
      std::cout << "Ordered SF2/SF3 stack: legacy banks/fallback, top/lower PCM, rollback, LV/sends and rate recovery passed\n";
      }
