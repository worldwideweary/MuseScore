# Audio v0.44: defer panel playback preparation and preserve the live seek cursor

Apply this increment after v0.43. The user accepted quiet export cancellation,
normal audio/export behavior and application mtests, then reopened the large-score
and D5–D7 click/seek investigations. The reported 197-page piano compilation opens
in roughly 5–6 seconds in another 3.7 build and 45–50 seconds in the audio build.
That score has not been supplied in this environment: these times are the user's
measurements, not a before/after benchmark of this patch.

## Opening and preparing playback

Full MIDI event generation is not intrinsically required to open a score. Live
playback uses a ten-measure minimum chunk and roughly ten 4/4 measures of buffer,
then collects more in the background. A chunk may extend further for dependencies
such as ties, hairpins, repeats and LV release windows. New GuitarBend playback,
and LV combinations needing isolated bend-lane allocation, retain their existing
stopped/full-render policy. Ordinary LV retains its controller prepass over its
selected measures and tie dependencies. Export still renders the full performance.

There is an unintended preparation entry point: PlayPanel::setScore() calls
Seq::canStart(), which calls collectEvents(). Once instantiated, even a hidden
Play Panel receives score switches. Its displayed tempo, speed and position only
need score metadata, so this patch removes that canStart() call. Actual play/seek
validation and event collection remain in the sequencer. Repeat-roadmap metadata,
notation loading/layout, synth reset and instrument initialization are still
performed; this change does not claim that opening is constant-time.

The sequencer's full-score bend/LV feature scans now run only when the playlist
changes, rather than on every subsequent start or seek. The underlying preparation
and controller policies are unchanged. The ordinary-tie chunk boundary rule also
exists in the pre-audio post-engraving base; this patch does not identify it as a
new audio regression or alter its bend-lifetime safeguards without profiling.

## D5–D7 seek investigation

The provided video contains a short fifth-pitched component on repeated seeks.
A concrete pre-existing sequencer defect can dispatch earlier tuning commands:
seekCommon() calls collectEvents() on the GUI thread before queuing SEEK, while
collectEvents() unconditionally assigns playPos to the beginning. A callback in
that interval reads the old clock and the rewound iterator, dispatching past notes
and controllers, including D4's +7 NRPN. Hard-stopping voices inside setPos() does
not prevent this earlier dispatch.

Collection now leaves playPos untouched while transport is PLAY. setPos() still
performs the actual cursor/clock change on the audio thread. Stopped initialization
and loop initialization retain their existing behavior. This fixes the demonstrated
iterator rewind; application listening is needed to establish whether it accounts
for all of the reported fifth. It is not a blanket thread-safety rewrite, a change
to pitch-wheel reset semantics, or a new synth/FX-tail reset.

## Verification in the patch environment

- A focused harness compiles the exact production collectEvents()/setPos() bodies
  with small score/renderer bridges. It forces a callback between collection and
  seek dispatch. The v0.43 body exposes seven stale events including one +7 NRPN;
  v0.44 exposes none. It also checks pending-chunk merge, distant collection,
  changed-playlist rejection during PLAY, stopped buffering, scan invalidation
  and stopped loop initialization.
- A focused harness compiles the exact PlayPanel::setScore() body against small
  control/score bridges. The previous body requests playback preparation once;
  the patched body requests none while setting tempo, speed, duration and
  position. Same-score, score-switch and null-score paths pass. Opt-in diagnostics
  emit output; an ordinary launch remains quiet.
- Real-header syntax checks pass for rendermidi.cpp, the MIDI mtest with the
  modern backend enabled/disabled, and the new sequencer mtest. These are not
  full application links or executions of the application mtests.
- The new tst_audiosequencer uses the real Seq class with prepared event maps,
  without a driver or SoundFont. Six rows exercise D5/D6/D7 collection with/without
  asynchronous results and verify that the running cursor/clock stay unchanged.
- The new tst_midi::lazyPlaybackChunks generates 120 ordinary measures, verifies
  that initial rendering covers ten measures and leaves the last note's playback
  events untouched, then requests the later chunk and verifies its preparation.

No MIDI export reference files or engraving baselines change. Full application
build/mtests, large-score timing and D5–D7 listening remain pending user checks.

## Application check and timing

Reconfigure/build normally so the new target is discovered, then run the full
mtest suite; the focused targets are:

```sh
ctest --output-on-failure -R '^(tst_midi|tst_audiosequencer)$'
```

Use the normal modern-backend settings, adding this optional launch variable:

```text
MSCORE_AUDIO_PREP_TIMING=1
```

Reopen the same large file and retain all `Audio preparation:` lines from before
opening through the first playback request. Nested scopes distinguish:

| Scope | What it measures |
| --- | --- |
| read score | File/import, playback metadata, update/layout/style, sanity check |
| open score | Reading, migration/layout, attaching score and panels |
| sequencer score switch | Stop/reset and instrument initialization |
| Play Panel metadata | Panel initialization without playback collection |
| collect playback | Renderer wait, feature scan/LV prepass, chunk rendering; requested tick, changed flag, policy, chunk/event counts and buffered end |
| MIDI roadmap/partition | Swing/capo metadata and chunk partitioning |
| LV controller prepass | Dependency selection, preparation and selected-range rendering |
| MIDI chunk | Written/unfolded tick range, note events, channel/velocity metadata, notes/spanners and controller work |
| full MIDI render | Complete rendering, normally used for export |

This is wall-clock tracing, not CPU profiling. A dense LV score may emit many chunk
lines. Leave the variable unset for ordinary operation and clean timing comparisons.

Compare load time with the older build under matching score/style, panel visibility,
font and Debug/Release conditions. Check the initial Piano Roll and an unrelated
late passage before playback; opening the Play Panel should not prepare their
automatic note lengths. Then start/seek late and verify normal preparation/playback.
If opening still takes tens of seconds, the phase log (or the actual score) is
needed before attributing it to MIDI generation rather than parsing/layout/UI.

On D, start at D5, D6 and D7, click each destination repeatedly while running, and
seek backward/forward between them. D5 should be the reset A4, D6 should glide once,
and D7 should remain the straight C5 reference without an added fifth. Also rerun
stop/start, loops, repeat landings and the accepted LV/bend scores in playback/WAV.
Keep the large-score and fifth-artifact checks open until those results arrive.
