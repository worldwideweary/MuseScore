// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_FLUID2_SYNTHBANKS_H
#define MS_FLUID2_SYNTHBANKS_H

#include <fluidsynth.h>
#include <array>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include "audiobuffer.h"
#include "sendmix.h"

namespace Ms {

// No audio driver or worker thread. The adapter serializes access. SoundFont
// objects belong to their own synth; only FluidSynth's sample cache is shared.
class Fluid2SynthBanks {
      struct Preset {
            int font, bank, program;
            Preset(int f = -1, int b = 0, int p = 0) : font(f), bank(b), program(p) {}
            };
      struct Bank {
            fluid_settings_t* settings = nullptr;
            fluid_synth_t* synth = nullptr;
            std::vector<int> fonts;
            std::vector<int> offsets;
            std::map<std::pair<int, int>, Preset> presets;
            };
      std::array<Bank, 2> banks;
      unsigned count = 0;
      float rate = 0;
      bool separate = false;
      std::unique_ptr<Fluid2AudioBuffer> outputs;
      std::array<float, 1024> stereo {};
      Fluid2SendMix sends;
      // Host bank numbers include the legacy per-font offsets. Native synth
      // bank numbers cannot identify a lower-font choice during a rate reload.
      std::array<std::pair<int, int>, 256> programs {};
      std::array<bool, 256> selected {};
      struct ParameterSelection {
            int msb = 127, lsb = 127, generator = 0;
            bool nrpn = false;
            };
      std::array<ParameterSelection, 256> parameters {};

   public:
      enum { MidiChannels = 256, ChannelsPerBank = 128, TotalVoices = 512 };
      struct Channel {
            fluid_synth_t* synth;
            int channel;
            int font;
            Channel(fluid_synth_t* s = nullptr, int c = -1, int f = -1) : synth(s), channel(c), font(f) {}
            };
      Fluid2SynthBanks() = default;
      ~Fluid2SynthBanks() { clear(); }
      Fluid2SynthBanks(const Fluid2SynthBanks&) = delete;
      Fluid2SynthBanks& operator=(const Fluid2SynthBanks&) = delete;
      void clear();
      // groups=0 retains write_float; separate=true overrides groups to 128.
      bool init(float rate, unsigned groups, bool separate);
      // Stopped-time replacement. Failure leaves this engine untouched.
      // Success retains presets, stable controllers, parameter offsets/selection,
      // wheel range and sends;
      // voices, pedal captures, pitch bend and lane ownership start fresh.
      bool reinitialize(float rate, unsigned groups, bool separate, const char* font, double tuning);
      bool reinitialize(float rate, unsigned groups, bool separate, const std::vector<std::string>& fonts, double tuning);
      bool loadSoundFont(const char* path);
      // Highest priority first. Every font in every bank is staged before
      // voices, accepted presets or the old font list are changed.
      bool loadSoundFonts(const std::vector<std::string>& paths);
      // Both buffers are additive and must not overlap. Auxiliary output
      // requires isolated banks; unsupported requests fail before advancing.
      bool process(unsigned frames, float* output, float* auxiliary = nullptr);
      bool setSourceChannel(int channel, int source) { return sends.setSource(channel, source); }
      int sourceChannel(int channel) const { return sends.source(channel); }
      void clearSourceChannel(int channel);
      bool setSourceSend(int source, float gain) { return sends.setGain(source, gain); }
      float sourceSend(int source) const { return sends.gain(source); }
      bool select(int channel, int bank, int program);
      // 37E channel defaults, including +/-12-semitone bends. Does not change
      // the selected preset or source-send ownership/gain. Releases pedals.
      bool resetControllers(int channel);
      // Preserve legacy select-once SoundFont NRPN automation. Ordinary RPN,
      // portamento and other controllers retain upstream handling.
      bool controlChange(int channel, int controller, int value);
      bool tune(double hz);
      void noteOn(int channel, int key, int velocity, float cents);
      bool allSoundsOff(int channel);
      bool allNotesOff(int channel);

      unsigned bankCount() const { return count; }
      float sampleRate() const { return rate; }
      bool isolated() const { return separate; }
      unsigned groupCount() const { return outputs ? outputs->groups() : 0; }
      unsigned voiceLimitPerBank() const { return count ? TotalVoices / count : 0; }
      fluid_synth_t* synth(unsigned bank) const { return bank < count ? banks[bank].synth : nullptr; }
      unsigned fontCount(unsigned bank) const { return bank < count ? unsigned(banks[bank].fonts.size()) : 0; }
      int fontId(unsigned bank, unsigned priority = 0) const
            { return priority < fontCount(bank) ? banks[bank].fonts[priority] : -1; }
      int fontBankOffset(unsigned bank, unsigned priority) const
            { return priority < fontCount(bank) ? banks[bank].offsets[priority] : 0; }
      Channel channel(int logical) const;
      };
}
#endif
