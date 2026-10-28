# Audio v0.45: saved SoundFont engine selection

Apply after v0.44. Preferences > I/O > Audio Engine now offers Original Fluid
and FluidSynth 2. Original Fluid remains the default. Changing the selection and
applying shows a restart reminder. Cancel does not apply the selection. Builds
without modern support retain the original engine and disable the selector.

The factory captures the choice once for the process so newly created export
synths continue using the same backend as playback until restart. This is not
live engine replacement. The existing Restart Audio and MIDI Devices button
restarts devices; it does not apply a pending SoundFont engine change.

FluidSynth 2 selected by the application defaults to isolated routing, providing
instrument sends without a launch variable. Direct/default-constructed adapters
retain their previous stereo default for compatibility with tests/other callers.
Explicit MSCORE_USE_FLUIDSYNTH2 (1 modern, other values original) overrides the
saved choice; explicit MSCORE_FLUID2_ISOLATED_CHANNELS overrides routing. Remove
both variables for normal preference-controlled use. The SoundFont path override
and raw-group diagnostics retain their previous meaning.

Verification: real adapter checks pass for automatic isolated routing, unchanged
direct construction and explicit stereo/isolated overrides, plus the previous
three-mode controller, portamento, seek, font management and sample-rate checks.
The exact factory selection block passes 12 subprocess cases for saved choice,
environment overrides, disabled-backend fallback and retention after preferences
change. MIDI mtest real-header syntax checks cover both backend configurations;
UI XML parses. Full application build/UI behavior and mtests remain user checks.

Check: remove launch overrides; select FluidSynth 2, Apply, verify the reminder,
restart, check fonts/playback/instrument sends and WAV/MP3. Change to Original
Fluid, check that playback/export remain modern until restart, then verify the
original engine after restart. Change back and check selection persistence and
Cancel. Retain the existing feature limits: old Fluid has no host instrument
sends or equivalent LV sostenuto playback; this patch does not add those.

Acceptance update: the user reports v0.44 mtests pass and audio playback seems
okay. MP3 exports tested at 44.1/48 kHz sound okay. MP3 metadata inspection was
inconclusive: sample rate is stream information, not necessarily an ID3 tag.
Do not record independently verified output sample-rate metadata from that report.
Large-score load time is accepted; the click-fifth remains deferred, and legacy
LV extraction remains indefinitely deferred. No layout optimization is included.
