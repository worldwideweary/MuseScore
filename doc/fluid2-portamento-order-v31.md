# Audio v0.31: portamento attack ordering and seek replay

Apply after v0.30. The user reports mtests pass for v0.29 and v0.30. Update
November 6: v0.31 live listening, WAV and mtests also pass, accepting D6/D7.
Use the existing D score; no score workaround or replacement is needed.

## Diagnosis from the supplied MIDI

`D-parameters-and-portamento.mid` is PPQ480, quarter=96. At tick 9600 the D6
C5 note-on precedes CC5=4, CC37=0, CC65=0 and CC84=60. At tick 11520 the D7 C5
note-on has no additional portamento setup. CC84 selects a previous key for
the next attack only. Sending it after D6 therefore arms the glide for D7.
Starting at D6 appeared to work because seek replay sent its setup first.
Starting at D7 replayed the earlier CC84 without the note that consumed it.

The v0.30 native test correctly exercised the synth with controllers before
note-on. Its score test only checked their tick positions, so it missed the
renderer ordering problem. This patch strengthens both integration boundaries.

## Changes

Named MIDI actions containing portamento commands are inserted before the
first attack on their channel at the same tick, retaining authored controller
order and originating-staff metadata. Program changes keep their existing
preceding-tick placement. Note-offs and timestamps remain in place, and
ordinary actions without portamento keep their existing insertion behavior.

Seek controller replay skips CC84. Its past attack has already consumed it;
setup at the destination is dispatched by the normal event stream. Persistent
portamento time/switch controls still replay normally. The duplicate-controller
pass also retains repeated explicit CC84 commands, even with the same key.
This does not add a general reconstruction of arbitrary MIDI voice histories.

The fixes are in the shared renderer/sequencer and affect legacy Fluid,
FluidSynth2, WAV and MIDI export. The synth glide calculation, 512 ms timing,
LV scheduling and the explicitly deferred bend-release behavior are unchanged.

## Verification

* `audioParameterScore` now checks the actual rendered D6 setup occurs before
  note-on in the order 5/37/65/84. A temporary score variation also checks two
  identical one-shot commands survive rendering; the supplied D7 reference
  score is unchanged.
* `fluid2PortamentoPlayback` renders actual adapter PCM from D5, D6 and D7,
  using production same-tick insertion and seek eligibility. It checks D6's
  beginning/middle/end glide and D7's straight onset/end, in stereo, raw groups
  and both isolated banks including logical 255.
* Focused patch-environment checks pass against real FluidSynth2.3.4, including
  the new PCM body and earlier parameter, modulation, controller, font/XML and
  recovery bodies with narrow host/UI bridges. Negative controls restoring
  either late setup or CC84 seek replay fail the corresponding audible check.
* The actual collector and duplicate-normalization source blocks pass a narrow
  host-bridge check for setup order, note-off/metadata preservation, unchanged
  ordinary actions and repeated CC84. Restoring the old duplicate guard fails.
* Full MIDI-test source syntax passes with the backend enabled and disabled;
  renderer source and the exact changed seek method pass syntax checks. Full
  application/Seq compilation is unavailable here because development/generated
  dependencies are incomplete. No full CMake/CTest, score rendering or listening
  was performed here; the full score tests require the user's build.

Rebuild the application normally, then run the usual mtests, or target:

```sh
cmake --build . --target tst_midi
ctest --test-dir mtest --output-on-failure -R '^tst_midi$'
```

Reopen the existing `doc/fixtures/audio-compatibility-v30/D-parameters-and-portamento.mscx`.
Play from the beginning, D5 and D6: D6 should glide C4 to C5, followed by a
straight C5 in D7. Start directly on D7, then stop/restart it: it should remain
straight. Check live playback and WAV. With RPN/controller export enabled,
the four D6 setup controllers should now precede its note-on at tick 9600;
the tick and note gates themselves should be unchanged.

Final export/output-level checks and source bundling follow acceptance of
this correction. Large-score/resource testing and bend-release remain deferred.
