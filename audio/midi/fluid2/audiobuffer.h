// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_FLUID2_AUDIOBUFFER_H
#define MS_FLUID2_AUDIOBUFFER_H

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace Ms {

// FluidSynth's planar outputs, allocated outside the audio callback. Groups
// are raw synth outputs; identifying their owning instrument is a later step.
class Fluid2AudioBuffer {
   public:
      enum { MaxGroups = 128, BlockFrames = 512 };

   private:
      unsigned _groups;
      std::vector<float> storage;
      std::vector<float*> planes;
      std::array<unsigned, MaxGroups * 2> dirtyFrames {};
      std::array<float, BlockFrames> silence {};

   public:
      static bool validGroupCount(unsigned groups) { return groups > 0 && groups <= MaxGroups; }
      explicit Fluid2AudioBuffer(unsigned groups)
         : _groups(validGroupCount(groups) ? groups : 1),
           storage(_groups * 2 * BlockFrames), planes(_groups * 2)
            {
            for (unsigned plane = 0; plane < planes.size(); ++plane)
                  planes[plane] = storage.data() + plane * BlockFrames;
            }
      Fluid2AudioBuffer(const Fluid2AudioBuffer&) = delete;
      Fluid2AudioBuffer& operator=(const Fluid2AudioBuffer&) = delete;
      unsigned groups() const { return _groups; }

      // Render adds into zeroed planes, like fluid_synth_process(). Sum dry
      // stereo into the host buffer, preserving other synths' contributions.
      // A failed block contributes nothing; never retry consumed synth time.
      template<typename Render>
      bool process(unsigned frames, float* output, Render render)
            {
            return process(frames, output, render, [](unsigned, unsigned, unsigned, float**) {});
            }
      // Observe successfully rendered planes before dry summing. Failed
      // blocks cannot contribute stale samples to either dry or auxiliary.
      template<typename Render, typename Mix>
      bool process(unsigned frames, float* output, Render render, Mix mix)
            {
            if (!output) return frames == 0;
            bool success = true;
            for (unsigned start = 0; start < frames;) {
                  const unsigned count = std::min(unsigned(BlockFrames), frames - start);
                  // Silent planes stay zero between calls. Clear the previous
                  // written extent, including any tail beyond a short request,
                  // before allowing a later longer block to reuse it.
                  for (unsigned p = 0; p < planes.size(); ++p)
                        if (dirtyFrames[p]) std::fill_n(planes[p], dirtyFrames[p], 0.0f);
                  if (render(int(count), int(planes.size()), planes.data())) {
                        mix(start, count, _groups, planes.data());
                        for (unsigned p = 0; p < planes.size(); ++p)
                              dirtyFrames[p] = std::memcmp(planes[p], silence.data(), count * sizeof(float)) != 0 ? count : 0;
                        for (unsigned group = 0; group < _groups; ++group) {
                              if (!dirtyFrames[group * 2] && !dirtyFrames[group * 2 + 1]) continue;
                              const float* left = planes[group * 2];
                              const float* right = planes[group * 2 + 1];
                              for (unsigned frame = 0; frame < count; ++frame) {
                                    output[(start + frame) * 2] += left[frame];
                                    output[(start + frame) * 2 + 1] += right[frame];
                                    }
                              }
                        }
                  else {
                        // A failed renderer may have written any of its planes.
                        // Force clearing before reuse; contribute no failed PCM.
                        dirtyFrames.fill(BlockFrames);
                        success = false;
                        }
                  start += count;
                  }
            return success;
            }
      };
}
#endif
