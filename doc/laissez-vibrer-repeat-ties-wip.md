# 37E audio v0.10: LV endpoints across repeats and jumps

Apply after audio v0.09, retaining the existing notation backports.

Update: audio v0.11 supersedes the blanket full-render policy described below
with an ordinary-LV controller prepass. It also corrects the repeat-chain
fixture's adjacent landing to use a normal tie. See
`laissez-vibrer-lazy-controls-wip.md`; v0.10 has not yet passed the user's audio
verification.

## Accepted baseline

The user accepted v0.09 after passing MIDI tests, in-app audio and WAV export.
Earlier checks also accepted ordinary/chord LV, simple normal ties, original
3.x bends with LV, tuning, stop/seek and the basic repeat fixtures.

## Problem and changes

The repeat-aware tie renderer already follows actual playback destinations.
Its one note attack and final note-off can occur in different rendering
segments. The per-segment LV pass could not pair those events and rejected
endpoints beyond the attack's segment. Tracking visited written notes alone
also stopped traversal too early when a repeat revisited the same note.

Ordinary LV now runs once after all performance note events are collected.
It resolves the endpoint using written-note identity together with unfolded
performance time. Its release window is clipped to the endpoint's playback
segment, rather than the attack's segment. A tie explicitly continuing across
a jump can therefore reach its marked endpoint. Ordinary, untied LV retains
the existing boundary clipping. All ordinary channel requests are scheduled
together so a shorter overlapping hold cannot release a longer one.

`MidiRenderer::renderScore` finalizes LV after collecting its chunks. Callers
using `renderChunk` directly must call `renderLaissezVib` once after collecting
the complete map; calling it twice would duplicate controllers. The live
sequencer does this when it has rendered an LV score. Seeking without a new
render does not finalize it again.

## Live-rendering tradeoff

Scores containing LV are now fully rendered while stopped, using the existing
policy for newer bend spanners. This makes cross-segment holds deterministic
and allocates legacy-bend LV channels before audio playback. Scores without
LV or newer bend spanners keep the existing incremental rendering policy.

This WIP can increase startup time and event-map memory for large LV scores.
Check that on the user's older-system target before treating the policy as
final. It adds no new runtime dependency or Qt requirement. It does not
recreate historical notes when seeking into a hold that began earlier.

## Fixtures and tests

All files are under `mtest/libmscore/midi/`:

- `laissez-vibrer-repeat-tie.mscx`: an outgoing partial tie crosses the repeat
  into an incoming LV note. The initial landing note attacks normally on its
  first pass. The return has no new attack. Two overlapping channel captures
  share one final release, including across the segment boundary.
- `laissez-vibrer-repeat-tie-chain.mscx`: a normal tie plus repeat-aware partial
  ties carries one C through both passes, then into the LV endpoint in measure
  3. The same written notes are visited twice without terminating the walk.
- `laissez-vibrer-jump-tie.mscx`: a custom forward jump skips measure 2 and
  continues a single tied attack into measure 3's marked endpoint.

The existing partial tie encoding is unchanged: incoming = 0, outgoing = 1;
normal ties remain separate. No slur enum or score-file property changes are
included.

`tst_midi` adds `laissezVibRepeatTies_data` and `laissezVibRepeatTies`. It checks
exact attacks/gates, capture/release timing, repeated written-note traversal,
the live collect-all-then-finalize path with one-measure chunks, real MIDI
export/readback, and unchanged notes after removing LV. The first fixture is
also rendered with repeats disabled.

```sh
ctest --output-on-failure -R '^tst_midi$'
```

Listen through all three scores using modern FluidSynth. Tied landings should
have no new attack, and their final note should continue ringing into rests.
Check stop/seek during a cross-boundary hold, start playback at different
positions, and compare WAV/MIDI export. Rerun the v0.07 piano/repeat, v0.08
legacy-bend, v0.09 grace, and individual-tuning scores. Repeated seeks must
not accumulate duplicate captures or leave held notes behind.

The standalone C++11 harness compiled the production LV finalizer against
minimal event/score stubs and passed the repeat-landing, repeated-note,
overlapping-hold and forward-jump schedules. It also reran the earlier grace
schedules, 100 short gate lengths and excluded grace combinations. Fixture
XML and measure balances passed. These are not a full MuseScore build or
Qt/score-import/audio integration test; those remain pending the user's build.

## Remaining scope

Grace ties/bends, ornament-merged graces, LV on newer GuitarBend elements,
cross-segment legacy-bend chains, manual CC66 composition, and complex nested
volta/jump combinations remain deferred. The old Fluid/Zerberus backends still
do not implement the CC66 hold. Per-instrument reverb remains a separate stage.
