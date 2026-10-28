// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "audio/midi/auxiliaryreverbsettings.h"
#include "audio/midi/auxiliarybus.h"
#include "effects/zita1/zita.h"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <thread>
#include <vector>

namespace Ms { EffectGui* ZitaReverb::gui() { return nullptr; } }
using Settings = Ms::AuxiliaryReverbSettings;
static void require(bool condition, const char* message)
      { if (!condition) { std::cerr << message << '\n'; std::exit(EXIT_FAILURE); } }

static std::vector<float> render(const Settings::Values& values, bool apply, float dry)
      {
      Ms::ZitaReverb reverb;
      reverb.init(44100);
      reverb.set_opmix(1.0f);
      Settings settings;
      settings.replace(values);
      Ms::AuxiliaryBus<4096> bus;
      std::vector<float> result(44100 * 2 * 2, dry);
      for (unsigned start = 0; start < result.size() / 2; start += 513) {
            const unsigned n = std::min(513u, unsigned(result.size() / 2) - start);
            Settings::Values snapshot;
            if (apply && settings.consume(snapshot)) Ms::applyAuxiliaryReverbSettings(snapshot, reverb);
            bus.process(n, result.data() + start * 2,
                  [start](float* aux) { if (!start) aux[0] = aux[1] = 1.0f; },
                  [&]() { reverb.clear(); },
                  [&](unsigned count, float* input, float* wet) {
                        reverb.process(int(count), input, wet);
                        for (unsigned i = 0; i < count * 2; ++i) wet[i] *= values[Settings::ReturnLevel];
                        });
            }
      return result;
      }
int main()
      {
      Settings settings;
      Settings::Values snapshot;
      require(settings.consume(snapshot) && snapshot == Settings::defaults(), "initial defaults not delivered");
      require(!settings.consume(snapshot), "clean settings delivered twice");
      require(!settings.set(-1, 1) && !settings.set(Settings::Count, 1), "unknown parameter accepted");
      require(!settings.set(Settings::Delay, std::numeric_limits<double>::quiet_NaN()), "NaN accepted");
      require(!settings.set(Settings::ReturnLevel, std::numeric_limits<double>::infinity()), "infinite gain accepted");
      require(settings.snapshot() == Settings::defaults(), "invalid settings changed state");
      require(settings.set(Settings::ReturnLevel, 10), "finite high value rejected");
      require(settings.snapshot()[Settings::ReturnLevel] == 1, "return gain unbounded");
      require(settings.set(Settings::Delay, -1), "finite low value rejected");
      require(settings.snapshot()[Settings::Delay] == Settings::minimums()[Settings::Delay], "delay unbounded");
      settings.replace(Settings::defaults());
      require(settings.consume(snapshot), "reset not delivered");
      settings.requestApply();
      require(settings.consume(snapshot), "new DSP did not receive current controls");
      auto a = Settings::defaults(), b = a;
      a[Settings::EQ1Gain] = 3;
      b[Settings::MidRT60] = 5;
      b[Settings::ReturnLevel] = .25f;
      settings.replace(a); settings.consume(snapshot);
      std::atomic<bool> done { false };
      std::thread writer([&]() {
            for (int i = 0; i < 10000; ++i) settings.replace(i & 1 ? b : a);
            done.store(true, std::memory_order_release);
            });
      while (!done.load(std::memory_order_acquire)) {
            if (settings.consume(snapshot)) require(snapshot == a || snapshot == b, "torn parameter batch");
            else std::this_thread::yield();
            }
      writer.join();
      require(settings.snapshot() == b, "latest settings lost");
      const auto baseline = render(Settings::defaults(), false, 0);
      const auto defaults = render(Settings::defaults(), true, 0);
      require(defaults == baseline, "new controls changed the v0.19 default reverb PCM");
      auto custom = Settings::defaults();
      custom[Settings::Delay] = .075f;
      custom[Settings::MidRT60] = 5;
      custom[Settings::EQ1Gain] = 3;
      custom[Settings::ReturnLevel] = .25f;
      const auto changed = render(custom, true, 0);
      double difference = 0;
      for (unsigned i = 0; i < changed.size(); ++i) {
            require(std::isfinite(changed[i]), "non-finite return PCM");
            difference += std::abs(double(changed[i] - baseline[i]));
            if (i < 441 * 2) require(std::fabs(changed[i]) < 1e-12f, "return leaked direct dry impulse");
            }
      require(difference > .01, "room/EQ controls did not change the return");
      custom[Settings::ReturnLevel] = 0;
      const auto muted = render(custom, true, .125f);
      for (float x : muted) require(x == .125f, "muting return changed dry audio");
      std::cout << "Auxiliary settings: bounded batches, threaded handoff, default PCM parity and wet-only controls passed\n";
      }
