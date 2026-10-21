# Studio beam corner-case comparison — v0.9

These are visual diagnostics for Evolution with **Format → Style → Beams →
Use Studio 4 beam layout (experimental)** enabled, compared with Studio 4.7.5.
The pack adds scores and this guide; it makes no engraving-engine changes.
You can open the supplied MSCX files directly without applying the patch.
The incremental patch adds the same files to a source tree already at v0.8.

Open the same original MSCX in each application. Compare the beam connections,
beamlet directions, rest positions relative to staff lines, and tuplet-boundary
breaks. Different zoom levels and horizontal spacing are expected; compare
staff-space relationships rather than pixel distances. Keep manual beam/rest
offsets untouched for the initial comparison.

Every measure is 1/4. Short labels identify cases; explicit system breaks limit
each system to two measures. Each file starts with up-stem cases, then repeats
them with down stems. Take screenshots with the labels visible. A difference
report can simply name the file, case label, stem direction, and affected note
or rest. These scores do not assert what 4.7.5 ought to draw: it is the reference
for the comparison. Some cases intentionally exercise still-unported behavior.

## First pass: rests

File: `studio-corners-rests.mscx` — 20 measures.

| Label | Up measure | Down measure | Inspect |
|---|---:|---:|---|
| R1 | 1 | 11 | One leading beamed 16th rest: beam start and clearance |
| R2 | 2 | 12 | One trailing beamed 16th rest: beam end and clearance |
| R3 | 3 | 13 | Two leading rests: which inner levels extend over them |
| R4 | 4 | 14 | Two trailing rests: inner-level endings and floating segments |
| R5 | 5 | 15 | Two interior rests: continuous secondary beam or separate beamlets |
| R6 | 6 | 16 | 16th notes around an eighth rest: secondary-beam bridge |
| R7 | 7 | 17 | 32nd note, 16th rest, 32nd rest, two 16ths: level changes across a rest run |
| R8 | 8 | 18 | Secondary break on the first of two interior rests |
| R9 | 9 | 19 | Secondary break on the second of two interior rests |
| R10 | 10 | 20 | Secondary break on the final note after two rests |

R1–R6 are the first priority. R8–R10 distinguish a break at the start, inside,
or after a rest run; compare them together. All rest joins/breaks are explicit.

## Second pass: tuplets

File: `studio-corners-tuplets.mscx` — 12 measures.

| Label | Up measure | Down measure | Inspect |
|---|---:|---:|---|
| T1 | 1 | 7 | Normal 32nd, triplet 32nds, normal 32nd: control for the earlier embedded-tuplet test |
| T2 | 2 | 8 | Same triplet with its middle note replaced by a rest |
| T3 | 3 | 9 | Outer 16th triplet containing an inner 32nd triplet; normal 16ths flank the outer tuplet |
| T4 | 4 | 10 | Same nesting with a rest in the inner triplet |
| T5 | 5 | 11 | Five 32nds in the time of four, between normal 16ths |
| T6 | 6 | 12 | Adjacent distinct 32nd triplets between normal 16ths |

At each boundary, inspect primary, secondary and tertiary levels separately.
In T3/T4, distinguish outer-tuplet entry/exit from inner-tuplet entry/exit.
Tuplet chords use AUTO beam modes at boundaries and internally; explicit MID
joins are used for the beamed rest. This allows default grouping to participate.
T1/T2 finish with an unbeamed eighth rest to complete the measure. Tuplet number
or bracket placement may differ independently of beam grouping; report that
separately if noticed. T3, T4 and T6 are the first priority after the rest score.

## Third pass: contours and extra levels

File: `studio-corners-contours.mscx` — 10 measures.

| Label | Up measure | Down measure | Inspect |
|---|---:|---:|---|
| C1 | 1 | 6 | Rising notes around an interior rest: local clearance under a sloped beam |
| C2 | 2 | 7 | Falling notes around an interior rest: clearance on the opposite slope |
| C3 | 3 | 8 | Eighth, 32nd, two 64ths, 16th: third/fourth-level beamlets |
| C4 | 4 | 9 | Eight 32nds with a secondary break halfway through |
| C5 | 5 | 10 | Four repetitions of 32nd–64th–64th, with a tertiary break halfway through |

C4/C5 check that a break affects the intended inner levels while lower levels
stay joined. In Evolution's XML, begin32 means a secondary break and begin64
means a tertiary break; Studio's internal enum names differ. Loading the old
score format should handle that conversion.

## Interaction checks

After an initial comparison, try these on a copy of a score:

1. Toggle the score beam style off, then on. Compare with the initial on state.
2. Save, close and reopen; confirm the setting and resulting layout persist.
3. Make a reversible edit, then undo; look for rest drift or changed beam joins.
4. Widen/narrow a system. Slope may change with width; inspect for rest collisions
   and changes that fail to settle after another layout.
5. Move a beamed rest manually, then reset its position. Its saved offset should
   not be overwritten by automatic clearance. Disabling automatic placement
   should bypass the new rest-clearance adapter.

Known later work: multi-voice and merged/locked rest clearance, cross-staff rest
clearance, beamed tremolos, grace-note details and tablature. They are not
covered by this pack. The full Studio rest-segment traversal is also still
pending; the rest-run cases above help determine which differences matter.

## Validation and checkpoint

All 42 measure durations and all nested tuplet spans were checked with exact
rational arithmetic. XML parsing succeeds. These files have not been opened or
rendered in either application here; there is no claimed visual parity or new
application-test result. Existing engine code and tst_beam are unchanged.

Next: collect visual comparisons, then make a small engine patch for any
confirmed rest-run or nested-tuplet discrepancy before continuing to tremolos.
