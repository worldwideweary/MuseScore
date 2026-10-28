// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "synthbanks.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace Ms {

static bool setLegacyModulators(fluid_synth_t* synth)
      {
      // Legacy Fluid::alloc_voice installs this before instrument/preset
      // modulators. Set a synth default, so authored replacements still win.
      // This API allocates: run it only while constructing a stopped engine.
      std::unique_ptr<fluid_mod_t, decltype(&delete_fluid_mod)> pan(new_fluid_mod(), delete_fluid_mod);
      if (!pan) return false;
      fluid_mod_set_source1(pan.get(), 10, FLUID_MOD_CC | FLUID_MOD_LINEAR | FLUID_MOD_BIPOLAR | FLUID_MOD_POSITIVE);
      fluid_mod_set_source2(pan.get(), 0, 0);
      fluid_mod_set_dest(pan.get(), GEN_PAN);
      fluid_mod_set_amount(pan.get(), 1000);
      if (fluid_synth_add_default_mod(synth, pan.get(), FLUID_SYNTH_OVERWRITE) != FLUID_OK) return false;
      // Upstream adds a non-SF2 CC8 balance modulator; legacy Fluid has none.
      // Its legacy CC8=0 default would otherwise mute the right output. Leave
      // the CC value and any authored SoundFont routes intact, zero only this
      // implicit generator route. The API copies each definition.
      fluid_mod_set_source1(pan.get(), 8, FLUID_MOD_CC | FLUID_MOD_CONCAVE | FLUID_MOD_BIPOLAR | FLUID_MOD_POSITIVE);
      fluid_mod_set_dest(pan.get(), GEN_CUSTOM_BALANCE);
      fluid_mod_set_amount(pan.get(), 0);
      return fluid_synth_add_default_mod(synth, pan.get(), FLUID_SYNTH_OVERWRITE) == FLUID_OK;
      }

void Fluid2SynthBanks::clear()
      {
      for (Bank& bank : banks) {
            if (bank.synth) delete_fluid_synth(bank.synth);
            if (bank.settings) delete_fluid_settings(bank.settings);
            bank = Bank();
            }
      outputs.reset();
      count = 0;
      rate = 0;
      separate = false;
      sends.clear();
      programs.fill({0, 0});
      selected.fill(false);
      parameters.fill(ParameterSelection());
      }

bool Fluid2SynthBanks::init(float rate, unsigned groups, bool isolated)
      {
      clear();
      if (!std::isfinite(rate) || rate <= 0 || groups > Fluid2AudioBuffer::MaxGroups) return false;
      separate = isolated;
      count = separate ? 2 : 1;
      if (separate) groups = ChannelsPerBank;
      if (groups) outputs.reset(new Fluid2AudioBuffer(groups));
      for (unsigned i = 0; i < count; ++i) {
            Bank& bank = banks[i];
            bank.settings = new_fluid_settings();
            if (!bank.settings
                || fluid_settings_setnum(bank.settings, "synth.sample-rate", rate) != FLUID_OK
                // Legacy Fluid normalizes samples without an additional synth
                // gain. Host master gain/boost already applies to both engines.
                || fluid_settings_setnum(bank.settings, "synth.gain", 1.0) != FLUID_OK
                || fluid_settings_setint(bank.settings, "synth.midi-channels", separate ? ChannelsPerBank : MidiChannels) != FLUID_OK
                || fluid_settings_setint(bank.settings, "synth.audio-groups", groups ? int(groups) : 1) != FLUID_OK
                || fluid_settings_setint(bank.settings, "synth.audio-channels", groups ? int(groups) : 1) != FLUID_OK
                || fluid_settings_setint(bank.settings, "synth.polyphony", int(voiceLimitPerBank())) != FLUID_OK
                || fluid_settings_setint(bank.settings, "synth.chorus.active", 0) != FLUID_OK
                || fluid_settings_setint(bank.settings, "synth.reverb.active", 0) != FLUID_OK
                || fluid_settings_setint(bank.settings, "synth.threadsafe-api", 1) != FLUID_OK) {
                  clear();
                  return false;
                  }
            // Optional host extension in the bundled engine. SYSTEM keeps its
            // own release semantics when the setting is unavailable.
            if (fluid_settings_get_type(bank.settings, "synth.mscore-release-pitch-lock") == FLUID_INT_TYPE
                && fluid_settings_setint(bank.settings, "synth.mscore-release-pitch-lock", 1) != FLUID_OK) {
                  clear();
                  return false;
                  }
            bank.synth = new_fluid_synth(bank.settings);
            if (!bank.synth
                || fluid_synth_count_midi_channels(bank.synth) != (separate ? ChannelsPerBank : MidiChannels)
                || fluid_synth_count_audio_groups(bank.synth) != (groups ? int(groups) : 1)
                || fluid_synth_count_audio_channels(bank.synth) != (groups ? int(groups) : 1)
                || !setLegacyModulators(bank.synth)) {
                  clear();
                  return false;
                  }
            }
      this->rate = rate;
      for (int ch = 0; ch < MidiChannels; ++ch) {
            // Physical 255 is also FluidSynth 2.3.x's unused-voice marker.
            // Two-bank mode provides all logical channels safely.
            if (!separate && ch == 255) continue;
            if (!resetControllers(ch)) { clear(); return false; }
            }
      return true;
      }

bool Fluid2SynthBanks::resetControllers(int logical)
      {
      const Channel target = channel(logical);
      if (!target.synth || (!separate && logical == 255)) return false;
      fluid_synth_t* synth = target.synth;
      const int ch = target.channel;
      // Let upstream clear generators/RPN state and release sustain/sostenuto
      // first. Then restore the explicit defaults from legacy Channel::initCtrl.
      if (fluid_synth_cc(synth, ch, 121, 0) != FLUID_OK) return false;
      parameters[logical] = ParameterSelection();
      for (int cc = 1; cc < 120; ++cc) {
            // Bank selection belongs to the host. Do not execute data-entry,
            // relative parameter commands, or portamento's previous-note CC.
            // Keep upstream's null RPN/NRPN selectors to avoid accidental data
            // entry into RPN zero after resetting a modern synth.
            if (cc == 6 || cc == 32 || cc == 38 || cc == 84 || (cc >= 96 && cc <= 101)) continue;
            const int value = cc == 7 || cc == 11 || cc == 43 ? 127 : cc == 10 || cc == 42 ? 64 : 0;
            if (fluid_synth_cc(synth, ch, cc, value) != FLUID_OK) return false;
            }
      if (fluid_synth_pitch_bend(synth, ch, 8192) != FLUID_OK
          || fluid_synth_pitch_wheel_sens(synth, ch, 12) != FLUID_OK
          || fluid_synth_channel_pressure(synth, ch, 0) != FLUID_OK) return false;
      for (int key = 0; key < 128; ++key)
            if (fluid_synth_key_pressure(synth, ch, key, 80) != FLUID_OK) return false;
      return true;
      }

bool Fluid2SynthBanks::controlChange(int logical, int controller, int value)
      {
      const Channel target = channel(logical);
      if (!target.synth || (!separate && logical == 255)
          || controller < 0 || controller > 127 || value < 0 || value > 127) return false;
      if (controller == 121) return resetControllers(logical);
      ParameterSelection& parameter = parameters[logical];
      fluid_synth_t* synth = target.synth;
      const int ch = target.channel;
      if (controller == 6 && parameter.nrpn && parameter.msb == 120) {
            // Upstream clears the SF2 generator index after each data write;
            // legacy keeps it for subsequent data-entry automation. Reselect
            // using public CCs, preserving upstream scaling and voice updates.
            // Incomplete/out-of-range indices must not fall through to gen 0.
            if (parameter.lsb >= 100 || parameter.generator >= GEN_LAST) return true;
            if (fluid_synth_cc(synth, ch, 99, 120) != FLUID_OK
                || fluid_synth_cc(synth, ch, 98, parameter.generator) != FLUID_OK) return false;
            }
      if (controller == 98 && parameter.msb == 120) {
            // Bound upstream's accumulator too, even for arbitrarily long
            // malformed selector streams. The retained index is applied above.
            if (fluid_synth_cc(synth, ch, 99, 120) != FLUID_OK) return false;
            }
      if (fluid_synth_cc(synth, ch, controller, value) != FLUID_OK) return false;
      if (controller == 99) {
            parameter.msb = value; parameter.lsb = 0;
            parameter.generator = 0; parameter.nrpn = true;
            }
      else if (controller == 98) {
            parameter.lsb = value; parameter.nrpn = true;
            if (parameter.msb == 120) {
                  const int increment = value == 100 ? 100 : value == 101 ? 1000 : value == 102 ? 10000 : value < 100 ? value : 0;
                  parameter.generator = std::min(int(GEN_LAST), parameter.generator + increment);
                  }
            }
      else if (controller == 100 || controller == 101) parameter.nrpn = false;
      return true;
      }

bool Fluid2SynthBanks::reinitialize(float rate, unsigned groups, bool isolated, const char* font, double tuning)
      {
      return reinitialize(rate, groups, isolated,
            font && *font ? std::vector<std::string> { font } : std::vector<std::string> {}, tuning);
      }

bool Fluid2SynthBanks::reinitialize(float rate, unsigned groups, bool isolated, const std::vector<std::string>& fonts, double tuning)
      {
      Fluid2SynthBanks next;
      if (!next.init(rate, groups, isolated) || !next.loadSoundFonts(fonts) || !next.tune(tuning)) return false;
      for (int ch = 0; ch < MidiChannels; ++ch) {
            next.setSourceSend(ch, sourceSend(ch));
            const Channel from = channel(ch), to = next.channel(ch);
            if (!from.synth) continue;
            if (selected[ch] && !next.select(ch, programs[ch].first, programs[ch].second)) return false;
            // In the single 256-channel layout, FluidSynth 2.3.x also uses
            // 255 as the unused-voice marker. Modulating that channel can
            // inspect uninitialized voices. The isolated layout maps the
            // same logical channel to local 127 and restores it normally.
            if (!isolated && ch == 255) continue;
            // Reapply ordinary controls without replaying RPN/NRPN commands,
            // data increments, portamento's previous key or pedal captures.
            for (int cc = 1; cc < 120; ++cc) {
                  if (cc == 6 || cc == 32 || cc == 38 || cc == 64 || cc == 66 || cc == 84
                      || (cc >= 96 && cc <= 101)) continue;
                  int value = 0;
                  if (fluid_synth_get_cc(from.synth, from.channel, cc, &value) != FLUID_OK
                      || fluid_synth_cc(to.synth, to.channel, cc, value) != FLUID_OK) return false;
                  }
            int range = 2;
            if (fluid_synth_get_pitch_wheel_sens(from.synth, from.channel, &range) != FLUID_OK
                || fluid_synth_pitch_wheel_sens(to.synth, to.channel, range) != FLUID_OK) return false;
            // Copy effective parameter offsets, not data-entry commands which
            // could apply them twice or modify a different selected generator.
            for (int gen = 0; gen < GEN_LAST; ++gen)
                  if (fluid_synth_set_gen(to.synth, to.channel, gen,
                        fluid_synth_get_gen(from.synth, from.channel, gen)) != FLUID_OK) return false;
            // Restore data LSB and both selector pairs without executing CC6.
            // The last pair restores RPN/NRPN activation; SF2's accumulated
            // index is retained separately for subsequent data writes.
            for (int cc : (parameters[ch].nrpn ? std::vector<int> {38, 101, 100, 99, 98}
                                              : std::vector<int> {38, 99, 98, 101, 100})) {
                  int value = 0;
                  if (fluid_synth_get_cc(from.synth, from.channel, cc, &value) != FLUID_OK
                      || fluid_synth_cc(to.synth, to.channel, cc, value) != FLUID_OK) return false;
                  }
            next.parameters[ch] = parameters[ch];
            }
      // Only fully configured replacements become visible. next then tears
      // down the old engine, including its voices and cached dry/send PCM.
      banks.swap(next.banks);
      std::swap(count, next.count);
      std::swap(this->rate, next.rate);
      std::swap(separate, next.separate);
      outputs.swap(next.outputs);
      stereo.swap(next.stereo);
      std::swap(sends, next.sends);
      programs.swap(next.programs);
      selected.swap(next.selected);
      parameters.swap(next.parameters);
      return true;
      }

Fluid2SynthBanks::Channel Fluid2SynthBanks::channel(int logical) const
      {
      if (logical < 0 || logical >= MidiChannels || count == 0) return Channel();
      const unsigned bank = separate ? logical / ChannelsPerBank : 0;
      const int local = separate ? logical % ChannelsPerBank : logical;
      return Channel(banks[bank].synth, local, fontId(bank));
      }

void Fluid2SynthBanks::clearSourceChannel(int logical)
      {
      const Channel target = channel(logical);
      if (target.synth) sends.clearSource(logical, 2 * fluid_synth_get_internal_bufsize(target.synth));
      }

bool Fluid2SynthBanks::loadSoundFont(const char* path)
      {
      return loadSoundFonts(path && *path ? std::vector<std::string> { path } : std::vector<std::string> {});
      }

bool Fluid2SynthBanks::loadSoundFonts(const std::vector<std::string>& paths)
      {
      if (!count) return false;
      std::array<std::vector<int>, 2> loaded;
      std::array<std::vector<int>, 2> offsets;
      std::array<std::map<std::pair<int, int>, Preset>, 2> presets;
      const auto rollback = [&]() {
            for (unsigned bank = 0; bank < count; ++bank)
                  for (int id : loaded[bank])
                        if (id >= 0) fluid_synth_sfunload(banks[bank].synth, id, 0);
            };
      // Stage every load before killing voices or unloading any previous font.
      // Partial failure releases only new objects; old presets stay usable.
      for (unsigned bank = 0; bank < count; ++bank) {
            loaded[bank].assign(paths.size(), -1);
            // FluidSynth pushes each load onto the front of its stack.
            // Reverse loading keeps both its stack and our table top-first.
            for (size_t p = paths.size(); p-- > 0;) {
                  if (paths[p].empty()) { rollback(); return false; }
                  const int id = fluid_synth_sfload(banks[bank].synth, paths[p].c_str(), 0);
                  loaded[bank][p] = id;
                  fluid_sfont_t* font = id < 0 ? nullptr : fluid_synth_get_sfont_by_id(banks[bank].synth, id);
                  if (!font) { rollback(); return false; }
                  }
            // Match legacy Fluid::updatePatchList: each font gets its own
            // disjoint bank range, including duplicate melodic/drum presets.
            int offset = 0;
            for (size_t p = 0; p < paths.size(); ++p) {
                  offsets[bank].push_back(offset);
                  fluid_sfont_t* font = fluid_synth_get_sfont_by_id(banks[bank].synth, loaded[bank][p]);
                  fluid_sfont_iteration_start(font);
                  fluid_preset_t* preset = fluid_sfont_iteration_next(font);
                  if (!preset) {
                        rollback();
                        return false;
                        }
                  int highest = 0;
                  do {
                        const int nativeBank = fluid_preset_get_banknum(preset), program = fluid_preset_get_num(preset);
                        if (nativeBank < 0 || nativeBank >= std::numeric_limits<int>::max() - offset) {
                              rollback(); return false;
                              }
                        highest = std::max(highest, nativeBank);
                        presets[bank][{offset + nativeBank, program}] = Preset(loaded[bank][p], nativeBank, program);
                        } while ((preset = fluid_sfont_iteration_next(font)));
                  offset += highest + 1;
                  }
            }
      for (unsigned i = 0; i < count; ++i) {
            Bank& bank = banks[i];
            fluid_synth_all_sounds_off(bank.synth, -1);
            if (!bank.fonts.empty()) {
                  // Voice-off is queued until DSP runs. Finish those voices
                  // before unloading, completing old-font voice teardown.
                  // Loading already stops playback;
                  // discard these reset frames rather than emit old audio.
                  fluid_synth_process(bank.synth, 2 * fluid_synth_get_internal_bufsize(bank.synth), 0, nullptr, 0, nullptr);
                  for (int id : bank.fonts) fluid_synth_sfunload(bank.synth, id, 0);
                  }
            bank.fonts.swap(loaded[i]);
            bank.offsets.swap(offsets[i]);
            bank.presets.swap(presets[i]);
            }
      sends.clearSources();
      return true;
      }

bool Fluid2SynthBanks::select(int logical, int bank, int program)
      {
      const Channel target = channel(logical);
      if (!target.synth) return false;
      programs[logical] = {bank, program};
      selected[logical] = true;
      if (target.font < 0) return true;
      const unsigned index = separate ? unsigned(logical) / ChannelsPerBank : 0;
      const auto& presets = banks[index].presets;
      auto preset = presets.find({bank, program});
      // Same fallback as legacy Fluid::program_change; do not leave the
      // previously selected instrument playing after an unavailable request.
      if (preset == presets.end()) preset = presets.find({0, program});
      if (preset == presets.end()) preset = presets.find({0, 0});
      if (preset == presets.end()) {
            fluid_synth_unset_program(target.synth, target.channel);
            return false;
            }
      const Preset& chosen = preset->second;
      if (fluid_synth_set_channel_type(target.synth, target.channel,
                  chosen.bank == 128 ? CHANNEL_TYPE_DRUM : CHANNEL_TYPE_MELODIC) != FLUID_OK) return false;
      return fluid_synth_program_select(target.synth, target.channel, chosen.font, chosen.bank, chosen.program) == FLUID_OK;
      }

bool Fluid2SynthBanks::tune(double hz)
      {
      if (!count || !std::isfinite(hz) || hz <= 0) return false;
      double pitches[128];
      const double offset = 1200.0 * std::log2(hz / 440.0);
      for (int i = 0; i < 128; ++i) pitches[i] = i * 100.0 + offset;
      for (unsigned i = 0; i < count; ++i) {
            if (fluid_synth_activate_key_tuning(banks[i].synth, 0, 0, "37E master tuning", pitches, 1) != FLUID_OK) return false;
            const int channels = separate ? ChannelsPerBank : MidiChannels;
            for (int ch = 0; ch < channels; ++ch)
                  if (fluid_synth_activate_tuning(banks[i].synth, ch, 0, 0, 1) != FLUID_OK) return false;
            }
      return true;
      }

void Fluid2SynthBanks::noteOn(int logical, int key, int velocity, float cents)
      {
      const Channel target = channel(logical);
      if (!target.synth) return;
      if (!std::isfinite(cents) || cents == 0.0f) {
            fluid_synth_noteon(target.synth, target.channel, key, velocity);
            return;
            }
      // Snapshot IDs within this bank, so other held/releasing notes stay tuned.
      std::array<fluid_voice_t*, TotalVoices> voices {};
      std::array<unsigned int, TotalVoices> previousIds {};
      fluid_synth_get_voicelist(target.synth, voices.data(), TotalVoices, -1);
      int size = 0;
      for (fluid_voice_t* voice : voices) {
            if (!voice) break;
            previousIds[size++] = fluid_voice_get_id(voice);
            }
      std::sort(previousIds.begin(), previousIds.begin() + size);
      if (fluid_synth_noteon(target.synth, target.channel, key, velocity) != FLUID_OK) return;
      voices.fill(nullptr);
      fluid_synth_get_voicelist(target.synth, voices.data(), TotalVoices, -1);
      for (fluid_voice_t* voice : voices) {
            if (!voice) break;
            if (fluid_voice_get_channel(voice) != target.channel || fluid_voice_get_key(voice) != key
                || std::binary_search(previousIds.begin(), previousIds.begin() + size, fluid_voice_get_id(voice))) continue;
            fluid_voice_gen_incr(voice, GEN_FINETUNE, cents);
            fluid_voice_update_param(voice, GEN_FINETUNE);
            }
      // Voice generator updates queue DSP events outside a synth API call.
      // Complete a lightweight API boundary to flush them before rendering;
      // otherwise the last tuned attack can begin with its previous pitch.
      (void)fluid_synth_get_gain(target.synth);
      }

bool Fluid2SynthBanks::process(unsigned frames, float* output, float* auxiliary)
      {
      if (!count || (!output && frames)) return false;
      if (auxiliary) {
            if (!separate || frames > std::numeric_limits<std::size_t>::max() / (2 * sizeof(float))) return false;
            const std::uintptr_t dry = reinterpret_cast<std::uintptr_t>(output);
            const std::uintptr_t aux = reinterpret_cast<std::uintptr_t>(auxiliary);
            const std::size_t bytes = std::size_t(frames) * 2 * sizeof(float);
            if ((dry > aux ? dry - aux : aux - dry) < bytes) return false;
            }
      bool success = true;
      for (unsigned bank = 0; bank < count; ++bank) {
            const bool compactDry = separate && (!auxiliary || !sends.hasRoutedGain(bank));
            if (compactDry) {
                  // The public process API wraps every dry channel into the
                  // supplied stereo pair. When no send needs individual lanes,
                  // avoid clearing/copying/summing 128 host stereo planes.
                  // Keep the two synth banks, voices and internal timeline intact.
                  float* planes[2] = { stereo.data(), stereo.data() + 512 };
                  for (unsigned start = 0; start < frames;) {
                        const unsigned n = std::min(512u, frames - start);
                        std::fill_n(planes[0], n, 0.0f);
                        std::fill_n(planes[1], n, 0.0f);
                        if (fluid_synth_process(banks[bank].synth, int(n), 0, nullptr, 2, planes) == FLUID_OK) {
                              for (unsigned j = 0; j < n; ++j) {
                                    output[(start + j) * 2] += planes[0][j];
                                    output[(start + j) * 2 + 1] += planes[1][j];
                                    }
                              }
                        else success = false;
                        start += n;
                        }
                  }
            else if (outputs) {
                  for (unsigned start = 0; start < frames;) {
                        const unsigned n = std::min(512u, frames - start);
                        // Match compact output's bank-local summation order:
                        // changing a send must not change the dry PCM rounding.
                        std::fill_n(stereo.data(), n * 2, 0.0f);
                        const bool rendered = outputs->process(n, stereo.data(), [this, bank](int count, int channels, float** planes) {
                              return fluid_synth_process(banks[bank].synth, count, 0, nullptr, channels, planes) == FLUID_OK;
                              }, [this, bank, auxiliary, start](unsigned offset, unsigned count, unsigned groups, float** planes) {
                                    if (auxiliary) sends.mix(bank, start + offset, count, groups, planes, auxiliary);
                              });
                        if (rendered) {
                              for (unsigned j = 0; j < n * 2; ++j) output[start * 2 + j] += stereo[j];
                              }
                        success = rendered && success;
                        start += n;
                        }
                  }
            else {
                  for (unsigned start = 0; start < frames;) {
                        const unsigned n = std::min(512u, frames - start);
                        if (fluid_synth_write_float(banks[bank].synth, int(n), stereo.data(), 0, 2,
                                    stereo.data(), 1, 2) == FLUID_OK) {
                              for (unsigned j = 0; j < n * 2; ++j) output[start * 2 + j] += stereo[j];
                              }
                        else success = false;
                        start += n;
                        }
                  }
            }
      if (success) sends.advance(frames);
      return success;
      }

bool Fluid2SynthBanks::allSoundsOff(int logical)
      {
      if (!count || logical < -1 || logical >= MidiChannels) return false;
      bool success = true;
      if (logical < 0) {
            for (unsigned i = 0; i < count; ++i)
                  success = (fluid_synth_all_sounds_off(banks[i].synth, -1) == FLUID_OK) && success;
            }
      else {
            const Channel target = channel(logical);
            success = fluid_synth_all_sounds_off(target.synth, target.channel) == FLUID_OK;
            }
      for (int ch = logical < 0 ? 0 : logical; ch < (logical < 0 ? MidiChannels : logical + 1); ++ch) {
            const Channel target = channel(ch);
            fluid_synth_cc(target.synth, target.channel, 64, 0);
            fluid_synth_cc(target.synth, target.channel, 66, 0);
            }
      if (success) {
            if (logical < 0) sends.clearSources(2 * fluid_synth_get_internal_bufsize(banks[0].synth));
            else clearSourceChannel(logical);
            }
      return success;
      }

bool Fluid2SynthBanks::allNotesOff(int logical)
      {
      if (!count || logical < -1 || logical >= MidiChannels) return false;
      for (int ch = logical < 0 ? 0 : logical; ch < (logical < 0 ? MidiChannels : logical + 1); ++ch) {
            const Channel target = channel(ch);
            fluid_synth_cc(target.synth, target.channel, 64, 0);
            fluid_synth_cc(target.synth, target.channel, 66, 0);
            }
      if (logical >= 0) {
            const Channel target = channel(logical);
            return fluid_synth_all_notes_off(target.synth, target.channel) == FLUID_OK;
            }
      bool success = true;
      for (unsigned i = 0; i < count; ++i)
            success = (fluid_synth_all_notes_off(banks[i].synth, -1) == FLUID_OK) && success;
      return success;
      }
}
