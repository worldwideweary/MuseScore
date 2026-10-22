# Short-duration standalone tremolos — v0.22

Apply v0.22 after v0.21, rebuild, and run tst_beam. Open the same original
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

Compare stem lengths, tremolo slopes and staff placement, and stroke spacing.
With the experimental style enabled, both flags should be absent in each pair,
matching Studio 4.7.5. With it disabled, Evolution's legacy flags should return.
Start with F1/F2/F4/F7 in both directions. Keep the labels and staff visible.
Horizontal spacing still differs between applications, so compare vertical
distances in staff spaces. Check Evolution's style off restores legacy layout.

The v0.21 comparison exposed a missed rule: Studio's Chord::shouldHaveHook()
excludes two-note tremolos. The written duration does not imply an actual flag.
v0.22 suppresses flags for pitched two-note tremolos under the experimental
style and removes the inappropriate flagged-stem calculations from v0.21.
Supported standalone pairs use the unflagged tremolo stem/anchor rules, counting
only tremolo strokes. Both chord segment shapes are refreshed. The score file
is unchanged; its F labels continue to identify the written durations.

TAB retains its existing flag handling. Flag suppression applies to pitched
two-note tremolos generally under this score style, including arrangements
whose modern anchors are not implemented yet. Ordinary notes and single-note
tremolos retain flags. Modern-anchor limits remain: full-size non-grace pitched default-style pairs, one measure
on the same unmoved staff, standard line spacing and unmodified stems.
Opposite stems, cross-staff pairs and small/grace/manual cases remain separate.

XML parsing and exact duration/pair checks passed in v0.21. Production and test
C++11 syntax checks pass for v0.22. The isolated Qt geometry harness covers
10,000 standalone cases (5,000 with short written durations and suppressed
flags), including repeated layout, plus 10,000 ordinary/inset-beam comparisons
with v0.21. These checks use simplified
contexts and do not replace application tests or establish complete Studio
parity. Full tst_beam and rendering of the corrected fixture remain to be verified
in the application builds.
