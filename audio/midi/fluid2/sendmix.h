// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_FLUID2_SENDMIX_H
#define MS_FLUID2_SENDMIX_H

#include <array>
#include <algorithm>
#include <cmath>
#include "channelrouting.h"

namespace Ms {

// Host audio sends, separate from SoundFont generators and MIDI CC91. Access
// is serialized by the Fluid2 adapter; this class does not allocate or lock.
class Fluid2SendMix {
      Fluid2ChannelRouting routing;
      std::array<float, Fluid2ChannelRouting::MidiChannels> gains {};
      std::array<unsigned, Fluid2ChannelRouting::MidiChannels> pending {};

   public:
      void clear() { clearSources(); gains.fill(0.0f); }
      void clearSources(unsigned resetFrames = 0) { routing.clear(); pending.fill(resetFrames); }
      void clearSource(int channel, unsigned resetFrames = 0)
            {
            if (!Fluid2ChannelRouting::validChannel(channel)) return;
            routing.clear(channel);
            pending[channel] = resetFrames;
            }
      // Advance once per complete engine render, not once per bank or lane.
      void advance(unsigned frames)
            { for (unsigned& remaining : pending) remaining = remaining > frames ? remaining - frames : 0; }
      bool setSource(int channel, int source) { return routing.setSource(channel, source); }
      int source(int channel) const { return routing.source(channel); }
      bool setGain(int source, float gain)
            {
            if (!Fluid2ChannelRouting::validChannel(source) || !std::isfinite(gain) || gain < 0.0f || gain > 1.0f)
                  return false;
            gains[source] = gain;
            return true;
            }
      float gain(int source) const { return Fluid2ChannelRouting::validChannel(source) ? gains[source] : 0.0f; }
      bool hasRoutedGain(unsigned bank) const
            {
            if (bank >= 2) return false;
            for (unsigned group = 0; group < Fluid2ChannelRouting::MaxGroups; ++group)
                  if (gain(routing.bankGroupOwner(bank, group)) != 0.0f) return true;
            return false;
            }

      // Only the independent two-bank layout may call this. Keep the bank in
      // the address so logical 0 and 128 can have different owners and gains.
      void mix(unsigned bank, unsigned start, unsigned frames, unsigned groups,
               float* const* planes, float* auxiliary) const
            {
            if (!auxiliary || !planes || bank >= 2 || groups != Fluid2ChannelRouting::MaxGroups) return;
            for (unsigned group = 0; group < groups; ++group) {
                  const float amount = gain(routing.bankGroupOwner(bank, group));
                  if (amount == 0.0f) continue;
                  // Old cached PCM can outlive an all-sounds-off request.
                  // Suppress its send while a newly assigned owner settles.
                  const unsigned remaining = pending[bank * Fluid2ChannelRouting::MaxGroups + group];
                  const unsigned skip = remaining > start ? std::min(frames, remaining - start) : 0;
                  const float* left = planes[group * 2];
                  const float* right = planes[group * 2 + 1];
                  for (unsigned frame = skip; frame < frames; ++frame) {
                        auxiliary[(start + frame) * 2] += amount * left[frame];
                        auxiliary[(start + frame) * 2 + 1] += amount * right[frame];
                        }
                  }
            }
      };
}
#endif
