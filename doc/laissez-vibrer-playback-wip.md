# 37E audio v0.07: LV playback WIP

Apply after audio v0.06 on a tree that already contains the verified LV
notation backport. The notation baseline restored in the preparation checkout
is integration context only; this incremental patch does not repeat it.
Individual note tuning was verified by the user in playback and WAV export
before starting this increment.

## Scope

The common MIDI renderer now emits CC66 sostenuto for LV on ordinary notes and
normal tied-chain endpoints. Note-on/off timing remains unchanged; the pedal
stream controls the additional ringing. Modern FluidSynth and compatible
external MIDI instruments can respond. The old Fluid and Zerberus backends
still play the original note duration: their CC66 support is not implemented.

Requests on one MIDI channel keep each distinct capture time, coalesce marks
on simultaneous chord notes, and release only after the longest overlapping
request. This preserves repeated pedal-on messages that capture later notes.
CC66 is excluded from generic controller deduplication and automatic copying
to independent bend lanes. It is a channel-wide hold, not per-note isolation:
unmarked notes sounding on that channel at capture time may also be held.

Capture follows 10 percent of the emitted attack's duration, before its normal
note-off. The release reaches the end of the LV owner's following written
measure plus that offset, clipped at the current playback segment's end.
The final measure releases at score end. Timing is computed in ticks in this
WIP; tempo changes within the note may differ slightly from 4.7's time-based
calculation. Capture/release values are runtime MIDI data and do not alter LV
layout, minimum drawn length or the notation's saved properties.

Continuous rendering chunks do not break immediately at or after an LV
measure, so the release window remains available to the MIDI pass. Repeat/jump
segments can still split there: pending holds are explicitly released at the
discontinuity and the next pass starts with fresh captures. This conservative
boundary behavior needs comparison with 4.7, especially ties spanning voltas.

The sequencer's stop path now sends CC66-off alongside CC64-off. The modern
adapter's allNotesOff clears both pedals before releasing notes, covering the
same backend call used during stopping/seeking. Seeking does not recreate
notes attacked before the destination; controller replay may restore pedal
state, but it does not restart those historical notes. Verify live behavior
and external MIDI as well as the event map.

## Deferred combinations

- At v0.07, bent notes are excluded pending extended lane reservations and
  delayed wheel reset. Audio v0.08 adds simple original 3.x bends with LV;
  see `laissez-vibrer-bends-wip.md` for its scope and remaining combinations.
- Audio v0.09 adds simple, separately emitted grace notes; see
  `laissez-vibrer-grace-wip.md`. Audio v0.10 adds ordinary tie endpoints across
  repeat/jump segments; see `laissez-vibrer-repeat-ties-wip.md` for its
  full-performance rendering policy and deferred combinations.
- Hand-authored CC66 actions overlapping generated LV requests have not been
  composed into a single controller state machine.
- Pause/resume, looping a selected range, and unusual tempo changes require
  listening tests. Do not count those as verified by the interval tests.
- This increment adds no reverb sends or synth fallback approximation.

## Automated checks

`tst_midi` adds `laissezVibIntervals`, `laissezVibPlayback_data` and
`laissezVibPlayback`. The event tests cover all marks on a chord, LV at a
normal tied-chain endpoint, preservation of note attacks/gates, no CC66 after
removing LV, and captures/releases on repeated traversals.

```sh
ctest --output-on-failure -R '^tst_midi$'
```

The pure C++ interval scheduler was additionally compiled with GCC and checked
against interval-union coverage for 10,000 generated request sets, including
invalid/empty requests, nesting, touching intervals and separated holds. This
test passed. XML duration checks and patch/whitespace checks were also run.
The Qt tests, full application compilation and audio tests were not run in
the preparation environment.

## Listening checks

Open `mtest/libmscore/midi/laissez-vibrer-playback.mscx` in the experimental
backend. Keep the same font override as the tuning tests.

1. All three notes of the opening chord should continue ringing after their
   quarter-note duration. The tied C's LV belongs to the chain's endpoint and
   should sustain the already sounding chain without an extra note attack.
2. Compare with LV removed, using piano and a sustained instrument such as
   organ/strings. Piano's natural sample decay may obscure the hold length.
3. Stop during the held sound, seek to the rests, restart, and repeat this at
   different positions. No previous held note should remain stuck.
4. Open `laissez-vibrer-playback-repeat.mscx`: the first repeat jump releases
   its hold; the repeated notes capture again. On the final exit, the natural
   release can extend briefly into the next measure.
5. Export WAV and compare with live playback. Re-run the existing individual
   tuning score to check that its behavior remains intact.

Next checkpoint: fix any build/test/runtime issues from these checks before
extending bent/grace notes or implementing reverb routing.
