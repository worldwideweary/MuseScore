# Audio v0.28: controller defaults, reset and bank sequencing

Apply after audio v0.27. The user accepts its mtests, both comparison scores
and lower-slot SoundFont selection after reload. Large-score/resource checks
remain explicitly deferred. Tick-based Piano Roll CC automation is a later
enhancement; no automation editor is added in this compatibility checkpoint.

Accepted November 5: the user reports mtests pass, the ordinary bend reaches D,
the remaining controller checks work and WAV agrees with live playback.
v0.29 corrects the interaction of the restored legacy CC8=0 default with
upstream's nonstandard stereo-balance modulator; see `fluid2-pan-modulators-v29.md`.
The pre-existing bend-release artifact heard in 3.6.2 is separately deferred.

## Changes

Legacy `FluidS::Channel::initCtrl()` establishes a +/-12-semitone wheel range,
centered wheel, channel pressure zero, polyphonic key pressure 80, volume 127/0,
pan 64/64 and expression 127/127 (MSB/LSB). Most remaining CCs start at zero.
The new backend previously relied on upstream defaults, including a +/-2
semitone wheel range. Explicit RPN 12 on LV bend lanes masked that difference
for those lanes; ordinary bend events could still use the wrong range.

The bank core now establishes the legacy defaults at initialization and when
the adapter receives Reset All Controllers (CC121). Upstream CC121 first clears
generator/RPN state and releases hold pedals, then explicit host defaults are
restored. Reset keeps the selected preset, requested host bank/program,
master tuning, source-send gain and release-tail ownership. Explicit RPN range
changes remain effective until a later CC121; they are not overridden per note.
The existing unsupported physical channel-255 modulation in single-synth mode
is not exercised; isolated mode maps logical 255 to local 127 and restores all
defaults safely.

Reset does not replay data-entry/increment commands or portamento's prior-note
controller. Upstream's null RPN/NRPN selectors are retained rather than forcing
legacy's raw zero selector bytes, which would accidentally select RPN zero in
the modern library. This preserves explicit RPN support without spurious
parameter changes after reset. Modern CC121 also releases sostenuto/sustain
captures; the old C++ implementation can leave already sustained voices held
until a later release/stop. The modern backend intentionally retains upstream's
correct pedal-release behavior.

Bank-select MSB now clears pending LSB bits as legacy `Channel::setcc()` does.
A separate pending MSB is needed because legacy CC121 resets that MSB to zero
while retaining the selected bank. Thus Program Change immediately after reset
still uses the prior selected bank; a subsequent LSB-only request starts from
MSB zero. These host bank numbers still use v0.27's per-font offset mapping.

No system FluidSynth source, notation timing, reverb algorithm or audio driver
is changed. Existing A/B scores, LV and saved-send tests remain relevant.

## Listening check

Open `doc/fixtures/audio-compatibility-v28/C-reset-and-bend.mscx` with the same
SoundFont in each backend, effects set to NoEffect, sends at zero and tuning 440.

* C2's ordinary bent C should rise a whole tone to the following D; C3 is a
  separate D reference. C4 returns to ordinary C.
* C5 is quiet and fully right. C6's CC121 restores centered pan and full
  volume/expression, with the strings program still selected.
* C7 uses piano with sustain. C8 resets controllers: the modern engine releases
  the pedal captures and keeps the piano program. Legacy may retain some old
  sustained voices until transport stops, as described above.
* C9 restores the earlier strings baseline; C10 is a rest. Check live/WAV,
  stopping during sustain, starting at a later measure, and replay from the start.

## Automated checks and limits

`tst_fluid2_controllers` renders original sine fixtures in stereo, raw-group and
isolated modes. It checks bend PCM before initialization events, explicit RPN
changes, reset state and native preset identity, generator clearing, another
channel's independence, poly-pressure PCM at 0/80/127, and LV release tails with
unchanged send ownership/gain. The test-only font helper adds optional bank,
pitch and pressure-modulator variants; its existing default output is unchanged.

`tst_midi::fluid2ControllerCompatibility` sends actual adapter events and checks
the resulting PCM in both synth banks: default/reset bend range, RPN, MSB-only
selection, retained lower-font selection after CC121, and LSB-only selection
after reset. `audioControllerResetScore` checks the new score's rendered attack
count, bend events and CC121 timing with either backend build configuration.

```sh
cmake --build . --target tst_midi tst_fluid2_controllers
ctest --test-dir mtest --output-on-failure -R '^(tst_midi|tst_fluid2_controllers)$'
```

Full application/CTest and listening remain for the user's build. Focused
patch-environment checks passed against FluidSynth 2.3.4:

* New native controller checks in all three modes, also under ASan/UBSan on
  our code (prebuilt runtime libraries uninstrumented, leak checking disabled).
  Reset preserves a held key's per-note/master tuning and another channel's CCs.
* All existing native banks, lifecycle, SF2/SF3 loading, shared Zita return and
  font-stack tests. The default generated SF2 remains byte-identical (88778 bytes,
  SHA-256 `edf75c61c95b3444b517daf67350f6d7c16f54a4937fb0b66ccb62c2703f14e9`).
* New actual adapter controller-test method body plus earlier management and
  recovery bodies using narrow host/XML/UI bridges, three modes. Replacing the
  adapter with its v0.27 version fails the new audible controller regression.
* Complete MIDI test source syntax with the backend enabled/disabled; actual
  adapter/core/GUI syntax using real GUI headers.
* New score XML/action references, complete measure durations and 13 written
  attacks checked structurally. Its score-render test and application engraving
  still require the user's mtest/application build.

These checks do not substitute for complete score rendering or full app execution.

Remaining compatibility work includes the old forced-pan modulator, broader
channel/key pressure and SoundFont-modulator behavior, and portamento/legato
and NRPN edge cases. Passing this checkpoint does not certify those paths or
large-score performance. Source bundling follows compatibility verification;
EQ, stereo width, additional master slots and Piano Roll automation stay later.

References checked: legacy `audio/midi/fluid/chan.cpp`; upstream FluidSynth
2.3.4 `src/synth/fluid_synth.c` CC121 and RPN handling and the public channel
message APIs. No upstream internal structures are accessed by the adapter.
