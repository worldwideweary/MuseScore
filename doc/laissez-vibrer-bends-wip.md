# 37E audio v0.08: original 3.x bends with LV

Apply after audio v0.07, retaining the previously installed notation backports.
This increment uses the existing note-owned `<Bend>` element. It does not
require importing or using Studio's newer GuitarBend notation.

## Verified baseline before this increment

The user built v0.07 and passed `tst_midi`. Ordinary/chord LV, the repeat
fixture, live playback, WAV export, and basic stop/seek passed listening checks.
The user's exported repeat MIDI was independently parsed: CC66-on at ticks
48, 1056, 3888 and 4896; CC66-off at 3840 and 7776, on Piano channel 1.
Individual note tuning had already passed live and WAV checks in v0.05.

## Changes

- A simple legacy bend with LV, including one bend within a normal tied chain
  ending in LV, uses an isolated pitch-wheel channel. Other notes cannot
  overwrite its wheel while it rings.
- NoteEvent attack/gate lengths are preserved. The original point time scale
  and wheel displacement are retained, sampled with exact segment endpoints.
  In the existing playback conversion, point pitch 100 produces a whole step
  with the 12-semitone wheel range; the PitchValue header's semitone comment
  does not describe that renderer conversion accurately.
- CC66 captures the isolated note. The wheel stays at the curve's final value
  through CC66-off and resets one tick afterward. The channel allocator checks
  both note gates and pedal intervals before allowing reuse, even when voices
  were collected out of chronological order.
- Bend channel setup explicitly emits the pitch-wheel range RPN, covering
  newly allocated channels in live playback and audio/MIDI export.
- The release uses v0.07's following-measure window and clips at repeat/jump
  boundaries. Unsupported legacy combinations skip generated LV rather than
  holding a note whose wheel will reset prematurely.

Legacy bends without LV keep their original playback path. The existing
backported GuitarBend renderer shares the channel allocator, so its later
notes also respect legacy LV reservations. LV generation for the newer
GuitarBend elements remains deferred in this increment.

## Tests and manual checks

`tst_midi` adds `laissezVibBends_data` and `laissezVibBends` with three scores:

- `mtest/libmscore/midi/laissez-vibrer-bends.mscx`: simultaneous rising and
  releasing bends, an unbent note, a second same-key bend during the first
  hold, and a later bend that can reuse a released channel.
- `mtest/libmscore/midi/laissez-vibrer-bends-repeat.mscx`: the same passage
  with a repeat, checking seven capture/release pairs.
- `mtest/libmscore/midi/laissez-vibrer-bend-tie.mscx`: one legacy bend tied
  normally to a note carrying LV, with one attack and one hold.

The tests check channel isolation/reuse, capture/release placement, final
wheel values, absence of wheel changes during a hold, delayed reset, and
explicit wheel-range setup. They export real MIDI files, read them back and
compare every CC66 timestamp/channel/value. Removing LV must preserve note
attacks/gates and remove all generated CC66 events.

```sh
ctest --output-on-failure -R '^tst_midi$'
```

Listen with the modern FluidSynth backend and the same font as earlier tests.
The opening C should rise a whole step and retain that pitch; E should bend
down to its written pitch and retain that pitch. G should remain unbent.
The second C should have its own rising attack while the earlier C rings.
Repeat with stop/seek during a hold, then compare WAV and MIDI export.
Re-run the v0.07 piano/repeat scores and the per-note tuning fixture.

## Remaining scope and validation limits

This is a WIP for simple legacy bends and continuous normal ties. Grace notes,
ornaments, multiple legacy bends in one chain, mixed legacy/new bend gestures,
and chains crossing playback-segment boundaries remain deferred. New
GuitarBend LV, manual CC66 composition, range loops, and pause/resume need
separate work. Sample release tails may remain audible after pedal release;
this stage reserves channels through the LV hold, not through an unbounded
SoundFont release tail. The old Fluid/Zerberus backends still lack CC66 support.

Preparation checks cover fixture XML/bar balance, patch whitespace, and a
standalone randomized check of 100,000 reservation windows using the production
channel scan, with an independent interval-overlap oracle. These checks passed.
Full MuseScore compilation, Qt tests, and listening require the user's build;
they were not run in the preparation environment. No reverb routing or
licensing changes are included.
