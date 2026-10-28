// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_FLUID2_CHANNELROUTING_H
#define MS_FLUID2_CHANNELROUTING_H

#include <array>

namespace Ms {

// Runtime mixer-channel ownership. This does not remap MIDI channels or imply
// that FluidSynth's modulo audio groups are independent instrument outputs.
class Fluid2ChannelRouting {
      std::array<int, 256> sources;

   public:
      enum { MidiChannels = 256, MaxGroups = 128, Unknown = -1, Mixed = -2 };
      Fluid2ChannelRouting() { clear(); }
      void clear() { sources.fill(Unknown); }
      void clear(int channel) { if (validChannel(channel)) sources[channel] = Unknown; }
      static bool validChannel(int channel) { return channel >= 0 && channel < MidiChannels; }

      bool setSource(int channel, int source)
            {
            if (!validChannel(channel)) return false;
            if (!validChannel(source) || (sources[channel] != Unknown && sources[channel] != source)) {
                  // Ownership cannot be changed while old release tails may
                  // still exist. Stay ambiguous until an all-sounds-off reset.
                  sources[channel] = Mixed;
                  return false;
                  }
            sources[channel] = source;
            return true;
            }
      int source(int channel) const { return validChannel(channel) ? sources[channel] : int(Unknown); }
      // Two-bank mode has one physical output per logical channel. The bank
      // is part of its address: bank 0/group 0 differs from bank 1/group 0.
      int bankGroupOwner(unsigned bank, unsigned group) const
            {
            if (bank >= 2 || group >= MaxGroups) return Unknown;
            return sources[bank * MaxGroups + group];
            }

      // A send can use one gain for this raw group only when every registered
      // member belongs to the same mixer channel. Empty/invalid groups are
      // Unknown; Mixed must never be interpreted as an instrument identifier.
      int groupOwner(unsigned group, unsigned groups) const
            {
            if (groups == 0 || groups > MaxGroups || group >= groups) return Unknown;
            int owner = Unknown;
            for (unsigned channel = group; channel < MidiChannels; channel += groups) {
                  if (sources[channel] == Unknown) continue;
                  if (sources[channel] == Mixed) return Mixed;
                  if (owner != Unknown && owner != sources[channel]) return Mixed;
                  owner = sources[channel];
                  }
            return owner;
            }
      };
}
#endif
