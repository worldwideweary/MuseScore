# 37E audio v0.20: shared return controls and persistence

Apply after audio v0.19. The user reports full tst_midi and tst_master_layout
passed (2.17 s and 0.00 s, respectively), clearing the startup/test blocker. The
existing mixer row fix is accepted. Previously deferred live/audio-export and
large-score startup checks remain pending; passing MIDI tests do not replace them.

## Completed

The Synthesizer window now has an **Instrument Reverb** tab, appended after the
existing tabs so previous tab indices stay valid. It has two form-layout columns:
Room, and Tone and return. The control is separate from the master Zita instances
and the per-instrument Audio reverb send percentages in Mixer.

| Control | Range | Default |
| --- | --- | --- |
| Pre-delay | 20–100 ms | 40 ms |
| Crossover | 50–1000 Hz | 200 Hz |
| Low decay (RT60) | 1–8 seconds | 1.4 seconds |
| Mid decay (RT60) | 1–8 seconds | 2 seconds |
| HF damping | 1500–24000 Hz | 3000 Hz |
| Low EQ frequency / gain | 40–2500 Hz / -15 to +15 dB | 160 Hz / 0 dB |
| High EQ frequency / gain | 160–10000 Hz / -15 to +15 dB | 2500 Hz / 0 dB |
| Return level | 0–100% | 100% |

These defaults match the actual v0.19 auxiliary Zita initialization. The unit
stays 100% wet; Return level scales only the return samples before they are
added to dry audio. Master A/B and master gain still operate after that sum.
A zero return level preserves dry audio and mutes the shared return while its
internal tail continues to decay. Reset shared reverb restores these controls
and requests an allocation-free history clear. Zita's normal sample-rate damping
clamp remains in effect when a requested damping frequency is above its limit.
No new reverb library, license change or new environment option is involved.

## Save/load behavior

Use the Synthesizer window's existing **Save to Score** to store changes in the
score, and save the score file normally. Load from Score restores them. Set as
Default / Load Default also include the shared return settings. GUI changes mark
the synthesizer controls dirty, following the existing master-effect workflow.
This does not change per-instrument sends or MIDI CC91.

The new Synthesizer group is named `AuxiliaryReverb`, distinct from `Zita1`.
Its stable IDs 0–8 are physical room/EQ values in the table order; ID 9 is return
level from 0 to 1. Nine significant digits preserve stored float values. Old
states with no group reset to factory defaults rather than leaking another
score's room settings. Unknown IDs and malformed/nonfinite values are ignored;
finite values outside bounds are clamped. The wet/dry mix is never read from
this group or exposed for editing.

Room settings are retained and round-trippable in nonexperimental/unsupported
backend builds too. The tab shows them disabled when isolated FluidSynth 2 is
unavailable. Keep MSCORE_USE_FLUIDSYNTH2=1 and MSCORE_FLUID2_ISOLATED_CHANNELS=1
for playback. The retired MSCORE_FLUID2_REVERB_SEND setting remains removed.

## Threading and object layout

GUI/state writers update a small mutex-protected value snapshot. The render
thread uses try_lock to consume a complete changed snapshot, then releases the
lock before touching DSP. If a writer is busy, rendering uses the previous DSP
parameters and retries on the next block. The new handoff never waits for a GUI
writer or skips synth rendering. Only rendering (or initialization before it)
sets the running auxiliary Zita parameters. Source-send gains retain their v0.17
sequencer-queue path.

The values live behind MasterSynthesizer's unconditional private pointer. Public
class layout remains identical across backend defines. The small settings state
now exists in all builds so unsupported backends preserve score data; DSP/buffers
remain conditional inside the implementation. Sample-rate reinitialization
requests the current controls for the replacement reverb before its first block.
There is no new allocation in parameter consumption or rendering. Return level
changes are block-boundary steps; extra smoothing remains separate work.

## Verification actually run

- New standalone tst_auxiliary_settings: bounds/nonfinite/unknown-ID handling,
  default/reset/reapply handoff, and 10000 alternating complete batches exchanged
  between a writer and consumer thread without observing a torn snapshot.
- Production Zita + settings/bus: new default controls yield bit-identical impulse
  PCM to the v0.19 defaults; edited room/EQ values change the tail; output is
  finite and remains wet-only; return level zero leaves existing dry samples
  bit-exact. These tests passed under AddressSanitizer/UndefinedBehaviorSanitizer.
- The public-header mixed-define layout regression still passes. Focused production
  constructor/destructor/reset/parameter API checks pass 100 cycles under ASan/UBSan
  with allocator and implementation compiled using opposite backend defines, in
  both directions, including settings access without an active backend.
- Source review covered score/default serialization, live and general WAV/MP3
  host paths and the two form-layout columns. Whitespace checks passed.

The Zita test uses the existing narrow Qt bridge; mixed-define checks use Qt/effect
doubles. System/full Qt tests are not claimed. Leak inspection remains disabled
for environment restrictions; these were not ThreadSanitizer runs. This environment
still has no Qt SDK or CMake executable for a full application build.

Added but not run here: tst_midi::audioReverbReturnState verifies distinct state
namespaces, float round trips, reset/missing/invalid-value behavior, preservation
without an active backend, and saving/reopening a real score with custom return
settings. The user's v0.19 MIDI success predates this new case.

From the configured build directory, build and run the relevant targets:

```sh
cmake --build . --target tst_midi tst_master_layout tst_auxiliary_settings
ctest --output-on-failure -R '^(tst_midi|tst_master_layout|tst_auxiliary_settings)$'
```

## Pending application checks and next action

Open Instrument Reverb with the isolated backend enabled. Check the two columns
fit, values/units are correct, and Reset shared reverb restores the defaults.
With master A/B set to NoEffect, use the existing two-instrument fixture and
independent mixer sends. Compare short versus long decay, pre-delay/EQ, and 0/100%
return level. Save to Score, save/reopen, and Load from Score; also verify the
default buttons. Confirm the master effects' own controls stay independent.
Export WAV (normalization off/on), check live stop/seek/repeats/LV/bends with the
edited return, and keep earlier deferred startup/large-score checks recorded.

Next after these controls are verified: measure large-score/resource behavior
and broader SF3/font compatibility; decide on further smoothing or SoundFont
management work using those results. No newer DSP-library import is part of
this stage.

Changed files: audio/midi/auxiliaryreverbsettings.h, msynthesizer.h/.cpp and
midi.cmake; SynthControl header/source; audio test registration/new settings test;
the new tst_midi return-state case; audio-verification.md and this checkpoint.

## v0.21: isolate the reported return-state failure

Apply this diagnostic follow-up after v0.20. The user's three-target run passes
tst_auxiliary_settings and tst_master_layout. The attached Qt 5.15.2 XML records
one failing MIDI case, audioReverbReturnState; all other MIDI cases pass. Its
array-equality message appears at both the XML-state load and full-score load,
so the result does not uniquely locate the failing stage or parameter.

The expanded test retains exact float equality and adds checks for the original
serialized strings, an in-memory setState before XML, parsed group name/count
and each parsed ID/string. Numeric comparisons report the stage, parameter ID,
expected value and actual value with nine significant digits. The full-score
comparison has its own stage label. No playback or persistence implementation
is changed, and this patch does not claim a production fix.

Locally, staged Qt 5.15.13 Core headers and the installed runtime were used for
focused float conversion and XML checks. All ten expected values round-trip
exactly through QString::number/toDouble and the production SynthesizerState
read/write methods with narrow XML reader/writer adapters. This is not a full
MuseScore build or the user's Qt 5.15.2 setup. Diagnostic formatting was checked
against matching values and deliberately changed values. Full tst_midi remains
pending on the user's build.

From the build directory, rebuild tst_midi and run the single case to stdout:

```sh
cmake --build . --target tst_midi
./mtest/libmscore/midi/tst_midi audioReverbReturnState -v2
```

Next action: use that failure's stage and values to make a focused correction,
then rerun the three-target CTest selection before accepting persistence. Keep
all previously deferred audio, UI and large-score checks pending.

Changed files: mtest/libmscore/midi/tst_midi.cpp, audio-verification.md and this
checkpoint. This stage is independently recoverable and changes tests/docs only.

## v0.22: enable synthesizer state during the score-save test

Apply after v0.21. The user's verbose Qt 5.15.2 output confirms that setters,
in-memory state, standalone XML, reset, old-state defaults and malformed-state
handling all pass. Only reopening the saved score restores factory defaults.

The cause is in the existing Score::write path (libmscore/scorefile.cpp): it
calls _synthesizerState.write only for a top score with MScore::testMode false.
MTest enables test mode globally, so the score-save part of this new test never
wrote its synthesizer state. This was a test setup error, not evidence of a
numeric conversion failure or a loss of settings in normal application saves.

The test now uses QScopedValueRollback<bool> to disable test mode only around
saveScore. The flag is restored before reopening the score, including when
QVERIFY returns early. It also checks that the saved file actually contains
AuxiliaryReverb and that reopening yields all ten parameter entries before
comparing the values exactly. Playback, DSP and production save/load code are
unchanged. The v0.21 diagnostic comparisons remain useful on future failures.

A focused Qt 5.15.13 Core check reproduced the existing suppression, then
verified writing and reading all ten values with the scoped override. It also
checked flag restoration on normal exit and an early return. This uses the
production SynthesizerState methods with narrow XML adapters and the existing
score-save guard; it is not a full score/application build. The full Qt 5.15.2
tst_midi must still be rerun by the user.

```sh
cmake --build . --target tst_midi
ctest --output-on-failure -R '^(tst_midi|tst_master_layout|tst_auxiliary_settings)$'
```

Changed files: mtest/libmscore/midi/tst_midi.cpp, audio-verification.md and this
checkpoint. Next action: confirm the three tests pass, then verify the new tab,
Save to Score/reopen/default buttons and edited live/WAV return settings. Earlier
deferred audio and large-score checks remain pending.

## Acceptance checkpoint: October 30, 2026

After applying v0.22, the user reports all three tests pass: tst_midi,
tst_auxiliary_settings and tst_master_layout. This clears the return-state test
blocker and verifies the automated score save/reopen case. No further code
change is required for that failure.

The user also verifies Save to Score and reload, with the settings honored after
reopening the score. Live playback and WAV export use current reverb edits
without requiring Save to Score. The supplied application screenshot shows the
Instrument Reverb tab fitting its two columns, labels and buttons without overlap.

Reset shared reverb and Set as Default / Load Default have not been separately
reported as tested. Broader return regressions (including normalized export and
stop/seek/repeats with edited room settings) remain pending unless already
explicitly accepted for the earlier stages. Large-score/startup checks are still
explicitly deferred. Next verification: reset/default workflows and independent
two-instrument sends; then broader SF3/font compatibility and resource work.
