# 37E audio v0.24: sample-rate and SoundFont reload recovery

Update: the user reports v0.24 application build and mtests pass. v0.25 extends
this recovery to an ordered font list and adds management controls; see
`fluid2-font-management-wip.md`.

Update v0.30: generator offsets and RPN/NRPN selection/data LSB are now retained
across reconstruction; see `fluid2-parameters-v30.md`. Data-entry writes are
still not replayed, and this does not serialize alternate MIDI tuning tables,
mono/legato histories or arbitrary manually programmed channel state.

Apply after audio v0.23, including a copy rebased onto the current notation
branch. This increment changes the Fluid2 adapter/core and focused tests; it
does not change tie resolution, LV event scheduling or engraving. User listening
and reverb acceptance, with large-score behavior still deferred, is recorded in
`audio-verification.md`.

## Behavior

Changing the host sample rate used to destroy the active engine and patch list
before trying to reload the remembered font. A missing/unusable font could
therefore leave the adapter empty; another initialization at that same requested
rate returned early without retrying.

The replacement is now staged with its SoundFont, master tuning, selected
presets, ordinary controller values, pitch-wheel range and source-send gains.
Only a fully prepared engine is committed. Each bank must succeed. Existing
explicit font replacement keeps its v0.23 transactional behavior.

Successful rate changes are stopped-time operations: old voices and cached
dry/send PCM are discarded, pedal captures and pitch bend return to neutral,
and lane ownership is rebuilt by subsequent events. Ordinary mix/modulation
controls are retained. RPN/NRPN commands, data increments and portamento's
previous-note controller are not replayed; this is not a complete serialization
of arbitrary manually programmed MIDI channel state. Existing single/raw-synth
channel-255 limitations in FluidSynth 2.3.x remain: preset/send state is retained,
but controller restoration avoids that unsafe physical slot. Isolated mode
restores all 256 logical channels, including 255 mapped to local channel 127.

On failure the accepted engine, font path, patch objects and send settings are
retained for recovery. Its voices are stopped and drained. The host-facing sample
rate records the request, but the adapter contributes no dry/send audio and
ignores incoming performance events while its retained engine has the wrong rate.
This avoids wrong-speed/pitch playback or resuming an interrupted LV hold.
Other synths' additive output is left untouched. A diagnostic explains the mute.
The existing void `Synthesizer::init` interface has no initialization-error return;
this stage does not introduce an application-wide error dialog or export error
propagation.

Recovery paths:

- Restore the file and initialize again at the pending sample rate. Failed
  initialization no longer takes the normal same-rate early return.
- Explicitly load a usable font: it completes the pending rate change and
  updates accepted path/patch metadata only on success.
- Return to the retained engine's rate, even if the original file is missing.
- Explicitly unload with an empty font list, then load a valid font.

Ordinary initialization at the already working rate remains a no-op and does
not reopen files or disturb voices. Positive but unsupported engine rates fail
through the staged path; nonfinite/nonpositive requests are ignored as before.
Recovery honors the existing SoundFont environment override.

Staging temporarily needs both engine instances and both sets of font objects.
FluidSynth controls sample-cache sharing. This is not an allocation-free sample
rate switch and does not establish large-font/large-score memory acceptance.
The deprecated FluidSynth sample-rate setter is deliberately not used: upstream
2.3.4 documents its deferred failure/reporting problems in `fluid_synth.c`.

## Regression coverage and verification

New native target `tst_fluid2_lifecycle` uses original minimal SF2/SF3 fixtures:

- Stereo, raw 1/16-group and isolated two-bank configurations.
- 44.1/48/22.05/96 kHz changes; melodic/drum presets, master/per-note tuning,
  wheel range, volume/pan/expression, source gains and isolated channel 255.
- Missing and no-preset fonts, invalid rates, and Linux-injected first/second
  bank load failures, unusable fonts and synth allocation failures.
- Failed core replacement retains held LV voices, captures, routing and matching
  dry/send PCM. The adapter separately stops these voices at a failed host-rate
  transition. Successful replacement emits no old voice/cache audio; newly
  attacked audio has the expected pitch and send gain.
- Explicit unload/reload and successful empty-state reinitialization.

The experimental build adds `tst_midi::fluid2SampleRateRecovery`, with stereo,
raw-group and isolated-bank rows using the real adapter. It moves the active
font, checks retained metadata and muted output, verifies that interrupted LV
and blocked new attacks cannot resume, and exercises each recovery path. Run
environment overrides are scoped and restored, including on an early failure.

Verification actually run in the patch environment:

- The new native lifecycle test passes against FluidSynth 2.3.4, including Linux
  bank-load/empty-font/allocation failure injection. It also passes ASan/UBSan on
  our code; leak inspection is disabled by the environment, and the prebuilt
  FluidSynth/runtime dependencies are not sanitizer-instrumented.
- Existing native banks, SoundFont and shared-return regressions were rebuilt
  against the changed core and pass.
- A focused Qt 5.15.13 check compiles the real adapter and executes the actual
  new Qt regression method body with direct assertion wrappers in all three
  modes. A separate real-adapter check measures the recovered PCM's frequency.
  Only the GUI constructor and installed-font lookup are bridged; this is not
  a full GUI/application test. The same regression fails against v0.23 because
  a failed rate change loses the accepted font state.
- The complete updated `tst_midi.cpp` passes a syntax check with the experimental
  backend enabled. Strict C++11 native warnings and whitespace checks pass.

Full application/CMake/Qt test execution is still required on the user's build;
these focused checks do not substitute for that run.

From the main configured experimental build directory:

```sh
cmake --build . --target tst_fluid2_lifecycle tst_fluid2_soundfonts tst_fluid2_banks tst_fluid2_return tst_midi
ctest --test-dir mtest --output-on-failure -R '^(tst_fluid2_lifecycle|tst_fluid2_soundfonts|tst_fluid2_banks|tst_fluid2_return|tst_midi)$'
```

For an application smoke check, use the same font at 44.1 and 48 kHz playback/
export, retaining tuning and independent sends. Listen for correct pitch/rate
and absence of stale notes after reinitialization. Missing-file recovery is
covered by the automated adapter case; the font management GUI remains the next
stage, rather than a prerequisite for exercising these tests. Large-score and
resource measurements remain explicitly deferred. EQ, stereo widening, master
effect expansion and original-synth LV extraction remain later work.
