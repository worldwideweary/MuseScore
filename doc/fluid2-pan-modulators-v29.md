# Audio v0.29: legacy pan range and SoundFont modulators

Apply after audio v0.28. Its mtests and live/WAV controller checks are accepted.
The bend-release artifact is explicitly deferred; this patch changes no bend
events, note gates, sequencer timing, SoundFont ordering or saved-state format.
Large-score/resource behavior, effects enhancements and Piano Roll automation
remain outside this checkpoint.

## Confirmed differences and corrections

Legacy `Fluid::alloc_voice()` replaces the default CC10-to-pan modulator amount
500 with 1000. An instrument sample may already have intrinsic pan of +/-500;
the larger range lets the opposite endpoint overcome that offset. Modern
FluidSynth's ordinary default of 500 can leave such a sample centered instead
of moving it fully to the opposite side. The adapter now installs amount 1000
with the public default-modulator API while constructing each stopped synth.

This is a default replacement, not a per-voice override after loading the
SoundFont. Legacy installs its default before instrument/preset modulators;
SoundFont-authored modulators are still allowed to replace it. In particular,
an authored amount of zero may intentionally disable CC10, and an authored
amount of 500 keeps its narrower range. The generator's intrinsic pan at CC10
center remains intact; a stereo instrument need not have equal left/right
energy when centered.

The new tests also caught a v0.28 interaction: its restored legacy CC8=0 default
met upstream's additional, non-SF2 CC8-to-GEN_CUSTOM_BALANCE modulator. Legacy
Fluid has no such default route; upstream's route attenuated the right output.
v0.29 zeros only that implicit modulator's amount, preserving CC8's legacy raw
value and authored SoundFont uses of CC8. Merely replacing CC8's default with
64 would hide the attenuation while changing authored controller behavior.

Both definitions are copied into each synth before SoundFonts/voices are
created, and are recreated on sample-rate replacement. Modulator allocation
does not occur in note-on or audio processing. Failure during setup rejects the
new engine using the existing recovery path. The system library source is
unchanged; the legacy backend and Zerberus are unchanged.

## Modulator checks

Generated original SF2 variants exercise intrinsic pan +/-500, authored pan
amounts zero/500, polyphonic and channel pressure mapped to pitch, and an
authored CC8-to-pitch route. They check held-note updates and CC121, including
logical 255 through the isolated layout. Source sends must retain their ratio
in both stereo outputs as pan changes.

The old ambiguous default velocity-to-filter mapping is disabled by legacy
Fluid. The new native check compares FluidSynth 2.3.4 with an explicit zero
replacement across velocities 40/80/127 near an audible filter cutoff; PCM
matches. No extra velocity/filter workaround or global filter disable is added.
Other authored filter, envelope, pitch and controller definitions remain in
the upstream loader; this test does not certify every possible SoundFont.

## User verification

Rebuild the application normally, then:

```sh
cmake --build . --target tst_midi tst_fluid2_modulators
ctest --test-dir mtest --output-on-failure -R '^(tst_midi|tst_fluid2_modulators)$'
```

Re-use `doc/fixtures/audio-compatibility-v27/A-controllers.mscx`. Use the same
ordered SoundFont list, NoEffect in both master slots and zero Audio reverb
sends in each backend. Listen to A4 fully left and A5 fully right: both should
remain audible on the corresponding side. Then test centered/intermediate pan
positions and changing Mixer pan while a note is held. Recheck one SF2 and one
SF3 if available, Save to Score/reopen and WAV output. A stereo instrument's
intrinsic image is retained when centered. No new comparison score is needed.

## Automated verification scope

`tst_fluid2_modulators` checks actual stereo PCM, the first output chunk and
held pan changes, authored default replacements, CC8/pressure behavior, reset,
source-send ratios and sample-rate reconstruction in stereo/raw-group/isolated
modes. `tst_midi::fluid2ModulationCompatibility` tests actual adapter CC10,
ME_AFTERTOUCH, ME_POLYAFTER, CTRL_PRESS and authored CC8 event paths, including
both isolated banks and source ownership.

The pre-v0.29 core fails the right-output check. A separate negative control
retaining the balance fix but reverting pan amount to 500 fails the
opposite-side check. Focused native/adapter checks do not replace the user's
full application/CMake/MTest, SoundFont-specific listening or platform builds.

Patch-environment verification passed against native FluidSynth 2.3.4:

* New modulator test in stereo, raw-group and isolated modes, plus ASan/UBSan
  on our code (prebuilt dependencies uninstrumented, leak checking disabled).
* Existing native banks, lifecycle, SF2/SF3 loading, Zita return, font-stack and
  controller tests, including CC8 remaining zero after initialization/reset.
* New actual adapter modulation-test body and previous controller, management
  and recovery bodies using narrow host/XML/UI bridges, in all three modes.
* Complete MIDI test source syntax with the backend enabled/disabled and actual
  adapter/core/GUI syntax using real GUI headers. This is not full app/MOC/CTest.
* The default generated SF2 remains byte-identical: 88778 bytes, SHA-256
  `edf75c61c95b3444b517daf67350f6d7c16f54a4937fb0b66ccb62c2703f14e9`.

Source references reviewed: legacy `Fluid::alloc_voice()` and instrument/preset
modulator application in `audio/midi/fluid/sfont.cpp`; FluidSynth 2.3.4
`src/synth/fluid_synth.c` default pan/balance definitions and
`fluid_synth_add_default_mod()` copy/overwrite behavior.

Next compatibility checkpoint: portamento/legato and remaining RPN/NRPN edge
cases, followed by final verification and source bundling. Bend-release stays
deferred: the old renderer centers wheel pitch while an outgoing bent note's
release tail is audible, briefly returning that tail to its original pitch.
The supplied `bend.wav` showed the C return around 1.6 seconds; a controlled
FluidSynth/strings comparison reproduced it with the reset and removed it by
retaining final pitch. A robust fix needs independent release-tail pitch and
safe channel reuse, not a one-tick timing adjustment.
