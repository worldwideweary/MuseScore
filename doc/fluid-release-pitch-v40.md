# Audio v0.40: retain bent pitch in released voices

Apply this patch directly on delivered v0.39. It includes the previously
unpublished v0.39 application-test and export-timing acceptance notes.
Qt 5.15.2 remains the supported application baseline.

## Problem and behavior

An ordinary bend ends with a pitch-wheel reset so following notes start
centered. A note-off starts a release envelope rather than immediately
silencing the sample. The reset therefore retuned that still-audible tail,
causing the brief return toward C before the reference D in C2 of
`doc/fixtures/audio-compatibility-v28/C-reset-and-bend.mscx`.

Original Fluid and bundled FluidSynth now retain the wheel position and wheel
sensitivity per voice when it is told to enter release. Resetting the channel
then prepares the next note without retuning the old tail. This also covers
exclusive-class release. Held and sustain/sostenuto-held voices keep using
the live wheel until actual release; normal voice initialization and modern
legato retrigger restore live wheel input. Legacy sustain is checked directly;
its existing sequencer handling of sostenuto is unchanged.

The latch applies to every actual release, not just automatic score-bend
resets: deliberately moving the wheel after release no longer bends that
tail. This retains only the wheel sources, including authored modulators
using the wheel or its sensitivity as either source. Other controllers,
LFOs, generators and release envelopes continue to operate. CC121 after
release cannot replace the latched wheel contribution when it recomputes
modulators. This does not redefine controller-reset behavior on held voices.
There is no new tail cutoff, sample-zone transposition or delayed MIDI event.

The authored secondary-wheel regression exposed an existing legacy scaling
omission: source 2 used the generic 7-bit range for the 14-bit wheel, unlike
source 1. It now uses the same 0x4000 range as source 1. This avoids extreme
pitch offsets for valid SoundFonts using the wheel as the amount source.

Bundled FluidSynth has a creation-time, opt-in integer setting
`synth.mscore-release-pitch-lock` (default 0). The adapter enables it when
available. This is the first intentional host-specific synthesis-source
customization of the pinned 2.3.4 bundle. Source provenance and hashes are
updated; the license and original authorship remain intact.

Unmodified SYSTEM FluidSynth lacks the setting and keeps upstream release
behavior. External MIDI remains channel-wide and also keeps the receiving
synthesizer's behavior. MIDI events, export code, sequencing, notation,
normalization and effects are unchanged. The separate D5–D7 note-click fifth
in the parameter/portamento score remains deferred.

## Verification

- Bundled optimized Debug native build/CTest: 17/17 pass. The new test covers
  44.1/48 kHz, stereo/raw/isolated rendering, both isolated banks, held wheel
  changes, sustain/sostenuto, changed sensitivity, CC121 after release,
  independent following attacks, pool reuse, authored wheel modulation in
  either source, and exclusive-class release. The raw setting-off check
  retains wheel-sensitive tails; setting-on retains their release pitch.
- Original Fluid production-code PCM check: all 48 generated-SF2 cases pass
  across rates, channels 0/128/255, sustain, authored modulation, CC121,
  exclusive classes, following attacks and voice reuse. The same regression
  fails at the first release-tail comparison against pre-fix production
  objects. It is included as `TestMidi::audioLegacyReleasePitch`, also when
  the experimental backend is disabled.
- Real bundled FluidR3Mono_GM.sf3 strings comparison: wheel reset versus no
  reset produces bit-exact release/next-note output in the focused C-to-D
  reproducer. This is PCM verification, not a user listening result.
- SYSTEM build/CTest: 15/15 pass. Dependency-disabled standalone tests: 7/7
  pass. These establish compatibility, not the release fix in SYSTEM.
- Focused production Qt adapter regressions pass in all three rendering
  modes. Full MIDI-test translation-unit syntax checks pass with the modern
  backend enabled and disabled.
- AddressSanitizer/UndefinedBehaviorSanitizer pass for the new native
  regression and production adapter/buffer code. The linked bundled library
  is the regular optimized Debug build. Leak detection is disabled because
  the environment cannot perform the required process inspection.

The patch environment has a focused SDK, not a complete application build.
On 2026-11-06 the user reports mtests pass, playback and WAV export perform
well, and the bend-reset problem is no longer audible. This closes the user
acceptance check for the ordinary C2 release fix. This report does not imply
separate SYSTEM/external-MIDI, legacy-backend, sample-rate metadata or
large-score listening/resource acceptance.
No MIDI export/reference files need updating because the event stream is
unchanged; existing score/export tests should continue passing.

## Application check

Keep the current bundled backend and launch settings. Rebuild the application
and mtests. In the existing C score, play C1 through C3 and then start directly
at C2: the C should bend toward D, its release should stay there, and the
following reference D should start at D without the brief return toward C.
Repeat as WAV export with the same SoundFont. Check stop/restart and the
C7 pedal/C8 reset/C9 baseline actions; optionally repeat with original Fluid.
No new test score or launch variable is needed.

Large-score resource behavior, the D5–D7 click artifact, optional master
effects enhancements and regular tie/slur engraving remain separate work.
