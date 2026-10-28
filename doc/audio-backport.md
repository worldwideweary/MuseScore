# Audio backport: configuration and current checkpoint

Updated November 6, 2026. This is the current entry point; the versioned
documents and `audio-verification.md` retain the development history.

## Current configuration

The supported application baseline is Qt 5.15.2. Keep these CMake options:

```text
MSCORE_EXPERIMENTAL_FLUIDSYNTH2=ON
MSCORE_FLUIDSYNTH2_PROVIDER=BUNDLED
```

Use these launch settings for modern playback and independent instrument sends:

```text
MSCORE_USE_FLUIDSYNTH2=1
MSCORE_FLUID2_ISOLATED_CHANNELS=1
```

Normally leave `MSCORE_FLUID2_SOUNDFONT` unset. It is a file-path override,
not a Boolean; setting it fixes the font selection and disables list editing.
Use the Fluid tab for discovered fonts, ordering, removal and defaults. The
chooser honors installed/extension fonts and the configured SoundFonts folders.
`MSCORE_FLUID2_AUDIO_GROUPS` is an earlier raw-output diagnostic, not needed
with isolated channels. `MSCORE_FLUID2_REVERB_SEND` is retired; use Mixer sends.
`MSCORE_AUDIO_EXPORT_TIMING=1` enables optional export timing diagnostics.
`MSCORE_AUDIO_PREP_TIMING=1` traces score opening and playback preparation.

Bundled FluidSynth 2.3.4 builds from checked-in source without a download.
GLib/GThread, libsndfile and codec dependencies remain external. SF3 support is
required. Provenance, source modifications, integrity checks and the bundled
license are documented in `thirdparty/fluidsynth2/README.md`.

`MSCORE_FLUIDSYNTH2_OPTIMIZE_DEBUG=ON` retains symbols while optimizing the
modern engine and Zita DSP in Debug. It defaults ON. Other application Debug
settings are retained. The build/runtime opt-ins also remain deliberate:
this cleanup does not change which engine existing users launch.

## Compatibility and controls

| Configuration | Playback and sends |
| --- | --- |
| Bundled provider, modern runtime, isolated channels | All 256 logical channels, independent instrument sends, shared wet-only Instrument Reverb, v0.40 release-pitch fix |
| Modern runtime without isolated channels | Stereo/raw-group playback; independent instrument sends unavailable |
| SYSTEM provider | Modern routing using installed FluidSynth >=2.3.3; unmodified libraries retain native pitch-wheel release behavior |
| Original Fluid runtime | Existing playback and v0.40 release-pitch fix; independent modern instrument sends unavailable |
| Experimental backend disabled | Original Fluid/Zerberus remain available; no bundled FluidSynth dependency |

Audio reverb send in Mixer is separate from MIDI CC91. Old scores default to
zero host send. Source instrument volume/pan and mute behavior are retained;
isolated bend lanes follow their source's send. Instrument Reverb is one
shared return before the existing master A/B slots, with its own room/tone/
return settings. Save to Score / Set as Default cover these settings. Zerberus
and external MIDI retain their existing paths; this feature does not add host
instrument sends to Zerberus or independent per-note pitch control to MIDI.

The C score's staff-text `pedal` action explicitly selects program 0 as well
as setting controllers. CC121 resets controllers while keeping that program;
`baseline` explicitly restores program 48 on the same channel. These actions
do not create instrument-change objects or alter the score's preview timeline.

## Accepted checkpoint and remaining checks

v0.40 is user-accepted: mtests pass, playback and WAV export sound good, and
the ordinary C2 bend-reset artifact is no longer audible. Earlier application
checks accepted font discovery/order/defaults and saved selection, independent
sends and return persistence, transport/repeats/LV, controllers, pan, parameters,
portamento, tuning and normalized/unnormalized WAV behavior. The native v0.40
baseline passes bundled 17/17, SYSTEM 15/15 and dependency-disabled 7/7.

v0.41 closes export failure-path cleanup and supplies one generated-font mtest
for raw rendering, normalization, cancellation, failed/short writes, output-open
failure and sample-rate restoration. Focused production-renderer/file-wrapper
checks cover 44.1/48 kHz file metadata and pitch as described in
`audio-export-cleanup-v41.md`. These are automated component results, not a new
manual application acceptance. Full application build/mtests and one normal
export/cancel/re-export smoke check remain for v0.41.

The first v0.41 application mtest build exposed a duplicate-symbol link failure
in `tst_midi`. v0.42 separates the common renderer from the main-window export
object and supplies the test factory explicitly. Focused archive and export
checks pass; full application mtests and the cleanup smoke check remain pending.
See `audio-export-test-link-v42.md`.

The user now reports v0.42 mtests pass and audio tests seem okay. Canceling
works but displays the generic write-error popup. v0.43 passes cancellation
separately so this action is quiet while actual I/O failures still report an
error. Quiet cancellation/retry and v0.43 mtests remain the final application
check for this correction; see `audio-export-cancel-v43.md`.

The user subsequently confirms v0.43 cancellation stays quiet and mtests pass.
This accepts the cancellation correction and closes the current audio cleanup
phase. Keep v0.43 as the stable delivered audio checkpoint when planning the
standalone LV extraction below. The explicit deferred boundaries still apply;
this acceptance does not establish additional platform/resource/MP3 checks.

The user then reopened large-score preparation and the D5–D7 click/seek fifth.
v0.44 removes MIDI collection from Play Panel initialization, avoids repeated
feature scans on unchanged playlists and preserves the running event iterator
until the audio thread handles a seek. Optional timing diagnostics distinguish
file/layout work from playback preparation. Component checks pass; the reported
197-page score's new load time and application listening/mtests remain pending.
See `audio-large-score-seek-v44.md`. Keep v0.43 as the accepted fallback until
these application checks are complete; standalone LV extraction has not started.

## Current TODO and acceptance scope

The user accepts the large-score load-time check for now: the same score takes
40.400 s in standard 3.x and 40.714 s in audio v0.44. This establishes no substantial
opening-time regression in this comparison. Layout optimization is outside the
current audio work. It does not establish broader resource or Piano Roll results.

v0.45 implements the restart-required engine preference below; application
verification of that preference remains pending. The user accepts v0.44 mtests
and playback and reports MP3 44.1/48 kHz listening okay. Their metadata inspection
was inconclusive; do not infer a verified stream sample rate from absent tags.
See `audio-engine-preference-v45.md`.

Remaining finishing work:

- Verify the saved Audio engine preference: Original Fluid / FluidSynth 2 (when built),
  applied after restarting. Keep the current engine until restart, explain the
  restart requirement, and select the supported isolated routing automatically
  for ordinary modern-engine use. Environment variables may remain explicit
  diagnostic overrides. Verify switching both ways and SoundFont/state behavior.
- Obtain application build/mtest acceptance for v0.45. Finish with a bounded transport, saved
  state and WAV smoke check using the preference-selected engines.
- Close application-level 44.1/48 kHz output stream metadata verification.
  MP3 listening is now user-accepted. Existing generic
  float-stream/component checks do not establish LAME output listening.
- Consolidate supported configurations, engine-specific feature limits, accepted
  checks and recoverable pre-audio/current checkpoints in the final documentation.

Deferred work, not release blockers for this bounded Linux feature:

- The D5–D7 click/seek fifth, also present in 3.6.2, is deferred again for a later
  audio update. v0.44 fixes a reproduced iterator rewind, but elimination of the
  user's audible fifth has not been confirmed.
- Standalone LV extraction and LV support in the original Fluid engine are
  indefinitely deferred. Treat the current updated engine as the supported LV
  playback path; retain the original engine as a documented fallback with feature
  limits. Do not start extraction just to create another safety checkpoint.
- Broader large-score memory/CPU/Piano Roll checks, complete macOS/Windows builds
  and arbitrary SoundFont/controller combinations remain unestablished.
- Master EQ, stereo widening, more than two master effects slots and Piano Roll
  CC automation are optional later enhancements. Regular tie/slur engraving and
  layout performance work remain outside this audio finishing phase.

v0.45 follow-up: the user reports mtests pass, but first-run engine choices can
be duplicated and Original Fluid has no default font. v0.46 populates the selector
once, adds original-engine default recovery/shared path resolution, and hides
unsupported mixer send controls. Focused original-engine font checks and MIDI
header checks pass; application mtests and UI/font acceptance remain pending.
See `audio-engine-fixes-v46.md`.
