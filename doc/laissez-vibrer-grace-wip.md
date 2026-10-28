# 37E audio v0.09: grace-note LV playback

Apply after audio v0.08 with the existing notation backports retained.

## Accepted baseline

The user passed v0.08's MIDI tests and listening/WAV checks, then confirmed
no stuck notes during stop/seek and no problems through the repeat fixture.
Those checks supplement the previously accepted ordinary-note LV and tuning.

## Changes

The ordinary LV event pass now handles simple, separately emitted grace notes.
It uses each grace note's actual MIDI attack/gate for capture timing and the
owning measure for the following-measure release window. Before-note graces,
after-note graces, and marked notes within a grace group use the existing
grace playback timing; no note attacks or gates are extended or rescheduled.
Simultaneous marked grace chord notes coalesce into one channel capture.

Rendering-chunk boundaries now inspect LV attached to grace chords as well
as principal notes. Otherwise a small live rendering chunk could truncate a
grace hold even when a full-score render or WAV export was correct.

The CC66 mechanism remains channel-wide: an after-grace capture can also hold
other notes still active on that channel. This increment adds no per-note or
per-string isolation. Notes with fewer than three emitted ticks are skipped
because no interior capture tick exists before note-off.

Tied grace notes, grace bends, grace glissandi, and graces absorbed into an
ornament remain deferred. They need separate event-ownership/channel work;
the new pass does not infer marks from an ornament's principal note.

## Fixtures and validation

Open either fixture under `mtest/libmscore/midi/`:

- `laissez-vibrer-grace.mscx`
- `laissez-vibrer-grace-repeat.mscx`

Both have seven measures at quarter = 120. The marked gestures are a before
grace C/E chord, an after grace D/F chord, the second grace in a two-note
group, and an after grace A in the final measure. Expect the marked grace
notes to continue ringing after their normal gates through the following
measure; the last hold releases at score end. Try piano and a sustained
instrument, then compare live playback and WAV export. Stop and seek while
a grace note is held, and listen through the repeat fixture for stuck notes.

`tst_midi` adds `laissezVibGrace_data` and `laissezVibGrace`. It checks exact
capture/release ticks, capture ownership within marked grace gates, unchanged
note attacks/gates after removing LV, full-score versus short-chunk rendering,
and real MIDI export/readback of CC66 timestamps/channels/values.

```sh
ctest --output-on-failure -R '^tst_midi$'
```

Also rerun the earlier piano/repeat, legacy-bend, and individual-tuning scores.
The old Fluid and Zerberus backends still do not implement LV's CC66 hold.

The preparation environment has no Qt development installation, so full
MuseScore compilation, Qt tests, and listening are pending the user's build.
The standalone harness compiles the production LV event pass with minimal
score/event stubs; it does not replace integration testing of score loading,
grace event generation, MIDI export, or live rendering.

That harness passed with C++11 and warnings treated as errors. It exercised
the fixtures' grace/repeat event schedules, grace attachment detection, 100
short gate lengths, and exclusion of tied/bent/glissando grace combinations.
Fixture XML, seven-measure duration balance and patch whitespace checks also
passed.

## Next scope

Audio v0.10 adds ordinary tie endpoints across playback-segment boundaries;
see `laissez-vibrer-repeat-ties-wip.md`. Newer GuitarBend LV and grace tie/bend
combinations remain deferred. Reverb routing remains a separate stage.
