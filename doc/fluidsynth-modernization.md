# 37E audio v0.01: FluidSynth modernization audit

Status: feasibility and integration checkpoint, October 28, 2026. No playback
behavior or build configuration changes are included in this checkpoint.

## Branch and references

- Work branch: `3.7-Evolution/backports/fluidsynth-modernization`.
- Initial 37E base: `31f21002ac9e248685ebbc7fa5bbdf554361d3d7`, the existing
  backports branch used by the slur/tie project. This audit branch does not yet
  contain the subsequent LV increments; merge those before implementing LV.
- Studio reference: official `4.7`, resolved during this audit to
  `96480d58445e3b24b230a3fe7d4725d639aa2eb3`.
- The reference's `src/framework/audio/thirdparty/fluidsynth/fluidsynth.cmake`
  selects its bundled `fluidsynth-2.3.3` tree. Preserve and review MuseScore's
  modifications rather than assuming an unmodified system library is identical.
- Reference adapter:
  `src/framework/audio/engine/internal/synthesizers/fluidsynth/fluidsynth.cpp`.

## Intended result

Use modern FluidSynth for SF2/SF3 synthesis inside 37E, with a tested MS Basic
comparison configuration and functional LV playback. Keep the existing 37E
sequencer, notation-to-MIDI rendering, audio drivers, and export infrastructure.
This is not a promise of identical MS4 playback from a backend substitution:
MS4's articulation/event processing and instrument routing can generate
different input events even when the synth and SoundFont are the same.

MS Basic is a SoundFont, not the name of the whole playback engine. Compare
exactly the same SoundFont bytes first; record their SHA-256 and the selected
bank/program. A better sounding guitar alone does not identify an engine fix.

## Source findings and integration boundaries

1. `audio/midi/synthesizer.h` provides the adapter boundary: initialization,
   MIDI events, audio rendering, tuning, patch enumeration, SoundFont loading,
   and saved synthesizer state.
2. The old backend is an adapted C++ implementation in `audio/midi/fluid`;
   replacing its source files with modern FluidSynth is not a direct update.
   Build the modern library separately and put an adapter at this boundary.
3. `audio/midi/fluid/fluidgui.h` casts the generic synthesizer to
   `FluidS::Fluid`. Its progress/cancel/loading methods require an explicit
   compatibility design; a new backend cannot simply reuse that cast.
4. `Fluid::updatePatchList()` maps multiple SoundFonts using accumulated bank
   offsets and preserves a SoundFont ID. Preserve mixer patch selection and
   font precedence, including bank 128 percussion and conflicting presets.
5. `Fluid::state()` and `setState()` serialize the loaded font list as the
   existing Fluid synthesizer group. Retain compatibility with saved scores.
6. `MasterSynthesizer::process()` expects each active synth to add to the output
   buffer, then applies host effects and master gain. Render modern FluidSynth
   into scratch buffers and accumulate it; do not overwrite another synth's
   output. Avoid unintended duplicate effects or duplicated gain.
7. The 4.7 adapter sets polyphony 512, uses default interpolation, disables
   FluidSynth's internal chorus/reverb, and controls its IIR low-pass filter
   through a MuseScore setting. Its gain constant is 4.8. These are reference
   settings, not values to copy blindly into 37E's different mixing chain.
8. 37E already contains resonant filter processing and has a deliberate change
   to its default velocity-to-filter modulator. Check those paths before
   attributing differences to MS4's restoration of its low-pass filter.
9. 4.7 uses 16 MIDI channels per synth instance. 37E has different channel
   allocation. Preserve scores exceeding 16 mapped channels, pitch-bend lanes,
   per-note tuning, percussion routing, and instrument changes.
10. SoundFont loading currently uses locks and asynchronous GUI operations.
    Establish synth ownership and synchronization explicitly; do not borrow
    MS4's disabled thread-safe API setting without its threading guarantees.

## Recoverable implementation stages

1. **Backend adapter:** pin the reference library and introduce a selectable
   modern backend with the existing backend available for comparison. First
   gate: one SF2/SF3 preset produces sound through normal playback and export.
2. **Compatibility:** implement font ordering, patch enumeration, mixer and
   saved-state compatibility, controller mapping, tuning, and loading UI.
   Verify playback stop/seek, sample-rate changes, multiple instruments and
   repeated notes before making the modern backend the preferred choice.
3. **MS Basic synthesis comparison:** render identical timed MIDI input through
   37E and the reference library using identical font, gain, interpolation,
   controllers and effects. Then compare score-level output separately to
   expose differences in event generation. Listen to guitar, piano, sustained
   strings and percussion; measure clipping and stereo output levels.
4. **LV integration:** merge the verified notation patches; generate CC66
   events with the reference release window. Verify tied-chain ends, repeated
   pitches, overlapping LV requests, mixed marked/unmarked chords, sustain plus
   sostenuto, stop/seek/reset, repeats, and real-time versus exported audio.
   CC66 is channel-wide: do not claim arbitrary per-note isolation from it.
5. **Packaging and regression:** verify Qt 5.9.9 builds and dependency packaging
   on the supported platforms. Make the modern backend the default only after
   the compatibility and audio checks pass. Keep SFZ/Zerberus a separate scope.

Save each functional milestone as its own commit and versioned patch. Do not
replace the LV notation verification branch with this experimental work.

## Current verification and limitations

37E interface, GUI coupling, font-bank mapping, mixing, saved-state code, and
4.7 adapter/build selection were inspected. No implementation, compilation,
mtests or audio comparison has been performed. This preparation environment
currently lacks CMake, pkg-config, Qt development tools and a FluidSynth build.
The next action is stage 1, beginning with the library build and adapter seam.

## 37E audio v0.02: opt-in adapter WIP

This increment adds a system-library FluidSynth adapter, disabled at build and
runtime by default. It does not vendor the reference library yet. The build
requires pkg-config and FluidSynth >= 2.3.3, including development headers.
This deliberately separates adapter validation from dependency packaging.

Configure the existing 37E build with:

```sh
cmake -S /path/to/source -B /path/to/audio-build \
  -DMSCORE_EXPERIMENTAL_FLUIDSYNTH2=ON
cmake --build /path/to/audio-build
```

Launch the resulting executable with these environment variables. Use a
separate configuration folder for this experiment, since the first WIP can
only preserve one SoundFont in saved synthesizer state:

```sh
MSCORE_USE_FLUIDSYNTH2=1 \
MSCORE_FLUID2_SOUNDFONT=/absolute/path/to/MS\ Basic.sf3 \
  /path/to/mscore --config-folder /absolute/path/to/audio-test-config
```

Run the same executable without MSCORE_USE_FLUIDSYNTH2=1 to select the old
backend. The font override is optional: without it, this WIP loads the first
font in score/default settings and resolves missing paths using 37E's existing
SoundFont search directories. Additional fonts are ignored with a diagnostic.
Use the override for an unambiguous MS Basic comparison.

Implemented in this stage:

- Factory selection shared by live playback and audio export, retaining the
  Fluid synthesizer identity and the old backend as the default.
- Single-font SF2/SF3 loading, patch enumeration, bank/program selection and
  percussion channel type selection. A failed font replacement retains the
  previous loaded font. SF3 requires a FluidSynth build with SF3 support.
- Note on/off, channel/key pressure, pitch bend and normal CC events including
  CC66. The score renderer does not yet generate LV CC66 events.
- 256 MIDI channels and 512 voices; master tuning using MIDI tuning tables.
- Interleaved float rendering into a fixed scratch buffer, accumulated into
  the host buffer without overwriting other synths or allocating in rendering.
- Internal reverb and chorus disabled; host effects retained. Gain is 0.2,
  deliberately uncalibrated at this stage. This is not yet an MS4 sound match.
- A minimal experimental Synthesizer panel; it avoids the old FluidGui cast.

Known limitations: no multi-font bank-offset mapping or font editing UI,
per-note tuning, asynchronous loading/progress/cancel, library bundling, sound
calibration, or LV score-event generation. Sample-rate reinitialization reloads
the font but does not restore all transient controllers. Font loading is
serialized; rendering skips a block when loading owns the mutex. This WIP is
not a realtime loading design. Avoid saving production scores with it.

First build/audio gate for the next iteration:

1. Build with the option OFF, then ON. Confirm the old backend remains selected
   without the environment variable, and the experimental panel appears with it.
2. With one known SoundFont, play piano/guitar notes and chords, repeated pitches,
   percussion, and a sustained instrument; verify program changes and stop.
3. Export WAV and check that sound is present in both channels. Compare exports
   with live playback using the same font. Match volume before judging timbre.
4. Check master tuning, pitch bends, normal pedal events, and reopening a test
   score in the isolated configuration. Per-note tuning remains unsupported.

Checkpoint validation: source/API inspection and whitespace/patch checks only.
Compilation, Qt mtests, live playback and exported audio remain unverified.

## 37E audio v0.03-v0.04: build integration corrections

v0.03 uses the same plain target_link_libraries signature as the existing
audio target. v0.04 moves the experimental adapter into a separate static
target. Only that target receives FluidSynth's pkg-config compile requirements;
audio and its consumers receive the link dependency without those compile
flags. This prevents feature-test definitions such as _DEFAULT_SOURCE from
invalidating audio's shared ms_pch precompiled header.

The adapter compiles without a PCH and includes the common header textually
for legacy declarations. Existing audio sources continue reusing ms_pch.
Reconfigure and rebuild after applying; existing audio objects may rebuild once
because their compiler flags changed. No clean rebuild should be necessary.
A standalone CMake 3.28.4/GCC 13.3 reproduction built successfully: a private
dependency's _DEFAULT_SOURCE macro reached only the adapter, while audio reused
the shared PCH and the final executable linked. Full application compilation
and runtime checks remain pending.

## 37E audio v0.05: per-note tuning and next milestones

Per-note tuning now adds PlayEvent::tuning(), in cents, to the fine-tuning
generator of every sample voice created by that attack. Existing voice note
IDs are captured before note-on; only new matching voices are modified. This
preserves SoundFont fine tuning, master tuning and channel pitch bend without
altering earlier held/releasing notes or other notes in the chord. Untuned
attacks use the normal path; offsets do not persist to the next attack. Voice
lists use fixed storage bounded by the configured 512-voice polyphony limit.
The adapter mutex serializes voice access and rendering.

Open `doc/fixtures/fluid2-note-tuning-v1.mscx` in the experimental build:

1. Bar 1: written C5 at 0 / +100 / -100 / 0 cents should sound C / C-sharp /
   B / C. The final C must not retain the previous tuning. Bar 2 repeats this
   with +50 / -50 cents for subtler differences.
2. Bars 3-4: compare an ordinary C-major chord with one whose E alone is
   raised 100 cents. C and G must stay unchanged.
3. Bars 5-6: the same written C on two simultaneous staves has opposite
   offsets. The instruments/channels must remain independent.
4. Change master tuning, repeat, stop/seek and export WAV. Compare with the
   old backend. With sustained instruments, listen for unintended changes in
   previously ringing notes while a newly tuned note starts.

Source review checked the FluidSynth 2.3.3 public voice API and its fine-tune
generator update path. XML and patch checks were performed. Compilation,
frequency measurements and audio verification of this increment remain pending.
Prior user checks confirmed piano playback/export, rapid repeated notes and
basic stop/seek in v0.04. Repeat navigation remains a separate test.

Continue with these independently recoverable stages:

1. Verify this tuning increment, then instrument changes, percussion, repeats,
   controller restoration and sample-rate changes.
2. Merge verified LV notation increments and implement score-to-CC66 events,
   checking tied-chain ends, overlapping requests, repeat jumps and stop/seek.
   The old backend needs a separate fallback policy; CC66 alone is insufficient
   there. Test both live playback and audio export.
3. Restore multi-font bank mapping/loading and UI compatibility, then calibrate
   gain, filtering and effects using the same SoundFont and input events as 4.7.
4. Expose per-instrument audio before summing it, add saved send amounts and a
   shared auxiliary reverb return, and retain master Effect A/B after that mix.
   Keep sends zero by default for older scores. First test sends with an
   existing compatible effect, independently of any new reverb algorithm.
5. Evaluate Muse Reverb as an optional additional unit. The inspected 4.7
   ReverbProcessor source is GPL-3.0-only, while this checkout's LICENSE.GPL
   specifies GPL version 2. Direct copying requires a resolved license
   compatibility/permission path, as well as an audio-framework adapter.
   No Muse Reverb source is copied by this patch. Zita and the existing master
   effects remain available regardless of the eventual new-unit decision.
Next: build and fix this seam before extending compatibility or LV behavior.
