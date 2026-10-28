// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_AUXILIARY_REVERB_SETTINGS_H
#define MS_AUXILIARY_REVERB_SETTINGS_H

#include <algorithm>
#include <array>
#include <cmath>
#include <mutex>

namespace Ms {
// Physical Zita room/EQ values; the return is always wet-only. These IDs belong
// to AuxiliaryReverb, not Zita1's master-effect parameter/state namespace.
class AuxiliaryReverbSettings {
   public:
      enum Parameter { Delay, Crossover, LowRT60, MidRT60, HFDamping,
            EQ1Frequency, EQ1Gain, EQ2Frequency, EQ2Gain, ReturnLevel, Count };
      using Values = std::array<float, Count>;
      static const char* stateGroup() { return "AuxiliaryReverb"; }
      static Values defaults()
            { return {{ .04f, 200.0f, 1.4f, 2.0f, 3000.0f, 160.0f, 0.0f, 2500.0f, 0.0f, 1.0f }}; }
      static Values minimums()
            { return {{ .02f, 50.0f, 1.0f, 1.0f, 1500.0f, 40.0f, -15.0f, 160.0f, -15.0f, 0.0f }}; }
      static Values maximums()
            { return {{ .10f, 1000.0f, 8.0f, 8.0f, 24000.0f, 2500.0f, 15.0f, 10000.0f, 15.0f, 1.0f }}; }
      static bool update(Values& target, int id, double value)
            {
            if (id < 0 || id >= Count || !std::isfinite(value)) return false;
            target[id] = float(std::max(double(minimums()[id]), std::min(double(maximums()[id]), value)));
            return true;
            }

   private:
      mutable std::mutex mutex;
      Values values = defaults();
      bool dirty = true;

   public:
      Values snapshot() const
            { std::lock_guard<std::mutex> lock(mutex); return values; }
      bool set(int id, double value)
            {
            std::lock_guard<std::mutex> lock(mutex);
            Values next = values;
            if (!update(next, id, value)) return false;
            if (next != values) { values = next; dirty = true; }
            return true;
            }
      void replace(const Values& requested)
            {
            Values next = defaults();
            for (int i = 0; i < Count; ++i) update(next, i, requested[i]);
            std::lock_guard<std::mutex> lock(mutex);
            values = next;
            dirty = true;
            }
      void requestApply()
            { std::lock_guard<std::mutex> lock(mutex); dirty = true; }
      // The render thread never waits for a GUI/state writer. If busy, keep the
      // previous DSP parameters and retry on the next block. No synth is muted.
      bool consume(Values& target)
            {
            std::unique_lock<std::mutex> lock(mutex, std::try_to_lock);
            if (!lock.owns_lock() || !dirty) return false;
            target = values;
            dirty = false;
            return true;
            }
      };

// Called only on the render thread, or before rendering starts. The output mix
// remains 100% wet; ReturnLevel scales the wet samples separately in the host.
template <typename Reverb>
void applyAuxiliaryReverbSettings(const AuxiliaryReverbSettings::Values& v, Reverb& reverb)
      {
      using S = AuxiliaryReverbSettings;
      reverb.set_delay(v[S::Delay]);
      reverb.set_xover(v[S::Crossover]);
      reverb.set_rtlow(v[S::LowRT60]);
      reverb.set_rtmid(v[S::MidRT60]);
      reverb.set_fdamp(v[S::HFDamping]);
      reverb.set_eq1fr(v[S::EQ1Frequency]);
      reverb.set_eq1gn(v[S::EQ1Gain]);
      reverb.set_eq2fr(v[S::EQ2Frequency]);
      reverb.set_eq2gn(v[S::EQ2Gain]);
      }
}
#endif
