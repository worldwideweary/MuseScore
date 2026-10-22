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
