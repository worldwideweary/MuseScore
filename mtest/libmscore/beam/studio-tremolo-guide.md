# Tremolo comparison scores — v0.13

Open the same original files in Evolution and Studio 4.7.5. Enable Evolution's
experimental Studio beam setting. Compare stem length, slant, beam/tremolo
clearance and note/tremolo clearance, in staff-space units rather than pixels.
Every measure is 1/4. Up-stem cases precede matching down-stem cases. System
breaks limit each system to two measures. These are diagnostic inputs, not
reference outputs. No visual parity has been established here.

## studio-tremolo-single.mscx

| Case | Up measure | Down measure | Inspect |
|---|---:|---:|---|
| S1 | 1 | 11 | One stroke on the first eighth note |
| S2 | 2 | 12 | Two strokes on the last eighth note |
| S3 | 3 | 13 | Three strokes on the first eighth note |
| S4 | 4 | 14 | Four strokes on the last eighth note; stem extension |
| S5 | 5 | 15 | Three strokes on an interior 16th note |
| S6 | 6 | 16 | Three strokes on a 32nd in an eighth/16th/32nd group |
| S7 | 7 | 17 | Three strokes inside a rising beam |
| S8 | 8 | 18 | Three strokes on a wide chord inside a beam |
| S9 | 9 | 19 | Buzz-roll fallback control |
| S10 | 10 | 20 | Small-note tremolo fallback control |

Start with S1, S4, S5 and S6 in both directions. Toggle the setting off/on and
save/reopen. After a small edit and undo, check for stem or tremolo drift.
Manual beams, cross-staff groups, grace/small tremolos and buzz rolls need
separate handling; the initial ordinary single-note stage is narrower.

## studio-tremolo-two-note.mscx

| Case | Up measure | Down measure | Inspect |
|---|---:|---:|---|
| D1 | 1 | 6 | Two tremolo strokes between an inset eighth-note pair |
| D2 | 2 | 7 | Three strokes between the same pair |
| D3 | 3 | 8 | Four strokes between the same pair |
| D4 | 4 | 9 | A sloped inset pair with three strokes |
| D5 | 5 | 10 | Standalone two-note tremolo, without an ordinary beam |

D1–D4 have normal 16ths flanking the tremolo pair. In MuseScore 3 XML, each
written eighth in the pair has an actual duration of 1/16; the two-note tremolo
is attached only to its first chord. D5 uses written quarters with actual
1/8 durations. This follows Evolution's existing tremolo serialization.
Two-note tremolos retain the legacy path until their beam/anchor adapter is
implemented. Use this score to identify the next concrete differences.

All measure durations were checked with exact fractions; XML parsing passes.
Application loading/rendering still needs checking. The v0.13 checkpoint adds
these diagnostics only; see later checkpoint notes for engine changes.

## v0.14 engine checkpoint

Apply v0.13 after v0.12, then v0.14 after v0.13. Rebuild and run tst_beam.
S1–S8 now exercise the initial full-size single-note beam/tremolo adapter when
the setting is enabled. S9/S10 remain fallback controls, as do all D cases.
The initial adapter uses Studio's default note-side and outer-side padding;
it does not add new score-style padding controls. Manual/cross-staff beams,
standalone single tremolos and nonstandard staff-line distances remain on the
legacy path. Begin visual comparison with S1/S4/S5/S6, then S7/S8. Source/math
checks pass; the new application tests and visual comparisons are pending.

## v0.15 stroke-span checkpoint

Apply v0.15 after v0.14 and run tst_beam. In D1–D4 the enabled setting now
changes horizontal stroke span: endpoints leave 1sp from the outside stem
edges, with reduced gaps for close pairs. Compare D1/D3 in both directions.
D5 remains the standalone control. Vertical placement and stroke spacing are
still legacy for two-note tremolos. D4 actually renders a flat beam in both
applications, so it does not yet test the future sloped-anchor adapter.

## v0.16 standalone span

D5 (measures 5 and 10) now uses the same fixed-gap span when the setting is on.
Compare it with Studio 4.7.5 too. This adds ordinary standalone pairs with two
same-direction stems; it does not change their vertical position or tilt.

## v0.17 vertical-anchor comparison inputs

`studio-tremolo-anchors.mscx` contains twelve 1/4 measures. V1–V6 use up stems;
measures 7–12 repeat them with down stems. Use the same original file in both
applications, enabling Evolution's Studio beam score style.

| Case | Up measure | Down measure | Inspect |
|---|---:|---:|---|
| V1 | 1 | 7 | Rising inset pair, two strokes |
| V2 | 2 | 8 | Falling inset pair, three strokes |
| V3 | 3 | 9 | Rising standalone pair, three strokes |
| V4 | 4 | 10 | Falling standalone pair, three strokes |
| V5 | 5 | 11 | Flat standalone pair, four strokes |
| V6 | 6 | 12 | Rising inset 16th pair, three strokes, mixed ordinary beam levels |

The inset contours are strictly monotonic, avoiding the interior maximum that
flattened D4. Confirm V1/V2 visibly slant in the applications as the first check;
this fixture has not been rendered here. Compare stroke tilt, vertical distance
to the ordinary beam, notehead clearance, and stem lengths. For V3/V4 compare
how far each stroke end sits below/above its stem tip. V5 is the flat vertical
stack control. Please keep case labels visible in screenshots.

This checkpoint adds a diagnostic score only. Two-note vertical anchors, tilt
and stroke spacing are still on Evolution's previous path; the new width rule
from v0.15/v0.16 applies where supported. Do not infer full 4.7.5 parity from the
stroke spans. All measure durations and the monotonic inset pitch sequences
were checked; application loading/rendering remains to be confirmed.

## v0.18 inset anchor comparison

Apply v0.18 after v0.17 and run tst_beam. Reuse this same anchor score, comparing
V1/V2/V6 in both directions with Studio 4.7.5. The enabled style now counts
inset tremolo strokes when solving the enclosing beam and places the strokes
from that beam's final anchors. Inspect slope, distance to the ordinary beam,
stroke spacing and stem lengths. V6 up was nearly flat in the supplied Studio
image; this adapter does not impose a zero slope.

V3/V4 standalone vertical anchors and tilt remain the next stage. Their fixed-gap
width rule stays enabled, but their vertical layout is still Evolution's prior
calculation. Toggle the style off to check restoration of the legacy inset layout.
