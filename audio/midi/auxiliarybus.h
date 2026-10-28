// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_AUXILIARYBUS_H
#define MS_AUXILIARYBUS_H

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>

namespace Ms {

// One bounded stereo send bus. Only requestReset may run outside the render
// thread. The effect owns its delay state; reset/process must not allocate.
template <std::size_t MaxFrames>
class AuxiliaryBus {
      std::array<float, MaxFrames * 2> input {};
      std::array<float, MaxFrames * 2> wet {};
      std::atomic<bool> resetRequested { true };
      bool ringing = false;

   public:
      void requestReset() { resetRequested.store(true, std::memory_order_release); }

      template <typename Render, typename Reset, typename Effect>
      bool process(unsigned frames, float* dry, Render render, Reset reset, Effect effect)
            {
            // Reject before invoking callbacks or consuming a pending reset.
            if (!dry || !frames || frames > MaxFrames)
                  return false;
            if (resetRequested.exchange(false, std::memory_order_acq_rel)) {
                  reset();
                  ringing = false;
                  }
            const std::size_t samples = std::size_t(frames) * 2;
            std::fill_n(input.data(), samples, 0.0f);
            render(input.data());
            // Zita has tiny internal denormal-prevention offsets. Do not start
            // it until a send contains a signal: unused sends remain bit-exact.
            if (!ringing)
                  ringing = std::any_of(input.begin(), input.begin() + samples,
                        [](float value) { return value != 0.0f; });
            if (ringing) {
                  std::fill_n(wet.data(), samples, 0.0f);
                  effect(frames, input.data(), wet.data());
                  for (std::size_t i = 0; i < samples; ++i)
                        dry[i] += wet[i];
                  }
            return true;
            }
      };
}
#endif
