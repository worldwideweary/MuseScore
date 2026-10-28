# Audio v0.32: legacy output level

Apply after v0.31. The user accepts v0.31 live listening, WAV and mtests.
Portamento is now accepted at the score/sequencer boundary as well as in the
native synth checks. Bend-release and large-score/resource checks stay deferred.

## Confirmed gain difference

Legacy `Voice::update_param(GEN_PAN)` normalizes signed 16-bit samples with
1/32768 and no additional synth gain. The adapter initialized each modern
synth with `synth.gain=0.2`, before the shared master gain/boost. A comparison
using the same generated SF2, note, velocity and controllers measured legacy
RMS 0.110217 versus modern 0.0210677: ratio 0.191147, about -14.37 dB.

Each modern bank now initializes with gain 1.0, including reconstruction at a
different sample rate. Master gain/boost, Mixer settings, source-send fractions,
shared return settings, effects and saved XML are unchanged. The modern dry
signal and its source-send input both increase fivefold relative to v0.31
(about +13.98 dB). This restores the legacy gain scale; it does not calibrate
every SoundFont/instrument to identical RMS or peak values.

Normalization can hide a constant gain difference by scaling the result back
to 0.99 peak. Compare unnormalized output when checking this correction.
Ordinary master gain still controls mix headroom. No limiter or per-font
automatic gain has been added. Existing custom boosts used to compensate for
the quieter modern backend will still be present in saved settings.

## Regression coverage

`fluid2OutputLevelCompatibility` renders actual legacy Fluid and Fluid2 PCM
from an original SF2. It compares steady stereo RMS for single notes/chords,
velocities 64/127, logical channels 0/128 and isolated 255, in stereo/raw-group/
isolated modes. The same modern adapter is reconstructed through 44.1, 48 and
44.1 kHz. The test allows 10% RMS variation for different DSP/envelopes; it does
not compare samples for equality. In the focused check, all 84 comparisons
measured ratios 0.954586..0.955912, within about 0.4 dB of legacy.

The fixture builder gains an optional padded-loop variant for the old loader.
Default SF2/SF3 generation is unchanged; the default SF2 remains 88778 bytes,
SHA-256 `edf75c61c95b3444b517daf67350f6d7c16f54a4937fb0b66ccb62c2703f14e9`.

Patch-environment verification against FluidSynth2.3.4:

* The actual new test body passes with both real synth implementations and
  narrow host/UI bridges. Legacy SF3 decoding is disabled in that isolated
  comparison build; these level comparisons use SF2, not decoded SF3.
* Linking the same level test with v0.31's core fails at RMS ratio 0.191147,
  demonstrating the gain regression.
* Existing native banks, lifecycle, SF2/SF3 loading/rollback, font-stack,
  controllers, modulators, parameters and shared Zita-return checks pass at
  the restored gain.
* Previous actual adapter portamento/parameter/modulation/controller/font/XML
  and recovery bodies pass in all three modes using host/UI bridges.
* Full MIDI-test syntax passes with the backend enabled/disabled; production
  adapter/core/GUI syntax passes with real GUI headers. This is not a full
  application, CMake/CTest or end-to-end file-export run.

Rebuild normally, then run the usual mtests or:

```sh
cmake --build . --target tst_midi
ctest --test-dir mtest --output-on-failure -R '^tst_midi$'
```

## Final application check

Use existing A/B scores and familiar SF2/SF3 fonts. Compare legacy and modern
with the same font order, Mixer/master settings, NoEffect in both master slots
and zero sends first. Compare approximate level, including the simultaneous
parts in B; exact timbre/peaks need not match. Re-enable normal effects/sends
and confirm the usual mix remains clean at appropriate master gain.

Export B unnormalized at 44100 and 48000 Hz, then one normalized version.
Confirm normal pitch/speed, duration and tails at both rates, and no unexpected
distortion. Resume live playback and check the selected font/instrument and
current settings. WAV files can be inspected for rate, duration and peaks if
provided. These manual rate/cross-font checks are pending, not inferred from
the passing component tests or previous WAV acceptance.

After acceptance: preserve this compatibility checkpoint, clean up integration
and bundle the pinned FluidSynth source. Platform/Qt5.9.9 packaging remains a
separate verification step. Optional effects, Piano Roll automation and a
standalone LV extraction follow the agreed later stages.
