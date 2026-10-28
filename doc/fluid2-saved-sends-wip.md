# 37E audio v0.17: saved source sends and mixer controls

Apply after audio v0.16. The user ran v0.16 and confirmed audible shared reverb
with both master Effect A/B slots set to NoEffect. That specific live independence
check is accepted. Broader WAV/MP3, normalization, stop/seek/repeat and large-score
startup checks remain pending/deferred; see `audio-verification.md`.

## Completed behavior

Mixer details now has an **Audio reverb send** slider and percentage box below
Pan, outside the MIDI controller group. Each source channel stores a separate
integer percentage from 0 to 100. The existing Reverb field in the MIDI section
continues to control MIDI CC91. The audio field creates no MIDI events and does
not change CC91. Part-level editing applies to that part's instrument channels,
including channels for instrument changes. Channel-level editing affects only
the selected channel.

Edits use the score undo stack, so they mark the score modified and support
undo/redo. Undo retains the saved instrument channel and resolves its current
playback copy on each use, so MIDI remapping cannot leave a dangling playback
pointer on the undo stack. The existing playback-to-instrument settings link copies the property
back into the saved channel. The channel's value is an atomic integer with value
copy semantics, allowing sequencer reads during mixer edits while cloned channels
remain independent. Atomic accessors use the older Qt API available to the 37E
baseline. Percentages are clamped on input. This setting does not invalidate or
rebuild note events, require MIDI rendering, or alter file-load rendering.

A nonzero percentage is written as `<audioReverbSend>37</audioReverbSend>` under
`Channel`. Zero is omitted. Missing values default to zero, including reading an
older channel into a reused object. Channel copies and normal MIDI remapping
retain the saved value. This custom audio field is kept even in builds using the
legacy backend; those builds cannot play the new return or edit its disabled
control. It is not a Studio 4 mixer parameter migration.

Live events, general audio/WAV export and the separate MP3 export loop apply the
saved percentage before dispatching each event. Initialization events apply it
as well, including explicit zero values, so reused slots cannot inherit another
score's send when initialized. Bend lanes always resolve the canonical parent's
current value, rather than a potentially stale copied channel value. Distinct
logical sources still share one return; they do not allocate one reverb each.

Mixer edits and undo/redo enqueue an internal sequencer message to update already
sounding sources at the next audio callback. That message reads the current
score/mapping/value when consumed, rather than retaining an old slider value or
Channel pointer. It sends no external MIDI and avoids taking the synth mutex
from the GUI for gain updates. The production gain is still a step change at a
render boundary; smoothing and slider-gesture undo grouping are separate work.

## Launch and migration from the temporary diagnostic

Keep the experimental build setting enabled, and launch with:

```sh
MSCORE_USE_FLUIDSYNTH2=1 \
MSCORE_FLUID2_ISOLATED_CHANNELS=1 /path/to/mscore
```

**Remove `MSCORE_FLUID2_REVERB_SEND` from Qt Creator's run environment.** It was
only v0.16's uniform-send listening aid. It is now ignored with a warning so it
cannot mask independent saved mixer values. Set the desired percentage in the
mixer. This stage intentionally does not migrate a process environment value
into every score's saved settings.

The isolated backend now prepares one shared wet Zita instance when the sample
rate is configured, even if all sends are zero. This permits the first mixer
edit or score event to enable it without allocation during rendering. The bus
skips the effect until a nonzero signal arrives. Older scores therefore start
dry through this return; their original master effects still operate normally.
Legacy Fluid, Zerberus and raw modulo-group mode have no send capability, and
the new control is disabled for them. The two-bank resource/voice-split caveats
from v0.14 still apply. Zita's room controls remain fixed defaults in this stage.

## Verification actually run

- Focused harness compiled the current production MasterSynthesizer dispatch,
  source-send methods, mixer undo command, queue method and queue-handler branch
  with narrow Qt/backend/score test doubles. Strict C++11 and ASan/UBSan passed
  with experimental compilation enabled and disabled. It checks gain-before-event
  ordering, optional legacy API behavior, capability/invalid source/index guards,
  canonical bend ownership, queued latest values, and undo/redo synchronization
  after replacing a playback mapping.
- Native FluidSynth 2.3.4 + production Zita/bus regression from v0.16 passed under
  ASan/UBSan: independent weighted source/bend stems, zero sends and cleared return
  history. The DSP/core did not change in this stage. System FluidSynth is not
  instrumented, and leak inspection is disabled for environment restrictions.
- Mixer UI XML and two-part score fixture parsed successfully; control names and
  percentage limits were checked. Source review covered all five live/export/
  initialization dispatch sites and existing reset/tail ordering.

These checks do not compile the whole Qt application or run the GUI. The new
`tst_midi::audioReverbChannels` test has been added but **not run here**, because
this environment has no Qt SDK or CMake executable. It tests channel XML round
trips, old-file defaults, bounds, independent clone state, init-list/CC91
independence, full two-instrument score save/reopen, playback-to-saved linkage,
byte-identical MIDI export, and deliberately stale bend-channel copies.

After building, run the new case directly or the full target:

```sh
./tst_midi audioReverbChannels
ctest --output-on-failure -R '^(tst_midi|tst_fluid2_.*|tst_auxiliary_.*)$'
```

## Application checks still pending

Open `mtest/libmscore/midi/audio-reverb-sends.mscx`. Set both master effect slots
to NoEffect. Use a 30% Piano send and 0% Flute send, soloing each to hear the
difference. Swap those values. Verify the new row fits the mixer and that MIDI
Reverb remains independent. Test part and individual-channel controls, held notes,
undo/redo, save/reopen, score switching and an instrument-change score. Confirm
that an old score with no field stays at zero.

Repeat live stop/seek/repeat and LV/original-bend checks, then export WAV with
normalization off/on (and MP3 if available), confirming the same independent
levels and tails. A source's existing return tail can continue after its send
is lowered to zero; that is shared reverb history, not a stuck voice. Export's
existing hard tail limit remains unchanged. Large-score startup/Piano Roll and
broader SF3/resource checks remain deferred as previously recorded.

## Next action and changed files

After application validation, expose and save the shared return's own controls
and check resource/startup cost. Keep master Effect A/B independent. SoundFont
management and broader SF3 compatibility remain separate tasks.

Changed files are the Synthesizer/Fluid2/MasterSynthesizer interfaces and adapter,
`libmscore/instrument.h/.cpp`, `libmscore/score.h` (saved-channel accessor),
`mscore/seq.h/.cpp`, both audio export loops,
MixerDetails header/source/UI and MixerTrackItem header/source, the new MIDI
regression case and two-instrument fixture, and the verification/checkpoint docs.
