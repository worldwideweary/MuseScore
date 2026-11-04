# Slur/tie properties v0.11: rebase onto the repeat updates

Prepared November 4, 2026 against worldwideweary/MuseScore,
3.7-Evolution/backport/slurTieProperties at
ae8ce1fe85b01ef435fc123f4c98c5bf2c692c95.

Apply this full replacement properties patch directly to that clean tip with
git am. It includes the supplied original 34-file diff, reconciled with the
newer repeat work, plus a focused owner/placement regression and this checkpoint.
Do not apply it on top of the old conflicting properties commit. No audio-engine
changes are included.

## Conflict resolution

- Keep the tip's ChordRest::writeProperties unchanged. Its writeStart/writeEnd
  rules preserve both connectors for a one-note partial slur and suppress
  invalid same-note normal slurs.
- Keep Tie::layoutPartial, incomingStartOffset, incomingEndOffset and
  calculateDirection unchanged. The older center-of-notehead/measure-end
  layoutPartial block is superseded by the repeat layout's calculated anchors,
  header/repeat-dot clearance, next-visible-item clearance and direction logic.
- Integrate calculateIsInside in Tie::slurPos, used by both partial rendering
  and the incoming-spacing probe. Per-tie Auto/Inside/Outside and score styles
  therefore feed the same anchor path.
- The incoming owner is stored in startNote(), with no endNote(). The new
  isOuterTieOfChord helper resolves either end of a partial tie to that owner;
  otherwise outside incoming outer ties would be treated as inner ties.
- Retain the tip's existing partial-tie minimum-length policy: the longest
  incoming chord tie gets minHangingTieLength while shorter ties get minTieLength.
  Exclude partial ties from the old properties patch's blanket finalize minimum,
  which could move their shared free endpoint across repeat/header clearance.
  Ordinary broken ties retain the added minimum-length handling.
- Keep minHangingTieLength once in the style enum/table. Add the other new style
  entries beside it; remove obsolete skip branches for the newly supported tags.
- Register LAISSEZ_VIB once after the existing gradual-tempo element types in
  both the type enum and name table. Keep only one Tie::readProperties declaration,
  with both placement and partial-direction parsing in its implementation.
- Preserve partial-slur values Normal=-1, Incoming=0, Outgoing=1, Both=2 and the
  existing word-based XML mapping. Keep the repeat Inspector panel after the
  new property rows.

The notation follow-ups in the supplied diff are retained: curved LV profile,
range-selection tie-chain ends, linked-staff propagation, TAB fret bounds,
legacy LV palette removal and l.v. search. The separate audio series stays separate.

## Review of the supplied Studio 4.7 reader changes

No direct error was found in the shown additions. The explicit tiePlacementChord
guard prevents the pre-4.2 compatibility default from replacing an explicitly
supplied Evolution value. The Tie loop retains the ordinary SlurTie property
reader while recognizing placement. The note-owned LaissezVib import reads its
placement/minimum length and marks the length unstyled. In native 4.7 Note::add
sets track, owner and tick for that LV, so an extra setTrack is not required there.

This review is limited to the shown diff and the official 4.7 code paths. The
user's earlier PartialTie/slur companion changes are not fully present in that
diff. A separate PartialTie reader must also accept placement; handling it in
TRead::read(Tie*) alone does not establish that. The C/D import behavior still
needs native 4.7 fixtures and inspection of the full companion reader. This
checkpoint makes no new 4.7 reader changes.

## Verification

- The four baseline Tie methods listed above and all of chordrest.cpp were
  compared directly with ae8ce1fe85b and are unchanged.
- XML parsing passed for both edited .ui files, the workspace and both included
  .mscx fixtures. Style/element registrations were checked for duplicates and
  enum/table alignment. Whitespace and conflict-marker checks passed.
- An isolated C++11 harness using the actual three placement/outer-anchor/
  finalize method bodies passes 86 matrix cases under ASan/UBSan. It covers
  incoming/outgoing owners, chord placement/overrides, ordinary endpoints,
  partial free-end preservation, broken-tie minima and edited/nudged endpoints.
  It uses small score/note/segment test doubles and real Qt Core points, not a
  complete MuseScore build. Leak inspection is disabled for environment limits.
- The mtest placement-resolution case now checks both ends of incoming and
  outgoing partial ties against their shared owner. Existing round-trip, linked
  TAB, LV range and style tests from the original patch remain included.

A full Qt application build, mtests and visual rendering have not been run here.
From the user's configured build directory:

```sh
cmake --build . --target tst_element tst_spanners tst_repeat tst_repeatcourtesy
ctest -j1 --output-on-failure -R '^(tst_element|tst_spanners|tst_repeat|tst_repeatcourtesy)$'
```

## Outstanding and next action

First confirm this rebased code builds and the four test targets pass. Then
prepare native Studio 4.7 C/D comparison scores and investigate the missing C
ties, partial-slur contour differences, and outer LV chord anchors shown in the
screenshots. Those visual corrections are deliberately outside this rebase.
The earlier audio/reverb and large-score checks remain at their previous status.

Changed files: libmscore property/style/element/notation code, the Score/Inspector
and palette/style interfaces, original element/spanner tests and fixtures,
workspace migration, and this checkpoint. The patch header lists each path.

## v0.12: QtTest Direction comparison

The user confirms the application builds and the tie visual verification score
looks unchanged after v0.11. The mtest build stops in testLaissezVibRoundTrip:
QCOMPARE on Ms::Direction finds Ms::toString through argument-dependent lookup,
whose const char* result does not match QtTest's owned char* diagnostics API.
Compare both operands as int, retaining the same value assertion. Other
Direction comparisons in this element test already use integer casts.

The attached result(20261104-052227).xml records 57 tests with zero failures,
but it does not clear this compile blocker: the current test translation unit
failed to compile. A new build/run remains required. A focused compile check
against staged Qt 5.15.13 reproduces the enum diagnostic and accepts the integer
comparison. No full application/mtest build is claimed here.

Apply v0.12 after v0.11. Changed files: tst_element.cpp and this checkpoint.
Next: rebuild tst_element and run the four previously listed test targets;
native 475 C/D reference scores and LV anchor verification remain queued.

## v0.13: repeat contours and test setup corrections

The user's new reports identify one element assertion, 25 repeat assertions in
two functions, and one spanner assertion. These are distinct causes:

- testLaissezVibRoundTrip changed the low-level minimum-length value without
  clearing its STYLED flag. The writer intentionally omits styled properties.
  The test now explicitly sets UNSTYLED, as a user override does, and verifies
  that the reloaded override flag survives. No LV serialization code changes.
- The properties patch's contourY pass overwrote repeat-aware endpoints already
  calculated by slurPosChord, including the one-note 0.25sp constraint. Restrict
  this pass to ordinary slurs across system breaks. Partial slurs preserve their
  baseline contours; the explicit angle-away style still applies when enabled.
- The fixture's incoming partial slur followed an end repeat but was not a jump
  destination. resetInvalidPartialSlurs correctly normalized it, leaving only
  one partial slur. Add a second repeat region starting at that incoming measure
  and ending at the final measure, and relabel it accurately. Keep the assertion
  that both partial slurs survive parsing and save/reopen.

Checks run here: fixture XML/repeat-boundary validation, exactly two partial
declarations, whitespace checks, and direct comparison confirming that the
default partial-slur layout path (including all subsequent staff clearance and
autoplace code) matches ae8ce1fe85b. Full Qt build and mtests remain pending.

Apply after v0.12. Changed files: slur.cpp, tst_element.cpp,
tie-layout-styles.mscx and this checkpoint. Rebuild/run the same four targets
listed above before returning to the C/D and LV visual comparison work.

## v0.14: native 475 comparison scores

The user reports 100% mtest success after v0.13. Record that checkpoint as
passed; no full local Qt build is claimed here.

Add eight paired native-4.70/37E XML files under
`mtest/libmscore/spanners/manual-tie-properties/`, a generator and a comparison
guide. C covers hanging ties and explicit placement, D covers repeat slurs,
alternate endings, ordinary system breaks and one-note partial slurs in both
contour modes, and E isolates LV chord anchors by stem/direction. The 475 files
are authored directly from native writer syntax, avoiding the legacy import
reader as a prerequisite for comparison. They have not been saved/rendered by
a 475 executable here. Initial load and visual/round-trip validation are pending.

Local validation: eight XML parses, 4/4 rhythm in every measure, reciprocal
Slur/Volta connector locations, expected object counts, and four musically
equivalent file pairs after format/header normalization. No engine code changes.

Apply after v0.13. Changed files: this checkpoint and the manual comparison
directory (eight scores, generate.py, README.md). Next: compare native 475 and
paired 37E C/D/E cases, then resolve any import/contour and outer-LV anchor
differences in separate patches. Audio/reverb and large-score checks retain
their deferred status.

## v0.15: correct comparison fixtures after first visual load

The user reports no C ties in Studio, while the supplied blue Evolution image
shows partial arcs, and an angle-away Studio assert in
adjustSlurFloatingEndPointAngles. The C setup was incomplete: its outgoing
chords preceded rests rather than repeat jumps, so Studio cannot build active
jump destinations. Replace the isolated C examples with three matching
two-measure repeats. Incoming cases occur first, outgoing cases last:
C2/C1, C4/C3, C6/C5. C6 and C5 are separated by a system break.

The Studio helper asserts exactly one hanging end; both-ended D4/D12 violate
that assumption. The v0.15 angle-away comparison explicitly excludes those
two partial slurs (D4 becomes a normal control and D12 has no slur). Preserve
them in follow-contour mode for round-trip tests and guarded Studio testing.
Remove the redundant voice-level opening clef from all fixtures; Staff's
default clef already supplies it. E musical/notation cases remain unchanged
and visual testing remains deferred as requested.

Changed files: generator, comparison guide, and eight v0.14 scores replaced by
v0.15 files, plus this checkpoint. Local checks: all XML/rhythms, reciprocal
connectors, native/37E musical equivalence, expected object counts and paired
C repeat boundary placement. No application rendering or full mtest run here.
Next: separate minimal Studio assertion guard and equivalent Evolution
two-hanging-end policy, then repeat C/D visual checks before E.

## v0.16: retain contours when both slur endpoints hang

Studio's angle helper uses the opposite endpoint as an anchored reference.
Its IF_ASSERT_FAILED returns without adjustment when both endpoints hang in
non-asserting builds. Evolution previously adjusted both, producing a different
result. Restrict angle-away adjustment to exactly one hanging end in Evolution;
preserve its existing contour for two hanging ends. Add a focused spanners
regression covering two both-ended examples (including a one-note slur) and
six single-ended controls, using the v0.15 follow-contour fixture.

A separate Studio 4.7 format patch adds an explicit early return for the
both-hanging case before its assertion. This preserves the non-asserting
build's existing fallback while retaining the assertion for the invalid
neither-hanging case. It does not define a new two-ended engraving algorithm.

Changed files in Evolution: slur.cpp, tst_spanners.cpp, this checkpoint.
Local checks: C++ syntax check for the test translation unit, extracted angle
math checks for up/down and every hanging-end combination, whitespace and
patch replay checks. Full builds, tst_spanners and visual rendering remain
pending. Next: load corrected C/D pairs, run tst_spanners, and enable angle-
away on the follow-contour D score only after applying the Studio guard.
E and large-score/audio checks remain deferred.

## v0.17: partial-tie notehead clearance and source direction

The user confirms both D modes look acceptable and the Studio guard stops the
assertion. C now exposes overlapping inner endpoints and a middle incoming tie
with the wrong direction. Add a note-local short-tie attachment calculation:
optical notehead center/cutouts for Outside, edge for Inside, and absolute
collision clamps against neighboring heads, stems, dots and accidentals.
Preserve the complete TAB fret box and account for nearby ledger-line height.
Use it for partial ties, including the spacing probe, without applying the old
edge-relative horizontal adjustment a second time. Ordinary tie geometry keeps
its existing attachment path.

Incoming direction resolves the enabled source through the existing repeat
roadmap, matching Studio's primary source note instead of using destination
stem direction. Tie-direction counting includes outgoing partials and LV marks,
which Evolution stores outside tieFor. Single-note short ties use the normal
opposite-stem rule. No playback tie-chain links are added.

Changed files: tie.cpp, tie.h, tst_spanners.cpp and this checkpoint. Add a C
regression for source/destination direction under destination stem flips and
neighboring-head clearance. Local syntax/extracted geometry checks are run;
full tst_spanners and visual checks remain pending. Next: reuse the attachment
calculation for LV, then align chord LV ends after dot layout.

## v0.18: LV inner/outer anchors, dots and shared ends

Use the v0.17 attachment calculation for LV: Outside does not mean every mark
is outer. Only the top Up/bottom Down note is outer; other marks clear nearby
heads and dots. Stem-side outer marks use collision clearance rather than
unconditionally starting beyond the head, restoring the bottom Down/Outside
anchor in E3/E8. Auto direction counts the note-owned LV marks as outgoing ties.

Calculate a common chord endpoint from all starts plus their individual minimum
lengths. Curves extend to that point without shifting their starts; SMuFL marks
shift horizontally to align their rendered right edges. Honor the full TAB
fret box. Run the chord pass after all Note::layout2 dot positions are available
in pitched and TAB horizontal layout, reserve its width then, and refresh after
final chord layout. Remove the earlier per-note reservation using stale dots.

Changed files: laissezvib.cpp/.h, chord.cpp, tst_spanners.cpp, the comparison
guide and this checkpoint. New regression covers all eight E chords, shared
curved/SMuFL endpoints, minimum lengths, the bottom outer anchor and dot
Before/After placement. Syntax checks pass for changed implementation and test
translation units. Extracted real attachment/LV layout functions with Qt path
geometry pass focused inner/outer, stem, dot, minimum-length and common-end
checks; these do not replace application tests. Both versioned patches are
replayed with git am against their parents. Full tst_spanners/tst_repeat and
C/E rendering remain pending. No new comparison files are required: reuse v0.15.
Large-score/audio checks remain deferred.

## v0.19: update the repeat anchor regression for Studio attachments

The user confirms C and E now look good after v0.17/v0.18; D was already
visually accepted. The earlier tst_element report was withdrawn as belonging
to another branch. The correct result(20261104-162623).xml identifies six
failed rows, all in partialTieChordPlacement (leading space 0/12, directions
Auto/Up/Down). Each stops at the assumption that a partial anchor must equal
the legacy ordinary tie's anchor. No other repeat case has a failure entry.

V0.17 intentionally gave partial ties a Studio attachment path while retaining
ordinary 3.x tie geometry. Replace that obsolete coordinate comparison with
independent physical checks: Inside endpoints clear the appropriate head edge,
neither endpoint overlaps visible notehead ink in any voice on the staff,
anchors stay near their head cluster, and vertical side/direction are correct.
Keep ordinary ties as direction and Inside/Outside controls. Retain all six
rows, both incoming/outgoing directions, 14 attachments per row and both
save/reopen passes. Add a note-local coordinate snapshot across save/reopen
with a 0.001sp tolerance. No engraving implementation changes.

Changed files: mtest/libmscore/repeat/tst_repeat.cpp, comparison guide and this
checkpoint. Local checks: test translation-unit syntax with staged Qt 5.15,
focused physical-constraint positive/negative checks, whitespace and git-am
replay. Full tst_repeat execution remains pending in the user's build. Next:
rebuild/run tst_repeat; retain C/D/E visual acceptance unless a new discrepancy
appears. Audio/reverb and large-score checks remain deferred.

## v0.20: final-height cross-voice clearance and incoming Auto source

result(20261104-181207).xml reports four failed rows, all still in
partialTieChordPlacement. Up rows now reach the independent head-ink assertion:
the outgoing partial on track 5 / pitch 43 overlaps pitch 48. This is a real
gap in v0.17, not a test tolerance issue. The initial attachment is computed
before TieSegment::layoutSegment shifts the curve away from staff lines.
An outer tie of its own single-note chord can consequently enter another
voice's head after that shift.

Recheck visible noteheads at the final endpoint height in the partial branch
of adjustX, clamping only the note-owned endpoint horizontally. Keep the free
endpoint, final Y and edited/nudged segments unchanged. Include this vertical
layout step in incomingEndOffset's unregistered spacing probe so spacing and
drawing use the same clearance. Ordinary ties and LV layout retain their paths.

Auto rows still compare against the ordinary tie's cached direction, an
assumption v0.19 did not remove. Check outgoing Auto against its owner chord's
stem, and incoming Auto against the enabled repeat source identified by R05's
ordinary tie. Assert that the source actually reaches the landing through the
repeat roadmap and that this fixture has concurrent voices. Forced directions
remain explicit assertions. Preserve every ink-overlap and round-trip check.
Also initialize an incoming Auto tie's unbeamed non-percussion source stem:
the backwards jump can reference a later system not laid out yet. Leave
beam-controlled/percussion stems and forced tie directions to their existing
rules. Improve failure diagnostics with track, pitch, source, and coordinates.

Changed files: tie.cpp, tst_repeat.cpp, comparison guide and this checkpoint.
Local syntax checks and extracted production-code checks pass: reproduce the
post-Y overlap and clear it for both endpoint sides and leading spaces 0/12;
verify idempotence/manual-edit preservation; resolve an initially stale Auto
source without overriding beam/percussion/forced-direction controls. Verify
git-am replay. Full application tests and rerendering remain pending. The
previous C/D/E visual acceptance is retained; recheck C after this focused fix.
Next: rebuild and run tst_repeat and tst_spanners. Audio/large-score checks
remain deferred.

## v0.21: repeat-to-first-chord spacing at a system start

The user reports the mtests pass after v0.20. Record that checkpoint as passed.
The new comparison shows excess space between start-repeat dots and the first
chord/rest at a system start, compared with a mid-system repeat boundary. This
also affects whole notes in the D comparison score.

Measure::computeMinWidth carries a system-header flag through to the first
chord/rest. Segment::minHorizontalDistance consequently adds systemHeaderDistance
(2.5sp by default) after a start repeat, instead of the ordinary barNoteDistance
(1.3sp). The earlier non-kerning repeat-dot clamp prevents overlap but cannot
reduce this extra header padding. Studio 4.7's
HorizontalSpacing::needsHeaderSpacingExceptions explicitly lists clefs, keys,
meters and ambitus; it excludes start-repeat segments.

Normalize the header flag to false only for StartRepeatBarLine -> ChordRest.
Both locations then use the same existing bar-note/accidental padding and the
same painted-repeat-dot clearance. Keep ordinary clef/key/meter header spacing
and the separate hanging-tie reservation rules intact. Source inspected:
official 4.7 at 02a82facca4060be0a532adc55f01ed8217cef78,
src/engraving/rendering/score/horizontalspacing.cpp.

Changed files: segment.cpp, tst_repeat.cpp, new
start-repeat-header-spacing.mscx, comparison guide and this checkpoint. Seven
new data rows cover quarter/half/whole notes, ledger/on/above-staff positions,
an accidental and a quarter rest. Compare actual ink gaps between system and
mid-system repeats with header distance 2.5sp and 6sp; retain minimum clearances
and an ordinary clef-header control. Local syntax checks, extracted spacing
calculations (including old-behavior negative controls), Qt DOM duration/rhythm
checks and git-am replay pass. Full application rendering and updated mtests
remain pending. Next: run tst_repeat and tst_repeatcourtesy, then compare D3/D4
and D12 using the existing v0.15 files. Audio/large-score checks remain deferred.

## v0.22: recognize explicit clef/key/time headers

The user reports all mtests pass after v0.21, then shows insufficient opening
time-signature-to-note space in v0.14 E. This is a separate path: v0.21 only
changes StartRepeatBarLine -> ChordRest. The v0.14 file supplies an opening
Clef. Reading creates a HeaderClef segment without setting header();
addSystemHeader only sets that flag when it creates a missing segment.
computeMinWidth consequently passes false at the later time-signature-to-note
interval. A low note can kern under that signature and receive barNoteDistance
instead of the system-header time-signature margin. The same missed header
classification can affect key signatures.

In Segment::minHorizontalDistance, recognize a tick-zero header clef, key,
meter or ambitus before a chord/rest in the same system-start measure (also
after an HBox), independently of that generated-header flag. Use Studio's
header-type list from horizontalspacing.cpp at official 4.7 commit
02a82facca4060be0a532adc55f01ed8217cef78. Exclude above-staff time signatures
from this in-staff rule. Preserve existing margin values, mid-system signature
rules, and v0.21's distinct repeat-to-note padding. No score generator changes.

Add 40 regression rows: generated/file-supplied opening clef; time, sharp key,
flat key, combined key/time, or clef alone; low half/whole notes, high whole
note, or rest. Check actual layout and required header distance with default
and enlarged styles. Keep the repeat parity tests; update their clef control
to verify style response rather than expecting an incorrect false-header gap.

Local checks: both changed C++ translation units pass syntax checking against
Qt 5.15 headers; extracted production classifier/spacing cases reproduce the
old missed margins and retain repeat parity and context controls; real Qt DOM
fixture transformations retain the intended signatures and 4/4 durations;
git-am replay matches the committed tree. Full application rendering and
updated mtests remain pending. Next: tst_repeat and tst_repeatcourtesy, compare
the original v0.14 E1 plus repeat-gap D3/D4 and D12. Audio/large-score checks
remain deferred.

## v0.23: refresh MusicXML system-distance layout reference

The supplied result(20261104-202040).xml identifies systemDistance as the sole
failed MusicXML test function. This test explicitly exports layout, including
horizontal coordinates, from testSystemDistance.mscx. That fixture supplies an
opening Clef followed by 4/4, exercising the header flag corrected in v0.22.
Its first note now receives systemHeaderTimeSigDistance (2.0sp), replacing the
misclassified barNoteDistance (1.3sp): the reported 7.00 MusicXML tenths shift
is exactly 0.7sp. Justification distributes the extra header width across the
first system, changing subsequent note positions and measure widths there.

Update only the 19 horizontal attributes reported by the user's actual test
run in testSystemDistance_ref.xml: measure 1's width and two default-x values;
measures 2-9's widths and second-note default-x values. Keep all later measures,
new-system/page markers, system distances, margins, note default-y values, and
musical content unchanged. Preserve exportLayout=true and the exact comparison;
do not suppress the intended header spacing or weaken the export test.

Local XML checks confirm those are the only changed attributes and all other
tree content is identical; the source's clef/time order and style defaults
explain the coordinate change. Patch replay reproduces the committed tree.
This is a reference-only update plus documentation, with no production-code
changes. Full tst_mxml_io execution remains pending in the user's build; rerun
it after applying. C/D/E visual checks and deferred audio work are unchanged.

## v0.24: ledger padding before a measure's first chord/rest

The user reports all mtests pass after v0.23 and the score tests generally look
good. Two remaining comparisons show the first chord farther from the left
barline in 37E: D2 after a single barline and D8 after the first ending's end
repeat. The discrepancy belongs to chord/rest placement; the slur follows it.

37E takes the whole chord shape's minimum left extent, including ledger lines,
then adds the full barNoteDistance. Studio 4.7 instead uses per-item padding:
BAR_LINE -> NOTE uses barNoteDistance, while BAR_LINE -> LEDGER_LINE uses
max(barNoteDistance - ledgerLineLength, 0.25sp). The ledger's overhang therefore
does not ordinarily enlarge the notehead gap. Confirmed against official 4.7
commit 02a82facca4060be0a532adc55f01ed8217cef78, rendering/paddingtable.cpp and
rendering/score/horizontalspacing.cpp. The two versions' default barNoteDistance
values are also slightly different (37E 1.3sp, Studio 1.25sp); retain 37E's style
value and users' overrides rather than changing the global style default.

Add Segment::minLeftAfterBarline for this measure-boundary calculation. Copy
the segment shapes only when a chord has ledger lines. Match the ledger
rectangles using the same translations that built the shape, then compensate
their left bound for the distinct ledger padding. Keep every other rectangle,
spacing marker and the original shapes intact. Missing ledger matches retain
their original conservative clearance. Use this at a first chord/rest only
when the preceding item is another measure within the same system. Leave
system headers, HBoxes, signatures, start repeats, incoming-tie reservations
and leading-space addition on their existing paths. Add a const ledger-lines
accessor to Chord. No slur or tie curve code changes.

Add 22 repeat-test rows across single/end-repeat boundaries: half and whole
notes below/on/above the staff, accidentals, rests, arpeggios and grace notes.
Vary ledger length over 0.35/0.9/1.5sp to exercise the minimum ledger padding;
check actual first-segment X, independent head/ledger minima, other left-side
notation, preservation of the original shapes, and 2sp user leading space.
Reuse the v0.21 fixture through DOM transformations. No new manual files.

Local checks: segment.cpp, measure.cpp and tst_repeat.cpp pass Qt 5.15 syntax
checking. Extracted production helper plus real Shape code verifies 24 spacing
cases, reproduces eight old excess gaps, checks custom margins, accidental and
fallback cases, and preserves source shapes. The real Qt DOM transformations
check all 22 rows / 110 measures for the intended boundaries and 4/4 rhythms.
git-am replay reproduces the committed tree. Existing MusicXML layout-export
fixtures start with rests or on-staff notes and need no speculative reference
refresh. Full application rendering and updated mtests remain pending: rerun
tst_repeat, tst_repeatcourtesy, tst_spanners and tst_mxml_io, then compare D2/D8
against 475. Audio and large-score checks remain deferred.

## v0.25: include whole-note centering in the spacing test

The supplied result(20261104-221354).xml has four failures, all in the new
barlineChordRestSpacing rows: single/end-repeat, pitch 72/84, whole duration.
The actual-layout-versus-helper assertion passes; the subsequent independent
head/ledger-only expectation fails. The v0.24 expectation omitted the general
horizontal-spacing marker retained by Chord::shape. Whole notes are wider
than the nominal notehead and Score::layoutChords centers them; the logical
chord spacing reservation can exceed the notehead/ledger ink minimum.

Include that zero-height general marker in the expected minimum, retaining
the exact-position assertion, head/ledger minima, original-shape comparison,
leading-space and accidental/attachment checks. Keep the >0.2sp regression
check for the half-note ledger cases represented by D2/D8. For whole notes,
verify the new position does not exceed the original distance: centering can
limit their reduction to less than 0.2sp. Add pitch/duration/ledger-length and
position diagnostics to the exact-position assertion. No engraving changes.

Local checks use the real Bravura glyph bounds (whole width 1.688sp; nominal
1.18sp), the existing whole-note centering rules, the production Shape code
and v0.24 helper. All 36 boundary/pitch/duration/ledger-length combinations
pass the revised expectation. The old expectation reproduces the four failing
data rows (ten combinations across the three ledger lengths); it also exposes
two reduction-threshold failures previously masked by the first assertion.
Half-note regression checks and original shape preservation still pass.
The repeat translation unit passes Qt 5.15 syntax checking and git-am replay
reproduces the committed tree. Full tst_repeat remains pending in the user's
build. D2/D8 visual verification and deferred audio/large-score work remain
unchanged.

## v0.26: suppress incoming partial-tie attacks on the first visit (superseded)

The first-visit policy and 18-attack target below were incorrect. V0.27 restores
first-pass attacks and corrects the native-fixture loader; retain this section
as the history of the rejected change, not as the desired playback behavior.

The user reports the visuals and mtests look good after v0.25. Audio testing
of C exposes attacks on incoming tied chords that 475 does not retrigger.
The supplied 37E MIDI has 27 note-ons, versus 18 in the 475 MIDI. Both use
480 PPQ; compare score/performance ticks rather than elapsed time because
their tempo values differ. The extra attacks are C/E/G at performance quarters
0, 16 and 32: the first visits to C2, C4 and C6 (written measures 1, 3 and 5).
Actual repeat returns at quarters 8, 24 and 40 are already suppressed in 37E.

The source is collectNote's tieBack predicate. Native Studio keeps incoming
partial ties in Note::tieBack and its compatibility MIDI renderer suppresses
a single-event note with any tieBack, including the first visit. Evolution
stores partial ties separately and checks only whether a source was reached
in actual playback time, so those first visits attack. Confirmed against
official 4.7 at 02a82facca4060be0a532adc55f01ed8217cef78,
src/engraving/compat/midi/compatmidirenderinternal.cpp (collectNote and
calculateTieLength), and the provided MIDI event streams.

Include note->partialTie(true) in the attack-suppression predicate. Keep the
existing single-event/glissando exceptions, event-time roadmap helpers and
outgoing durations. The 475 compatibility export also omits partial-tie
duration extension: its outgoing notes end at the repeat boundary while
37E sustains into an enabled landing. Preserve that existing 37E behavior;
do not copy the shorter export durations as part of this attack fix. No
engraving, score-format, note Play property, or other audio-branch changes.

Add eight full-render EventMap regression rows in tst_repeat: 37E/475 source
files, expanded/linear playback, and incoming ties retained/removed. Verify
exact attack and release ticks/pitches, including first visits, repeat
returns, ongoing repeat sustains, and restored attacks after tie removal.
Reuse the v0.15 C files; no new fixtures or exported MIDI baselines.

Local validation: rendermidi.cpp and tst_repeat.cpp pass Qt 5.15 syntax checks.
Compile the actual collectNote event-generation core in a focused harness;
its previous predicate reproduces every note event in the supplied 37E MIDI,
and the corrected 18 attacks match the supplied 475 MIDI exactly. All eight
repeat/tie-policy controls match the independent expected event lists;
repeat sustain durations and multi-event/glissando exceptions remain intact.
git-am replay reproduces the committed tree. Full application playback/export
and updated mtests remain pending in the user's build. Next: tst_repeat and
tst_midi, then listen to/re-export C and compare incoming attacks. Deferred
reverb/font-management and large-score checks remain unchanged.

## v0.27: restore first-pass attacks and load the native playback fixture

V0.28 supersedes the per-group first-visit expectation below. The user clarified
that only the score's initial C2 should attack; C4/C6's first arrivals must
continue the immediately preceding C1/C3 sources through forward transitions.

The user rejects v0.26 because the chords do not sound on the first pass.
The new v0.15-C-hanging-ties-37E(1).mid confirms the regression: first-visit
attacks at performance quarters 0, 16 and 32 are absent. The original 37E
export already suppresses actual repeat-return attacks at quarters 8, 24 and
40. Matching the 475 compatibility export's 18 note-ons was the wrong target;
that export also omits first-pass notes. Restore the actual-predecessor check
from v0.25: an incoming partial tie does not by itself prove a sounding source.
The original 27 attacks return, repeat returns still sustain without attacks,
and disabled incoming ties restore landing attacks. Add an explanatory comment
to collectNote; no further duration or engraving changes.

The supplied result(20261105-010742).xml has four failures, all at score loading
in partialTieAttackPlayback's 475 rows. MTest::readCreatedScore calls loadMsc
with ignoreVersionError=false, which rejects the fixture's 4.70 version.
Load the comparison fixtures explicitly, opting into the existing backport
reader only for the native 475 case. Report FileError directly and lay out the
loaded score. Retain all eight rendering rows. Correct their expected first
visits to sound; verify exact attack and release lists, repeat continuations,
linear playback and removed-tie controls. The native input rows test import
compatibility and 37E playback, not the flawed 475 export attack policy.

Local checks compile the actual collectNote event-generation core, reproduce
every event in the new v0.26 MIDI and exactly restore every event in the original
37E MIDI with the corrected predicate. First-visit/return positions, linear and
removed-tie event lists, release durations and glissando/multi-event exceptions
pass. The real loader version predicate reproduces the old rejection and
accepts the corrected fixture-specific opt-in; musical content of the paired
fixtures matches. Both changed translation units pass Qt 5.15 syntax checks;
git-am replay reproduces the committed tree. Full application playback/export
and updated mtests remain pending: rerun tst_repeat and tst_midi, then check C.
This restores the valid pre-v0.26 traversal behavior; it does not claim the
original MIDI showed a repeat-return retrigger. Deferred audio work and
large-score checks remain unchanged.

## v0.28: continue partial ties forward into C4/C6

The user reports mtests pass after v0.27 and the score's opening chord sounds.
C4/C6 still attack on their first arrivals, though their later repeat returns
tie correctly. These first arrivals have real preceding sources: C1 on the
final pass continues forward into C4, and C3 continues forward into C6. They
must be held continuations, distinct from the score-start C2 with no source.

repeatTieDestinations deliberately exposes only non-linear jump destinations
for notation/editing. Playback had relied exclusively on that list for partial
ties and omitted forward transitions. Add a playback-only forward lookup for
an outgoing partial ending exactly at its measure boundary, with a sounding
source, actual performed destination equal to the immediately following
measure's onset, and a matching incoming partial tie. Retain pitch, unison,
part, written-staff and preferred-voice selection. Factor the existing onset
lookup for reuse; keep the notation destination list jump-only. Incoming
playback queries also inspect the immediately preceding measure and validate
it against the performed timeline and forward tie query. Ordinary ties,
backward jumps, endpoint removal and end-of-score handling retain their paths.
Update collectNote's explanatory comment; no engraving or format changes.

Extend the full-render regression to 16 rows across both source formats,
repeats enabled/disabled, and each incoming/outgoing endpoint enabled/removed.
Remove outgoing tags in an in-memory copy to preserve the incoming-only control.
Expect only the opening C2 attack plus outgoing chord attacks: 21 note-ons
with repeats (versus v0.27's 27), or 12 in linear playback. C4/C6 initial attacks
are removed while their sources extend through performance quarters 18/34.
Verify exact note-off ticks too; suppressing a destination without extending
its source is insufficient. Removing either endpoint restores independent
attacks. Native 4.70 loading keeps v0.27's explicit opt-in.

Local checks execute the actual production repeat-neighbour, onset lookup,
next/previous playback queries and collectNote event-generation core against
the comparison fixture's performed route. The old queries reproduce the
provided original MIDI exactly; the corrected queries remove precisely six
C4/C6 first-arrival note-ons, preserve all other attacks, and extend their
sources. Check endpoint/pitch/staff/part/rest/unison/voice selection, sources
ending early or not sounding, ordinary ties, backward returns and final exit.
Notation/menu destination membership remains unchanged. Real Qt DOM/buffer
preparation checks preserve all incoming ties and remove only requested
outgoing ties in both versions. Changed C++ translation units pass Qt 5.15
syntax checking, and git-am replay reproduces the committed tree. Full mtests,
live playback and export remain pending in the user's build. Next: tst_repeat
and tst_midi, then listen to C2's first note and C4/C6's forward arrivals.
Deferred reverb/font-management and large-score checks remain unchanged.
