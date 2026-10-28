// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "audio/midi/auxiliarybus.h"
#include "effects/zita1/zita.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <vector>

static unsigned allocations = 0;
void* operator new(std::size_t n)
      {
      ++allocations;
      if (void* p = std::malloc(n ? n : 1)) return p;
      throw std::bad_alloc();
      }
void operator delete(void* p) noexcept { std::free(p); }
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete[](void* p) noexcept { ::operator delete(p); }

// The production widget implementation is intentionally outside this DSP test.
namespace Ms { EffectGui* ZitaReverb::gui() { return nullptr; } }
static void require(bool test, const char* message)
      {
      if (!test) { std::cerr << message << '\n'; std::exit(EXIT_FAILURE); }
      }

static std::vector<float> impulse(float rate, unsigned block)
      {
      Ms::ZitaReverb reverb;
      reverb.init(rate);
      reverb.set_opmix(1.0f);
      Ms::AuxiliaryBus<4096> bus;
      const unsigned frames = unsigned(rate * 2);
      std::vector<float> result(frames * 2, 0.0f);
      const unsigned beforeRendering = allocations;
      for (unsigned start = 0; start < frames; start += block) {
            const unsigned n = std::min(block, frames - start);
            bus.process(n, result.data() + start * 2,
                  [start](float* aux) { if (!start) aux[0] = aux[1] = 1.0f; },
                  [&]() { reverb.clear(); },
                  [&](unsigned count, float* in, float* wet) { reverb.process(int(count), in, wet); });
            }
      require(allocations == beforeRendering, "Zita/bus allocated during rendering");
      double early = 0.0, tail = 0.0;
      for (unsigned i = 0; i < frames * 2; ++i) {
            require(std::isfinite(result[i]), "non-finite Zita return");
            if (i < unsigned(rate * 0.01) * 2) early += std::fabs(result[i]);
            if (i >= unsigned(rate * 0.2) * 2) tail += std::fabs(result[i]);
            }
      require(early < 1e-12, "wet-only return leaked direct dry impulse");
      require(tail > 0.01, "Zita impulse produced no reverberant tail");
      // A pending reset must discard the old tail and leave silent sends exact.
      bus.requestReset();
      std::vector<float> dry(8192, 0.125f);
      const unsigned beforeReset = allocations;
      bus.process(4096, dry.data(), [](float*) {}, [&]() { reverb.clear(); },
            [&](unsigned n, float* in, float* wet) { reverb.process(int(n), in, wet); });
      require(allocations == beforeReset, "Zita/bus allocated during reset");
      for (float value : dry) require(value == 0.125f, "reset leaked old reverb into dry audio");
      // Reuse the same allocation after clear: a new impulse matches a fresh unit.
      std::vector<float> after(8192, 0.0f);
      bus.process(4096, after.data(), [](float* aux) { aux[0] = aux[1] = 1.0f; },
            [&]() { reverb.clear(); },
            [&](unsigned n, float* in, float* wet) { reverb.process(int(n), in, wet); });
      for (unsigned i = 0; i < after.size(); ++i)
            require(std::fabs(after[i] - result[i]) < 1e-8f, "cleared Zita differs from fresh impulse");
      return result;
      }

int main()
      {
      for (float rate : { 44100.0f, 48000.0f, 96000.0f }) {
            const std::vector<float> one = impulse(rate, 1);
            const std::vector<float> uneven = impulse(rate, 513);
            const std::vector<float> large = impulse(rate, 4096);
            for (unsigned i = 0; i < one.size(); ++i) {
                  require(std::fabs(one[i] - uneven[i]) < 1e-8f, "fragmented Zita return differs");
                  require(std::fabs(one[i] - large[i]) < 1e-8f, "large-block Zita return differs");
                  }
            }
      std::cout << "Production Zita: wet-only impulse, tail, clear/reuse and block parity checks passed\n";
      }
