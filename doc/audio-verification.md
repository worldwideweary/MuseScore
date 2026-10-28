# Audio backport verification checkpoint

Updated November 6, 2026. Record a test as passed only when it was actually run.
The user has authorized continuing in small recoverable stages while the
explicitly deferred checks below remain pending.

Supported local/online verification baseline: Qt 5.15.2, as confirmed by the
user. Qt 5.9.9 compatibility is no longer required for this feature.

Current configuration, acceptance/deferred boundaries and the LV extraction
plan are consolidated in `audio-backport.md`. The entries below are historical
stage records; their older pending checks are superseded only by explicit later
acceptance. v0.41 adds export failure-path cleanup; its application checks remain
pending. See `audio-export-cleanup-v41.md`.

The user's v0.41 build fails linking `tst_midi`: its new export regression pulls
the application bootstrap alongside lightweight test globals. v0.42 moves the
renderer into a separate object and makes its synth factory explicit. The
focused archive reproducer fails before and links/renders after this change;
full application build/mtests and v0.41 export smoke acceptance remain pending.
See `audio-export-test-link-v42.md`.

The user subsequently reports v0.42 mtests pass and audio tests seem okay,
accepting the link correction. Canceling an export displays the caller's
generic write-error popup. v0.43 separates cancellation from write failure
for that dialog; its quiet cancel/retry and mtest checks remain pending.
See `audio-export-cancel-v43.md`.

Acceptance follow-up, November 6, 2026: the user confirms cancellation remains
quiet and mtests pass after v0.43. The cancellation correction is accepted and
the current audio cleanup phase is closed. Previously explicit deferred checks
remain separate; no additional manual retry or platform check is inferred.

The user now reopens the large-score and D5–D7 click/seek checks. Their 197-page
piano compilation loads in about 45–50 seconds in the audio build versus 5–6
seconds in another 3.7 build. v0.44 removes playback event preparation from Play
Panel score initialization and restricts repeated feature scans to dirty playlists.
It also fixes a demonstrated seek interleaving: collection rewound the running
iterator before SEEK reached the audio thread, exposing earlier notes and +7
NRPN commands to an intervening callback. Exact-method component reproducers fail
against v0.43 and pass against v0.44. New application mtests cover running cursor
preservation and lazy initial/later chunks. Real-header syntax checks pass, but
full application build/mtests, the 197-page timing and D5–D7 listening remain
pending. Optional MSCORE_AUDIO_PREP_TIMING diagnostics separate opening/layout
from MIDI preparation. See `audio-large-score-seek-v44.md`; the standalone LV
extraction and optional effects remain outside this increment.

## Resumed application verification: November 5

The user reports that the repeat/jump LV listening checks, live playback and
WAV comparisons, transport checks, and the earlier ordinary/grace/bend/tuning
regressions requested when resuming audio all check out. These supersede the
older pending listening entries below. The supplied
`laissez-vibrer-repeat-tie.mid` was independently parsed: PPQ 480; C4 attacks at
0, 1920 and 5760; note-offs at 1919, 5759 and 7679; CC66 captures at 192 and
2304, release at 8064. These exactly match the fixture regression. The second
written note attacks normally; the repeat landing at 3840 does not attack.

The user also accepts the requested independent instrument sends, live edits
and undo/redo, shared reverb reset/default workflows, transport and normalized/
unnormalized WAV checks. Earlier saved/reopened and live-edited return settings
remain accepted. MP3 was conditional and has not been separately reported.

Large-score startup, memory/CPU behavior and unrelated Piano Roll laziness are
explicitly still deferred. No new automated test result was supplied with this
acceptance; do not infer a fresh full mtest pass from the listening report.

The user subsequently applied audio v0.24 and reports that the application
builds and mtests pass. Its automated lifecycle/recovery checkpoint is accepted;
a separate manual sample-rate/export smoke check has not been reported.

The user reports v0.25 mtests pass and confirms basic Zerberus playback still
works. Application testing found missing default-font loading and requested
restoring the original discovered-font chooser and SoundFonts preference roots.
These are corrected by v0.26. The user now reports mtests pass, the application
builds/starts, the default font appears, installed /usr/local and configured user
SoundFont folders are both discovered, and ordering, load/reload and defaults
behave correctly. This accepts the v0.26 application checkpoint.

The user reports v0.27 mtests pass, both comparison scores play correctly, and
a selected lower-slot SoundFont preset remains selected after save/reload.
This accepts the v0.27 compatibility and listening checkpoint.

The user reports v0.28 mtests pass, the ordinary bend reaches its target D,
the other controller checks work, and WAV/live playback agree. The brief return
of C before D was reproduced as an existing bend-release pitch-reset artifact,
also heard by the user in 3.6.2. Its fix is explicitly deferred.

The user accepts v0.29 A4/A5 fully left/right listening and now reports mtests
pass for both v0.29 and v0.30. See `fluid2-pan-modulators-v29.md` and
`fluid2-parameters-v30.md` for their focused test scopes.

v0.30 listening found a score/sequencer integration issue: D6's portamento
setup followed its note-on at the same tick, leaving CC84 armed for D7, and
seek replay treated past CC84 as persistent state. The supplied MIDI confirms
the event order. v0.31 corrects the shared named-action ordering and one-shot
controller handling. The user now reports its live listening, WAV and mtests
all pass; this accepts the v0.31 portamento checkpoint.
See `fluid2-portamento-order-v31.md`.

Next checkpoint: v0.32 restores legacy output gain. A controlled comparison
found the adapter's extra 0.2 synth gain made the generated SF2 about 14 dB
quieter than legacy before the common master stage. Native/adapter tests cover
the correction; cross-font level and explicit 44.1/48 kHz application export
checks remained pending at that stage. See `fluid2-output-level-v32.md`.
The user's subsequent acceptance is recorded below.

The user now reports v0.32 tests look good and mtests pass. A video of repeated
clicks during playback exposes a shared seek/release-tail pitch artifact, also
confirmed by the user in 3.6.2. v0.33 provides a bounded sequencer correction;
it changes no FluidSynth code. Its application check remains pending. The
earlier ordinary bend-release pitch-wheel artifact remains separately deferred.
See `audio-seek-release-pitch-v33.md`.

Follow-up after v0.33: the user still hears the brief fifth when clicking notes
in D5, D6 and D7; D1-D4 are unaffected. Mark this unresolved, not accepted as
fixed. The user explicitly defers further investigation. The focused replay
reproducer does not establish the complete cause of this persistent behavior.
No fresh v0.33 mtest pass was reported with this follow-up. Continue the audio
roadmap without expanding this shared pre-existing bug investigation.

v0.34 now bundles pinned FluidSynth 2.3.4, while retaining the SYSTEM provider
and the backend-disabled build. Bundled native tests pass 16/16; SYSTEM 15/15;
the dependency-free OFF configuration 7/7. Focused real-adapter Qt checks and
84 actual legacy/modern PCM level comparisons also pass. Provenance and the
remaining external GLib/libsndfile dependencies are documented in
`fluid2-bundling-v34.md`. Full application build/mtests and a short user smoke
check remain pending for this packaging stage. The two pitch artifacts above
remain deferred; source bundling changes no playback/DSP implementation.

The user now reports v0.35 builds, full mtests pass and playback works. Export
of C-reset-and-bend is much slower than the online 37E build (about ten seconds
versus a third of a second). v0.36 addresses measured unoptimized Debug DSP
costs and an unused synthesizer in the format wrapper; it also scopes known
upstream warning suppressions to the dependency. Native Debug checks pass
16/16. Full-score export speed parity remains pending; see
`fluid2-export-performance-v36.md` for measured scope and optional timing output.

The v0.36 user measurements are consistent: setup about 0.3 s, then roughly
3.4 s rendering/writing/cleanup. With effects and normalization both off,
the latter is 1.574 s (1.904 s total). An older Debug build without the audio
changes exports in under one second, confirming an additional modern-path
cost. v0.37 removes unnecessary host planar output work when sends need no
separate lanes. Exact dry parity across routing paths and all 16 bundled native
tests pass. End-to-end application speed parity remains pending; see
`fluid2-compact-dry-v37.md`. Empty parameterless effect-state groups are also
ignored instead of producing the harmless `unknown <>` message.

The user reports v0.37 mtests pass. Effects-on export without normalization
takes 1729 ms rendering; the latest mixer-send pair is 1864 ms at 100% and
1133 ms at zero. v0.38 extends the existing Debug DSP optimization option to
the shared Zita implementation, retaining the effect controls and other host
Debug settings. A focused two-instance DSP benchmark improves from 0.808 s
to 0.088 s with bit-exact raw samples; all 16 bundled tests pass. Full-score
export speed and user application checks remain pending for v0.38. See
`fluid2-zita-debug-v38.md` for the scope and PCH compatibility checks.

Latest v0.38 user timings with normalization off: 100% mixer send plus master
Zita is 1706 ms rendering (2030 ms total); zero send plus master Zita is 957 ms
(1234 ms total); zero send plus NoEffect is 903 ms (1217 ms total). v0.39
profiles the send-active engine and avoids repeated clearing/summing of exactly
zero planar outputs, with compact selection per routed bank. Native sparse
send-active rendering improves about 38%, with bit-exact dry/send/wet samples
against v0.38 across sparse/dense workloads. Bundled 16/16, SYSTEM 15/15,
backend-disabled 7/7 and focused adapter checks pass. Full application checks
and matched C export timing remain pending. See `fluid2-send-performance-v39.md`.

The user now reports v0.39 full application mtests pass and a brief WAV check
passes. Matched C exports are 1266 ms rendering / 1546 ms total at 100% send
with master reverb, 983/1276 ms at zero send with master reverb, and 901/1162 ms
at zero send with NoEffect. The user reaffirms the C2 ordinary pitch-wheel
bend-release artifact remains audible; at v0.39 it was still deferred and was not fixed
by these performance changes. A brief WAV acceptance does not independently
verify the earlier explicit 44.1/48 kHz metadata or large-score resource checks.

The user then requested the ordinary C2 bend-release fix. v0.40 retains the
wheel value and sensitivity on each actually released voice in the original
Fluid engine and the bundled FluidSynth provider. Held and pedal-held voices
keep following the wheel. Bundled native tests pass 17/17; SYSTEM 15/15 and
dependency-disabled 7/7 also pass. The new original-engine PCM regression
passes against rebuilt production Fluid code and fails against the pre-fix
objects. Focused Qt adapter checks and MIDI-test syntax checks pass with the
backend enabled/disabled. On 2026-11-06 the user reports v0.40 mtests pass,
playback and WAV export perform well, and no bend-reset artifact is audible.
The ordinary C2 release fix is accepted. SYSTEM and external MIDI retain their
native release semantics; no exported MIDI events changed. The D5–D7 note-click
fifth remains deferred. See `fluid-release-pitch-v40.md`.

Tick-based CC automation in the Piano Roll is a future enhancement, outside the
current compatibility work. The following stage table is historical;
the acceptance above updates the relevant listening/reverb columns.

| Stage | User verification | Outstanding |
| --- | --- | --- |
| v0.05 tuning | In-app individual tuning and exported WAV verified | Broader sample-rate/font compatibility remains separate |
| v0.07 ordinary LV | MIDI tests, in-app playback, WAV, repeats, stop/seek passed | Extended notation combinations remain deferred |
| v0.08 original bends + LV | MIDI tests, in-app playback, WAV, stop/seek and repeats passed | Cross-segment bend chains remain deferred |
| v0.09 grace LV | MIDI tests, in-app playback and WAV passed; user accepted this milestone | Grace ties/bends and merged ornaments remain deferred |
| v0.10 repeat-aware tie endpoints, corrected by v0.11 | v0.10 had one malformed-fixture failure; v0.11 `tst_midi` passed | Audio/WAV checks for repeat/jump endpoints deferred |
| v0.11 lazy LV controllers | User reported `tst_midi` passed | Audio/WAV, stop/seek regression and large-score startup/Piano Roll checks deferred |
| v0.12 optional audio-group buffers | User reported application build passed; standalone production-buffer checks passed in patch environment | Actual FluidSynth PCM comparison and listening pending; user has not reported the new buffer mtest result |
| v0.13 source-channel ownership | User reported patch applied; standalone routing and dispatch checks passed in patch environment | No user build/mtest result reported yet; audio/WAV and startup checks remain deferred |
| v0.14 isolated synth banks | Native FluidSynth 2.3.4 engine/PCM and standalone routing checks passed in patch environment | Full application build, Qt tests, real-score playback/export, SF3, large-score startup and resource measurements pending/deferred |
| v0.15 source sends and auxiliary mix | Production send/buffer tests and native 2.3.4 weighted-stem/reset checks passed in patch environment; user reports build running | Build completion and Qt tests not yet reported; user audio/WAV/startup checks remain deferred |
| v0.16 shared auxiliary reverb return | Native bus/Zita/2.3.4 checks passed; user ran the application and verified audible shared reverb with both master A/B slots set to NoEffect | Qt tests, normalized WAV/MP3 and live stop/seek/repeats with the return remain pending; earlier startup checks remain deferred |
| v0.17 saved source sends and mixer controls | Focused native checks passed; user observed the new mixer control | User reported Track color overlap and tst_midi link failure on Awl::Knob; corrected by v0.18. audioReverbChannels later passed as part of the v0.19 full tst_midi run; live mixer/send checks remain pending |
| v0.18 mixer row and Effects/AWL dependency fixes | User verified the mixer label now occupies the correct row; tst_midi now reports a runtime allocator abort rather than the prior link error | Runtime heap corruption corrected by v0.19; subsequent full MIDI/layout tests passed; live audio checks remain pending |
| v0.19 stable MasterSynthesizer allocation layout | User reports full tst_midi passed (2.17 s) and tst_master_layout passed (0.00 s), 2/2 successful | Startup/test blocker cleared; previously unverified live audio/export/large-score checks remain pending/deferred |
| v0.20 shared return controls and state | All three tests pass after v0.22. User verifies Save to Score/reload and honoring settings after reopening the score; live playback and WAV export use current edits without Save to Score. Screenshot confirms the tab's two columns fit without overlap | Reset/default buttons not separately reported; broader return transport/export checks remain pending; large-score checks explicitly deferred |
| v0.21 return-state failure diagnostics | User Qt 5.15.2 output confirms parameter setters, in-memory state, XML round trip, reset and malformed-state checks pass; only full-score reopen returns defaults | Identified existing Score::write test-mode suppression of synthesizer state; corrected in the test by v0.22 |
| v0.22 return-state score-save test correction | User reports tst_midi, tst_auxiliary_settings and tst_master_layout all pass; automated and application score save/reopen are verified | Default/reset workflows and broader return regression checks remain pending; earlier large-score checks remain deferred |
| v0.23 SoundFont replacement/SF3 | Native 2.3.4 SF2/SF3 failure recovery, held LV/dry/send PCM, replacement tuning and unload tests pass, including ASan/UBSan and either-bank failure injection. Existing banks/return tests and a focused real-adapter Qt 5.15.13/bundled SF3 check pass | User application build/new test pending. Sample-rate reload failure recovery and font management GUI remain separate stages. Reverb focused checks and large-score checks explicitly deferred |

At the earlier deferred-listening checkpoint, the requested order was the
v0.10 repeat landing, revisited
tie-chain and forward-jump fixtures. Check live playback and WAV, tied attacks,
release timing, stop/seek, repeated seeks and starting from different positions.
Then check a large mostly unmarked score for startup cost and unrelated Piano
Roll events staying at their original lengths until normally prepared. Do not
infer audio correctness or startup performance from the passing MIDI tests.
Those listening checks have since been accepted above; large-score checks
remain explicitly deferred and are not newly requested by this cleanup.

For v0.12, the existing stereo renderer remains selected unless the new
environment variable is explicitly enabled. Test that new path separately;
see `fluid2-audio-groups-wip.md`. Those raw modulo groups are not instrument sends.
v0.14's two-bank path is also opt-in; see `fluid2-isolated-banks-wip.md` for
its 256/256 voice split and native-test scope. Neither environment option is
needed merely to build or run the new standalone tests.
v0.15 adds runtime send APIs and an additive auxiliary output. v0.16 connects
that output to a separate, opt-in wet-only Zita return before the existing
master A/B effects. v0.17 replaces the temporary uniform send with saved channel percentages and an
Audio reverb send mixer control. Remove `MSCORE_FLUID2_REVERB_SEND`; retain the
modern-backend and isolated-channel launch options. Older scores default to zero.
See `fluid2-saved-sends-wip.md` for current launch settings and pending checks.

For v0.20, Instrument Reverb controls are in the Synthesizer window, with their
own AuxiliaryReverb state group. Use its existing Save to Score / Set as Default
buttons. Per-instrument sends remain in Mixer; master Effect A/B remain separate.
See `fluid2-return-controls-wip.md` for the current stage and test commands.

The user authorized the SoundFont phase while deferring the remaining reverb
checks. v0.23 covers explicit replacement and native SF2/SF3 verification; see
`fluid2-soundfont-loading-wip.md` for actual test scope and the next lifecycle
boundary. Do not infer large-score startup/resource acceptance from these tests.
