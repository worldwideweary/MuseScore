# Audio v0.30: parameter controls and portamento compatibility

Update November 6: the user reports v0.29/v0.30 mtests pass. D6/D7 listening
and the supplied MIDI exposed same-tick action ordering and seek replay errors
outside the synth-level check. v0.31 corrects these; see
`fluid2-portamento-order-v31.md`. The original tests checked controller ticks,
but did not require setup to precede note-on or exercise seek replay.

Apply after v0.29. Its A4/A5 left/right listening is accepted; its application
mtests were still building when this checkpoint began. Do not record them as
passed without a result. The bend-release artifact and large-score/resource
checks remain explicitly deferred. No notation or LV scheduling changes here.

## Corrections

Legacy `Channel::setcc()` retains the selected SoundFont NRPN generator after
CC6 data entry. FluidSynth 2.3.4 clears its internal generator index after each
write. Selecting generator 51 (coarse tuning) once and sending offsets +12,
0, +7 therefore left the modern channel at +12 after the first write; later
data writes targeted generator zero instead.

The core now retains a bounded selection per logical channel and reselects it
through public CC calls before each SF2 NRPN data write. Upstream still supplies
scaling and held-voice updates. Incomplete or invalid indices are ignored;
long malformed selector streams cannot overflow either accumulator or silently
write generator zero. No allocation is added to controller dispatch. The
adapter routes ordinary CCs through this method. CC121 clears the selection.
Other manufacturers' NRPNs and ordinary RPN handling stay with upstream.

Stopped-time sample-rate replacement now copies effective generator offsets,
data-entry LSB and parameter selection without replaying a CC6 write. This
retains NRPN offsets, RPN fine/coarse tuning, and subsequent select-once edits.
Failure leaves the old engine/state intact. Rebuilds still discard voices,
pedal captures, wheel position and source ownership; they do not serialize
alternate MIDI tuning tables or mono/legato voice histories. Saved-state XML
is unchanged. CC121 clears channel offsets for future notes; like the old
engine, it need not remove NRPN offsets already stored in a sounding voice.

## Portamento/legato audit

Legacy explicit CC84 portamento uses CC5*128+CC37 milliseconds. Real modern
PCM matches the tested 512 ms C4-to-C5 glide with CC65 off or on. CC84 is consumed
by one attack; the next ordinary attack returns directly to its written pitch.
No portamento timing workaround is needed for this path. Continuous score
glissandi still use the common renderer's pitch-wheel events, unchanged here.

The old channel handler has no dedicated CC68 legato implementation. Modern
legato/mono behavior remains upstream; this patch does not opt score channels
into mono mode or claim equivalence for every manually programmed channel-mode
sequence. Modern RPN bend range, fine/coarse tuning and null selection remain
available, although legacy CC6 did not implement those ordinary RPN branches.

## Verification

Added `tst_fluid2_parameters` checks actual held-note PCM, repeated NRPN writes,
wrong-generator/channel isolation, RPN range/fine/coarse/null selection,
unrelated/incomplete/invalid NRPNs, reset, both isolated banks including logical
255, 44.1/48 kHz replacements, failed reload and explicit portamento timing.
`tst_midi::fluid2ParameterCompatibility` exercises actual adapter events, held
notes, new attacks after reset, reconstruction and source-send ratios.
`audioParameterScore` renders the new score and checks its attacks/controllers.

Patch-environment verification against real FluidSynth 2.3.4:

* New native parameter PCM check in stereo, raw groups and isolated modes,
  including ASan/UBSan on our code (prebuilt dependencies uninstrumented;
  dependency leak checking disabled).
* Existing native banks, lifecycle, SF2/SF3, font-stack, controller, modulation
  and Zita-return checks.
* New actual adapter test body and earlier modulation, controller, font
  management/XML and recovery bodies, using narrow host/UI bridges, all modes.
* Negative controls fail for the original NRPN behavior and omitted generator
  copying, respectively. Full MIDI-source syntax passes enabled and disabled;
  actual adapter/core/GUI syntax passes with the real GUI headers.
* Listening-score XML/action/duration checks. No full application, MOC, CMake,
  CTest, score rendering or listening was performed in this environment.

From the configured main build directory:

```sh
cmake --build . --target tst_midi tst_fluid2_parameters
ctest --test-dir mtest --output-on-failure -R '^(tst_midi|tst_fluid2_parameters)$'
```

Open `doc/fixtures/audio-compatibility-v30/D-parameters-and-portamento.mscx` in
each backend with the same font, dry master effects and zero reverb sends.
D1-D5 are written A4: hear A4, A5, A4, E5, A4. D6 glides C4 to C5 in 512 ms;
D7 is straight C5. Check live playback and WAV. Repeat with exports at 44.1 and
48 kHz, then resume live playback; confirm pitch and the selected font remain
correct. Controller setup for each passage assumes playback from the beginning;
this patch does not add seek-time restoration of arbitrary text MIDI actions.

Next: final cross-font/output-level and sample-rate/export smoke acceptance,
then cleanup and source bundling. Optional effects and Piano Roll automation
remain later enhancements. Legacy Fluid and Zerberus production code are
unchanged.
