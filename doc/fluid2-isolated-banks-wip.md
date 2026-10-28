# 37E audio v0.14: optional independent outputs for all 256 channels

Apply after audio v0.13. The user reported v0.13 applied. User audio/WAV and
large-score startup tests remain explicitly deferred in `audio-verification.md`.

Update: v0.15 adds zero-default source gains and auxiliary signal generation
on these outputs; see `fluid2-auxiliary-sends-wip.md`. It does not yet attach
an audible host reverb return.

## Implemented behavior

The adapter now delegates its native synthesis operations to Fluid2SynthBanks,
a Qt-independent core covered by real FluidSynth tests. Without a new option,
it retains the single-synth interleaved renderer, or the v0.12 planar route if
that earlier option is selected. The old Fluid/Zerberus factory choice is
unchanged. New synthesis code is still built only with the existing experimental
FluidSynth option enabled.

To select actual channel isolation when audio testing resumes:

```sh
MSCORE_USE_FLUIDSYNTH2=1 MSCORE_FLUID2_ISOLATED_CHANNELS=1 /path/to/mscore
```

The new variable takes precedence over `MSCORE_FLUID2_AUDIO_GROUPS`. It is read
at synth initialization and is not saved in a score. It creates two synth banks,
each with 128 MIDI channels and 128 separate stereo dry outputs:

| Logical score channel | Synth bank | Internal MIDI channel / stereo output |
| --- | --- | --- |
| 0-127 | First | 0-127 |
| 128-255 | Second | 0-127 |

Thus logical 0 and 128 have different synth/output addresses. The existing
score events and external MIDI mapping retain their logical channels. Bank,
program, percussion type, CC, pressure, pitch wheel and note tuning use the
translated address. Master tuning is activated on both banks. Stop/reset can
target one logical channel or both banks; sustain and sostenuto are cleared
accordingly. Note release retains its source owner.

The scratch planar buffer is reused sequentially between banks, before their
dry audio is added into the host mix. Each output remains identifiable by bank
and group before summing. No render-buffer allocation occurs in this core's
process method. Internal FluidSynth reverb/chorus remain disabled. Existing
master Effect A/B and gain still process the summed output. Sends and a reverb
return are not enabled by this increment.

## Voice and font policies

The total configured voice budget stays 512: each isolated bank has 256 voices.
This preserves the configured aggregate ceiling but limits a passage concentrated
in one half of the channel range to 256 voices. Such a passage can steal voices
earlier than the default single synth's 512-voice pool. This is an explicit WIP
tradeoff; elastic sharing or another allocation policy can follow after tests.
It is not a guarantee of identical behavior at the polyphony limit.

Each bank owns its own loaded SoundFont object and preset metadata. The adapter
still exposes one logical font and one patch list, with each bank using its own
font ID internally. All new font objects are loaded before any old font is
removed. A second-bank load failure rolls back the first new load and retains
the old fonts/presets. Successful replacement stops old voices, runs two internal
DSP blocks into discarded output to complete their queued teardown, and unloads
the old fonts. This remains a stopped-loading design, not realtime font loading.

The default FluidSynth loader has a process-wide, reference-counted sample cache
keyed by filename, file metadata and sample range/type. Loading the same path
separately is therefore expected to share sample data with that loader, while
preserving separate owned SoundFont objects. This source-based expectation is
not a measured RAM guarantee for all custom loaders, SF3 files or platforms.
Preset metadata, an additional mixer and synth bookkeeping still cost memory.
Planar scratch storage is about 512 KiB plus pointers at 128 groups. CPU/RAM and
large-score startup measurements remain pending, especially on older hardware.

## Two findings from real-engine testing

Per-note tuning updates a voice generator outside a synth API call. In the
reviewed FluidSynth 2.3.x implementation, those DSP updates are queued until a
public synth API exit flushes them. The core now completes a lightweight getter
API boundary after tuning an attack. This prevents the final tuned note in a
bank from starting its first DSP block with the previous pitch. It also fixes
that timing defect in single-synth mode. A PCM regression compares a +100-cent
attack against a globally tuned reference, without an intervening inspecting
API call that would hide the missing flush. The implementation was checked
against 2.3.3/2.3.4; future FluidSynth changes to API flushing require review.

The old single-synth reference also crashed when controller modulation was sent
to internal channel 255. The reviewed 2.3.4 code uses 0xff as the unused voice's
channel marker, and its modulation walk can reach those unused voices on channel
255. The new isolated route never sends an internal channel above 127 and tests
logical 255 explicitly. This WIP does not patch the system FluidSynth library or
automatically enable isolation, so the default single-synth route retains that
upstream channel-255 limitation. Native PCM comparison against the default route
uses channels through 254; logical 255 is checked separately in isolated mode.

## Verification

The native test uses the production Fluid2SynthBanks source and Ubuntu's actual
FluidSynth 2.3.4 library. It generates an original sine-wave SF2 fixture, avoiding
a dependency on an installed SoundFont. It checks:

- All 256 translated addresses, distinct owned fonts, and the two native
  256-voice limits.
- Independent low/high-bank volume and pitch bend, logical-255 controllers,
  melodic/drum preset selection, per-note tuning and master tuning.
- Separate planar group contents in both banks, sostenuto holding through
  note-off after sustain is released, channel-specific sound reset, global note/pedal reset, release completion,
  successful font replacement/unload and sample-rate reinitialization.
- A deliberately failed second-bank load using Linux linker wrapping, including
  rollback font counts, retained held notes and a usable old preset. Other platforms check a failed
  load without the Linux wrapper.
- PCM agreement within 0.00001 with the single-synth reference for 1/16/128 raw
  groups and two-bank mode at 44.1/48 kHz, using variable block lengths, panning,
  bends and note tuning. This comparison stays below the polyphony limit.
- The first-block tuning regression described above.

These native checks pass under strict GCC C++11 compilation and with our core
and test instrumented by AddressSanitizer/UndefinedBehaviorSanitizer. The system
FluidSynth library itself is not sanitizer-instrumented. Leak detection is
disabled because this environment restricts LeakSanitizer's process inspection.
The existing standalone buffer/routing tests and v0.11 controller harness pass.

The native test is included when the experimental dependency is available:

```sh
ctest --output-on-failure -R '^(tst_midi|tst_fluid2_audio|tst_fluid2_routing|tst_fluid2_banks)$'
```

The standalone CMake project can still build its two dependency-free tests when
FluidSynth development files are absent; it adds the native test when pkg-config
finds FluidSynth >=2.3.3. Full Qt/application compilation and CMake integration
are not verified in this environment. Native PCM checks do not substitute for
the user's deferred score playback/WAV, SF3, repeat/seek and startup checks.

Source references inspected:

- https://github.com/FluidSynth/fluidsynth/blob/v2.3.3/src/sfloader/fluid_samplecache.c
- https://github.com/FluidSynth/fluidsynth/blob/v2.3.4/src/synth/fluid_synth.c
  (`fluid_synth_api_exit`, `fluid_synth_get_gain`, modulation and font loading).
- https://github.com/FluidSynth/fluidsynth/blob/v2.3.4/src/synth/fluid_voice.h
  (`NO_CHANNEL`).

## Next action

Add zero-default source send gains and mix a shared auxiliary input from these
independent outputs, using bank/group ownership for bend lanes. First verify the
dry and auxiliary signals independently, then attach an existing compatible
reverb as a wet return before master Effect A/B. Keep host-send state distinct
from MIDI-out CC91 until its compatibility policy is decided. No GPL3 DSP or
license changes are included here.
