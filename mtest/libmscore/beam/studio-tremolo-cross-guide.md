# Cross-staff tremolo comparisons — v0.25

This checkpoint adds diagnostic inputs only. Apply its patch after v0.24 to
keep them in the source tree, or open the supplied scores directly in the
current build. No engraving code or application tests change. The user reports
tst_beam passing through v0.24, and the O1-O12 capture looks broadly consistent
with Studio 4.7.5.

Open the same original files in Evolution and Studio 4.7.5. Both start with
Evolution's experimental beam style enabled. They each have eight 1/4 measures
on a two-staff piano, two measures per system. U/L mean displayed Upper/Lower
staff, not stem direction. All saved stem directions are Auto. Empty measures
on the other source staff contain hidden full-measure rests so those rests do
not distract from the tremolo comparison. The final system fills the width.

## X1–X8: standalone pairs

File: `studio-tremolo-cross-standalone.mscx`. Each pair uses written quarter
notes with actual 1/8 durations and explicitly disables ordinary beams.

| Case/measure | Source staff | Displayed pair | Strokes | Inspect |
|---|---|---|---:|---|
| X1 | Upper | Upper → Lower | 2 | Basic pair crossing downward |
| X2 | Upper | Lower → Upper | 3 | Same ownership, reversed order |
| X3 | Lower | Lower → Upper | 2 | Pair crossing upward from lower ownership |
| X4 | Lower | Upper → Lower | 4 | Reverse order and thick stack |
| X5 | Upper | Upper chord → Lower chord | 3 | Complete chord heights and clearance |
| X6 | Lower | Lower chord → Upper chord | 4 | Chord control with opposite ownership |
| X7 | Upper | Both on Lower | 2 | Entire pair moved down together |
| X8 | Lower | Both on Upper | 3 | Entire pair moved up together |

X7/X8 are moved-pair controls: both chords display on one staff, although their
source staff is the other staff. Start with X1-X4, then X5-X8.

## Y1–Y8: inset pairs within cross-staff beams

File: `studio-tremolo-cross-inset.mscx`. Four chords form one forced ordinary
beam group. The central two carry the two-note tremolo: written eighths with
actual 1/16 durations, surrounded by ordinary sixteenths. Each measure totals
1/4. The beam crosses staves in every case.

| Case/measure | Source staff | Four displayed chord positions | Tremolo pair | Strokes |
|---|---|---|---|---:|
| Y1 | Upper | U U L L | U → L | 2 |
| Y2 | Upper | L L U U | L → U | 3 |
| Y3 | Lower | L L U U | L → U | 2 |
| Y4 | Lower | U U L L | U → L | 4 |
| Y5 | Upper | U L L U | Both on Lower | 3 |
| Y6 | Upper | L U U L | Both on Upper | 2 |
| Y7 | Lower | L U U L | Both on Upper | 3 |
| Y8 | Lower | U L L U | Both on Lower | 4 |

Start with Y1-Y4, then Y5-Y8. The latter distinguish a tremolo crossing between
staves from a tremolo staying on one side of a cross-staff beam.

## What to compare

Keep both staves and the case labels visible. Compare stem directions first,
then tremolo slope, stack side, distance to the ordinary beams, notehead
clearance, and the staff gap. Do not compare widths or angles solely in pixels:
horizontal spacing can differ. A changed staff gap can itself be significant.
Report any collisions, extreme stems, or shifts after a repeated layout or a
small edit. For a suspicious case, a close-up of the entire piano system is
more useful than a crop of just the strokes.

Current modern tremolo anchor/direction adapters exclude staff moves and
cross-staff beams, so these are baseline comparisons; differences are expected
diagnostic results. The same-staff common-direction rule from v0.24 must not
simply be applied to all these cases. Studio has additional direction and
anchor branches for cross-staff tremolos. The optional pitched two-note flag
suppression from v0.22 still applies, though these inputs do not need flags.

## Validation and next action

XML parsing, both source-staff duration sums, legal destination staffs, pair
connections, written/actual duration matching, Auto properties, labels and case
counts were checked locally. These scores have not been loaded or rendered in
either application locally. No new application test was added for this
fixture-only checkpoint. After comparison, isolate the standalone and inset
differences and adapt them in separate recoverable stages. TAB, manual edits,
small/grace tremolos and multiple-voice handling remain separate work.
