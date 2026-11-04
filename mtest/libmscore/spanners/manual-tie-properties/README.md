# v0.15 native 475 / 37E comparison scores

This version replaces v0.14: the original C cases did not establish valid tie
jump connections, and D's angle-away file triggered Studio's assertion for
slurs with both endpoints hanging. In v0.15, E changed only its redundant
opening clef and version heading.

The user subsequently confirmed that both D modes look acceptable and the
Studio guard prevents the crash. C and E exposed notehead/dot/endpoint issues;
Evolution v0.17 fixes partial-tie anchors and source direction, and v0.18 fixes
LV inner/outer anchors and common chord endpoints. Reuse these same files to
verify those changes. The user confirms C/E look good after v0.17/v0.18. V0.19
updates the repeat test's obsolete ordinary-tie coordinate comparison without
changing engraving; its full tst_repeat run remains pending.

V0.20 addresses the subsequently exposed cross-voice partial-tie overlap after
staff-line adjustment, and resolves incoming Auto direction from the enabled
repeat source. The repeat test retains its physical clearance checks. Rerun
tst_repeat/tst_spanners and recheck C; D and LV implementation are unchanged.

The user reports mtests pass after v0.20. V0.21 addresses the extra repeat-to-
first-chord gap at system starts: repeat barlines use the same bar-note margin
there as at mid-system boundaries. Reuse D3/D4 for the half-note comparison and
D12 for a whole note. Compare from the painted repeat-dot edge to notehead ink;
the clef-to-repeat distance is a separate header interval. Incoming ties or
accidentals can still legitimately require additional room.

The user reports all mtests pass after v0.21. V0.22 addresses the separate
signature-to-first-note interval exposed by v0.14 E: an opening clef explicitly
read from the file can be HeaderClef without its generated-header flag. Infer
the header margin from the actual system-start clef/key/time segment, including
sharp and flat keys, while retaining ordinary repeat-dot-to-note spacing. Use
the original v0.14 E file to check this case; v0.15 omits its explicit clef.
Rerun tst_repeat and tst_repeatcourtesy and compare E1, D3/D4 and D12. Application
rendering and the updated mtests remain pending here.

V0.23 refreshes the MusicXML systemDistance export reference for v0.22's intended
opening-header spacing change, using the user's reported output. Only horizontal
positions/widths in measures 1-9 change; vertical system distances and musical
content remain identical. No engraving code changes. Rerun tst_mxml_io.

The user reports all mtests pass after v0.23. V0.24 addresses the remaining
first-chord gap after a single barline (D2) or end repeat (D8). Ledger lines
use Studio's smaller barline padding instead of adding their overhang to the
full notehead margin. Keep the configured bar-note margin; system headers and
start repeats retain their existing paths. Compare the barline-to-notehead
gap in D2/D8; slur curve code is unchanged. Reuse these same manual files.
Updated automated tests and application rendering remain pending here.

V0.25 corrects four whole-note expectations in v0.24's new repeat tests: include
the existing whole-note centering reservation as well as head/ledger ink.
Keep exact-position and clearance checks and the half-note gap regression.
No engraving changes; rerun tst_repeat. D2/D8 remain the visual checks for v0.24.

The user reports visuals and mtests look good after v0.25. V0.26 incorrectly
copied missing first-pass attacks from the 475 compatibility MIDI export.
V0.27 restores those attacks and fixes the playback test's native-fixture
loading: 4.70 needs an explicit opt-in to the backport reader. The user reports
mtests pass, but C4/C6 still retrigger on first arrival.

V0.28 connects their actual forward arrivals: C1 -> C4 and C3 -> C6 on the
final repeat pass. Only C2 at score start needs a new attack; C4/C6 sustain
from their preceding outgoing partial ties, as do later backward repeat
returns. Expected MIDI: 21 note-ons with repeats, 12 without; both endpoints
are required. Six C4/C6 first-arrival note-ons disappear and their sources
extend, rather than the target chords simply becoming silent. Recheck C
audio/MIDI export and run tst_repeat/tst_midi. Engraving is unchanged.

Open each `*-475.mscx` directly in Studio 4.7.5 and the corresponding
`*-37E.mscx` directly in the rebased Evolution build. Do this before testing
37E-to-475 import. The files use each version's XML format independently; the
475 files do not depend on the companion reader's legacy-format handling.

These are authored native-format XML, not files saved or rendered by a Studio
executable here. Their structure follows the official 4.7 writer and native LV
fixture. The first application load, appearance and save/reopen still need
verification. If an object is absent on the initial 475 load, report that first;
do not treat an absent object as the intended engraving reference.

The paired files contain identical pitches, durations, placement choices,
explicit stem directions, style values and forced system breaks. Version,
instrument identification and title-style/header syntax differ as appropriate.
These are small case-matched comparisons, not copies of the earlier combined
visual verification score. No manual grip offsets are supplied.

## Compare in this order

1. Open the 475 file and check the labeled objects exist. Save a separate copy
   from 475 and reopen it to establish a Studio-written reference as well.
2. Open the paired 37E file. Use Bravura in both builds and the same zoom.
   Small spacing differences are expected between engines; compare endpoints
   relative to the noteheads, dots, staff and barlines.
3. Capture a matching case label in both builds. Include the selected object's
   placement/direction and relevant score style if a discrepancy appears.
4. After direct native comparison, open the 37E file in patched 475 separately.
   A difference between that result and the native 475 file isolates import
   behavior from layout behavior.

## C: hanging ties

Each C chord has three independently note-owned `PartialTie` elements. These
are not ordinary ties connecting consecutive chords, and are not LV marks.

| Case | Direction | Placement | Stem |
| --- | --- | --- | --- |
| C1 | Outgoing | Auto | Up |
| C2 | Incoming | Auto | Down |
| C3 | Outgoing | Inside | Down |
| C4 | Incoming | Outside | Up |
| C5 | Outgoing | Auto | Up |
| C6 | Incoming | Auto | Up |

C cases are now three two-measure repeats. Each repeat's first chord has the
incoming ties; its last chord has the matching outgoing ties, with no trailing
rest between that chord and the repeat end. The chronological case order is
**C2, C1, C4, C3, C6, C5**. C5 and C6 are on separate systems and connect by
the repeat jump back from C5 to C6. There are no ordinary forward ties.
This supplies the active jump destinations needed by Studio; an isolated
outgoing partial tie before a rest is not a valid reference for this feature.
The score starts at 1.5sp minimum hanging length;
try 3.0sp in both builds. Change score-wide chord placement Outside to Inside.
Auto cases should follow the score setting while explicit C3/C4 remain fixed.
Check each note's direction, notehead anchor, free endpoint and save/reopen.
Do not assume the old screenshot's measure-long outgoing arcs are correct.

## D: partial slurs and ordinary system breaks

Use the default D pair first, then the `angle-away` pair. Studio 4.7 asserts
when its angle-away helper receives **two** hanging endpoints. D4 and D12
exercise that condition, so the angle-away files substitute an ordinary D4
slur and omit D12's slur, with explicit labels. The remaining cases retain
their notes, placement and partial directions. Slurs are forced Down and stems
Up to keep the comparison deterministic. Do not use the v0.14 angle-away file
in an unguarded Studio debug build.

| Case | Purpose |
| --- | --- |
| D1 | Normal slur, rising notes |
| D2 | Outgoing slur at repeat end |
| D3 | Incoming slur at repeat start |
| D4 | Incoming and outgoing slur in a one-bar repeat |
| D5 | Rising outgoing slur, repeat end and system break |
| D6 | Rising incoming slur at the next repeat start |
| D7/D8 | First-ending outgoing / second-ending incoming slurs |
| D9 | Rest closing the second ending |
| D10/D11 | One ordinary slur crossing a forced system break |
| D12 | One-note incoming-and-outgoing slur in a one-bar repeat |

Repeat starts/ends and two voltas give the partial slurs actual navigation
boundaries. D10/D11 distinguish ordinary broken slurs from repeat fragments.
D12 checks that both connectors on a single chord survive saving in follow-
contour mode. Compare free endpoints, contour and staff clearance. Both-ended
angle-away behavior needs the separate Studio guard patch; after that patch,
open the follow-contour file and enable angle-away in the application to test
those cases. The guard retains their contour instead of inventing an angle
relative to an opposite endpoint that is also floating.

## E: LV notehead anchors

The curved LV implementation is selected (`laissezVibUseSmuflSym=0`). Each note
owns its own LV. Length is 3.0sp except E1, which explicitly uses 2.0sp.

| Case | Notes | LV direction | Placement | Stem |
| --- | --- | --- | --- | --- |
| E1 | Single | Auto | Auto | Up |
| E2 | Chord | Up | Inside | Up |
| E3 | Chord | Down | Outside | Down |
| E4 | Dotted chord | Auto | Auto | Up |
| E5 | Chord | Up | Outside | Up |
| E6 | Chord | Down | Outside | Up |
| E7 | Chord | Up | Outside | Down |
| E8 | Chord | Down | Outside | Down |

Start with E3, the reported outer-LV anchor discrepancy. Compare where each
curve begins relative to its notehead. E5-E8 separate stem-side and direction
effects. E4 checks dot clearance. After curved comparison, toggle SMuFL LV
symbols in both builds as a separate check; do not mix glyph and curve modes.

## Provenance and local checks

`generate.py` regenerates all eight files using only Python's standard library.
The official Studio 4.7 source inspected was commit
`02a82facca4060be0a532adc55f01ed8217cef78`:

- `src/engraving/rw/write/twrite.cpp`: note-owned PartialTie/LaissezVib,
  placement, partial direction, minimum length and Slur properties.
- `src/importexport/mei/tests/data/laissez-vibrer-01.mscx`: native 4.70 LV XML.
- `src/engraving/dom/tie.cpp`: updatePossibleJumpPoints requires a following
  jump boundary for a partial source without an end note. Its allJumpPointsInactive
  state is true if there are no active matching destinations.
- `src/engraving/tests/partialtie_data/repeat_barlines-ref.mscx`: official
  repeat-destination incoming partial-tie example.
- `src/engraving/rendering/score/slurtielayout.cpp`: the angle-away helper's
  exactly-one-hanging-end assertion.

Local checks: all eight XML files parse; every voice totals 4/4; all Slur/Volta
next/prev locations are reciprocal; each pair has matching musical content
after format/header normalization. Each C file contains 18 partial ties, each
D follow-contour file 10 slurs, each D angle-away file 9 slurs, and each E file
22 LV marks. C repeat sources are final chords and destinations are matching
first chords inside the same repeat. Application rendering and
round-trip checks are pending. This milestone changes no engine code.
