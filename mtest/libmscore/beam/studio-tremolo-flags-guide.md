# Flagged standalone tremolos — v0.21

Apply v0.21 after v0.20, rebuild, and run tst_beam. Open the same original
`studio-tremolo-flags.mscx` in Evolution and Studio 4.7.5. The file starts with
Evolution's experimental beam style enabled and fills the final system.
There are sixteen 1/4 measures, two per system. F1–F8 use up stems; measures
9–16 repeat them with down stems. Both chords explicitly disable ordinary
beaming. Rests fill the remainder of each measure.

| Case | Up measure | Down measure | Written flags | Tremolo strokes | Notes |
|---|---:|---:|---|---:|---|
| F1 | 1 | 9 | Eighth | 1 | Rising C4–C5 |
| F2 | 2 | 10 | Eighth | 4 | Falling C5–C4 |
| F3 | 3 | 11 | Sixteenth | 2 | Rising C4–C5 |
| F4 | 4 | 12 | Sixteenth | 3 | Falling C5–C4 |
| F5 | 5 | 13 | Thirty-second | 3 | Low ledger notes C3–G3 |
| F6 | 6 | 14 | Thirty-second | 4 | High ledger notes C6–G6 |
| F7 | 7 | 15 | Sixty-fourth | 4 | Rising three-note chords |
| F8 | 8 | 16 | Eighth | 3 | Flat G4–G4 control |

Compare stem lengths, tremolo slopes and staff placement, stroke spacing, flag
attachment to the final stem tips, and clearance around flags and noteheads.
Start with F1/F2/F4/F7 in both directions. Keep the labels and staff visible.
Horizontal spacing still differs between applications, so compare vertical
distances in staff spaces. Check Evolution's style off restores legacy layout.

The enabled adapter now admits ordinary same-direction standalone pairs with
up to four traditional flags. It follows Studio's flagged-stem defaults:
no unflagged beam-count stem addition, ordinary 2sp staff-overlap target,
traditional-flag shortening capped at one quarter space, and no unflagged
optical or four-beam stem exception. The tremolo anchor solver still counts
only tremolo strokes, as in Studio. Flags follow the final stem tips and both
chord segment shapes are refreshed. This stage does not add flag collision
avoidance beyond those Studio defaults; report any visible collisions.

Evolution does not implement Studio's straight-note-flag style here. Other
limits remain: full-size non-grace pitched default-style pairs, one measure
on the same unmoved staff, standard line spacing and unmodified stems.
Opposite stems, cross-staff pairs and small/grace/manual cases remain separate.

XML parsing and exact duration/pair checks pass. Production and test C++11
syntax checks pass. The isolated Qt geometry harness covers 10,000 standalone
cases (5,000 flagged), including repeated layout and flag-tip attachment, plus
10,000 ordinary/inset-beam comparisons with v0.20. These checks use simplified
contexts and do not replace application tests or establish complete Studio
parity. Full tst_beam and rendering of this new fixture remain to be verified
in the application builds.
