# Optional Studio beam placement — experimental staged backport

Base: Jojo-Schmitz/MuseScore `3.x`, c1fe8b53e863a2a622e8113c040df7d881ee655c.
Reference: musescore/MuseScore tag `v4.7.5`; not `main` or the moving `4.7` branch.

Enable **Format → Style → Beams → Use Studio 4 beam layout (experimental)**.
The score/style XML field is `beamLayout4`. It defaults to false, including for
existing scores without the field. This is a style setting, not a metadata tag
in File → Score Properties. It uses the normal style save/load, reset, undo,
apply-to-parts and extraction mechanisms. No forward-port patch is provided.

## Implemented

- The default path retains Evolution's beam calculations.
- An independent placement path adapts Studio's width and interval limits,
  contour constraints, endpoint stem lengths, inner-stem collision clearance,
  staff-grid alignment, and middle-line/three-beam exceptions.
- Cross-staff placement distinguishes above, below and between-staff beams;
  uses Studio's directional constraints; centres inner beam stacks; and adds
  minimum staff clearance during system layout, respecting fixed spacers.
- Ordinary beams choose the new or legacy placement calculation. Cross-staff
  beams retain Evolution's stem-direction resolution before new placement.
- Since v0.5, automatic direction of supported single-staff pitched beams uses
  Studio's distinct-note outer-pair cancellation rule. Explicit beam direction,
  polyphonic voice direction and explicit chord directions retain priority.
  With the option disabled, Evolution's original direction rule remains.
- User-positioned beams retain their manual positions.
- The new path uses Studio's standard four-beam separation, rather than the
  legacy special four-beam widening. Other existing distance settings remain.
- Stem defaults use Studio's 3.5sp length, short-stem-start position 1 and
  2.5sp minimum when the stored value is the legacy 2.25sp default. Explicit
  non-default minimum-stem settings remain in force; the stored style is unchanged.

## Boundaries still requiring work

This is an experimental placement backport, **not verified complete 4.7.5
engraving parity**. Tablature and beams containing tremolos deliberately retain
legacy placement. Beam grouping, segment construction and
automatic stem-direction selection for unsupported cases retain Evolution's infrastructure. These
can differ from Studio, particularly with mixed subdivisions, rests and complex
cross-staff groups. The shared Studio two-note tremolo engine is not ported.

The latest stage is intended for review and local testing before expanding
those boundaries. Do not describe the whole feature as a finished 4.x engine.

## Validation

C++11 syntax checks were run with real Qt 5.15 headers for the changed engine
translation units, style definitions, style dialog and `tst_beam.cpp`. The style UI was parsed
by Qt uic and the test was processed by Qt moc. A grid-geometry harness passed
7,104 symmetry cases and staff-overlap checks. A separate harness using
simplified score objects exercised 3,000 randomized ordinary/cross-staff cases
for finite coordinates, flattening and repeatability. Those checks do **not**
validate MuseScore object lifetimes, actual score geometry or visual parity.

A full MuseScore application/test build has **not** been run here. The added
`tst_beam` cases are syntax-checked but have not been executed against libmscore:
style serialization, switch/revert stability, manual position preservation,
short-width slope limit and cross-staff repeated-layout stability.

Run from an existing configured MTEST build:

```sh
cmake --build . --target tst_beam
ctest -R '^tst_beam$' --output-on-failure
```

Compare the same score in Evolution with the option off/on and in Studio
4.7.5. Include both stem directions, two-note/wider groups, repeated peaks,
concave contours, mixed beam levels, rests, small/grace notes, moved staves,
manual positions and repeated full layout. Check off/on/off restores legacy
layout, and save/reopen retains the selected setting. Also test Qt 5.9.9.

The default legacy fixtures must continue to pass before merging. A runtime
failure is a blocker for declaring the placement stage ready; the numerical
harness and syntax checks are not substitutes for these tests.

## v0.5 direction checkpoint and next comparison

The D5-B4-A4-G4 group in `Beam-dir.mscx`, measure 4, exposes the
legacy/Studio tie-break difference: D5 and G4 balance around B4. Evolution
chooses down; Studio next compares B4 and A4 and chooses up. The minimal
`studio-direction.mscx` fixture isolates this group. The new policy was compared
with the unmodified v4.7.5 `ChordLayout::computeAutoStemDirection` function for
114,674 combinations of note positions, repeated pitches, order and staff centre.
The changed translation units and added test slots were syntax-checked;
the new application test slots still require a local `tst_beam` run.

The old `studioBeamCrossStaff` test has been renamed `studioBeamCrossMeasure`
to match its actual fixtures. A separate `studioBeamCrossStaff` now loads
`studio-cross-staff.mscx` and asserts that its beam really spans both staves.
It checks finite positions and repeated layout; it does not prove Studio parity.

For grouping and beamlets, open `studio-mixed-levels.mscx` in both versions
and enable the option in Evolution. It contains eight bars: four one-beat rhythm
patterns in both explicit stem directions, with rising, falling, level and
zig-zag contours within each bar. The first two patterns mix eighths, sixteenths
and thirty-seconds with connected third beams. The other two include isolated
third-beam beamlets. Each beat is explicitly joined at the primary beam level;
inspect which inner levels join, where they break, and beamlet side and slope.
This is a visual comparison fixture, not an expected-output automated test.

Further existing comparisons: `Beam-23.mscx` mixes eighths and sixteenths;
`test/fractional_beams.mscz` exercises fractional beams; Studio's
`vtest/scores/beams-3.mscx` and `beams-4.mscx` contain eighth/sixteenth patterns.
None of those alone is a comprehensive thirty-second-note matrix.
Next implementation stage: inner-beam grouping and beamlets, including rests,
forced secondary breaks and tuplets, then beamed tremolos. Tablature stays last.

## v0.6 beamlet-direction checkpoint

The earlier alleged mismatch in `studio-mixed-levels.mscx`, measure 3, was
withdrawn after close-up inspection. Both versions point the second note's
beamlet right and the fourth note's left. `studioBeamletControl` preserves
those directions as a regression check.

The optional path now adapts Studio 4.7.5's `calcIsBeamletBefore`, including
nearest-note searches past rests, boundary/tuplet precedence and rhythmic
subdivision checks. Supporting base-beam-mode and break calculations are local
to the optional engine; the legacy Groups code and legacy segment builder are
unchanged. The new policy runs only for the existing supported modern path;
the option-off path, manual beams, tablature and tremolo-containing beams keep
their prior handling. This stage does not port the full inner-segment/rest
layout engine or change horizontal spacing.

`studio-beamlet-rest.mscx` is a new diagnostic, not a visually confirmed report:
each beat contains a 32nd note, a manually beamed eighth rest, a 64th note,
a 16th note, then another 64th. On the first 64th, the legacy code compares the
adjacent rest (one beam) with the following 16th (two), favouring a right-facing
beamlet. Studio looks past the rest to the preceding 32nd (three), favouring
left. The score contains both stem directions. Compare it with the setting
off/on and in Studio; rest position and horizontal spacing may still differ.
`studioBeamletRestNeighbor` checks the actual new beamlet's left-facing line.

The adapted beamlet decision matched the original v4.7.5 function in 313,890
queries in a simplified-context harness, covering rests, manual break modes,
tuplet boundaries, grace flags, levels and local time stretches. This verifies
decision adaptation in that harness, not full application parity. Engine and
test syntax checks pass; the two added `tst_beam` cases have not been run here.

Next stage: forced secondary/tertiary beam breaks and their interactions with
rests and embedded tuplets. Then beamed tremolos. Tablature remains last among
beam cases. After beam work, investigate a separate legacy/4.x horizontal
spacing switch, beginning with the five-note isolated-32nds example.

## v0.7 inner-break checkpoint

The supported opt-in segment path now uses the same adapted Studio 4.7.5
break calculation as beamlet direction. This includes explicit secondary and
tertiary breaks, manually beamed rests and AUTO embedded-tuplet boundaries.
The legacy branch still calls Groups::endBeam unchanged. This stage does not
replace the rest-continuation/segment traversal with Studio's full builder;
longer runs of rests, nested tuplets and cross-staff mixed segments need further
comparison before that work can be considered complete. Tremolos still use the
legacy fallback.

`studio-inner-breaks.mscx` contains ten 1/4 measures. Measures 1–5 use up stems;
6–10 repeat the patterns with down stems:

1. Four 16ths, secondary break before note 3.
2. Eight 32nds, tertiary break before note 5.
3. Four 16th events, event 2 a rest with a secondary break.
4. Eight 32nd events, event 4 a rest with a tertiary break.
5. A normal 32nd, three triplet 32nds, another normal 32nd, then an eighth
   rest. The triplet occupies a 16th, so its entry/exit breaks only the third
   beam; the primary and secondary beams should remain joined.

`studioInnerBreaks` checks break levels and actual highest-level segment
sharing/separation. It has been syntax checked, but has not been run here in
the application. A simplified-context harness compared 418,000 break decisions
and 313,890 beamlet decisions against original v4.7.5 function bodies, with no
mismatches. This does not verify full score layout or visual parity.

The unused calculateAnchorsCross item parameter is now unnamed, matching the
adjacent unused score parameter. The earlier whitespace-only line is removed;
the full diff against the original base passes git diff --check.

Next: run tst_beam and compare this matrix with 4.7.5, then address remaining
rest-segment traversal differences and nested tuplets before beamed tremolos.
Horizontal spacing and tablature remain separate later work.

## v0.8 beamed-rest clearance checkpoint

The supported modern path now adapts Studio 4.7.5's
BeamLayout::verticalAdjustBeamedRests for single-voice, non-cross-staff beams.
It builds the collision shape from actual segments, subdividing sloped beams
as Studio does, and compares it with the rest shape excluding the saved user
offset. Padding is 0.2sp for 32nd rests and shorter, 0.35sp otherwise. Movement
is away from the beam, rounded up to whole staff-line-distance increments.

Eligible rests are reset with their normal layout before calculating beam
anchors, then adjusted after segment construction. This prevents provisional
or repeated beam layout from accumulating movement. Saved offsets are not
changed. Disabling automatic placement bypasses this adapter. The option-off
path, user-modified beams, TAB, tremolo beams and cross-staff beams keep their
previous path. Multi-voice rests are deferred because Studio additionally
resolves chord/rest and rest/rest collisions, including merged and locked rests.
Leading/trailing rests and interactions with adjacent objects need further
visual comparison; this is not a complete port of Studio's rest layout engine.

Reuse studio-inner-breaks.mscx: measures 3/4 and 8/9 contain beamed rests in
both stem directions. Compare rest position and beam clearance in 4.7.5 and
Evolution with the setting on. studioBeamedRestClearance checks movement
direction, whole-space quantization, repeated beam-only/full layout, saved
offsets, automatic placement disabled, and restoration after switching off.
The application test has been syntax checked but has not been run here.

A standalone harness using the real adapter math matched the original Studio
function bodies for 89,024 beam rectangles and 60,000 clearance queries;
120,000 movement checks passed for direction, sufficient clearance and minimum
whole-space rounding. Source/test syntax checks including unused-parameter
warnings pass, as does the full whitespace check against the original base.
These checks do not establish full application parity.

Next: run tst_beam and compare the rest positions, then continue with remaining
rest-segment traversal and nested-tuplet cases before beamed tremolos.

## v0.9 visual corner-case pack

Adds studio-corners-rests.mscx (20 measures), studio-corners-tuplets.mscx
(12 measures), studio-corners-contours.mscx (10 measures), and the accompanying
studio-corners-guide.md. All cases repeat with up/down stems and carry short
labels. Explicit system breaks limit each system to two measures.

Coverage: leading/trailing/consecutive rests; mixed rest lengths; forced breaks
before/on/after rest runs; embedded, nested and adjacent tuplets; a quintuplet;
sloped-beam rest clearance; mixed 64ths and breaks at multiple beam levels.
All measure durations and tuplet spans pass exact rational checks, and XML
parsing passes. No engine or automated-test changes; no application rendering
or tst_beam run is claimed here. These are diagnostic inputs, not reference
outputs asserting parity. See studio-corners-guide.md for the comparison map,
interaction checks, known coverage gaps, and recommended order.

Next: use the supplied scores to confirm any remaining rest-traversal or
nested-tuplet discrepancy, then address that behavior in a separate engine
checkpoint before beamed tremolos.

## v0.10 consecutive-rest segment endings

Fixes the confirmed R7/R10 differences in both stem directions. At an inner
segment break, the enabled modern path now backs over the entire trailing rest
run to the last chord, rather than only one rest. The legacy branch is retained.
A single remaining chord produces a normal beamlet instead of a segment ending
on a rest. This is a targeted endpoint correction, not a full segment-builder
replacement. Leading/trailing runs at the end of an unbroken beam retain their
prior handling.

studioRestRunSegmentEnds checks all four R7/R10 measures: the first chord's
highest beam is a right-facing beamlet of the configured length, and its segment
does not extend over the following rest. Engine/test syntax checks pass; the
application test still needs running. Compare the same v0.9 rest score again.
Next checkpoint: nested-tuplet layout order and skyline participation.

## v0.11 nested-tuplet stacking

With beamLayout4 enabled, non-cross-staff tuplet trees are laid out once per
system pass, recursively visiting children in reverse order before their
parent, following Studio 4.7.5's layoutTupletAndNestedTuplets. A processed-root
set prevents later inner-tuplet starts from registering the same shapes again.
Nested tuplets and their enclosing tuplets participate in automatic skyline
placement even when outOfStaff was suppressed for anchor calculation. The
existing automatic-placement and add-to-skyline controls still apply.

The option-off traversal and placement condition are retained. Cross-staff
tuplet placement is deferred. This stage adapts hierarchy order and skyline
participation, not the complete Studio tuplet bracket or direction algorithms.
Non-nested tuplets keep their existing outOfStaff placement condition.

studioNestedTupletStacking loads the v0.9 tuplet score and checks T3/T4 in both
stem directions: outer/inner numbers separate vertically; repeated full layout
is stable; disabling the style restores legacy outer positions. Production and
test syntax checks pass, and the full base-to-current whitespace check passes.
The application tests have not been run here and visual parity is unverified.

Apply v0.10 after v0.9, then v0.11 after v0.10. Run tst_beam. Compare R7/R10
(up: measures 7/10, down: 17/20) in studio-corners-rests.mscx and T3/T4
(up: measures 3/4, down: 9/10) in studio-corners-tuplets.mscx. Also check T1/T2,
T5/T6 for accidental placement changes. Next: confirm these two checkpoints in
the application, then continue into beamed tremolos; remaining multi-voice and
cross-staff rest cases and horizontal spacing remain separate work.

## v0.12 nested-tuplet test correction

The supplied Qt 5.9.9 result.xml reports one failure, in the number-separation
assertion of studioNestedTupletStacking. studioRestRunSegmentEnds and
studioBeamedRestClearance pass. The supplied screenshots show short R7/R10
beamlets and outward stacking of the nested tuplet numbers/brackets.

The old test required disjoint vertical projections of the number bounding
boxes, even when they are horizontally separated. That is stronger than the
2D skyline collision rule. Replace it with outward ordering of the number
centers plus non-intersection of the full number rectangles. Collapsed, inward
or overlapping number placement still fails. Failure messages now report both
rectangles and the stem-side flag. Repeated-layout and option-off restoration
checks remain in place.

Only the test and these notes change; engraving code is unchanged. Test syntax
and full base-to-current whitespace checks pass. The corrected application
test has not been run here; rebuild tst_beam and rerun the suite. A remaining
failure with the new coordinates would require further geometry investigation.

## v0.13 tremolo comparison inputs

The user reports tst_beam passing through v0.12. Adds a 20-measure single-note
tremolo matrix (1–4 strokes, interior notes, mixed levels, slope, chord, buzz
and small-note controls) and a 10-measure two-note matrix (2–4 inset strokes,
slope and standalone control). Both stem directions are explicit. See
studio-tremolo-guide.md for the measure map and comparison order.
All measures balance using exact durations; two-note XML uses written durations
twice the actual durations, following existing Evolution fixtures. These files
have not been opened/rendered here. This checkpoint changes diagnostics only.
Next: ordinary full-size single-note tremolo stems and placement, then inset
two-note tremolo anchors. Buzz/grace/small/cross-staff cases remain separate.

## v0.14 ordinary single-note tremolos in beams

With beamLayout4 enabled, full-size, non-grace single-note tremolos with 1–4
strokes now use the optional beam engine on unmodified non-cross-staff pitched
beams with normal staff-line distance. The whole beam's tremolo content is
checked consistently for direction, anchors and final tremolo placement. A
const Beam::elements accessor supports that read-only check.

Stem minima adapt the beamed single-tremolo branches of Studio 4.7.5
StemLayout::calcMinStemLength, including tremolo height, outside-staff offset
and the beam stack. Padding uses Studio defaults (1.25sp note side, 0.5sp outer
side); Evolution has no equivalent saved padding properties. Existing stroke
width/distance settings are retained. The minimum participates in endpoint
anchors and inner-chord collision checks. Final single-tremolo placement adapts
TremoloLayout::layoutOneNoteTremolo after the final beam stem is set. Evolution's
stem X coordinate is already the center line, unlike Studio's edge coordinate.
User tremolo offsets remain separate from the calculated layout position.

Two-note tremolos, buzz rolls, 5+ stroke tremolos, grace/small tremolo chords,
manual beams, cross-staff beams, TAB and nonstandard staff-line distances retain
the prior fallback. Standalone single-note tremolos also keep their prior path.
The option-off branch is retained. This is an initial single-note adapter, not
complete tremolo engraving parity or an inset two-note tremolo implementation.

studioSingleTremoloSwitch checks supported/fallback classification, finite
positions, beam clearance for flat single-level examples, repeated layout and
legacy restoration after switching off. studioTwoNoteTremoloFallback checks the
new two-note fixtures connect both chords and retain their tremolo positions
when the option is enabled. These application tests have only been syntax
checked here and need a real tst_beam run.

A simplified full-size beamed-context harness compared the actual adapted
minimum-stem and placement functions with original Studio 4.7.5 function bodies
in 10,000 contexts, including both directions, narrow/wide beam grids, varying
stroke counts, staff positions, scales and stem lengths; all matched. This
verifies the isolated decisions, not full score-layout parity. Production/test
syntax and full base-to-current whitespace checks pass.

Next: compare S1/S4/S5/S6 in both directions, then the slope and chord cases;
verify S9/S10 stay unchanged. D1–D5 provide a baseline for the next stage,
inset two-note tremolo anchors. Remaining corner cases and horizontal spacing
are still separate later work.

## v0.15 inset two-note tremolo span

The enabled score style now replaces the 0.62 proportional horizontal stretch
for ordinary default-style two-note tremolos within the same unmodified,
non-cross-staff beam. Stroke endpoints use Studio 4.7.5's outside-stem anchors
and 1sp gaps. For close anchors, gaps shrink toward 0.4sp to retain a 0.6sp
stroke where possible. Degenerate spans fall back to the prior path.
The modern span has no legacy 12sp maximum. A centered stroke path is stretched
to the new width; saved offsets remain separate. Full-size, non-grace pitched
pairs with matching stem directions in the same system are supported here.

This is a horizontal-span stage only: beam layout, vertical tremolo placement,
stroke spacing and tilt remain on their previous path for two-note tremolos.
Standalone tremolos, other stroke styles, manual/cross-staff beams, TAB,
small/grace pairs and opposite-stem pairs retain their prior span calculation.
The option-off calculation is retained. No new user-facing style control is
added; the old internal multiplier remains available for legacy layout.

studioTwoNoteTremoloFallback is renamed studioTwoNoteTremoloSpan and extended
to check actual endpoint gaps in D1–D4, unchanged vertical positions, unchanged
standalone D5, stable repeated layout and legacy width/position restoration.
The test has been syntax checked but has not been run in the application here.
The actual span helper matched original Studio gap calculations for 40,004
width/scale queries, allowing the intentional nonnegative-span guard. Production
and test syntax, whitespace and patch application checks pass.

Reuse studio-tremolo-two-note.mscx in both applications, especially D1/D3 and
both stem directions. D4 remains a flat-beam contour control despite its old
"slope" label; a genuine sloped fixture is still needed for the later full
anchor/tilt adapter. Next: verify spans, then continue two-note vertical anchors
and the remaining beam corner cases before horizontal spacing work.

## v0.16 standalone two-note tremolo span

Extends v0.15's fixed-gap horizontal span to ordinary standalone two-note
pairs: both stems must exist, point the same way, be on the same unmoved staff
and system, and belong to full-size non-grace pitched chords using the default
stroke style. Pairs attached to different beams remain on the prior path.
The same outside-stem anchors and adaptive 1sp gaps apply. Vertical position,
stroke spacing and tilt remain on the previous path. Option-off behavior is
retained. Stemless whole-note pairs, opposite stems, cross-staff, small/grace,
TAB and other stroke styles still need separate adaptations.

studioTwoNoteTremoloSpan now checks the actual 1sp endpoint gaps for standalone
D5 as well as inset D1–D4, in both directions. Repeated-layout and option-off
restoration checks remain. Production and test syntax checks pass; the updated
application test has not been run here. The unchanged span helper retains its
v0.15 numerical verification. Reuse D5 (measures 5 and 10) for visual comparison.
Next: genuine rising/falling inset and standalone inputs for vertical-anchor
comparison; the old D4 contour is a flat-beam control.

## v0.17 vertical-anchor diagnostic

Adds studio-tremolo-anchors.mscx and its measure map in studio-tremolo-guide.md.
Twelve measures repeat six cases in both stem directions: rising/falling inset
pairs, rising/falling standalone pairs, a flat four-stroke control and a rising
mixed-beam-level inset pair. Inset pitch contours are strictly monotonic to
avoid the peak that flattened the old D4 case. Exact-duration checks pass for
all twelve measures and XML parsing passes. The file has not been rendered in
either application here; first confirm that V1/V2 visibly slant.

No engraving or application-test changes in this checkpoint. Apply v0.16 then
v0.17 after v0.15. Run tst_beam and compare D5 for the standalone width change,
then V1–V6 against 4.7.5 for the next vertical-anchor/tilt adaptation. Two-note
vertical anchors, tilt and stroke spacing are not changed by these stages.

## v0.18 inset two-note tremolo anchors

Supported ordinary inset pairs now participate in the optional Studio beam
solver. Their tremolo strokes contribute to the solver's beam count and stem
requirements without drawing extra ordinary beams. After both final stems are
laid out, the tremolo follows the completed primary beam, offset by the ordinary
beam stack, with Studio's narrow/wide stroke grid. Both chord segment shapes
are refreshed afterward. Inset endpoints now use stem centers, correcting the
outside-edge assumption from v0.15; standalone endpoints retain outside edges.

The existing content guards remain: full-size, non-grace pitched default-style
pairs in one measure, shared unmodified non-cross-staff beam, matching ordinary
beam counts, standard staff spacing and at most four tremolo strokes. Unsupported
beams and the disabled score style retain the previous calculation. Standalone
vertical anchors and tilt are still pending. V6 is not forced flat: its slope
comes from the enclosing beam solver with the inset stroke counts included.

studioTwoNoteTremoloSpan now distinguishes inset stem-center endpoints from
standalone outside edges. studioInsetTremoloAnchors checks V1/V2/V6 in both
directions, interpolation, stroke spacing, repeated layout and option-off
restoration. C++11 production/test syntax checks pass with unused-parameter
warnings enabled. An isolated harness using the actual path adapter and real
Qt paths passed 5,000 geometry/repeat cases. Full tst_beam and visual comparison
remain for the application build; the isolated harness does not establish full
Studio solver parity. Next: compare V1/V2/V6, then adapt standalone V3/V4 anchors.

## v0.19 standalone two-note tremolo anchors

The user reports tst_beam passing through v0.18, and the supplied V1/V2/V6
images look broadly consistent with Studio. Standalone V3/V4 still showed
shallower slopes in Evolution. This stage shares the existing optional anchor
solver between beams and supported standalone tremolos. Standalone counts use
only tremolo strokes; stem defaults, shortening, target overlap and grid/slant
rules use that count. The completed anchors determine both stem tips and the
stroke polygons, with outside-edge endpoints and the existing adaptive gaps.
Stroke thickness and narrow/wide spacing follow the regular beam style.

Support is limited to default-style, full-size non-grace pitched pairs with
one to four strokes, same-direction stems, no ordinary beams or flags, one
measure and unmoved staff, standard line spacing and unmodified stem lengths
and offsets. Other cases retain the prior path. Saved tremolo offsets remain
separate from the staff-grid calculation. Both segment shapes are refreshed.
The score style remains default off; horizontal spacing is unchanged in scope.

studioTwoNoteTremoloSpan no longer requires the standalone vertical position
to remain legacy. New studioStandaloneTremoloAnchors covers V3/V4/V5 in both
directions: endpoint gaps, outer-stroke/stem alignment, contour direction, flat
control, repeated layout, manual-stem bypass and legacy position/path/stem
restoration. C++11 production/test syntax checks pass with unused-parameter
warnings enabled. An isolated harness using the actual adapters and Qt paths
passed 10,000 standalone geometry/stem/repeat cases and 10,000 comparisons of
the shared ordinary-beam solver against v0.18. These are isolated-context checks;
the full tst_beam and visual comparison remain for the application build.

Next: compare V3/V4 up/down with Studio 4.7.5, using V5 as the flat stack control
and V1/V2/V6 as inset regression controls. Then expand the tremolo corner-case
inputs before tackling unsupported stem arrangements or horizontal spacing.

## v0.20 tremolo corner diagnostic pack

The user reports mtest passing through v0.19. Enabled V1/V2/V6 and standalone
V3/V4/V5 up/down captures look broadly consistent with the earlier Studio
4.7.5 captures. The apparent V6 regression was a capture with the experimental
score style disabled; no code correction was needed.

Adds studio-tremolo-corners.mscx (K1-K12 in both directions, 24 measures),
studio-tremolo-tight.mscx (two dense 6/4 measures with 24 inset pairs each),
and studio-tremolo-corners-guide.md. Cases cover one to four strokes, complete
chords, low/high ledger notes, inset stacks and short spans. Both new scores
start with beamLayout4 enabled and lastSystemFillLimit zero to avoid misleading
legacy captures and compressed final systems. Existing fixtures are unchanged.

No engraving code or application tests change. XML parsing, exact measure sums,
pitch spelling, tremolo pair connections, counts and style settings pass local
checks. Loading/rendering and visual comparison remain for the applications.
Next: compare K4/K5/K7/K8, then K9-K12 and dense Q1/Q2; isolate discrepancies
before extending unsupported arrangements or starting horizontal spacing work.

## v0.21 flagged standalone tremolos

The supplied K1-K12 up/down and Q1/Q2 captures look broadly consistent with
Studio 4.7.5. This stage admits same-direction standalone pairs with up to four
traditional flags to the optional tremolo anchor solver. Stem defaults follow
Studio's flag branches: no beam-count addition to the normal length, ordinary
2sp overlap, reduction-count zero with the traditional-flag cap of one quarter
space, no optical shortening and no unflagged four-beam exception. Unflagged
and beamed defaults retain their calculations. Flags are repositioned at the
completed stem tips before refreshing both segment shapes. Unsupported cases
and the disabled score style retain the prior path.

Adds studio-tremolo-flags.mscx and studio-tremolo-flags-guide.md, with F1-F8 in
both directions: eighth to sixty-fourth flags, one to four tremolo strokes,
ledger notes, chords and a flat control. All sixteen measures have exact 1/4
duration and paired written/actual lengths; ordinary beams are explicitly off.
The score begins with beamLayout4 enabled. Straight flags are not implemented.

New studioFlaggedTremoloAnchors checks fixture connectivity and flags, modern
span, finite stems/path, final flag attachment, repeated layout and legacy
stem/flag/path restoration. Production and test C++11 syntax checks pass with
unused-parameter warnings enabled. An isolated Qt harness passed 10,000
standalone cases (5,000 flagged), including flag-tip attachment and repeat
checks, plus 10,000 ordinary/inset-beam comparisons against v0.20. These are
simplified-context checks, not full rendering or full Studio parity. Full
tst_beam and visual comparison remain for the application builds.

Next: compare F1/F2/F4/F7 up/down, check the previous V/K inputs for regressions,
and isolate any differences before adapting opposite-stem or cross-staff pairs.

## v0.22 two-note tremolo flag suppression

The user reports tst_beam passing through v0.21, but the F1-F8 captures expose
a missed Studio rule: v4.7.5 src/engraving/dom/chord.cpp, Chord::shouldHaveHook(),
explicitly requires !tremoloTwoChord(). Thus the v0.21 flagged-stem branches
were the wrong branches for these two-note tremolos. This stage removes them
and applies the existing unflagged tremolo defaults to short written durations.

Chord::layoutStem suppresses flags on pitched two-note tremolos only when the
experimental score style is enabled. The disabled style restores legacy flags;
TAB, ordinary notes and single-note tremolos retain their prior flag handling.
Suppression follows Studio for pitched two-note tremolos generally, even where
the modern anchor adapter remains deferred. The standalone adapter no longer
requires hooks on written eighths and shorter, and its obsolete flag attachment
step is removed. No score inputs change; reuse studio-tremolo-flags.mscx.

studioFlaggedTremoloAnchors now requires absent modern flags, stable repeat
layout and restored legacy flags, stems and tremolo paths. C++11 syntax checks
pass for chord.cpp, beamlayout4.cpp and tst_beam.cpp with unused-parameter
warnings enabled. The isolated Qt harness passes 10,000 standalone cases
(5,000 with short written durations in flag-suppressed contexts) and 10,000
ordinary/inset-beam comparisons against v0.21. Full tst_beam and visual
verification remain for the application build. Next: compare the same F cases,
especially F2/F5/F6/F7 where corrected stem defaults can change placement,
before proceeding to opposite-stem and cross-staff tremolos.
