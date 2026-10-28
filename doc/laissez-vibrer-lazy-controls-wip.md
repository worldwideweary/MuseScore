# 37E audio v0.11: lazy LV playback and repeat-chain fixture correction

Apply this increment after audio v0.10, retaining the notation backports.

User checkpoint, October 29, 2026: `tst_midi` passed with v0.11. Audio/WAV
verification and large-score startup testing are explicitly deferred, not
failed or accepted. See `audio-verification.md` for the outstanding checks.

## Reported failure

The supplied MIDI test report has one failure: `laissezVibRepeatTies`,
`revisit-written-note`, finds two attacks instead of one. The fixture used an
outgoing partial tie in measure 2 and an incoming partial tie in the adjacent
measure 3. Partial ties resolve discontinuous repeat/jump destinations; they
do not replace an ordinary tie between adjacent measures. The fixture now
retains the outgoing partial tie for the repeat return and adds a normal tie
from measure 2 to measure 3. The expected single attack is unchanged.

## Playback preparation

v0.10 requested full event collection for any LV score when preparing playback.
It did not add rendering to score-file loading, but could still delay playback
and prepare every note's automatic events earlier than necessary.

Ordinary LV now uses a controller prepass. A notation/roadmap scan finds marked
measures and tie chains leading to LV endpoints, including paths across repeat
segments. Only those measure ranges have playback events prepared in advance.
Their temporary note events are used to calculate the complete CC66 schedule;
only those controllers are retained in the live sequencer's event map.
Unrelated note events continue to use the existing incremental rendering path.
The prepass does not mark those later chunks as rendered.

Dependency resolution caches written-note/performance-time pairs to avoid
repeatedly walking long tie chains. Contiguous written measures are prepared
as one range. The scan still visits score metadata, and other notes sharing a
selected measure or covered by the existing tied-range extension may also be
prepared. This is not a guarantee of constant startup cost: a score with LV
throughout can still require substantial preparation. Large-score timing has
not yet been measured in the application.

Isolated bend dependencies retain the stopped/full-render policy needed for
channel allocation. The prepass returns no controllers or new bend channels
on that fallback path. The existing policy for newer GuitarBend spanners also
remains. Export/full rendering prepares the same LV dependencies before note
collection, keeping endpoint gate lengths consistent with live preparation.
Manual CC66 stays on the normal chunk path and is not copied twice by the
prepass; composition of manual CC66 with generated LV remains deferred.

## Verification

`laissezVibControls` adds seven data rows covering ordinary LV, repeated grace
LV, partial-repeat landings, revisited normal-tie chains, forward jumps,
unrelated piano events, and the isolated-bend fallback. They compare prepass
CC66 events against full rendering and check that no bend lane is allocated.

The new 20-measure `laissez-vibrer-lazy-controls.mscx` has a repeated LV tie
chain in its first three measures and an unrelated piano note in measure 20.
The regression requires the prepass to prepare the LV endpoint to length 950
while leaving the later note at 1000. Full rendering then prepares the later
note to 950 and must produce the same CC66 stream.

A standalone C++11 harness compiles the production tie resolver, dependency
selection, controller prepass, full-render entry point and LV finalizer against
minimal score/event stubs. It reproduces the malformed fixture's two attacks,
checks the corrected fixture's one attack, compares CC66 streams for the three
repeat/jump fixtures and the new lazy fixture, and checks the bend fallback.
It also checks linear dependency traversal and one preparation range for a
2,000-note tie chain, plus selection of an attached grace-only LV measure.
The harness and fixture XML/measure-balance checks pass. These are not Qt,
score-import, sequencer-thread, sound, or full application integration tests.

Run in the user's build directory:

```sh
ctest --output-on-failure -R '^tst_midi$'
```

Then check the three v0.10 repeat/jump scores in playback and WAV export,
including stop/seek, repeated seeks and different starting positions. Confirm
that a large mostly unmarked score keeps unrelated Piano Roll events lazy
until their usual rendering point. Rerun the accepted piano, grace, original
bend/LV and tuning checks. Qt development packages are unavailable in the
patch environment, so those checks remain pending the user's build.

## Remaining scope

v0.09 is the latest user-accepted audio milestone. v0.11 MIDI tests passed;
v0.10/v0.11 audio verification remains pending. Grace ties/bends, ornament-merged graces, LV on
newer GuitarBend elements, cross-segment legacy-bend chains, manual CC66
composition, and complex nested voltas/jumps remain deferred. The old Fluid
and Zerberus backends still do not implement CC66 LV holds. Reverb remains a
separate stage.
