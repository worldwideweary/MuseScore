# Horizontal spacing baseline — v0.29

This diagnostic checkpoint begins the spacing investigation after the beam-angle
milestone. It adds scores only: no spacing algorithm, style toggle or build
changes. Apply its source patch after v0.28, or open the supplied scores directly
in the current Evolution build. There are no tremolos in any input.

Open each same original file in Evolution and Studio **4.7.5**. Do not save one
application's converted version over the input before testing the other. The
beamLayout4 setting starts enabled in Evolution; Studio ignores that custom
setting. The future spacing setting will be separate and default off. It does
not exist yet.

Both inputs explicitly specify Leland/Leland Text, 1.75mm spatium, stem width
0.10sp, minimum note distance 0.35sp, bar-to-note distance 1.25sp, note-to-bar
distance 1.5sp, measureSpacing 1.5, accidental distance 0.25sp, accidental-to-note
distance 0.25sp and dot-to-note distance 0.5sp. These test-only overrides reduce
default-style differences; they do not change the application's defaults.
The applications may migrate styles on import, so record any visibly changed
font/style setting rather than assuming it stayed fixed. Other style defaults
and import behavior can still differ.

## Main score: studio-spacing-baseline.mscx

Eight 4/4 measures, one measure per system. All systems fill the width. Labels
are system text, so keep them in the captures. Start with H1-H3 and H5.

| Case | Material | Primary comparison |
|---|---|---|
| H1 | Eight repeated eighths, stems up | Equal interior notehead-center and stem gaps |
| H2 | Eight equal eighths with wide contour, stems up | Contour-dependent excess spacing |
| H3 | H2 pitches with stems down | Direction-dependent padding and alignment |
| H4 | Sixteen sixteenths, changing accidentals | Collision clearance without irregular rhythmic gaps |
| H5 | Quarter, eighth, eighth, sixteenth, sixteenth, eighth, quarter | Rhythmic proportions across different note values |
| H6 | Eighth-note chords containing seconds | Mirrored noteheads and required collision space |
| H7 | Upper quarters against lower eighths, two voices | Shared beat alignment and multi-voice constraints |
| H8 | Four groups of eighth-note triplets | Uniformity within each triplet and spacing between beats |

Measure gaps between notehead **centers**, and also between stem center lines.
Do not use the outer left edge of an entire chord as its rhythmic position:
mirrored noteheads can legitimately extend left. Equal-duration notes can need
unequal gaps when accidentals or other collisions require extra space. The
objective is consistent rhythmic spacing subject to those constraints.

Full-width systems make within-measure proportions easy to compare but conceal
overall natural-width differences. Use the short controls below for that.

## Natural-width controls

`studio-spacing-natural-h1.mscx`, `studio-spacing-natural-h2.mscx`, and
`studio-spacing-natural-h5.mscx` each contain just the corresponding 4/4 measure,
a time signature, and a final-system fill threshold of 1. They are intended to
remain short/unjustified at the supplied page size. If an application stretches
one to the full width, report that behavior rather than treating its width as
natural. These controls compare total note-group width and interior gaps.

## Cross-staff score: studio-spacing-cross-staff.mscx

Four 4/4 measures on a two-staff piano, one measure per system. Each measure has
eight ordinary eighth notes, with no tremolos. The other source staff contains
a hidden full-measure rest.

| Case | Source staff | Pattern |
|---|---|---|
| H9 | Upper | Two-note blocks alternate between displayed staves |
| H10 | Upper | Every note alternates displayed staff |
| H11 | Lower | Two-note blocks alternate between displayed staves |
| H12 | Lower | Every note alternates displayed staff |

Compare the gaps at each staff/stem-direction transition with gaps within one
staff. Record both stem and notehead-center distances: the two can differ when
stems switch sides. Keep the full piano system visible. Staff distance and beam
slope are secondary observations here; the primary target is horizontal rhythm
and collisions.

## Source audit and implementation boundary

The pinned reference is Studio v4.7.5:
[horizontalspacing.cpp](https://github.com/musescore/MuseScore/blob/v4.7.5/src/engraving/rendering/score/horizontalspacing.cpp).
Evolution's relevant starting points are Measure::computeMinWidth,
Measure::stretchMeasure, and Segment's horizontal-distance/shape calculations.

Studio computes duration stretch from a quarter-note reference using
`pow(measureSpacing, log2(duration / quarter))`, including a separate
polyrhythm branch. It derives a natural width from 3.5sp times that stretch and
spacing density, then spaces and justifies at the system level. Evolution's
measure stretch currently uses a logarithmic factor relative to the shortest
segment in the measure. Those formulas have different roles in their respective
pipelines; replacing one expression is not a complete spacing port.

Studio also has explicit cross-beam width corrections involving stem direction,
notehead width minus stem width, opposing beamlets, and other-voice constraints.
These are promising candidates for the observed unequal gaps, but their role in
any specific screenshot is not confirmed until the matched-input comparison.
Do not force every visible gap to be equal, or adjust beam slopes to hide a
spacing discrepancy.

Implement future changes behind an independent per-score spacing style, false
by default, with the legacy pipeline intact. The existing beamLayout4 setting
must not silently change which spacing algorithm is used. Start with a bounded
rhythmic/cross-beam change and verify repeat layout, score reload and legacy
restoration before expanding to general collision and system fitting behavior.

## Validation and next action

All five XML scores parse; every source voice totals exactly 4/4, including the
3:2 tuplets; staff destinations are valid; labels and case counts are present;
no tremolo elements exist. The files have not been loaded or rendered locally,
so application import and visual results remain pending. No automated rendering
or engraving-parity claim is made by this fixture-only checkpoint.

Next compare H1/H2/H3/H5, the three short controls, and H9/H10. H4/H6/H7/H8 and
H11/H12 then provide collision, voice, tuplet and reverse-ownership controls for
the first spacing implementation. Text-autoplace and unsupported tremolo corner
cases remain separate work.

## v0.30: first implementation — final-system justification

Apply the v0.30 incremental patch after v0.29 and rebuild. The independent,
default-off per-score XML style is `spacingLayout4`. Enable it at
**Format → Style → Page → Use Studio 4 horizontal spacing (experimental)**,
next to the final-system fill threshold. The reset button turns it off. It is
independent of the experimental beam setting and persists in the score.

This first stage changes only the decision to justify a short terminal or
section-ending system. Below the fill threshold, it adds no extra justification,
following Studio 4.7.5 SystemLayout::shouldBeJustified. Equality with the threshold
still justifies, as in Studio. A following measure/HBox requires filling; frames
and their section breaks are considered while looking ahead. The existing
no-horizontal-stretch override still wins. Off retains Evolution's exact prior
residual-space calculation, including its partial expansion of short systems.
The underlying natural widths still come from Evolution's existing spacing
pipeline; exact Studio widths and rhythmic proportions are not promised yet.

For the visual check use **studio-spacing-natural-h1.mscx**,
**studio-spacing-natural-h2.mscx**, and **studio-spacing-natural-h5.mscx**.
Use these short single-measure files in both applications, rather than comparing
a short control with the full-width H1-H8 overview. They already set the fill
threshold to 100%. In Evolution toggle the new spacing checkbox off/on: on should
remove the extra expansion, and off should restore the old width. Repeating
layout must be stable. Setting the fill threshold to 0% should make the final
system fill again. Beam angles need no adjustment for this comparison.

New studio-spacing-final-line.mscx and studio-spacing-final-section.mscx each
contain two short measures separated by the named break. At 100% threshold with
spacing on, the line-break case still fills its nonterminal first system; the
section-break case leaves both short systems unexpanded. The final-frame input
checks a short measure followed by a vertical frame. These inputs retain the
same shared style values and contain no tremolos.

Adds **tst_spacing** alongside tst_benchmark in the existing layout test
subdirectory. Build the new target (rerun CMake if necessary), then run:

```
ctest -R '^tst_spacing$' --output-on-failure
```

It checks default-off behavior, on/off score roundtrip, independence from the
beam setting, H1/H2/H5 final-system widths, threshold zero, repeat stability,
legacy restoration, nonterminal line breaks, terminal section breaks and a
trailing vertical frame. Local C++11 syntax checks pass for style.cpp, layout.cpp,
editstyle.cpp and tst_spacing.cpp; Qt uic/moc generation succeeds. An isolated
harness running the actual new branch matches the pinned Studio decision in
100,000 boundary/threshold/frame/no-stretch contexts and preserves the legacy
residual-space result when disabled. Full tst_spacing and application rendering
remain pending on the user's build. No rhythmic-curve or cross-beam spacing
correction is introduced yet; those are the next stages.

## v0.31: horizontal control independent of vertical justification

The user identified the cause of the apparently unresponsive v0.30 checkbox:
it was accidentally parented inside enableVerticalSpread. When that checkable
group was off, Qt disabled the horizontal checkbox and its reset button. The
explicit ON input still loaded and laid out correctly, which explains why the
layout/roundtrip tests passed while the dialog could not change the setting.

Move both controls into the outer Page grid beside/below the system-fill
threshold, outside both vertical-justification groups. Restore the nested
vertical group's original layout. No layout algorithm, style binding or XML
property changes. Vertical justification can remain disabled while horizontal
spacing is enabled, disabled or reset.

Qt uic generation and editstyle.cpp C++11 syntax checks pass. An isolated Qt
widget check generated from the actual Page subtree verifies that the checkbox
and reset remain enabled with either vertical mode selected and that checkbox
clicks change state and emit toggles. The test substitutes standard QGroupBox
for the paint-only RadioButtonGroupBox subclass; it is not a full application
run. Application build and live-dialog verification remain for the user's build.
Apply v0.31 after v0.30. The v0.30.1 ON/OFF probe was diagnostic data only, so
there is no intervening code patch. Existing tst_spacing covers the unchanged
engraving path. Next resume rhythmic spacing after confirming the dialog toggle.

## v0.32: duration-based natural widths and rhythmic springs

The user confirms v0.31 live toggling works with vertical justification disabled.
H2/H5 short controls are substantially closer to Studio, while H5 proportions and
H8 triplet uniformity are useful next targets. This checkpoint extends the same
independent spacingLayout4 option; default-off behavior remains legacy.

Adapt Studio v4.7.5 HorizontalSpacing::computeSegmentDurationStretch and
chordRestSegmentNaturalWidth. The quarter-note rhythmic reference is 3.5sp.
Duration weight is pow(measureSpacing, log2(duration / quarter)). When an onset
subdivides a longer chord/rest, use Studio's proportional shortest-duration rule,
including its preceding-onset refinement. Evolution's actualTicks supplies tuplet
ratios/local time stretch. The shortest-onset scan uses the available Evolution
staff/voice visibility API; advanced invisible-voice semantics are not a full
port of Studio's considerInvisible overload.

In ordinary measures, natural chord/rest segment width is the greater of the
rhythmic width and Evolution's collision minimum. Existing lookback collision
checks remain. The same rhythmic weight drives the existing measure spring
solver. Per-measure user stretch is clamped to 0.1..10 and applied once to those
widths/weights; suppress the old whole-measure measureSpacing multiplier under
the experimental option to avoid double stretching. Multi-measure rests keep
their prior rhythmic pipeline. No global style defaults are changed: the stored
Spacing value now controls the modern curve's slope when the option is on.
The supplied inputs explicitly use 1.5; custom values remain honored. Do not
reduce minimum note distance to zero to emulate the new pipeline.

This is an incremental rhythmic adapter, not the entire Studio spacing engine.
Evolution's collision geometry and distribution of width among measures in a
system remain. Cross-beam width corrections, cue-specific spacing, advanced
invisible voices, accidental/voice packing, and other kerning changes are
separate work. Beam layout remains independent. Final-system justification from
v0.30 and the UI-parent fix from v0.31 remain in place.

Added studio-spacing-natural-h8.mscx contains the same four eighth-note triplets
as overview H8, in a single short 4/4 measure at the 100% fill threshold. Its XML,
12 chords, four 3:2 tuplets and exact 4/4 sum are checked. It starts with the
spacing option off, like the existing comparison files. Open the same original
in both applications, enable experimental spacing in Evolution and compare
natural H5/H8 plus overview H5/H8. Recheck H1-H3, H4/H6/H7 and H9-H12 for collision
or alignment changes. Exact overall widths can still differ due to the retained
parts of Evolution's spacing pipeline.

Extend tst_spacing with duration ratios, triplet ticks, non-adjacent-onset cases,
the H8 natural control, H1-H8 natural-width floors/weights, H5/H8 justified spring
proportions, repeat stability and restoration of every saved legacy segment
position/width. Existing roundtrip, threshold and section/frame checks remain.

Local validation: C++11 syntax checks for measure.cpp and tst_spacing.cpp, plus
Qt moc/uic generation, pass. An isolated helper check matches the original pinned
Studio functions in 100,000 rhythmic/non-adjacent-onset contexts. A real-Qt harness
running the extracted actual old/new spring-loop code preserves exact legacy
positions/widths in 10,000 contexts with spacing off; 10,000 modern contexts
retain collision floors and meet their target width. These isolated checks do
not test full score import/layout. Full tst_spacing, the existing beam suite and
application rendering remain for the user's build. Next: compare H5/H8 results,
then address cross-staff stem-direction spacing corrections in a separate stage.

## v0.33: preserve cross-beam stem-side spacing during justification

The user reports tst_spacing and tst_beam passing after v0.32. Natural H5 and
the justified H5 rhythmic proportions visually agree closely with Studio 4.7.5.
Overview H8 is already uniform: equal-duration springs can give the same final
positions under either algorithm when justified to the same system width.
The earlier suggestion that this overview needed improved uniformity was wrong.
Use natural H8 to compare the unstretched width instead.

H9-H12 comparisons expose a separate difference: Evolution has almost equal
notehead gaps, whereas Studio alternates those gaps as stem direction changes.
Adapt the signed displacement in Studio v4.7.5 applyCrossBeamSpacingCorrection:
notehead width minus stem width, added for up-to-down and subtracted for
down-to-up. Keep this displacement fixed during spring stretching, as Studio
does with its segment widthOffset. Otherwise justification would erase it.
The existing collision floor is never reduced. An up-to-down transition also
retains Studio's minimum width of twice the displacement.

This first adapter is deliberately limited to one visible stream of ordinary,
normal-size eighth-note chords, in the same cross-staff beam containing more
than two elements. Both chords must end at their next segment boundary. Skip
competing visible voices, rests other than gap/full-measure rests, cue sizes,
two-element beams and secondary beams/beamlets. Those cases retain v0.32
spacing for now. Invisible elements and hidden source staves do not compete.
This is not the complete Studio cross-beam collision/kerning implementation.

The transient Segment offset is recomputed, not serialized, and starts at zero
for a cloned segment. It is ignored by the spring solver with spacingLayout4
off. Beam layout and spacing remain independent; tests cover both beam styles.
No score files or default style values change in this checkpoint.

Extend tst_spacing with H9-H12's signed offsets and justified gap proportions,
repeat-layout stability, finite widths and exact restoration of saved legacy
segment positions and widths after disabling the option. Reuse the existing
studio-spacing-cross-staff.mscx for visual checks: compare all four systems,
especially the alternating gaps in H10/H12. Keep the same style values in both
applications, and compare at matching staff scale rather than screenshot width.

Local C++11 syntax checks for measure.cpp, segment.cpp and tst_spacing.cpp,
plus Qt moc generation, pass. A harness running the actual extracted adapter
passes all 32 H9-H12 boundary cases and its visibility/voice/size/duration/beam
guards. The actual extracted spring loops preserve exact legacy positions in
10,000 contexts with the option off; 10,000 signed-offset contexts preserve
collision floors and meet target width. Full tst_spacing/tst_beam and rendering
remain for the user's build. Apply v0.33 after v0.32. Next compare H9-H12, then
test opposing beamlets and competing voices before expanding support.

## v0.34: fix cross-beam correction before final stem layout

The user's Qt 5.9.9 report for v0.33 has two failures: crossBeamSwitch's
legacy-beams and studio-beams rows both fail the signed-offset assertion.
All other test cases pass. The debug messages in the report are not additional
test failures.

The adapter incorrectly assumed Chord::up() was final during spacing.
Beam::layout1 sets provisional directions (including the source-staff chords),
while Beam::layout later resolves cross-staff directions around the beam's
vertical position. Thus opposing stems visible in the finished score can still
have equal provisional directions when computeMinWidth needs their offset.
The v0.33 mock harness supplied final directions and missed this ordering.

For the supported automatic case, infer the upper/lower stem-side transition
from destination staff indices rather than provisional up(). Limit this to two
adjacent destination staves, an unedited Auto beam and Auto chord directions.
If upper-staff notes extend below their staff, or lower-staff notes extend above
theirs, keep the existing fallback because final beam geometry may change the
split. Use the destination staff's line count. Do not change any stem directions
or run final beam layout early. Fixed offsets and the spring solver are unchanged.

The existing application test still requires the same nonzero H9-H12 offsets;
its expectations are not relaxed. Improve its failure message to include the
H label, note number, actual offset and expected offset. An extracted-code
harness now checks all H9-H12 boundaries under every provisional stem-direction
combination (8,192 cases), plus the existing guards and the new edited-beam,
explicit-direction and interstaff-gap fallbacks. All pass locally. C++11 syntax
checks for measure.cpp and tst_spacing.cpp pass. Full application tests still
require the user's build.

Apply v0.34 after v0.33. Run tst_spacing and tst_beam, then compare the unchanged
studio-spacing-cross-staff.mscx, especially H10/H12. This checkpoint fixes the
layout-order error; additional beamlet/voice support remains deferred.

## v0.35: cross-staff beamlet and competing-voice comparison inputs

The user reports tst_spacing/tst_beam passing after v0.34. H9-H12 screenshots
show the intended signed-gap pattern with spacing on, close to Studio 4.7.5;
spacing off retains the legacy nearly equal gaps. Keep that implementation.
This stage adds inputs/tests only; no layout rules change.

Open studio-spacing-cross-staff-corners.mscx in both applications. It contains
six labeled 2/4 measures, one per system, with the matched styles used in the
earlier comparisons. The spacing option starts off; enable it in Evolution.
Keep the same beam setting for an initial comparison (the input enables modern
beams). Tests also exercise legacy beams independently.

| Label | Contents | Visual focus |
| --- | --- | --- |
| H13 | Upper-source eighth/sixteenth/sixteenth pattern, repeated; forced secondary breaks | Short secondary beams at notes 2-3 and 5-6, especially the lower-to-upper transition |
| H14 | Lower-source mirror of H13 | The reverse transition and beamlet clearance |
| H15 | Eight continuous cross-staff sixteenths | Connected secondary-beam control |
| H16 | Alternating cross-staff eighths plus a same-source quarter-note voice | Shared onsets, rhythmic proportions and notehead/stem clearance |
| H17 | Alternating upper-source cross-staff eighths plus an independent lower-staff eighth-note voice | Simultaneous notes on the destination staff |
| H18 | Alternating lower-source cross-staff eighths plus a lower second voice starting with a sixteenth rest | Offbeat subdivisions, voice alignment and rest clearance |

Also open studio-spacing-natural-h13.mscx. It contains H13 alone with a 100%
last-system fill threshold, exposing short-system beamlet clearance without
full-page justification. Use this original in Studio as well; compare at the
same staff scale. The text labels identify cases but are not engraving targets.

H13/H14's explicit begin32 settings separate adjacent sixteenths' secondary
beams while preserving their common primary beam. Inspect the actual beamlet
directions in the application before deciding whether an opposing-beamlet
spacing correction is warranted. Their import/rendering is not verified by
XML inspection alone. H15 provides a connected-beam control for that comparison.

All these cases deliberately stay outside v0.34's ordinary-eighth-note signed
offset adapter. New data-driven tst_spacing checks require zero signed offsets,
finite nonnegative widths, repeat stability and restoration of legacy segment
positions/widths when the spacing option is disabled. They check each primary
beam is cross-staff, plus measure duration, chord counts and retained forced
secondary-break counts. Run them with both beam styles. A zero offset does not
mean that the rhythmic-spacing option has no effect on the total layout.

Local XML checks confirm every staff/voice fills exactly 2/4; the overview has
45 chords and four forced secondary breaks, and natural H13 has six chords and
two breaks. Qt moc and C++11 syntax checks pass. Full tst_spacing/tst_beam and
rendering require the user's build. Apply v0.35 after v0.34, run both suites and
compare H13-H18 plus natural H13. Next use those results to choose the smallest
beamlet or voice-spacing correction supported by the pinned Studio source.

## v0.36: cross-staff secondary-beam offsets and beamlet clearance

The user reports the v0.35 tests passing. In the supplied comparisons, natural
H13 exposes substantially tighter lower-to-upper transitions in Evolution at
notes 2-3 and 5-6. The full H13/H14 systems are useful secondary-break controls;
H15 is connected, and H16-H18 remain competing-voice controls. Proceed with this
focused spacing correction without changing beamlet direction or beam angles.

Extend the single-stream adapter from ordinary eighth notes to eighths and
sixteenths. Apply the same fixed signed stem-side displacement to supported
cross-staff transitions. Reject a beam containing tertiary or higher levels;
existing edited-beam, staff-side, cue-size and competing-voice guards remain.
For an up-to-down transition, retain the minimum of twice the displacement.
When both endpoint chords instead have isolated secondary beamlets, use the
Studio 4.7.5 minimum: displacement + 2 * beamMinLen + 0.35sp. The fixed 0.35sp
comes from that release's PaddingTable beam-to-beam entry. Respect the score's
existing beamMinLen; do not change style defaults to force a screenshot match.
Evolution's collision minimum still takes precedence if it requires more room.

Determine an isolated secondary beamlet from membership in a one-chord group
at beam level 1. Check connections to both neighbors using beam4CalcBreaks for
the modern beam option and Groups::endBeam for legacy beams. This uses current
grouping before final beam geometry exists, avoiding stale segments from the
previous layout. No final stem directions or beamlet pointers are read. Use the
same destination-staff inference from v0.34. The spring solver is unchanged.

Update crossBeamCorners to require signed offsets in H13-H15 and explicit
beamlet clearance at natural/justified H13's two lower-to-upper transitions.
H16-H18 must still have zero signed offsets. Retain finite-width, imported
content, repeat-layout and legacy-restoration checks with both beam styles.
No fixture or layout-default values change.

Local C++11 syntax and Qt moc checks pass. An isolated harness executing the
actual adapter passes 10,240 H13-H15 boundary/clearance cases with mocked grouping
calls for the inputs' explicit modes, across both grouping selections and all
provisional stem combinations. The earlier 8,192 H9-H12 boundary cases and
visibility/voice/size/duration/beam/edited/gap guards still pass. This does not
exercise full score layout or automatic grouping in the application. Full
tst_spacing/tst_beam and rendered comparisons require the user's build.

Apply v0.36 after v0.35. Compare studio-spacing-natural-h13.mscx first, especially
notes 2-3 and 5-6, then H13-H15 in studio-spacing-cross-staff-corners.mscx.
Recheck H16-H18 as competing-voice controls and H9-H12 for regressions. Next
address a verified competing-voice spacing difference after these checks.

## v0.37: sustained un-beamed voices beside cross-staff beams

The user reports all v0.36 tests passing. Natural H13's lower-to-upper beamlet
clearance is now close to Studio at matching staff scale. Some extra spacing
in the reverse transition remains consistent with Evolution's retained collision
minimums; do not lower those floors as part of this voice correction.

From this checkpoint onward, put the version near the front of patch names
(37E-v0.37-...) and new comparison-score names (v0.37-...). Existing filenames
remain unchanged so earlier comparison instructions still work.

Studio v4.7.5 computeCrossBeamSpacing distinguishes a longer un-beamed value at
the current onset from an un-beamed value at the following onset. The former
does not block a signed adjustment if actualTicks exceeds the segment interval;
the latter blocks it. A current un-beamed value ending at/before the next onset
also blocks it. Even when blocked, an identified up-to-down beam transition
still requires its minimum stem/beamlet clearance.

Adapt this bounded case by selecting one beamed chord per segment and treating
un-beamed chord voices as constraints on canAdjust. Independent beamed voices
still take the fallback, as do visible rests other than gap/full-measure rests.
Existing beam-size, edited-beam, staff-side and endpoint-duration guards remain.
No stem directions, collision floors, beam angles or spring rules change.
This is not the complete Studio multi-voice kerning or cross-beam implementation.

H16 now expects negative outgoing offsets at onsets 1 and 3, and zero signed
offsets at incoming onsets 2 and 4. H17/H18 retain zero-offset fallback checks.
Add tests with both beam styles, repeated layout and legacy restoration, plus
two new version-prefixed inputs:

| File | Cases | Visual focus |
| --- | --- | --- |
| v0.37-spacing-natural-h16.mscx | Existing H16 alone at the 100% fill threshold | Outgoing gaps beside sustained quarters versus incoming gaps at new quarters |
| v0.37-spacing-voices.mscx | H19: lower-source mirror with sustained quarters; H20: upper-source cross beam plus un-beamed eighths | Reversed signed adjustment; short-note blocking control |

Both inputs use matched earlier styles, 2/4 time and spacing initially off.
The natural file preserves H16's existing notes and voices. H19 expects positive
outgoing offsets at onsets 1 and 3. H20 expects zero signed offsets. All retain
up-to-down minimum clearance whether or not the signed adjustment is allowed.
Initial time signatures precede note/rest durations in both staffs. Every voice
fills exactly 2/4; XML checks find six chords in natural H16 and fourteen in the
two-measure H19/H20 overview.

Local C++11 syntax and Qt moc checks pass. The extracted-adapter harness passes
6,144 sustained/short-voice boundary and blocked-clearance cases across both
grouping selections and all provisional stem combinations; the earlier 10,240
secondary-beam and 8,192 ordinary cross-beam cases/guards still pass. Grouping
calls in this isolated harness are mocked for explicit modes. Full application
tests and rendering require the user's build.

Apply v0.37 after v0.36. Run tst_spacing/tst_beam; compare the two new files with
Studio 4.7.5 and recheck H16-H18 in studio-spacing-cross-staff-corners.mscx.
Then continue with remaining kerning/accidental cases supported by comparisons
and the pinned source. Continue automatically without a separate permission step.

## v0.38: correct and verify the short un-beamed voice control

The user reports v0.37 tests passing. H16/H19 screenshots show the intended
outgoing-gap adjustment. H20, however, shows an upper beam despite its label.
The fixture used BeamMode "none"; Evolution reads/writes "no" for Mode::NONE.
The invalid string fell back to automatic beaming in both applications, so H20
was testing an independent beamed voice rather than the intended short
un-beamed voice. The zero-offset assertions alone did not distinguish them.

Replace v0.37-spacing-voices.mscx with v0.38-spacing-voices.mscx, correcting its
six explicit no-beam values and updating its title. The unchanged natural H16
file keeps its v0.37 filename. Assert that secondary-voice chords have no beam,
the corrected controls retain Mode::NONE, their chord counts are correct, and
the primary beam remains cross-staff. Preserve all prior offset, clearance,
repeat-layout and legacy-restoration assertions. No engraving code changes.

Local moc and C++11 test syntax checks pass. XML inspection confirms all six
explicit no-beam tokens are "no" and every voice fills 2/4. Full application
tests/rendering remain for the user's build. Apply v0.38 after v0.37 and use
v0.38-spacing-voices.mscx for future H19/H20 comparisons. Continue automatically
with accidental/kerning comparison inputs in the following stage.

## v0.39: accidental and kerning comparison inputs

Add comparison inputs before changing collision/accidental packing. No engraving
rules change in this stage. Use the original files in both applications; enable
experimental horizontal spacing in Evolution. Keep the modern beam option on
for initial screenshots. Both beam styles are included in the regression tests.

| File | Cases | Visual focus |
| --- | --- | --- |
| v0.39-spacing-accidentals.mscx | H21: ascending sharps; H22: descending flats | Sign-to-note and preceding-note clearance under ordinary beams |
| same overview | H23: two-note chord seconds with sharps | Notehead mirroring and columns of adjacent accidentals |
| same overview | H24: upper-source chromatic cross beam; H25: lower-source mirror | Destination-staff accidental placement and spacing at staff transitions |
| same overview | H26: chromatic cross beam plus sustained accidental-bearing voice | Voice alignment, sign columns and signed spacing beside sustained notes |
| v0.39-spacing-natural-h24.mscx | H24 alone at the 100% fill threshold | Compact accidental clearance without full-system stretching |

The overview has six labeled 2/4 measures, one per system. Inputs preserve the
matched style values of earlier comparisons, Leland fonts and modern beams;
spacingLayout4 is initially off. Chromatic notes use explicit pitch and tonal
pitch class (TPC); accidentals are generated on import, not supplied as user
overrides. Read/render tests must verify this produces actual signs. No ties,
tremolos, tuplets or courtesy-accidental settings are mixed into these inputs.
Conventional cases retain the two-staff instrument with hidden full-measure
rests on the inactive staff for a consistent comparison setup.

Add accidentalInputs to tst_spacing, with the overview and natural control under
both beam styles. Check first-chord pitch/TPC, actual generated sharp/flat signs,
the two-note chord-second input, primary cross-staff beam status, expected fixed
offsets, finite widths, repeat stability and legacy position/width restoration.
These checks validate the input semantics and stability; visual collision and
Studio parity checks still require rendered comparisons. Existing collision
minimums are retained. A new sign or notehead must not be assumed correct merely
because segment coordinates are finite.

Local XML checks confirm all staff/voice durations fill exactly 2/4, pitch/TPC
spelling is consistent and initial signatures precede notes/rests. The overview
contains 26 chords/30 notes; natural H24 has four chords/four notes. Qt moc and
C++11 test syntax checks pass. Full tst_spacing/tst_beam and rendering remain
for the user's build. Apply v0.39 after v0.38, run both suites and compare H21-H26
plus natural H24. Next trace a specific rendered spacing difference against the
pinned Studio source before selecting the next collision/kerning change.

## v0.40: bounded cross-staff kerning; comparison scores open with spacing on

The user reports v0.38/v0.39 tests passing. H19/H20 now show the intended beam
states. H21-H26 show no obvious sign collisions at the supplied screenshot
scales; this is not proof of complete accidental-packing parity. Compact H24
still has extra space in Evolution's upper-to-lower transition.

Trace that case to Segment::minHorizontalDistance: after checking actual staff
shapes, Evolution imposes a blanket notehead-width floor plus minimum note
distance, even when adjacent notes are on different staves. Studio's down-to-up
cross-beam case permits cross-staff kerning. The existing negative signed offset
cannot achieve its intended compact gap while this unconditional floor wins.

Add an explicit, default-false allowCrossStaffKerning parameter to the segment
distance calculation. Measure::computeMinWidth passes it only when its bounded
cross-beam adapter returns a negative offset. Also require spacingLayout4 on,
two chord/rest segments and no system-header gap. Omit only the unconditional
notehead-width floor in that case. Preserve shape clearance, minNoteDistance,
manual extra-leading space, lookback collision checks and all other margins.
Up-to-down transitions, ordinary beams, blocked voice adjustments, edited beams,
unsupported contexts and the default-off path keep their previous handling.
Do not reduce configured minimum note distance or change accidental columns.
The spring solver and global style defaults are unchanged.

Per the user's request, new visual comparison inputs should save spacingLayout4
as 1 whenever testing modern spacing. The tests must explicitly disable it to
capture legacy baselines; this does not change the default for ordinary scores.
Rename the current accidental inputs to v0.40-spacing-accidentals.mscx and
v0.40-spacing-natural-h24.mscx, enable the flag in both and update their titles.
Their notes, voices, beam modes and other style values remain the same.

Update accidentalInputs to verify the initial on flag, capture the legacy
baseline with it off, and require the optional kerning argument to have no
effect while off. With it on, verify that the supported first transition uses
actual shape distance plus configured padding/manual leading space. Natural H24
must become narrower there than the old unconditional floor, while still
respecting the geometric minimum. Keep all prior sign/import, fixed-offset,
repeat-layout and legacy-restoration checks under both beam styles.

Local C++11 syntax checks for measure.cpp, segment.cpp and tst_spacing.cpp, plus
Qt moc generation, pass. A harness running the actual extracted old/new segment
distance functions passes 100,000 mocked-shape contexts, preserving exact old
results with the option off and retaining geometry, configured padding and
manual leading space in the supported relaxation. XML checks confirm both
inputs enable spacing and every staff/voice fills 2/4. These checks do not
replace application tests or rendered collision inspection.

Apply v0.40 after v0.39. Run tst_spacing/tst_beam and compare the two v0.40 files,
especially natural H24's first upper-to-lower gap. Recheck compact H13/H16 and
ordinary/cross-staff controls. Full application tests and rendering remain for
the user's build; use their results to select the next specific kerning change.

## v0.41: compare width distribution across multiple measures

The user reports v0.40 tests passing. At matching staff scale, compact H24 now
looks close to Studio in its first gap and overall note spacing. The H21-H26
overview does not expose a clear new accidental collision or column-placement
problem. Keep the current bounded kerning rather than selecting a speculative
accidental-packing change from these screenshots.

The next comparison covers multiple measures sharing a system. Earlier controls
mostly put one measure on each system and cannot reveal how spare system width
is distributed among measures. Evolution's collectSystem still distributes
extra width using Measure::layoutWeight times basicStretch; equal-duration
ordinary measures therefore get equal added width with spacingLayout4 on.
Studio v4.7.5 HorizontalSpacing::justifySystem instead collects rhythmic springs
across all measures in the system. This is a known remaining architectural
difference, not yet changed in this checkpoint.

Add v0.41-spacing-systems.mscx, with Studio horizontal spacing and modern beams
already on, matched Leland/spacing styles, nine 4/4 measures and explicit breaks
after measures 3 and 6. The 10-inch-wide page has an 8.8-inch printable width and
0.6-inch side margins so each three-measure comparison fits in both algorithms.
The final-system fill threshold is zero to keep every group justified.

| System | Cases | Comparison |
| --- | --- | --- |
| 1 | H27 quarters, H28 eighths, H29 sixteenths | Barline positions and note spacing as density increases |
| 2 | H30 sixteenths, H31 eighths, H32 quarters | Reversed-order control; separate density from header position |
| 3 | H33 mixed rhythm, H34 chromatic sixteenths, H35 eighth-note triplets | Collision constraints and nonbinary durations within one system |

H33/H34/H35 reuse the notes and voices of earlier H5/H4/H8 controls; H27-H32 use
repeated E4 with four/eight/sixteen onsets. No tremolos, cross-staff beams or
multiple voices are mixed into the first two systems. Compare the same original
in both applications at matching staff scale. Capture the full three-measure
systems so barline positions remain visible. Also compare option off in Evolution
if useful; these inputs open with it on as requested.

Add systemInputs to tst_spacing under both beam styles. Verify the input opens
with spacing on, disable it to capture the legacy baseline, then check the three
groups of three, nine complete 4/4 measures, expected onset counts, generated
accidentals in H34, actual 1/12 triplet durations in H35, finite positions/widths,
zero cross-beam offsets, repeated-layout stability and legacy restoration of
measure/segment positions and widths. Do not encode the current cross-measure
width ratios as parity expectations before changing the distributor.

Local XML checks confirm all nine voice durations are 4/4, the onset counts are
4/8/16/16/8/4/7/16/12, and the explicit triplet ratios are valid. Qt moc and C++11
test syntax checks pass. No engraving code changes in this checkpoint. Full
tst_spacing/tst_beam and rendered system comparisons require the user's build.
Apply v0.41 after v0.40. Next use these results to implement a bounded system-wide
justification stage behind spacingLayout4 while preserving the default-off path.
