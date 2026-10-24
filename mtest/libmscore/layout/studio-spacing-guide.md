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

## v0.42: share rhythmic spring force across supported systems

The user reports v0.41 tests passing. The system comparison shows Evolution
allocating relatively more width to quarter-note measures and Studio allocating
relatively more to dense measures. Address that distribution with the existing
spacingLayout4 option; its default remains off and beamLayout4 stays independent.

In collectSystem, pool flexible rhythmic segment widths across eligible systems
and solve for one force before allocating each measure's target width. This
follows pinned Studio v4.7.5 HorizontalSpacing::justifySystem and
stretchSegmentsToWidth. Subtract signed cross-beam offsets before solving, keep
collision floors until their pre-tension is exceeded, and use the same duration
and user-stretch weights as Evolution's experimental per-measure springs.
Existing Measure::stretchMeasure applies the targets and retains its element
layout and rest-centering work. System collection and line fitting are unchanged.

The pooled path requires at least two ordinary measures, positive spare width,
and at least one visible rhythmic spring in each measure. Systems containing
frames, multi-measure rests, timed non-ChordRest segments, invalid spring values,
or minimum-measure-width slack retain the current measure distribution. A short
terminal system left natural by the fill threshold and noHorizontalStretch also
retain their current behavior. The default-off path uses the original formula.
The spacing checkbox tooltip now describes justification as part of this option.

Reuse **v0.41-spacing-systems.mscx**; it already enables experimental spacing.
Compare the same three systems in both applications, especially H27-H29 and the
reversed H30-H32. Inspect barline positions and within-measure note intervals.
H33-H35 cover collision floors, chromatic notes, mixed rhythm and triplets.
Recheck the option-off layout as well. No new visual fixture is needed for this
algorithm change.

Add springForce unit cases for rhythmic weights, unequal collision floors,
zero spare width and invalid spring input. Extend systemInputs under both beam
styles to require a common segment force across the first two spacious systems,
while retaining content, grouping, finite geometry, repeated-layout stability
and exact legacy-restoration checks.

Local Qt moc/uic generation and C++11 syntax checks for layout.cpp, measure.cpp
and tst_spacing.cpp pass. An isolated harness using the actual extracted system
allocator and Measure::stretchMeasure spring loop passes 100,000 generated
systems against the pinned Studio solver. It checks total allocated width,
collision floors, signed fixed offsets and frame/MMRest/slack fallbacks. These
checks use mocked score objects and do not replace a full application run.

Apply v0.42 after v0.41. Full tst_spacing/tst_beam and rendered comparisons remain
pending on the user's build. Next use the three-system comparison to decide
whether another width-allocation correction is needed before expanding the
eligible contexts or changing system fitting.

## v0.43: source audit and implementation map

The user reports v0.42 tests passing. Add
[v0.43-horizontal-spacing-audit.md](v0.43-horizontal-spacing-audit.md), covering
all 54 HorizontalSpacing definitions plus its collection/finalization callers,
shape/padding dependencies and selected geometry producers. Verify reference
bytes against the pinned v4.7.5 Git blob IDs. No engraving code or score changes.

The remaining major gaps are item-tagged collision geometry and pair policy,
cross-measure placement/width reconstruction, minimum-width spring enforcement,
and collection/squeeze integration. Current duration/common-force adaptations
remain useful components; current bounded cross-staff handling is not general
Studio kerning. The audit records exact scope, fallbacks and a dependency-ordered
implementation plan. Next start the spacing-only geometry/identity adapter and
source-derived collision policy in recoverable WIP increments. Existing H1-H35
fixtures remain the integration controls. No application test rerun is needed for
this documentation-only checkpoint.

## v0.44 WIP: spacing-only item identity and geometry snapshots

Add spacinggeometry4.h/.cpp as the first implementation step from the source
audit. The adapter retains every rectangle from the native element/staff shape
in its original order and coordinates, and attaches a borrowed Element pointer
where native component geometry identifies the owner. Rectangles that have no
owner or competing owners remain explicitly UNCLASSIFIED or AMBIGUOUS. Their
geometry is retained; neither state is eligible for a future item-policy path.

The snapshot supports identity-preserving copy/translation. `complete()` reports
identity coverage only, not collision-policy support. Exact coordinate matching
is deliberate: no debug-name interpretation, tolerance-based guessing or
candidate-order assignment. Unknown candidates prevent a known overlapping
candidate from claiming the rectangle. Two native coincident rectangles cannot
both be attributed to one candidate. Floating-point translation differences can
leave geometry unclassified; preserve that fallback rather than guess an owner.

Tag common noteheads, note dots, accidentals, stems/hooks/stem slashes, ledger
lines, arpeggios, native clefs/keys/time signatures/barlines and ordinary rest
bodies. Reconcile composed chord shapes against native geometry so omitted
cross-staff stems are not inserted. Staff snapshots use visual staff ownership;
barlines use the same layoutRect as Segment::createShape. Add a const ledger-line
accessor to Chord so the adapter can inspect lines without casting away const.
The CMake library source list includes the new implementation.

Explicit remaining gaps include rest dots/MMRest body and number identification,
aggregated lyric/harmony/synthetic spacing rectangles, unfamiliar composites,
and any ambiguous or non-exactly-matching primitive. This is an identity adapter,
not Studio's padding/kerning implementation. It intentionally does not recreate
every Studio shape producer. Candidate reconciliation is a conservative WIP
boundary and must not be mistaken for full source-provenance instrumentation.

There are no production layout callers yet: existing native Shape, skyline,
collision calculations and spacing dispatch are unchanged. The adapter is
currently exercised explicitly by tests. No visual layout change is expected
with either experimental option state; no new visual MSCX is needed.

Extend tst_spacing with geometryResolution and geometryInputs. The first checks
native notehead/dot/accidental identity, translation, unknown spacers, ambiguous
overlaps, candidate-order independence, near-match rejection, and coincident
primitives with insufficient ownership. The score-based cases use existing
H1-H8, H21-H26 and H19-H20 fixtures under both beam styles. They retain all native
staff rectangles, check notehead destination staff/coordinates and known stem/
accidental/cross-staff identity, and check that inspection leaves native shapes,
measure widths, segment positions/widths and fixed offsets unchanged.

Local Qt moc generation and C++11 syntax checks pass for the adapter in debug
and NDEBUG configurations and for tst_spacing. A standalone run linked to the
actual resolver/translation code passes 100,000 generated identity cases,
covering geometry retention, ownership states, translation, candidate order,
and exact-match rejection. These tests do not run the full DOM shape builders.
Full application builds and the new score-based tests remain for the user's
build; the implementation is not being declared integrated from the isolated
checks alone.

Apply v0.44 after v0.43, build and run tst_spacing (tst_beam remains a useful
regression check). Next use the identity coverage results to complete common
notation inputs and implement the source-derived item-pair padding/kerning
policy. Keep the current layout dispatch until those inputs and the connected
placement/width-reconstruction path are ready. Profile reconciliation before
putting it into a repeated production spacing pass.

## v0.45 WIP: common-notation item-pair collision policy

The user reports compilation and mtests passing after v0.44. Add
spacingcollision4.h/.cpp, still without production layout dispatch. The new
module consumes the temporary identities from spacinggeometry4 and supplies the
first source-derived pair policy for the connected spacing engine.

Snapshot the 12 supported type rows/columns from Studio v4.7.5
PaddingTable::createTable: NOTE, LEDGER_LINE, HOOK, NOTEDOT, REST, CLEF, BAR_LINE,
KEYSIG, TIMESIG, ARPEGGIO, ACCIDENTAL and STEM. Preserve asymmetric padding and the
upstream ordering of note-row/column copies followed by stem-specific overrides.
Use existing score style values rather than changing Evolution's defaults.
Rebuild this table after style changes; it is scoped to a layout pass.

spacingPairPolicy4 adapts average item magnification, the same-voice/adjacent
note intersection rule for minimum padding, ledger-rest glyph padding, accidental
versus general vertical clearance, same-voice kerning restrictions, ordinary
header-item restrictions, mid-measure clef right-edge kerning and full-measure
rest handling across source staves. For supported ordinary notes the chord
kerning-exception flags are unrestricted: notes with articulations, attached
slurs, ties or other note spanners are explicitly rejected until their metadata
has an adapter. Grace notes, tablature, MMRest items and types outside these 12
are also outside this stage. This is a bounded common policy, not a replacement
for every upstream computePadding/computeKerning branch.

spacingPairDistance4 implements the upstream rectangular constraint math:
vertical clearance, absolute minimum padding, squeeze factor, non-kerning,
free kerning, allowed collisions, left/center/right edge limits and zero-width
constraints. A pair imposing no constraint returns -max(qreal), preserving
upstream semantics; it does not mean an origin-to-origin distance of zero.
The shape-level query combines constraints only if every relevant primitive
has supported provenance and context. It returns false with the caller's output
untouched when any primitive is unsupported, ambiguous or malformed. Synthetic
spacing rectangles have no inferred barrier policy yet and require fallback.
Do not mix half-adapted query results with a guessed spacing floor.

The query receives staff-scaled shape spatium separately from the table's score
spatium. Preserve that distinction when integrating small staves. Header minimum
distances, cross-staff stem exceptions, natural rhythmic widths and group-wide
placement still belong to the later segment/system pass, not this item-pair
kernel. The slur-context eligibility scan is deliberately conservative; cache
pass metadata and profile it before enabling repeated production queries.

Extend tst_spacing with collisionPolicy tests for asymmetric table entries,
same-voice and different-voice constraints, average magnification, accidental
clearance, header versus mid-measure clefs, full-measure rests, unknown geometry,
unadapted ties/grace notes and unchanged outputs on rejection. Explicitly test
clearance, zero-width and edge-kerning math. Extend geometryInputs to exercise
actual adjacent score shapes under both beam styles, require supported queries,
check squeeze monotonicity and retain the prior nonmutation checks. Reuse the
existing ordinary/accidental/cross-staff/voice scores; no new visual file.

Local moc generation and C++11 syntax checks for the adapter in debug/NDEBUG
configurations and for tst_spacing pass. An isolated test extracts the actual
table constructor and compares all 144 supported entries against the complete
pinned Studio table constructor for 1,000 randomized style sets (144,000 entries).
A second harness links the actual rectangular-constraint function and compares
100,000 cases against the pinned Studio shape-distance loop using controlled
pair policies, including kerning modes, squeeze, clearance and zero dimensions.
These isolate table/math parity; they do not validate full DOM adaptation or
establish complete engraving parity. The live-score tests remain for the user's
application build.

Apply v0.45 after v0.44. Run tst_spacing/tst_beam; no visual change or screenshots
are expected because layout dispatch is unchanged. Next connect the common
collision inputs to a measure-group position stream, including explicit barrier
handling, cross-measure lookback, width reconstruction and minimum-width springs.
Retain whole-context fallback for unsupported features until each adapter is
ready. Do not replace the new pipeline with more screenshot-specific constants.

## v0.45.1: correct the live-score policy test boundary

The user's Qt 5.9.9 result.xml reports six failures, all at geometryInputs'
`collisionQueries > 0` assertion (three fixtures under both beam styles).
geometryResolution, collisionPolicy and the existing spacing/layout tests pass.

The v0.45 test incorrectly required successful complete staff-shape queries.
Native Chord::shape adds a horizontal-spacing rectangle for its computed
_spaceLw/_spaceRw. The geometry adapter deliberately leaves these synthetic
rectangles unclassified, and the collision module correctly rejects a query
containing unadapted geometry. The test conflated live item-pair support with
whole-shape support, which is not implemented yet.

Keep the full-shape queries and explicitly require fallback with unchanged
output when they contain relevant opaque primitives. Probe identified native
item pairs separately, retaining finite-distance and squeeze-monotonicity checks.
Require common plain note pairs to succeed, require live item/note-pair coverage,
and require actual opaque-shape fallback coverage in every fixture/beam row.
The existing geometry and score nonmutation checks remain. Partial probes are
test observations only; they are never used to derive a whole-shape floor.

This checkpoint changes tst_spacing.cpp and this guide only. No production
collision, geometry or layout rules change. Synthetic barrier handling remains
part of the connected placement work, not something to bypass to pass a test.
Local moc and C++11 test syntax checks pass; full tst_spacing/tst_beam remain for
the user's build. Apply v0.45.1 after v0.45 and rerun the tests. No screenshots
are needed for this correction.

## v0.46 WIP: connected common-notation placement plan

The user reports v0.45.1 tests passing. Add spacingplan4.h/.cpp, a nonmutating
system plan using the tested collision policy. This connects placement,
cross-measure lookback, width reconstruction, minimum-width springs and pooled
justification. It does not change Score::collectSystem dispatch yet: application
layout remains at the existing experimental path until this plan has passed
the user's live-score checks.

### Native aggregate geometry has explicit provenance

Add LEGACY_SPACER metadata for Chord::shape's aggregate _spaceLw/_spaceRw bound
and Segment::createShape's synthetic barline spacing rectangles. Reconstruct
their rectangles with the actual native addHorizontalSpacing producer, rather
than recognizing arbitrary zero-height geometry. Add read-only chord accessors
for those native bounds. Preserve every rectangle and producer identity in the
snapshot. Resolver conflicts between direct-item and aggregate identities remain
ambiguous, and unknown geometry still has no inferred owner.

spacingCollisionProjection4 creates a separate view containing direct supported
items. It excludes a tagged aggregate only when its owning chord's notes have
supported contexts (or its owner is a barline) and an identified component
footprint is present. Unsupported/ambiguous rectangles reject the whole
projection with its output untouched. Ordinary native shapes remain intact;
the existing whole-shape collision query still rejects unprojected aggregate
records, as the v0.45.1 test requires. A raw zero-height rectangle is not a
barrier or a discardable aggregate by default.

### Position stream and reconstruction

buildSpacingPlan4 caches projected staff geometry, flattens active segments into
system coordinates and applies the ordinary branches of Studio's spaceSegments,
spaceAgainstPreviousSegments and stopCheckingPreviousSegments. Collision
clearance is redistributed through intervening chord/rest positions, including
across barlines. Duration/user stretch comes from segmentSpacingStretch4;
natural advances use the pinned 3.5sp quarter reference. The missing Studio
systemHeaderMinStartOfSystemDistance style uses its explicit v4.7.5 default,
1.25sp. Spacing density is the current default 1.0, not a new persisted setting.

The pass reconstructs local spans and measure boundaries after positioning,
using the ordinary setPositionsAndWidths boundary rules. End barlines retain
their actual projected right extent. Minimum width follows the pinned
computeMinMeasureWidth policy: available-width cap, shortened-measure scaling
and first-chord/header/accidental adjustment. Deficits are distributed through
rhythmic springs and following measures are moved, rather than leaving
unrepresented measure bounding-box slack.

justifySpacingPlan4 applies one common force to a built plan and reconstructs
positions while retaining fixed nonrhythmic widths and first-segment offsets.
It validates the resulting total width before publishing the result. Both APIs
work on temporary data; no segment/measure positions, widths, stretch values,
styles or ticks are written to the score. Pointers belong to the current pass
and must not survive edits or a new layout.

### Scope, checks and next integration

The first plan supports visible ordinary measures, normal chord/rest segments,
header clefs/keys/time signatures, begin/end barlines and supported common item
geometry. Reject frames, MMRest, hidden staves, inactive rhythmic segments,
manual leading space, cross-beam offsets, cross-staff chords/beams, cue/grace
contexts, invisible notes, unadapted decorations and ordinary mid-measure clefs,
breaths/trailers. Unadapted annotations are rejected before native shape builders
can lazily lay them out. Existing ignored nonrhythmic segments are retained with
zero width. Unsupported input leaves the caller's plan untouched and supplies
a reason. This is an explicit first pipeline scope, not full Studio parity.

Extend geometryResolution with aggregate projection, orphan-aggregate rejection
and opaque-rectangle preservation. Extend geometryInputs to recognize tagged
aggregate owners while retaining the full native geometry/fallback checks.
Add placementPlan under both beam settings using v0.41-spacing-systems.mscx
(H27-H35). Require three successful three-measure plans, continuous reconstructed
spans, repeatability, justified target-width/floor conservation, reduced-spacing
controls, large minimum-width expansion and unchanged native score geometry.
Check rejection without output mutation for invalid factors, impossible
justification, null systems and a cross-staff input. No new visual fixture.

Local moc and C++11 debug/NDEBUG syntax checks pass for geometry, collision and
planner modules and the test source. An isolated harness uses the actual planner
body and the pinned placement/lookback/reconstruction/minimum-width spring
routines with controlled DOM and distance inputs. It checks 10,000 generated
systems, including pickups, narrow width caps and ignored nonrhythmic segments,
and checks justification target/floor conservation. These tests isolate the
connected math, not real font/DOM adaptation. The actual geometry resolver also
passes 100,000 cases plus new aggregate-identity/conflict cases. Full application
and live-score tests remain for the user's build.

Apply v0.46 after v0.45.1 and run tst_spacing/tst_beam. No visual change is
expected and screenshots are not needed yet. Next, after validating the live
plans, connect eligible final systems behind spacingLayout4, preserving the
whole-context legacy fallback and existing line collection. Later use the same
engine during collection/squeezing and extend the documented unsupported
contexts. Cache eligibility/pair metadata and profile before widening production
use; the conservative slur scan currently remains in common-item policy.


## v0.47: apply common-notation plans at final system placement

The user reports v0.46 builds and both tst_spacing and tst_beam pass without
failures. This checkpoint connects the previously isolated planner to
Score::collectSystem after collection, hidden-staff handling and final barlines
and trailers, immediately before final horizontal placement.

With spacingLayout4 enabled, build a whole-system candidate without changing
native positions. Use it only if every context is adapted and its natural width
fits the available system width. Evaluate terminal/section fill using the
candidate's natural width. Solve the complete justified plan, if needed, before
publishing any segment positions, widths or stretch values. Reconstruct measure
positions/widths from that plan and finalize elements and staff lines normally.
Do not run Measure::stretchMeasure on the solved widths: extract its existing
element-finalization body into Measure::layoutElementsAfterSpacing, shared by
both paths. The extracted body is byte-for-byte identical to the prior body.

Unsupported or overwide candidates retain the existing experimental path,
including the earlier system spring allocator and cross-staff adapters. With
the option off, dispatch and spring arithmetic are unchanged. The existing
noHorizontalStretch override bypasses the planner. The terminal fill helper is
shared with the fallback and preserves its prior rule, including trailing
frames and section boundaries. Whole-measure centered rests and tremolo chords
now explicitly reject planning: their moving geometry and anchor constraints
need adapters before this new final-placement path should handle them. Their
existing layout continues through the fallback. This boundary does not undo
any earlier beam or tremolo work.

Files changed:

- libmscore/layout.cpp: guarded final-system dispatch, shared fill predicate,
  solved segment application and measure reconstruction.
- libmscore/measure.cpp and measure.h: share the existing element finalizer.
- libmscore/spacingplan4.cpp: explicit centered-rest/tremolo fallback boundaries.
- mtest/libmscore/layout/tst_spacing.cpp: six finalPlacement rows covering the
  new overview and natural controls, with each beam option. Check actual
  system, measure and segment geometry against the solved plan, rhythmic
  stretch, repeatability, changed enabled layout and exact option-off restoration.
- Three new visual inputs below, plus this guide.

New comparison inputs (both experimental spacing and beams initially on):

- v0.47-spacing-placement.mscx: H27-H35, the same three fixed systems as the
  v0.41 overview, with a versioned title. Compare measure widths across each
  system, transitions between rhythmic values, H34 accidentals and H35 triplets.
- v0.47-spacing-natural-h5.mscx: H5 mixed rhythmic values at natural width.
- v0.47-spacing-natural-h8.mscx: H8 eighth-note triplets at natural width.

Open those same files in Studio 4.7.5 for the visual comparison. For the natural
controls compare inter-onset gaps and header/barline distances; for the overview
compare the relative widths of measures sharing one justified system. Existing
H9-H26 cross-staff controls remain relevant fallback regression checks. Existing
line collection still determines how many measures fit: v0.47 does not yet use
the planner to select breaks or squeeze an overwide system.

Local validation: moc plus C++11 syntax checks in debug and NDEBUG modes pass
for layout.cpp, measure.cpp, spacingplan4.cpp and tst_spacing.cpp, with unused
parameter diagnostics enabled. The actual planner again passes the isolated
10,000-system comparison against pinned 4.7.5 placement/lookback, reconstruction
and minimum-width spring routines with controlled DOM/distance inputs;
justification preserves floors and target width. This harness is not a full
application/font test. All three new score XML files parse and save both options
enabled. Patch whitespace and apply checks are required before delivery.

Apply v0.47 after v0.46, rebuild, and run tst_spacing and tst_beam. Full runtime
validation and screenshots remain for the user's build. Next, after validating
final placement, feed the same engine into bounded collection/squeezing so line
fitting and final spacing share one calculation. Preserve whole-system fallback
until each unsupported context has an explicit adapter; profile/cache eligibility
and pair metadata before widening the production scope. Continue using version
prefixes and enabling experimental spacing in new comparison files.


## v0.48: bounded natural-width system fitting

The user reports v0.47 tst_spacing/tst_beam passing. In the supplied comparison
captures, natural H5/H8 horizontal gaps and the H27-H35 overview's relative
measure widths look close to Studio 4.7.5 after accounting for zoom. No obvious
new horizontal collision stands out. This is visual evidence for the tested
inputs, not a claim of complete engraving equivalence.

Connect the existing nonmutating plan to the ordinary collectSystem fit query:
when another measure is tentatively appended, ask systemFittingWidth4 for the
whole prefix's natural width and use that width to decide whether to break.
Do not apply candidate positions, and keep the native minWidth accumulator
unchanged for fallback, removal and no-break backtracking. Final placement and
justification continue through v0.47, using a newly built plan with the final
system header/barline geometry.

This first fitting adapter requires stable boundary geometry. It rejects
frames, no-break groups, repeat starts/ends, courtesy trailers, possible beams
joined into the next measure, and active clef/key/time headers after the first
measure. Hidden-staff policy and noHorizontalStretch bypass the adapter. The
planner's other unsupported-context guards remain in force. Failure leaves
the supplied fitting width untouched, preserving the existing decision. A
single overwide measure still follows Evolution's existing at-least-one-measure
rule. The stage does not squeeze an overwide candidate or fit changing headers.

The narrow boundary guards matter because accepting a prefix with group
spacing must not leave an overfull legacy-width system if finalization later
needs unsupported geometry. In this stage, ordinary accepted prefixes have
unchanged end-barline/header geometry; changing-boundary cases retain the
previous collection policy. Existing explicit line/page/section breaks remain
in the existing collector and continue to apply.

Files changed:

- libmscore/layout.cpp: guarded systemFittingWidth4 and the candidate fit query.
- mtest/libmscore/layout/tst_spacing.cpp: fittingSwitch under both beam options.
- mtest/libmscore/layout/v0.48-spacing-fitting.mscx: H27-H35 without forced line
  breaks, experimental spacing/beams on, visible-staff policy, natural terminal
  fill. The versioned title identifies the new automatic-fitting comparison.
- This guide: validation, limits and continuation checkpoint.

The new test first lays out all nine measures on a wide page, then independently
measures the planner's group width and the prior per-measure fitting estimate.
Choose a printable width between those estimates, require the resulting system
count to follow the group estimate, and verify every planned system fits.
Check repeated layout plus on/off/on/off restoration of geometry and grouping.
This tests the production fitting decision, not only finite output. Existing
forced-break overview and cross-staff cases remain regression controls.

Local validation: moc and C++11 syntax checks pass in debug and NDEBUG modes for
layout.cpp and tst_spacing.cpp, with unused-parameter diagnostics enabled. An
isolated harness compiles the actual fitting guard against controlled DOM and
planner stubs: all 4,096 guard combinations pass, rejected queries retain the
caller's width, and initial headers remain eligible. This checks dispatch
boundaries, not live-score font geometry. The new score XML parses, has nine
measures and no LayoutBreak elements, and saves both experimental options on.
Patch whitespace and parent-index apply checks are required before delivery.
Full tst_spacing/tst_beam execution remains for the user's build.

Apply v0.48 after v0.47. Rebuild and run tst_spacing/tst_beam, then open
v0.48-spacing-fitting.mscx in 37E and Studio 4.7.5. Compare measures per system
and the onset gaps around automatic breaks at the saved page width. If useful,
change printable width in both applications to expose a nearby fitting
threshold. Do not expect every Studio break yet: the next stage is bounded
squeezing, followed by explicit changing-header/courtesy boundary preparation.
Retain default-off legacy restoration and whole-context fallback throughout.


## v0.48.1: compare layout independently of cached inactive headers

The user's Qt 5.9.9 result(3).xml reports two failures, both fittingSwitch beam
rows, at the reenabled snapshot-length comparison: 235 values versus 233.
The fitting threshold, per-system width checks and repeated enabled layout
passed, as did the other test functions. The snapshot stored two coordinates
for every segment, so the difference represents one retained segment.

Evolution's addSystemHeader creates a missing generated header segment when a
measure becomes the first in a system. removeSystemHeader disables that segment
but retains it in the measure for reuse. Switching spacing off can create a new
header at a different break; switching back on removes it from visible layout
while leaving the cached segment in the container. The test incorrectly treated
container growth as a difference in the restored layout.

Change only fittingSwitch's snapshot: exclude disabled header/trailer cache
entries, and add segment type, tick and enabled state to each retained record
alongside its x/width. This preserves checks for system grouping, measure
positions/widths, active header geometry and rhythmic segment structure while
allowing the native header cache lifecycle. Existing on/off/on/off comparisons
and fitting/width assertions remain. No production engraving or score fixture
changes are made.

Files changed: tst_spacing.cpp and this guide. Local moc and C++11 debug/NDEBUG
syntax checks, targeted snapshot lifecycle checks, whitespace and parent-index
patch checks are required before delivery. Full mtests remain for the user's
build. Apply v0.48.1 after v0.48 and rerun tst_spacing/tst_beam, then continue
with the same v0.48-spacing-fitting.mscx visual comparison. Bounded squeezing
remains the next implementation stage after fitting validation.


## v0.49: first three Studio squeeze passes for adapted final systems

The user reports v0.48.1 mtests passing. Both automatic-fitting captures place
H27-H31 on the first system and H32-H35 on the second. Horizontal gaps and
measure proportions look close after accounting for image positions. This is
validation of the supplied control, not proof of all score-layout equivalence.

Add squeezeSpacingPlan4 to spacingplan4.h/.cpp. First build the ordinary natural
plan. If it exceeds the target width, recompute from native geometry using the
first three positive-duration passes from Studio v4.7.5's
HorizontalSpacing::squeezeSystemToFit:

| Pass | Duration-spacing multiplier | Padding/clearance multiplier |
| --- | --- | --- |
| 1 | 0.67 | 0.80 |
| 2 | 0.34 | 0.60 |
| 3 | 0.01 | 0.40 |

Use the source's repeated subtraction of 0.33 and 0.20, and override minimum
measure widths for these squeeze passes, as Studio does. Return the first
fitting whole-system plan. Every query, including unsuccessful passes, leaves
native positions and widths unchanged; unsuccessful queries also preserve the
caller's output plan. Reject nonpositive/nonfinite targets. Existing geometry
and unsupported-context guards still apply.

At final placement, collectSystem now asks this wrapper for a fitting plan,
then applies the existing fill decision and pooled justification before
publishing geometry through v0.47's application path. A naturally fitting system
has the same behavior as v0.48.1. Collection still uses natural group widths,
matching the source's distinction between collection and final squeezing; the
new squeeze passes do not accept extra candidate measures during collection.

This checkpoint deliberately stops before Studio's negative duration advances
and later collision-allowing width reductions. If the positive passes cannot
fit, retain the previous Evolution fallback rather than publish a partial
compressed result. Changing headers, cross-staff constraints, centered measure
rests, tremolos and other unsupported contexts keep their prior handling.

Files changed:

- libmscore/spacingplan4.cpp and spacingplan4.h: nonmutating bounded squeeze API.
- libmscore/layout.cpp: use the fitting/squeeze wrapper at final placement.
- mtest/libmscore/layout/tst_spacing.cpp: squeezePlan under both beam settings.
- mtest/libmscore/layout/v0.49-spacing-squeeze-h5.mscx: versioned narrow-page
  mixed-rhythm control, user stretch 2, terminal fill 0, both experimental
  options enabled. A small title keeps the narrow comparison readable.
- This guide: completed work, boundaries, validation and continuation.

The new test chooses widths between successive pass results and requires the
wrapper to select each first-fitting pass exactly. Check natural-width bypass,
invalid/impossibly narrow targets, preserved output/native geometry on failure,
and the actual final compressed-and-justified measure/segment geometry. Check
repeatability and off/on/off restoration. The production target is computed
from live geometry to avoid reliance on particular font metrics.

Local validation: moc and C++11 debug/NDEBUG syntax checks pass for layout.cpp,
spacingplan4.cpp and tst_spacing.cpp with unused-parameter diagnostics enabled.
The actual planner again passes 10,000 controlled cases against pinned 4.7.5
placement/lookback, reconstruction and minimum-width routines. A separate
10,000-case harness compiles the actual squeeze controller with controlled
planner results, checking exact source factors, first-fit selection,
unsupported-pass rejection, output preservation and invalid targets. These
are isolated checks, not full application/font execution. Score XML and patch
whitespace/apply checks are required before delivery. Full mtests remain for
the user's Qt build.

Apply v0.49 after v0.48.1, rebuild, and run tst_spacing and tst_beam. Open
v0.49-spacing-squeeze-h5.mscx in 37E and Studio 4.7.5 at the saved page width.
Compare the seven onsets, rhythmic gap proportions and final barline. Recheck
v0.48-spacing-fitting.mscx as the natural-fitting control. Next address explicit
changing-header/courtesy preparation before widening collection/final fitting,
then consider the remaining squeeze boundary cases. Keep default-off behavior
and existing unsupported-context fallback intact.


## v0.50: terminal key/time courtesy signatures

The user reports v0.49 mtests passing. The earlier squeeze H5 captures look
close overall, with a modest header-to-first-note difference still worth
tracking; screenshots alone do not identify its cause. Continue with the
explicit boundary cases rather than tune a spacing constant to that image.

Add the first courtesy-signature adapter to buildSpacingPlan4. Ordinary
KeySigAnnounce and TimeSigAnnounce segments are eligible only when marked as
trailers, at the owning measure's end tick, in the system's final measure.
Their native leaf geometry already has explicit item identity and uses the
existing Studio key/time-signature collision policy. Their segment types remain
distinct from leading headers, so they do not acquire the system-header-to-note
exception. Any unadapted geometry or context still rejects the entire plan.

Pinned Studio 4.7.5 HorizontalSpacing::setPositionsAndWidths assigns the final
trailer its minRight plus systemTrailerRightMargin. Reconstruct that same
terminal width in the temporary plan; the margin is fixed, not scaled by the
squeeze factor and not a rhythmic spring. Existing pooled justification/reflow
retains it. Do not change Evolution's creation, suppression, ordering or styling
of courtesy glyphs. Do not widen collection across changing headers in this
checkpoint: systemFittingWidth4 still rejects trailer/changing-header prefixes.
This extends final placement only, after native boundary preparation.

Files changed:

- libmscore/spacingplan4.cpp: terminal courtesy eligibility and right margin.
- mtest/libmscore/layout/tst_spacing.cpp: courtesyPlan under both beam options.
- mtest/libmscore/layout/v0.50-spacing-courtesy.mscx: four forced systems with
  both experimental options enabled and explicit courtesy generation enabled.
- This guide: source rule, completed stage, validation and next integration.

Visual controls in v0.50-spacing-courtesy.mscx:

| Label | Boundary/control |
| --- | --- |
| C1 | Key courtesy before C major changes to D major |
| C2 | Time courtesy before 4/4 changes to 3/4 |
| C3 | Combined key/time courtesies before E-flat major and 2/4 |
| C4 | Final system with the resulting key/time header |

The first three systems are justified; the final system stays at natural width.
Compare the last note-to-barline gap, barline-to-courtesy gap, spacing between
combined courtesies and space after the final courtesy. Also compare the next
system's header-to-first-note gap. The new tests require the expected generated
courtesy types, measure-end placement, per-row rhythmic counts, live plan
eligibility and exact final measure/segment application. Changing only the
trailer margin must change natural width by exactly that amount. Removing an
announce segment's trailer flag must reject the query without publishing output.
Check repeated layout and option-off restoration, ignoring inactive native
header/trailer cache entries as established in v0.48.1.

Local validation: moc plus C++11 syntax checks in debug and NDEBUG modes pass
for planner/test sources, with unused-parameter diagnostics enabled. The actual
planner also passes 10,000 cases against pinned placement/lookback,
reconstruction and minimum-width routines with controlled DOM/distance inputs,
now including one/two terminal courtesies and fixed terminal-margin handling.
This validates the math in the isolated model, not live font/DOM integration.
Score XML, whitespace and parent-index patch checks are required before delivery.
Full tst_spacing/tst_beam execution and visual comparisons remain for the user's
Qt build.

Apply v0.50 after v0.49, rebuild and run tst_spacing/tst_beam. Compare
v0.50-spacing-courtesy.mscx in 37E and Studio 4.7.5. Next prepare changing
header/courtesy geometry for tentative collection boundaries before widening
systemFittingWidth4. Clef changes and repeat boundaries remain separate guarded
adapters. Keep existing cross-staff/tremolo handling and default-off restoration.


## v0.50.1: correct courtesy fixture break placement

The user's result(4).xml reports two courtesyPlan failures, one under each beam
option, at the system/measure pointer comparisons. The accompanying XML-reader
diagnostics explicitly identify LayoutBreak as unknown at fixture lines 126,
203 and 265. Other test functions, including squeezePlan and fittingSwitch, pass.

The v0.50 generator incorrectly nested those three LayoutBreak elements inside
voice. Measure::readVoice does not handle LayoutBreak; Measure::read delegates
measure-level properties to MeasureBase::readProperties, which does. Move each
break to be a direct child of Measure, matching the existing v0.41 fixture.
Keep every note, signature, label and style value unchanged. Add an early
courtesyPlan assertion requiring four loaded measures with lineBreak flags on
exactly the first three, so a malformed fixture is diagnosed before layout.
No production code changes. The comparison filename remains
v0.50-spacing-courtesy.mscx so applying the patch repairs the existing input.

Files changed: v0.50-spacing-courtesy.mscx, tst_spacing.cpp and this guide.
Validate XML structure, unchanged musical/style content, parser dispatch and
C++11 syntax; check whitespace and apply against v0.50 before delivery. Full
runtime execution remains for the user's build. Apply v0.50.1 after v0.50,
rerun tst_spacing/tst_beam and reopen the corrected courtesy comparison score.
Changing-header collection remains the next implementation stage after this
terminal-courtesy validation.


## v0.51: simultaneous polyrhythm validation checkpoint

The user reports v0.50.1 mtests passing and supplies courtesy comparison images.
Those controls look close, but do not close chord/rest, multi-voice or accidental
work. At the user's request, prioritize the polyrhythm example from
https://musescore.org/en/node/330793 (Engraving improvements in MuseScore 4.0).
Direct retrieval returned HTTP 403; the supplied figure is the source of the
rhythmic reconstruction. The implementation reference remains pinned v4.7.5,
not the historical article's promise of equivalence for every case.

The duration branch already exists in segmentSpacingStretch4 and
spacing4DurationStretch. It uses onset intervals, the shortest actual chord/rest
duration at that onset, and the preceding non-adjacent onset when appropriate.
Actual durations include tuplets. Earlier H8/H35 controls checked ordinary
triplets, not the four interlocking streams in the supplied figure. That missing
end-to-end validation is the point of this checkpoint.

Add two comparison inputs, both with experimental spacing and beams enabled:

- v0.51-spacing-polyrhythms.mscx: P1 is 3 against 2, P2 is 5 against 4, P3
  reconstructs the four simultaneous streams in the figure. Every measure is
  2/4, with two voices on each piano staff. Each measure has its own justified
  system, using measure-level line breaks.
- v0.51-spacing-polyrhythm-natural.mscx: P3 alone, final-system fill limit 1,
  for a natural-width comparison that does not hide excess space by filling
  the page. Other font/style inputs match the overview.

P3 has these streams:

| Staff/voice | First quarter | Second quarter |
| --- | --- | --- |
| Upper voice 1 | Four sixteenths | Two eighths |
| Upper voice 2 | Five sixteenths in four | Three eighths in two |
| Lower voice 1 | Six sixteenths in four | Four sixteenths |
| Lower voice 2 | Seven sixteenths in four | Five sixteenths in four |

Rhythms follow the figure; constant, separated pitches isolate spacing from
chromatic, chord-second and cross-staff geometry. This is not a note-for-note
facsimile of the figure's pitches. Compare identical files in 37E and 4.7.5.
Look for smooth gaps within each voice, aligned simultaneous onsets, accurate
beat alignment at the quarter boundary, and excess natural width. Collision
constraints can legitimately prevent perfectly uniform visible gaps; compare
the reference rather than imposing equal distance between every adjacent
segment in the merged onset list.

Add polyrhythmInputs for both files under both beam settings. Verify every
voice's exact count, actual duration and rational onset; all four streams must
fill exactly 2/4. Require the intended system grouping. Compare rhythmic weights
against the pinned adjacent/non-adjacent formula using live chord durations.
Check finite nonnegative widths, repeatability and option-off restoration while
ignoring inactive native header caches. These tests establish valid test inputs,
correct rhythmic math and stable switching. They do not claim that every
multi-voice shape is adapted to the new full-system planner; ambiguous or
unsupported geometry can still take the documented fallback. Visual/source
investigation must distinguish a rhythmic-weight issue from a collision-floor
or dispatch boundary before changing the algorithm.

No production code changes in this checkpoint. Files changed: tst_spacing.cpp,
this guide and the two new score files. Local moc and C++11 debug/NDEBUG syntax
checks pass. XML/rational-duration checks confirm two staves, balanced tuplets,
valid measure-level breaks and exact 2/4 totals in every voice. Whitespace and
parent-index patch checks are required before delivery. Full Qt mtests and
visual comparisons remain for the user's build.

Next validation sequence:

1. Compare P1/P2 and both forms of P3 with 4.7.5. Trace discrepancies through
   actual durations, rhythmic weights, active collision floors and full-planner
   eligibility, in that order.
2. Add accidentals, chord seconds/unisons and sustained/rest voices to the same
   rhythms, then cross-staff moves. Separate timing from geometry constraints.
   Individual accidental glyphs are children of Notes inside chord/rest
   segments; keys/time signatures have their own segments. Between-item
   accidental clearance is partly adapted, but Evolution's within-chord and
   multi-voice accidental packing has not been replaced wholesale.
3. Extend nested tuplets, mixed tuplets and transitions across barlines; keep
   natural, justified and squeezed controls and test both beam settings.
4. Return to changing-header collection, clefs and repeats once the core
   interlocking-rhythm controls are understood. The terminal courtesy adapter
   is a boundary milestone, not completion of the entire spacing port.

Apply v0.51 after v0.50.1 and run tst_spacing/tst_beam. Compare the two new
files in both applications. Preserve version prefixes, enabled comparison
settings, independent beam/spacing switches and exact legacy-off restoration.


## v0.52: retain proven shared aggregate producers in multi-voice geometry

The user's v0.51 images expose a clear remaining issue in natural P3: 37E is
much wider and has uneven gaps inside the four streams, whereas Studio 4.7.5
is compact. Do not proceed to accidental packing while this control differs.
The user did not separately confirm v0.51 mtest results in this message; full
runtime validation remains pending.

A concrete adapter fallback exists when two voices emit identical legacy
Chord::shape general-spacing rectangles. Their notes can occupy different staff
positions while their aggregate horizontal bounds coincide at the same y.
Previously the resolver classified different owners at the same rectangle as
ambiguous, including these already identified legacy bounds. Projection then
correctly rejected the unresolved whole shape. This checkpoint addresses that
provenance gap; the user's build must verify that it removes the observed P3
fallback rather than assume every discrepancy has the same cause.

SpacingShapeElement4 now has aggregateOwners for this one case. Keep item null
rather than pick an arbitrary owner. Retain the complete, sorted producer set
only if all matching candidates are known legacy aggregates, there are at least
two distinct owners, no unknown/mixed metadata is present, and native/candidate
multiplicities match exactly. Preserve every native rectangle and its order.
Direct-item ambiguity and mixed-role collisions still fall back. Candidate
ordering and translation must preserve the same producer set.

The separate collision projection validates every aggregate owner: supported
chord notes (or a barline), plus an identified component footprint for each.
Unknown, unsupported, absent, null, duplicate or contradictory owners reject the
whole query without publishing output. Full snapshots still contain the legacy
bounds; only the validated collision projection excludes them. No global native
Shape, skyline or note/accidental geometry is changed.

Strengthen polyrhythmInputs: all P1/P2/P3 systems must now build a full plan and
the actual system/segment geometry must match its natural/justified result.
The prior input/duration, repeatability and option-off checks remain. Report the
planner's rejection reason if another unsupported context is encountered.
Extend geometryResolution with shared producers, ordering, missing footprint,
count mismatch and unknown-producer cases. Update geometryInputs to recognize
the explicit multi-owner legacy metadata rather than require a single item.
Reuse the unchanged v0.51 overview and natural comparison scores.

### Deferred centered cross-staff beam report

Preserve the user's original test-grandstaff.mscz unchanged as
v0.52-beam-centered-grandstaff.mscz. This is a queued reproduction, not a new
current-stage visual comparison or a change to beam geometry. The first measure
has a lower-source-staff beam moved partly to the upper staff, with flats and a
low A-flat moved upward in staff assignment. Several chords have explicit up
stem directions in the supplied file. The user reports that with the centered
beam, 4.7.5 places the beam below that low note / changes the effective stem
side; 37E with experimental beams on puts the beam high and flat, while off has
a slight slant. The screenshots show the substantial placement difference.
Investigate cross-staff anchor placement and effective stem-side handling later;
do not assume a cause or change the score's stem settings to disguise it.
The user explicitly asked to register this without diverting the spacing work.

Files changed: spacinggeometry4.h/.cpp, spacingcollision4.cpp, tst_spacing.cpp,
this guide and the preserved v0.52 beam reproduction. Local moc/C++11 syntax
checks pass in debug/NDEBUG modes. Isolated actual projection checks with
controlled item/policy inputs verify every-owner footprints and atomic fallback.
The actual resolver regression harness covers 100,000 direct-identity cases,
plus shared producer counts/order/translation and mixed/unknown-role rejection.
These are local adapter checks; they do not replace live-score execution.
Whitespace, binary fixture identity and parent-index apply checks are required
before delivery. Full tst_spacing/tst_beam and visual confirmation remain for
the user's Qt build.

Apply v0.52 after v0.51. Rebuild and run tst_spacing/tst_beam. Recompare the same
v0.51 natural P3 first, then the justified overview. If the plan is still
rejected, use its exact test failure reason for the next adapter stage. If it
is applied but gaps still differ, inspect active pair floors and rhythmic
advances before adding accidental/rest/cross-staff variants. The registered
beam issue stays deferred, and default-off restoration remains required.

## v0.53: accidental controls for the same four-stream polyrhythm

The user reports full mtests passing after v0.52. The updated natural P3
screenshots show the excess width removed and regular gaps within each voice.
After allowing for zoom, overall width and beat alignment look close to 4.7.5.
This validates the plain-note control; it does not establish parity for every
collision combination.

This checkpoint changes test inputs and assertions only. Two new scores retain
the exact P3 durations, tuplets, octaves, voices and forced stem directions:

- v0.53-spacing-polyrhythm-accidentals-natural.mscx: natural-width A1.
- v0.53-spacing-polyrhythm-accidentals.mscx: the same A1 justified to page width.

Each voice cycles F-sharp, F-natural, F-flat, F-natural in its original octave.
This keeps the staff positions constant while introducing changing accidental
footprints at the irregular onsets. Both experimental options are already on.
The natural file isolates required spacing; the justified file checks how the
same floors interact with system expansion. Open the same files in 4.7.5.
Compare signs against neighboring noteheads and stems, the common second-beat
boundary, and the gaps within each voice. Do not expect equally spaced notes
where an accidental collision requires additional room.

Extend polyrhythmInputs with both files under both beam settings. Verify exact
pitches/spellings and generated signs in each voice, all original rational
durations, full-plan eligibility and actual application, repeat layout and
legacy-off restoration. The original P1/P2/P3 rows remain intact. These inputs
exercise inter-item spacing; within-chord accidental packing is still
Evolution's implementation.

Files changed: tst_spacing.cpp, this guide and the two v0.53 scores. Local
validation covers XML structure, unchanged rhythmic streams, enabled styles,
moc, C++11 debug/NDEBUG syntax, whitespace and parent-index patch application.
Full tst_spacing/tst_beam and live visual comparison remain for the user's
build. No production engraving code changes in this milestone.

Apply v0.53 after v0.52. Compare natural A1 first, then justified A1. If a plan
is rejected or a collision differs, trace the exact producer/pair floor before
changing layout policy. Next extend the controlled rhythms to chord seconds
and unisons, then rests/sustained voices and cross-staff combinations. The
registered centered-beam issue remains queued separately.

## v0.53.1: fix Qt 5.9 test comparison linkage

The new note-count assertion compared vector::size_type with an int literal.
On the user's Qt 5.9 build this instantiated an unavailable mixed-type
QTest::qCompare<unsigned long, int>, failing at link time. Cast the count to
int, matching the expected value and existing count assertions. Syntax-only
checks cannot detect that missing template definition; a small same-type
comparison now compiles, links and runs with the locally available Qt 5.15.
The user's full Qt 5.9 tst_spacing build/run remains the final check.

Files changed: tst_spacing.cpp and this guide. No score or production changes.
Apply v0.53.1 after v0.53, rebuild tst_spacing, then resume the two A1 visual
comparisons before extending the controls.

## v0.54: correct the accidental control spellings

The user reports mtests passing after v0.53.1. Both natural and justified A1
screenshots look close to 4.7.5, but also expose a fixture error: changing staff
positions and double flats were not the intended F-sharp/F-natural/F-flat cycle.
The original MIDI pitches were correct; the spelling values 25 and 1 were
incorrect. F-sharp is TPC_F_S (20), F-natural is TPC_F (13), and F-flat is
TPC_F_B (6). The old assertions repeated the erroneous values and therefore
did not detect the inconsistency. This is a test-input correction, not evidence
of a production accidental spacing defect.

Rename the two scores with the v0.54 prefix, update their titles and correct
the spellings. Rhythms, voices, pitches, forced directions, styles and natural
versus justified settings stay the same. Both experimental options remain on.
The renamed files replace the old v0.53 controls:

- v0.54-spacing-polyrhythm-accidentals-natural.mscx
- v0.54-spacing-polyrhythm-accidentals.mscx

Use named Tpc constants in the assertions. Independently check spelling-to-MIDI
pitch agreement, constant diatonic step, and generated sharp/flat signs on the
first/third notes of each voice. Retain complete-plan application, rational
rhythm, repeat-layout and off-restoration checks under both beam settings.

Files changed: the two renamed scores, tst_spacing.cpp and this guide. Local
validation covers all 36 MIDI/spelling pairs in each score against the actual
pitch-spelling functions, unchanged rhythmic content/settings, moc/C++11
debug/NDEBUG syntax, whitespace and parent-index patch application. The full
Qt 5.9 mtests and live comparison remain for the user's build.

Apply v0.54 after v0.53.1. Recompare natural A1 first, then justified A1 in
37E and 4.7.5. All noteheads in each voice should now occupy one staff position;
the accidental cycle should include ordinary sharps, naturals and flats.
Resolve any demonstrated pair-floor difference before adding chord seconds
and unisons. No production layout code changes in this checkpoint; the
registered centered-beam issue stays deferred.

## v0.55: chord seconds at irregular multi-voice onsets

The user reports full mtests passing after v0.54. Its corrected screenshots
show the intended ordinary sharp/natural/flat cycle on constant staff positions.
Natural width and justified spacing look close to 4.7.5, with no obvious
accidental collisions at the supplied scale. This is visual acceptance of A1,
not a claim of exact numeric equality or complete accidental-packing parity.

Add C1 in two files, both with experimental spacing and beams enabled:

- v0.55-spacing-polyrhythm-seconds-natural.mscx
- v0.55-spacing-polyrhythm-seconds.mscx

Keep every P3 rhythmic stream and A1 pitch/spelling. Add the corresponding G
above each F, with the same alteration: F-sharp/G-sharp, F/G, F-flat/G-flat,
F/G. This gives two-note seconds under up/down stems in all four voices.
The natural control exercises displaced notehead and two-accidental footprints;
the justified control adds system expansion to those same constraints.
No explicit notehead displacement or accidental offsets are stored in the
scores. Evolution still handles packing within each chord, so differences in
those local offsets should be distinguished from inter-segment spacing.

Extend polyrhythmInputs under both beam settings. Check exactly one F and one
G per chord without depending on note-vector ordering, source spelling/MIDI
agreement, first sharp and first flat signs, and actual mirrored noteheads.
Retain rational onsets/durations, full-plan eligibility and actual application,
repeat layout and legacy-off restoration. The earlier plain-note and A1 rows
remain. The two new files differ from A1 only in titles/labels and added notes.

Files changed: tst_spacing.cpp, this guide and the two C1 scores. Local checks
cover original rhythmic/settings preservation, all added pitch/spelling pairs
against the source pitch functions, moc/C++11 debug/NDEBUG syntax, whitespace
and parent-index patch application. Full Qt 5.9 mtests and visual comparison
remain for the user's build. No production engraving code changes here.

Apply v0.55 after v0.54. Compare natural C1 first, then justified C1 in 37E and
4.7.5. Inspect the seconds' opposing noteheads, accidental columns, neighboring
stems and shared beat boundary. Use an exact test failure reason or a visible
pair discrepancy to choose a production change. Next prepare unisons between
voices as a separate control; then rests/sustained voices and cross-staff.
The centered cross-staff beam reproduction remains deferred.

## v0.56: register the shared-notehead boundary with unison controls

The user reports full mtests passing after v0.55. Natural and justified C1
screenshots look close to 4.7.5 in overall spacing, displaced noteheads and
accidental columns. No clear production spacing defect is visible in that
comparison. Keep those controls unchanged.

Add U1 using the same four P3 rhythmic streams, with single F notes and no
accidentals. Both voices on the treble staff use F5; both bass voices use F3.
Opposing stems and the original tuplets remain. Both experimental options are
enabled, in natural and justified variants:

- v0.56-spacing-polyrhythm-unisons-natural.mscx
- v0.56-spacing-polyrhythm-unisons.mscx

Source inspection identifies a deliberate adapter limit: Chord::shape includes
each voice's Note::shape, while spacingResolveShape4 rejects coincident direct
rectangles with different item owners as AMBIGUOUS. v0.52 retained producer
sets only for legacy aggregate spacers, not direct noteheads. Shared heads
therefore need a separate ownership adaptation; choosing one voice would lose
potentially different pair policies. Do not claim these controls already use
the full Studio placement planner.

Extend polyrhythmInputs under both beam settings. Check exact unison pitches,
spellings, absence of accidentals, all original rational durations, finite
geometry, repeat layout and off restoration. For these U1 inputs specifically,
expect an unadapted-geometry rejection, require AMBIGUOUS entries in both
staff shapes at the first onset, and verify the failed planner leaves its
output untouched. All preceding eligible controls still require complete
plan application. The existing fallback remains active for U1.

Files changed: tst_spacing.cpp, this guide and the two U1 scores. Local checks
cover source-based shared-rectangle resolver behavior, unchanged rhythm and
settings, pitch/step agreement, moc/C++11 debug/NDEBUG syntax, whitespace and
parent-index patch application. Live Qt 5.9 mtests and screenshots remain for
the user's build. No production code changes in this diagnostic checkpoint.

Apply v0.56 after v0.55. Run tst_spacing/tst_beam, then compare natural U1 and
justified U1 with 4.7.5. Differences are useful baseline evidence, not a reason
to tune fallback widths. The next source stage is to retain every proven
direct-item producer and apply all relevant pair policies, with unknown or
incomplete ownership still rejected. After enabling that bounded case, change
U1's eligibility assertions to require full-plan application and recompare.
The centered cross-staff beam reproduction stays deferred.

## v0.57: retain both voices' shared-notehead policies

The user reports v0.56 tests passing, confirming the registered geometry
fallback. Its natural U1 screenshot is substantially wider than 4.7.5, while
the justified pair looks much closer because both fill the system. This is
the expected place to adapt provenance, rather than tune duration spacing.

At staff-shape assembly, spacingResolveShape4 can now retain a SHARED_ITEM
producer set when every coincident native primitive has exactly one distinct,
identified direct candidate. Candidate/native counts must match exactly;
unknown candidates, mixed roles and duplicated producers do not qualify.
Native rectangle count/order/coordinates remain intact. Ordinary element
resolution keeps the previous conservative behavior. Producer ordering is
stable and no single voice is selected as the owner.

The collision projection accepts these producer sets only for supported notes.
It expands each shared rectangle to every producer, so the existing all-pairs
distance query evaluates each voice's context and keeps the strongest floor.
Shared notes can also establish each chord's component footprint when removing
redundant legacy aggregate bounds. Unsupported shared types, malformed sets or
any unsupported owner reject the whole projection without publishing changes.
Raw shared snapshots still require projection; they are not complete single-
owner geometry. No native Shape, notehead packing or legacy layout code changes.

Update geometryResolution with default rejection, explicit staff-mode admission,
exact multiplicity, order/translation invariance, every-owner projection,
malformed/unsupported metadata and unchanged-output checks. Change U1 from its
diagnostic rejection to requiring shared producers on both staves, complete
plan acceptance and actual application. Keep all earlier polyrhythm rows,
both beam settings, repeat layout and legacy-off restoration.

Files changed: spacinggeometry4.h/.cpp, spacingcollision4.h/.cpp,
tst_spacing.cpp and this guide. No new scores are necessary: reuse both v0.56
U1 files, with both experimental options already on. Local checks include
moc/C++11 debug/NDEBUG syntax for changed sources, 100,000 previous actual
resolver cases plus 10,000 shared direct-owner cases, actual projection-body
checks with controlled contexts, whitespace and parent-index patch application.
These checks do not replace live Qt 5.9 score execution.

Apply v0.57 after v0.56. Rebuild and run tst_spacing/tst_beam; compare natural
U1 first and justified U1 second against the same 4.7.5 screenshots. Full
mtests and rendered parity remain pending on the user's build. If another
producer is rejected, use the exact failure reason for the next bounded
adaptation. Once U1 is accepted visually, proceed to rests/sustained voices
before cross-staff combinations. The centered-beam issue stays deferred.

## v0.58: internal rests in independent rhythmic streams

The user reports v0.57 mtests passing. The updated natural U1 screenshot now
looks close to the previous 4.7.5 capture in width and note placement; the
justified pair also looks close. Keep the shared-notehead implementation and
its acceptance tests unchanged.

Add R1 using the original separated-octave P3 streams. Replace the second
chord/rest event in each voice with an ordinary rest of the exact same written
and actual duration. Retain its explicit middle beam mode, so these are
internal beamed rests. The other notes, rhythms, tuplets and forced note stem
directions remain the same. The rests occur at different rational onsets in
the four voices. These are not gap, cue or centered full-measure rests.

New scores, both with experimental spacing and beams enabled:

- v0.58-spacing-polyrhythm-rests-natural.mscx
- v0.58-spacing-polyrhythm-rests.mscx

Extend polyrhythmInputs under both beam settings. Require exactly the intended
rest event in every voice, actual beam membership, unchanged event counts and
rational durations, full-plan eligibility and actual application, finite
geometry, repeat layout and restoration when experimental spacing is off.
The existing shortest-duration formula checks now include rest onsets as well.
No production layout changes are introduced in this checkpoint.

Files changed: tst_spacing.cpp, this guide and the two R1 scores. Local checks
cover XML structure, exact restoration to P3 after undoing the four intended
rest substitutions, preserved beam modes/settings, moc/C++11 debug/NDEBUG
syntax, whitespace and parent-index patch application. Full Qt 5.9 mtests,
beam membership and live-score rendering remain for the user's build.

Apply v0.58 after v0.57. Compare natural R1 first and justified R1 second with
4.7.5. Inspect rest-to-neighbor clearance and gaps in the other voices at each
rest onset. Distinguish vertical rest placement from horizontal spacing before
choosing a code change. Once accepted, introduce sustained voices separately,
then cross-staff combinations. The centered-beam issue remains deferred.

## v0.59: bracket explanation and sustained-voice controls

The user reports v0.58 mtests passing. R1's 37E captures show automatic brackets
on the first quintuplet, sextuplet and septuplet, with none on the later triplet
and quintuplet. Tuplet::layout in libmscore/tuplet.cpp explains this directly:
AUTO_BRACKET is enabled when any element is a rest (or a nested tuplet), even
if the rest is beamed; an all-note group can omit it when every event is beamed.
The compared 4.7.5 captures omit these brackets. Do not change bracket policy
as part of horizontal spacing. Tuplet brackets/numbers are not inputs to this
spacing projection, though their display can affect vertical appearance.

R1 also has a small natural-width difference and visibly different rest heights.
These cannot be treated as exact spacing parity. The pinned restlayout-4.7.5.cpp
uses a newer rest placement/collision pipeline; computeVoiceOffset selects a
one-space offset by default (multiVoiceRestTwoSpaceOffset is false in pinned
styledef-4.7.5.cpp). Evolution's Rest::layout uses duration-specific offsets and
opposite-voice note-line avoidance. Different vertical shapes can activate
different horizontal collision floors. Record this for a bounded rest-layout
audit rather than compensating with rhythmic spacing constants. The current
tests establish full-plan application, not identical rest placement.

Add S1 as a separate sustained-voice control:

- v0.59-spacing-polyrhythm-sustained-natural.mscx
- v0.59-spacing-polyrhythm-sustained.mscx

Preserve P3's upper stream on each staff: six events on track 0 and ten on
track 4, including the sextuplet. Replace each lower stream with a single
half note at its original F pitch, lasting the entire 2/4 measure. No ties,
rests, accidentals or cross-staff moves are added. Both experimental options
remain on. Natural and justified versions isolate placement and expansion.

Extend polyrhythmInputs under both beam settings. Require counts 6/1/10/1,
exact original upper-stream onsets, half-note durations and pitches in the
lower voices, no beam/tuplet on those half notes, full-plan eligibility and
actual application, duration-formula agreement, repeat layout and off restoration.
Earlier control rows remain unchanged. No production layout changes here.

Files changed: tst_spacing.cpp, this guide and the two S1 scores. Local checks
cover XML structure, original upper streams/styles, exact lower-voice duration
and spelling, moc/C++11 debug/NDEBUG syntax, whitespace and parent-index patch
application. Full Qt 5.9 mtests and rendered comparisons remain pending.

Apply v0.59 after v0.58. Compare natural S1 first, then justified S1 with 4.7.5.
Inspect the moving notes' intervals above the sustained voices and the common
beat boundary. Continue with cross-staff combinations after S1 validation;
keep R1's rest-position difference and the centered-beam reproduction queued
as explicit remaining work.

## v0.60: cross-staff source audit and X1 fallback baseline

The user reports v0.59 tests passing. Natural and justified S1 look close to
4.7.5, including regular moving-note intervals over the sustained voices.
Continue with cross-staff while retaining the explicit R1 rest-position and
centered-beam differences as remaining work.

See v0.60-cross-staff-spacing-audit.md for the pinned source rules, existing
bounded fallback coverage and integration order. The full planner still lacks
cross-stem, signed fixed-displacement and nonadjacent-beam constraints. Its
guard stays in place. This checkpoint does not change production behavior.

Add natural/justified X1 with both experimental options enabled:

- v0.60-spacing-cross-staff-natural.mscx
- v0.60-spacing-cross-staff.mscx

X1 has one alternating cross-staff eighth-note beam above two sustained voices,
with no rests, tuplets, accidentals or beam-position overrides. The new
crossStaffPlanBoundary test covers both beam settings, exact source rhythm,
actual cross-staff beam membership, the current planner rejection, unchanged
output/live positions, repeat layout and default-off restoration.

Files changed: tst_spacing.cpp, this guide, the audit and two X1 scores. Local
checks cover XML/rational rhythm, intended moves/pitches/styles, moc/C++11
debug/NDEBUG syntax, whitespace and parent-index patch application. Full
Qt 5.9 tests and live comparison remain for the user's build.

Apply v0.60 after v0.59. Run tst_spacing/tst_beam and compare natural X1 first,
then justified X1 with 4.7.5. These are intentionally fallback baseline captures.
Next implement the bounded read-only cross-staff constraints described in the
audit, then require full-plan application for X1 before expanding the cases.

## v0.61 WIP: build the cross-staff context without changing placement

The user reports v0.60 tests passing. Natural and justified X1 are fairly
close to the supplied 4.7.5 captures, while the tests confirm intentional
fallback. Continue the audited implementation rather than inferring full
planner support from those screenshots.

Add CrossStaffContext4 and buildCrossStaffContext4 in spacingplan4.h/.cpp.
They validate a bounded automatic single-note eighth-note beam, infer effective
stem sides from proven staff placement, and describe signed fixed offsets
and minimum-width floors for adjacent pairs. Retain un-beamed companions in
the adjustment decision. See the appended v0.61 section in the cross-staff
audit for exact eligibility, rejected cases and remaining integration.

Extend crossStaffPlanBoundary to validate X1's context and pair values under
both beam settings, unchanged live positions/stem sides and atomic rejection
for a forced stem direction. Keep the planner's deliberate rejection and
repeat/off restoration checks. The builder has no production caller yet;
no rendering change or new scores/screenshots are expected at this checkpoint.

Files changed: spacingplan4.h/.cpp, tst_spacing.cpp, this guide and the cross-
staff audit. Local moc/C++11 debug/NDEBUG syntax checks pass. An isolated
actual-function-body harness with controlled score contexts covers 280 cases
of staff-side patterns, dimensions and sustained companion onsets, verifying
offsets/floors plus atomic rejection of forced directions, manual edits,
unsupported notes and interstaff positions. Full Qt 5.9 tests remain pending.
Whitespace and parent-index patch checks are required before delivery.

Apply v0.61 after v0.60 and run tst_spacing/tst_beam. No visual recapture is
needed. Next integrate explicit cross-stem geometry and fixed displacement
with the planner's reconstruction/justification, then require actual plan
application for X1. Rest positioning and the centered-beam case stay queued.

## v0.62 WIP: preserve fixed spacing during spring expansion

The user reports v0.61 compilation and mtests passing. Save the spring dependency
as a separate recoverable stage before connecting cross-staff geometry.

Add a zero-default widthOffset field to SpacingSegmentPlan4. The per-measure
minimum-width solver and whole-system justification both subtract this fixed
signed value before solving flexible widths, then restore it unchanged.
Reconstruction continues to use total widths. See the appended audit section
for the pinned Studio routines and remaining integration order.

Add fixedOffsetPlan with hand-solved floor/expansion cases for zero, positive
and mixed-sign offsets, exact target and position reconstruction, invalid
input preserving output, and no writes to native score positions. All prior
context, planner-boundary and default-off checks remain intact.

Files changed: spacingplan4.h/.cpp, tst_spacing.cpp, this guide and the cross-
staff audit. Local moc/C++11 debug/NDEBUG syntax checks pass. Actual spring
function-body checks with controlled measures cover 5,000 signed-offset plans
against an independent bisection solution, plus minimum-width reflow and atomic
failure. The earlier 10,000 zero-offset source-placement comparisons also pass.
Full Qt 5.9 mtests remain for the user's build. No new score files are needed.

Apply v0.62 after v0.61 and run tst_spacing/tst_beam. No rendering change is
expected because production plans still carry zero offsets and retain the
cross-staff guard. Next connect stem constraints, context offsets and final
placement, then require X1 full-plan application and request visual comparison.

## v0.63 WIP: stem-anchor constraints saved before integration

The user reports v0.62 compilation and mtests passing and has little usage
remaining. Save the next bounded dependency as a recoverable checkpoint.

CrossStaffChord4 now records a horizontal stem center inferred from the source
glyph-attachment calculation. CrossStaffPair4 retains the explicit up-to-down
stem clearance in addition to its existing fixed-offset/minimum-width values.
Manual note offsets and invalid stem dimensions fail without publishing a
partial context. See the cross-staff audit for exact scope and source mapping.

Files changed: spacingplan4.h/.cpp, tst_spacing.cpp, this guide and the audit.
The live X1 test compares inferred anchors with actual stem centers under both
beam settings and verifies the explicit stem floor. Local moc/C++11 debug/
NDEBUG syntax and controlled actual-context checks pass. Full Qt 5.9 mtests
remain pending. Whitespace and parent-index patch application are checked.

Apply v0.63 after v0.62 and run tst_spacing/tst_beam. No visual recapture is
needed: the full planner still rejects cross-staff geometry. Next integrate
the bounded geometry and fixed offsets with placement, retaining fallback for
unsupported contexts. This WIP is not completion of cross-staff integration.

## v0.64: enable the bounded cross-staff baseline in the placement planner

The user reports v0.63 tests passing. Connect X1's context to temporary stem
geometry, pair floors, natural advances and fixed-offset reconstruction.
Final layout now replaces each native offset with the planned value after a
whole fitting plan succeeds. See the appended audit section for exact scope
and the conservative treatment of unknown vertical stem length.

Eligibility remains narrow: one complete alternating eighth-note beam in one
two-staff system measure, with only start-aligned measure-long companions.
Other cross-staff contexts retain fallback. No native shapes, stem directions,
beam anchors or legacy-off algorithms are changed. The broader centered-beam
reproduction and R1 rest-placement difference remain queued.

Update crossStaffPlanBoundary to require full-plan acceptance and actual
positions, widths and fixed-offset application for X1 under both beam settings.
Keep rejection/atomicity for forced directions and repeat/off restoration.
No new score files: reuse v0.60-spacing-cross-staff-natural.mscx and
v0.60-spacing-cross-staff.mscx, with both experimental options enabled.

Files changed: spacingplan4.cpp, layout.cpp, tst_spacing.cpp, this guide and
the audit. Local syntax, controlled zero/signed-offset planner checks and
injected cross-constraint builder checks pass as detailed in the audit.
Full Qt 5.9 mtests and rendered comparison remain pending; the controller
harness does not substitute for live cross-staff geometry/dispatch tests.
Whitespace and parent-index patch application are checked before delivery.

Apply v0.64 after v0.63 and run tst_spacing/tst_beam. If they pass, compare
natural X1 first, then justified X1 with the existing 4.7.5 captures. Use any
failure reason or visible difference to refine this scope before expanding
to same-side stem crossings and interleaved moving voices.

## v0.65: same-staff runs within the bounded cross-staff beam

The user reports v0.64 mtests passing. Natural and justified X1 captures look
close to Studio 4.7.5; this is visual acceptance of the bounded baseline, not
proof of general cross-staff parity.

Remove the alternating-only guard. The existing context already represents
same-staff neighbors with zero signed offset. All other eligibility limits,
temporary stem geometry, collision floors and fallback behavior remain.

New comparison files (both experimental options enabled):
- v0.65-spacing-cross-staff-runs-natural.mscx
- v0.65-spacing-cross-staff-runs.mscx

X2 has four eighths on upper/upper/lower/lower staves, with the same held
companion voices as X1. Compare natural width first, then justified gaps,
especially the middle staff change. Open each original in both applications.
The test requires full-plan acceptance and actual placement under both beam
settings, one nonzero fixed offset, repeat stability and legacy-off restoration.

Files changed: spacingplan4.cpp, tst_spacing.cpp, this guide, the cross-staff
audit and the two new scores. Local moc/C++11 debug/NDEBUG syntax and fixture
structure checks pass. Full Qt 5.9 mtests and X2 visual comparison are pending.
Apply after v0.64 and run tst_spacing/tst_beam. Next audit interleaved moving
voices and nonadjacent beam-neighbor constraints before widening that guard.

## v0.66: reverse-run control before moving companions

The user reports v0.65 tests passing; natural and justified X2 screenshots
look close to Studio 4.7.5. Add X3, lower/lower/upper/upper, reversing X2's
staff moves and pitches while preserving rhythm, held voices and styles.
This exposes the positive fixed offset and up-to-down minimum stem floor;
X2's only staff change uses a negative offset and no such floor.

New files, with both experimental styles enabled:
- v0.66-spacing-cross-staff-reverse-runs-natural.mscx
- v0.66-spacing-cross-staff-reverse-runs.mscx

Extend crossStaffPlanBoundary under both beam settings to require actual
placement, one fixed offset, the explicit stem-distance floor, repeat stability,
forced-direction rejection and legacy-off restoration. No production algorithm
changes. Files changed: tst_spacing.cpp, these two scores, guide and audit.
Local syntax/fixture/patch checks precede delivery; full mtests and rendering
remain pending. Apply after v0.65 and compare natural X3 before justified X3.

Next audit moving companions and nonadjacent beam neighbors. Pinned Studio's
minStemDistOnNonAdjacentCross finds the preceding chord on the beam even when
another voice inserts a segment between the two chords; it enforces stem-center
separation plus stem padding for up-to-down neighbors. Signed advance adjustment
is a separate adjacent-segment rule. Our current held-voice scope avoids that
interleaving, so do not remove its guard without adapting both paths.

## v0.67 WIP: interleaved companion baseline X4

The user reports v0.66 mtests passing. Natural and justified X3 captures look
close to Studio 4.7.5, including the lower-to-upper transition. Preserve that
accepted scope while adding the next unsupported input, X4.

New files (both experimental options on):
- v0.67-spacing-cross-staff-interleaved-natural.mscx
- v0.67-spacing-cross-staff-interleaved.mscx

Keep X3's four eighths and lower held half note. Replace the upper held half
note with eight un-beamed sixteenths on the same C4 pitch, stems down. This
inserts one segment between every pair of consecutive beam chords. The flags
are intentional: there is only one beam in the score, isolating interleaving
from independent-beam handling. Compare each original in both applications,
natural first. Differences are diagnostic at this checkpoint, not parity claims.

The new crossStaffInterleaved test validates exact onset/duration/staff/pitch
structure, the un-beamed companion, finite placement, atomic context/plan
rejection, repeat stability and legacy-off restoration under both beam settings.
No production code changes. Files changed: test, guide, audit and two scores.
Local moc/C++11 debug/NDEBUG syntax and XML structure checks precede delivery;
full live mtests and visual comparison remain pending. Apply after v0.66.

Source audit: buildCrossStaffContext4 currently requires each beam chord's
segment duration to equal its eighth-note duration. X4 deliberately violates
that condition (segment 1/16, chord 1/8). prepareCrossStaff4 also limits companion
onsets to measure-long held chords at tick zero. These are separate guards.
Do not simply delete them: the current pair minimumWidth includes adjacent
cross-beam correction floors, whereas Studio's nonadjacent rule uses only the
beam-neighbor stem-center distance plus stem padding. Signed rhythmic offsets
also belong to adjacent segments and must not be attached across an inserted
segment. The temporary extended-stem geometry needs review for moving targets.

Next adapt adjacency classification, nonadjacent stem constraints and companion
geometry together in a bounded placement stage; then change this test from
expected fallback to actual plan application. This WIP saves the reproduction
and rejection boundary before production changes.

## v0.68 WIP: connect X4 interleaving to the placement planner

The user reports v0.67 mtests passing. X4's justified capture looks close to
Studio 4.7.5, while Evolution's fallback natural layout is slightly wider.
Reuse the same two v0.67 files; this checkpoint changes the algorithm, not the
musical input. Both experimental options already start enabled.

Classify consecutive beam chords by whether their chord/rest segments are
adjacent. An inserted onset permits a shorter positive segment duration than
the eighth-note chord duration. Nonadjacent pairs receive no signed rhythmic
offset and no adjacent cross-beam minimumWidth floor. The explicit up-to-down
stem-center distance plus stem padding remains a pair-distance constraint.
Ensure lookback reaches that beam neighbor even when the ordinary geometric
lookback would stop at an intervening segment. Adjacent X1-X3 corrections stay
covered by their existing actual-application tests.

Placement now admits one complete un-beamed ordinary sixteenth-note companion
stream in the same single-measure grand-staff context, alongside start-aligned
held companions. Validate every companion onset, including segments without a
beam chord. Other moving rhythms, additional moving streams, companion beams,
rests, tuplets, manual cross-beam stems and wider system contexts still fall
back. Temporary cross-stem geometry retains the conservative extension used
in X1-X3; this stage is not the complete Studio stem-shape pipeline. Its visual
behavior against moving targets must be evaluated with the supplied captures.

Change crossStaffInterleaved from expected fallback to full-plan acceptance
and actual application: inferred nonadjacency, zero fixed offsets, explicit
stem clearance, system/segment positions and widths, repeat stability and
legacy-off restoration under both beam settings. Forced cross-beam direction
and an invalid companion duration must reject atomically.

Files changed: spacingplan4.cpp/.h, tst_spacing.cpp, guide and audit. Local
moc/C++11 debug/NDEBUG syntax checks pass. An actual context-body harness with
controlled DOM covers 560 adjacent/interleaved cases across staff patterns,
anchors, offsets and rejection. An actual builder/distance-body harness with
injected pair constraints and controlled geometry covers 1,000 nonadjacent
lookback/floor/justification cases; removing the required lookback makes the
regression fail. These harnesses do not validate live companion eligibility,
stem rectangles or application dispatch. Full Qt 5.9 mtests and rendered X4
comparison remain pending on the user's build.

Apply v0.68 after v0.67, run tst_spacing/tst_beam, then reuse
v0.67-spacing-cross-staff-interleaved-natural.mscx followed by
v0.67-spacing-cross-staff-interleaved.mscx. Compare natural width and the
lower-to-upper beam transition, then justified rhythm and flag clearance.
Next resolve any X4 discrepancy before widening to accidentals/chords and
additional moving voices. No legacy-off or vertical beam-anchor policy changes.

## v0.69: grouped interleaved accidental controls X5/X6

The user reports v0.68 mtests passing. X4 now looks close to Studio 4.7.5 in
natural and justified captures; the previous natural-width excess appears
resolved. Keep that bounded implementation and probe its typed accidental
geometry without changing production algorithms.

New files, both experimental styles enabled:
- v0.69-spacing-cross-accidentals-natural.mscx
- v0.69-spacing-cross-accidentals.mscx
- v0.69-spacing-companion-accidentals-natural.mscx
- v0.69-spacing-companion-accidentals.mscx

X5 changes only the four beam notes to F-sharp3/F-natural3/F-flat4/F-natural4.
X6 changes only the moving companion to C-sharp4/C-natural4/C-flat4/C-natural4,
repeated twice. All original X4 rhythm, staff moves, beam modes, held voice and
style settings remain. Accidentals are generated from pitch/spelling rather
than manually positioned. Keep source originals when opening in Studio.

Extend crossStaffInterleaved under both beam settings to validate pitches,
named TPC values, diatonic steps and actual generated accidentals. Require the
same full-plan application, explicit stem floor, repeat stability, atomic
rejection and legacy-off restoration as X4. These cases must not silently pass
through the fallback. Compare natural X5 then natural X6, then their justified
versions: inspect accidental-to-note and accidental-to-preceding-stem/flag
gaps, rhythm and the lower-to-upper beam transition.

Files changed: tst_spacing.cpp, this guide, audit and four scores. No production
code changes. Local moc/C++11 debug/NDEBUG syntax checks, fixture structure
comparisons and source pitch/step validation pass before delivery. Full Qt 5.9
mtests and visual comparisons remain pending. Apply after v0.68.

Next use these results to fix any accidental/cross-stem discrepancy, then
address multi-note cross-staff chords and additional moving voices as the next
group. Broader systems/compression and the deferred centered-beam/rest-position
issues remain outside this checkpoint.

## v0.70 WIP: two-note cross-staff chords, X7/X8

The user reports v0.69 mtests passing. Natural and justified X5/X6 captures
look close to Studio 4.7.5, without an obvious accidental-clearance discrepancy.
Extend the bounded beam context from one note to at most two ordinary,
unmirrored notes per chord. Companion voices retain their previous scope.

Derive horizontal stem center from the appropriate outer reference note,
matching Chord::stemPosX: upNote for up stems, downNote for down stems. Cover
the complete chord stem in the temporary collision shape by starting at the
opposite outer note: downNote for up stems, upNote for down stems. Validate
visibility, notehead type, offsets, pair policy and within-staff line bounds
for every note rather than only the first. Manual/mirrored/interstaff cases
still reject. Existing single-note behavior and legacy-off paths are retained.
This is still the conservative bounded stem geometry, not full Studio parity.

New files (both experimental styles enabled):
- v0.70-spacing-cross-chords-natural.mscx
- v0.70-spacing-cross-chords.mscx
- v0.70-spacing-cross-accidentals-chords-natural.mscx
- v0.70-spacing-cross-accidentals-chords.mscx

X7 adds an A a major third above each F of X4, preserving the lower/lower/
upper/upper staff pattern and moving C4 companion. X8 does the same to X5's
accidental cycle: both notes use sharp/natural/flat/natural. Open the same
originals in both applications. Compare natural X7 then natural X8 before
the justified versions. Inspect stem attachment through both noteheads,
accidental clearance, natural group width and the staff-change gap. Native
within-chord accidental packing remains Evolution's handling.

Extend crossStaffInterleaved to require exact two-note pitch/TPC/step structure,
unmirrored heads, full-plan actual application, inferred/native stem-anchor
agreement, explicit nonadjacent stem clearance, repeat stability and legacy-off
restoration under both beam settings. A manually offset second note must reject
atomically, as must the existing forced-direction and invalid-companion cases.

Files changed: spacingplan4.cpp/.h, tst_spacing.cpp, guide, audit and four scores.
Local moc/C++11 debug/NDEBUG syntax checks pass. Actual context-body checks with
controlled DOM cover 1,120 single/dyad and adjacent/interleaved cases, including
different attachment anchors on the second note and rejection of its offset or
interstaff line. XML comparisons preserve all other musical/style structure;
source pitch/step functions validate F/A spellings. These checks do not validate
live chord shapes, collision projection or dispatch. Full Qt 5.9 mtests and
rendered comparison remain pending. Apply after v0.69.

Next resolve any X7/X8 mismatch, then broaden moving companion voices. Mirrored
seconds, larger chords and broader system/compression combinations remain
outside this checkpoint; they must not be implied by a passing thirds control.

## v0.71 WIP: two moving companion voices, X9/X10

The user reports v0.70 mtests passing. Blue-paper Evolution captures of X7/X8
look close to Studio 4.7.5 in natural/justified spacing and stem attachment.
Use blue paper as the Evolution identifier for subsequent user comparisons.

Extend only the companion-stream eligibility: maintain a separate onset count
per track and require every moving stream to span the complete measure in
ordinary un-beamed sixteenths. Admit at most two such streams. Existing
single-note companion, visibility/manual-context, system and beam limits
remain. Signed offsets, nonadjacent stem floors and collision projection are
unchanged; geometry is already collected on both staves. The conservative
cross-stem extent is still in use, so visual review remains necessary.

New files, both experimental styles enabled:
- v0.71-spacing-cross-chords-two-voices-natural.mscx
- v0.71-spacing-cross-chords-two-voices.mscx
- v0.71-spacing-cross-chords-two-voices-accidentals-natural.mscx
- v0.71-spacing-cross-chords-two-voices-accidentals.mscx

X9 keeps X7's four cross-staff F/A eighth-note thirds and upper moving C4
sixteenths. Replace the lower held C3 half note with eight un-beamed C3
sixteenths, stems down. X10 changes both companions to the repeated
sharp/natural/flat/natural cycle, leaving the beam chords natural. Compare
natural X9 and X10 first, then justified versions. Check gaps/flags on both
staves, accidental clearance and the lower-to-upper stem transition. Within-
chord accidental packing and vertical beam-anchor policies remain unchanged.

Extend crossStaffInterleaved to validate every lower companion onset, duration,
pitch and spelling in addition to the existing upper-voice checks. Require
full-plan acceptance and actual system/segment placement under both beam
settings, zero fixed offsets, explicit stem clearance, repeat stability and
legacy-off restoration. Invalid durations in either companion must reject
atomically; retain forced-direction and second-note offset rejection.

Files changed: spacingplan4.cpp, tst_spacing.cpp, guide, audit and four scores.
Local moc/C++11 debug/NDEBUG syntax checks pass. A controlled harness exercises
the actual production companion-validation fragment in 170 cases: zero/one/
two/three streams, either track ordering, held companions, missing onsets,
invalid ticks/durations, beamed and stemless items. XML checks preserve the
beam/rhythm/style structure and source pitch/step functions validate all new
companion notes. The fragment harness does not exercise live geometry or
layout dispatch. Full Qt 5.9 mtests and rendered comparison remain pending.

Apply after v0.70 and run tst_spacing/tst_beam. Resolve any X9/X10 mismatch,
then move to multi-measure systems, fitting/justification and compression
checks for the bounded cross-staff scope. Larger/mirrored chords, additional
independent beams and other unsupported musical contexts still use fallback.

## v0.72 WIP: multiple cross-staff measures in one system, X11/X12

The user reports v0.71 tests passing. Blue-paper X9/X10 captures look close
to Studio 4.7.5 in natural and justified spacing on both staves.

Split cross-staff preparation into a per-measure adapter and an atomic system
collector. Each measure can contain one independently supported complete beam
stream and the previously admitted companions. Publish the combined chord/pair
maps only after every measure succeeds. Ordinary measures contribute empty
maps. Multiple independent cross beams in one measure and beams crossing a
measure boundary remain unsupported. Other musical and geometry limits are
unchanged. Collection/fitting and final placement use the existing shared
planner; no new width accumulator, beam policy or legacy-off path is added.

New comparison files, both experimental settings enabled:
- v0.72-spacing-cross-staff-system-natural.mscx
- v0.72-spacing-cross-staff-system.mscx

X11 is X9's plain two-moving-voice measure; X12 immediately follows with X10's
companion accidental cycle. Remove only the repeated time signature from the
second measure and update labels/title. Compare natural width and the barline
to the second measure's first notes/accidentals, then whole-system justified
proportions. This two-score control changes the system context, not the music.

New crossStaffSystems test runs both files under both beam settings. Require
two measures in one system, distinct cross-staff beams, exact onset/rhythm/pitch
structure, full-plan acceptance and actual measure/segment positions and widths.
Build the first positive compression pass, choose a target between that width
and natural width, then require squeeze plus justification to reach the target
while preserving both beam-neighbor stem floors. These compression operations
are read-only checks; the provided visual scores show natural/justified layout,
not a narrow-page compression fixture. An unsupported forced stem in measure 2
must reject the whole output atomically. Retain repeat and legacy-off checks.

Files changed: spacingplan4.cpp, tst_spacing.cpp, guide, audit and two scores.
Local moc/C++11 debug/NDEBUG syntax and fixture identity checks pass. A harness
of the actual system-wrapper body with controlled measure adapters covers 91
merge/ordinary-measure/later-failure/frame cases, including unchanged output
on failure. It does not exercise live geometry, fitting or compression. Those
paths are covered by the new Qt test, pending the user's full Qt 5.9 build;
rendered comparison is also pending. Parent-index patch and whitespace checks
precede delivery. Apply after v0.71 and run tst_spacing/tst_beam.

Next address any boundary/placement discrepancy, then run narrow-page automatic
system fitting/backtracking controls and summarize the bounded milestone's
supported scope and remaining fallbacks. Broad decorations, grace notes,
additional beams, mirrored/larger chords, and the deferred centered-beam/rest
placement issues remain outside this checkpoint.

## v0.73: automatic cross-staff system fitting control

The user reports v0.72 tests passing. The two-measure natural/justified
captures look close to Studio 4.7.5, including the middle barline gap. Its
read-only compression and justification checks also passed on the live build.

Add one score: v0.73-spacing-cross-staff-fitting.mscx. F1-F6 alternate the
accepted X9 plain-companion and X10 accidental-companion measures. Preserve
music and styles; omit repeated time signatures after the first measure.
Use a 6-inch page with 4.8-inch printable width, no explicit line/page/no-break
markers, and final-system fill threshold zero. Both experimental styles are
on. Short labels identify measures if line breaks differ between applications;
the smaller title avoids overflowing the narrower page. Compare automatic
system breaks, regenerated clefs and barline/accidental/flag clearance.

New crossStaffFitting test runs both beam settings. First lay out all six
measures on a wide page and get their combined natural planner width. Then
use widths at 1.1, 0.55 and 0.35 times that natural width. Require one system at
the wide setting and multiple systems at the narrower settings, retaining all
six measures. Every resulting system must accept a full plan, fit naturally
and apply whole-system justification with matching measure/segment positions
and widths. Narrowing must not reduce the number of systems. Check repeat
stability and spacing-off restoration at every width, ignoring retained but
disabled header/trailer caches. This exercises fitting/backtracking and header
regeneration; v0.72 separately covers positive squeeze passes. Do not describe
the visual fixture as forcing all six measures onto an overfull single system.

Files changed: tst_spacing.cpp, this guide, audit and the new score. No
production algorithm changes. Local moc/C++11 debug/NDEBUG syntax checks and
XML comparisons against the accepted measures pass before delivery. Full
Qt 5.9 tests and rendered automatic-break comparison remain pending. Apply
after v0.72 and run tst_spacing/tst_beam, then open the same original score
in Evolution and Studio 4.7.5 at its supplied page width.

This is the final planned validation group for the current bounded cross-staff
spacing milestone. After live acceptance, summarize supported behavior and
remaining fallbacks before expanding scope further. The current cross-staff
adapter covers ordinary visible grand staves, automatic complete eighth-note
beam streams confined to each measure, up to two unmirrored normal notes per
beam chord, and held or up to two complete un-beamed sixteenth companion
voices. Existing typed accidental geometry participates. Temporary cross-stem
extent is still conservative. Independent beams in one measure, cross-barline
beams, mirrored/larger chords, mixed moving rhythms, rest/tuplet/grace/tremolo
cross contexts and unsupported decorations retain fallback. These boundaries
are cross-staff-specific; ordinary non-cross notation has broader earlier
coverage. Legacy-off, default style values and native within-chord packing are
unchanged. The registered centered-beam and rest-position issues stay separate.

## v0.74: milestone acceptance and next focused issue

The user reports v0.73 tests passing. Both applications break the fitting score
into F1-F2/F3-F4/F5-F6 without an obvious collision. Record the bounded milestone
in v0.74-spacing-milestone.md. A remaining visual difference is apparent extra
clef-to-first-note spacing on Evolution's later systems (F3/F5), with slightly
tighter justified interior gaps. Confirm coordinates/style migration before
changing spacing. Next audit generated system headers using the existing
v0.73 score; do not expand musical scope or alter rhythmic constants to mask it.
This checkpoint changes documentation only; no rebuild or new visual test is
required. It does not claim full Studio parity or remove existing fallbacks.

## v0.76 WIP: matched generated-header settings

The source audit has not established a header algorithm defect: the adapted
header gap formula matches the pinned source, and the known clef-left default
difference is only 0.05sp. Add v0.76-spacing-header-control.mscx with explicit
matching header style values, preserving v0.73's music/page/labels. Reuse the
three-width fitting test under both beam settings; no production changes.
See v0.76-header-spacing-audit.md for source findings, values and next steps.
Apply after v0.75, run both mtests and compare F3/F5 in the new control in both
applications. Local checks pass; live checks are pending. If the gap remains,
compare effective styles and SVG coordinates before selecting a fix.

## v0.77 WIP: header control accepted; grace-group source located

The user reports v0.76 tests passing. Explicit-style captures bring the later
header gaps into close agreement, so no header algorithm change is justified.
The pinned grace-group layout was located in TLayout::layoutGraceNotesGroup,
with source blob hashes verified against the pinned tree. See
v0.77-grace-group-source-audit.md for shape accumulation, group-to-main safety
bound, call order, coordinate/geometry obligations and the first adapter scope.
Documentation only; no rebuild or new score. Next implement ordinary before-
grace group spacing behind the optional style, keeping slurs and broader grace
contexts separate until the core spacing and full-plan geometry are validated.

### v0.78 WIP: before-grace placement plan

Added a read-only adapter and `v0.78-spacing-grace-before.mscx` (G1-G6, both
experimental settings on). The new test checks final group positions, pair
constraints, stale-position independence and rejection atomicity under both
beam modes. Live grace layout remains unchanged until its width contribution
and geometry lifecycle are connected. See `v0.78-grace-before-adapter.md`.

### v0.79: correct down-stem grace controls

The supplied Qt 5.9.9 result reports two failures in graceBeforePlan, one per
beam mode: the expected down-stem group was up. Evolution's grace beam layout
uses the beam's direction, defaulting to up in one voice; per-chord down stems
alone do not force the group down. The adapter assertions for the earlier up
controls passed, as did the existing spacing tests.

Renamed the control to `v0.79-spacing-grace-before.mscx`, set explicit Beam
StemDirection before G1-G4's first grace, and added a beam-direction assertion.
G5/G6 remain unbeamed. Removed the new control's unrecognized title fontSize
field. Production code is unchanged. Re-run tst_spacing and tst_beam; this is
still a read-only adapter checkpoint with no live layout change. Next connect
placement/extents to live layout after these controls pass.

### v0.80 WIP: live ordinary before-grace spacing

`v0.80-spacing-grace-before.mscx` enables both experimental settings and reuses
G1-G6. Accepted groups now use the new positions and left extent before native
shape/width collection. Full Studio system-plan grace admission is still
pending; these systems keep native spacing with the new footprint. New tests
check final applied positions/shapes, repeat stability, on/off restoration and
unsupported manual-offset fallback under both beam modes. Compare internal
G1-G4 gaps and terminal G1-G6 gaps with 4.7.5 after full mtests pass. See
`v0.80-grace-layout-integration.md` for scope and lifecycle limitations.

### v0.81 WIP: grace groups enter full system spacing

Accepted ordinary before-grace groups now enter the typed full-system planner
only after all group contexts and published positions validate. Default collision
queries still reject graces. New `v0.81-spacing-grace-neighbors.mscx` and
`v0.81-spacing-grace-neighbors-natural.mscx` provide N1-N6 with ordinary notes
before and after each group; both experimental settings are on. Tests cover
actual full-plan positions/widths, read-only compression, rejection boundaries,
repeat stability and spacing-off restoration under both beam modes. See
`v0.81-grace-system-spacing.md`; actual narrow-page fitting is the next stage.

### v0.82: automatic grace fitting and actual compression

Test-only checkpoint. `v0.82-spacing-grace-fitting.mscx` has P1-P6 without
explicit breaks on a narrower page; `v0.82-spacing-grace-squeeze.mscx` has one
stretched sharp/flat group (S1) on a narrow page. Both options are saved on.
New tests exercise actual automatic system collection at three widths and
actual compressed/justified placement, both beam modes, group-position
stability, repeated layout and spacing-off restoration. See
`v0.82-grace-fitting-checkpoint.md`. No production engraving code changes.

### v0.83 WIP: fitting accepted; grace-slur source audit

The v0.82 user mtests pass; supplied fitting captures agree on 3+3 measures.
No production change or rebuild in this documentation checkpoint. The pinned
slur source audit identifies separate horizontal-admission, endpoint/anchor,
direction and collision differences. Both versions already include the grace
parent in endpoint coordinates. Evolution skips entire endpoint segments in
slur collision checks, whereas Studio filters individual items within them.
Next add paired local grace-to-own-main slur controls and preserve the new
horizontal spacing with a narrowly validated slur allowance before changing
curve geometry. See `v0.83-grace-slur-source-audit.md`.

### v0.84 WIP: local slurs coexist with optional grace spacing

A narrowly validated first-before-grace to own-main slur no longer forces the
optional group/system spacing to fall back. Ordinary queries still reject it
without a scoped or prepared grace context. Slur drawing is unchanged. Compare
`v0.84-spacing-grace-slurs.mscx` (L1-L6) with 4.7.5 and its unslurred companion,
`v0.84-spacing-grace-slurs-plain.mscx`; both options are saved on. New tests cover
resolved/round-tripped endpoints, equal horizontal layout with/without slurs,
full-plan application, unsupported edits, repeat layout and legacy restoration.
See `v0.84-grace-slur-coexistence.md` before evaluating curve differences.

### v0.86: slurred grace groups with accidental neighbors

v0.84/v0.85 user mtests pass; the paired plain/slurred captures look broadly
close to Studio, so no slur-renderer change is indicated by them. Test-only
A1-A6 controls now add main-note sharps/naturals and neighboring sharps/naturals:
`v0.86-spacing-grace-accidental-neighbors.mscx` and its `-natural.mscx` companion.
Both options are saved on. Existing full-plan/compression/stability tests now
cover these inputs and explicitly require the expected accidental glyphs and
local-slur endpoints. See `v0.86-grace-accidental-neighbors.md`.

### v0.90: A1-A6 accepted; accidental-neighbor fitting and squeeze tests

The user reports passing mtests after v0.89. Blue Evolution and white 4.7.5
captures look broadly comparable for A1-A6 in justified and natural layouts.
The scoped beam-shape preparation is retained; no slur-renderer change is
indicated by these captures. New `v0.90-spacing-grace-accidental-fitting.mscx`
(F1-F6 = A1-A6) and `v0.90-spacing-grace-accidental-squeeze.mscx` (S1 = A3)
exercise automatic narrow-system collection and actual compression under both
beam modes, with both experimental settings saved on. New runtime rows still
need target-build validation. See `v0.90-grace-accidental-fitting-checkpoint.md`.

### v0.91: fitting accepted; after-grace source audit

The user reports v0.90 mtests passing. The paired captures show the same 3+3
system grouping for F1-F6 and usable S1 compression, with some remaining
header-to-note redistribution in S1. Record this as a scoped passing checkpoint.
The documentation-only audit identifies Studio's following-segment attachment,
same-track merging, typed shape publication and final ownership-coordinate
conversion for after-graces. No production guards change. Next prepare AG1-AG6
baseline controls before adapting that separate lifecycle. See
`v0.91-grace-fitting-accepted-after-grace-audit.md`.

### v0.92: AG1-AG6 after-grace baseline controls

Test-only stage. `v0.92-spacing-after-grace-baseline.mscx` provides six separate
systems and `v0.92-spacing-after-grace-baseline-natural.mscx` a wide overview.
Both styles are saved on, but after-graces still use the legacy path. New tests
check actual ownership, musical ordering, following-target fixture semantics,
read-only planner rejection, stable layout, toggle restoration and round trips
under both beam algorithms. AG6 also contains before-graces; their existing
local adapter can still act despite system-level fallback. See
`v0.92-after-grace-baseline.md`. Next add a read-only ordinary target adapter.

### v0.93: correct the after-grace baseline equality assertion

All four v0.92 runtime rows failed the new whole-measure on/off equality check.
That contract was incorrect: the optional rhythmic widths/springs still act
when the typed system planner rejects after-graces. Compare AG1-AG5's native
after-group coordinates relative to the following target instead. All original
repeat-layout, off-restoration, ownership and rejection checks remain. No
production code or score changes. See `v0.93-after-grace-baseline-assertion-fix.md`.

### v0.94: baseline accepted; read-only after-grace packing

The user reports v0.93 mtests passing and provides paired AG1-AG6 captures.
New buildGraceAfterPlan4 returns target-segment-relative positions and bounds
for AG1-AG3 without changing live layout. AG4-AG6 and full system admission
still reject after-graces. The explicit after-group collision scope does not
broaden default or before-grace queries. New tests cover packing, stale-position
independence, atomic rejection, and complete native-layout nonmutation under
both beam modes. Reuse the v0.92 scores; no visible change is expected yet.
See `v0.94-after-grace-readonly-plan.md` for validation and the next integration.

### v0.95: guarded live after-grace packing

The user reports v0.94 building and passing. AG1-AG3 can now use the new local
packing when all source-to-grace constraints fit the existing native interval.
Legacy packing runs first and remains on unsupported or cramped input; full
system-plan after-grace admission is still pending. New v0.95 comparison files
are `v0.95-spacing-after-grace-packing.mscx` and its `-natural.mscx` companion.
Check AG2's D-flat-to-B gap explicitly. AG5's owning-C-to-after-D gap is also
an identified discrepancy, but its rest target remains unsupported here.
See `v0.95-after-grace-live-packing.md` for validation and remaining work.

### v0.96: live packing accepted; temporary target projection

The user reports v0.95 tests passing. AG2's D-flat-to-B gap is visibly closer
to Studio; AG5's compact owning-note gap remains unresolved. New read-only
geometry preparation removes old source grace footprints and appends each
planned component exactly once to a temporary target shape. Native shapes,
ownership, and live placement stay unchanged. New tests cover provenance,
coordinates, stale-position independence and atomic failure. No new scores or
screenshots are needed. See `v0.96-after-grace-target-projection.md` before the
next full-system admission/publication stage.

### v0.97: full ordinary after-grace system spacing

The user reports v0.96 mtests passing. Supported AG1-AG3 projections now enter
system fitting, full placement, and compression. After-group coordinates are
retained in the plan and published only after segment spacing is final. Mixed
unsupported systems still fall back atomically. New controls are
`v0.97-spacing-after-grace-system.mscx`,
`v0.97-spacing-after-grace-system-natural.mscx`, and
`v0.97-spacing-after-grace-squeeze.mscx`, with both
experimental settings on. AG5's rest target and owning-note gap remain pending.
See `v0.97-after-grace-system-spacing.md` for precise filenames and validation.


### v0.98: prepare after-grace beam geometry before spacing

The v0.97 native report has four failures: both beam modes for the three-case
natural system and AS1 compression. Earlier runtime cases pass. Source tracing
shows after-grace beam stems are finalized later than spacing, while early
preparation previously covered only before-graces. Prepare validated after
beams at planned internal distances, restore native x, and require stable typed
source/target projections before fitting or placement. Keep all original test
expectations and add numeric failure diagnostics. Reuse the three v0.97 scores.
Native confirmation of this targeted correction is still required. AG5 remains
pending. See `v0.98-after-grace-beam-preparation.md`.


### v0.99: trace the remaining native after-grace failures

The next native report retains the same four failures after v0.98. Its squeeze
opening-clef delta equals the first pass's clef-left-margin reduction; the beam
preparation change has not established the cause. This diagnostic checkpoint
records PRE natural plans, APPLY decisions/rejection reasons, and POST rebuilt
plans plus actual coordinates. Only the v0.97 runtime test rows enable tracing;
ordinary application use is quiet. All assertions and layout behavior remain.
Rerun tst_spacing normally and provide its XML; failures may remain. See
`v0.99-after-grace-plan-trace.md`. No new comparison scores are needed.


### v0.100: preserve nested spacing-bound provenance

The v0.99 trace establishes that grouped after-graces fall back before final
spacing: their owner segments fail geometry admission while the same post-layout
queries succeed. AG1's single grace applies normally. Coincident unpositioned
grace spacing bounds acquire complete producer sets at chord resolution, then
lose them when those already resolved records pass through staff resolution.
Preserve identical complete sets only with exact native/candidate/set counts;
unknown, mixed, inconsistent or malformed sets still reject. Add a two-level
resolver regression and invalid-set controls. All layout assertions remain.
Apply after v0.99 and rerun tst_spacing and tst_beam; native confirmation remains
pending. Keep the trace temporarily to verify actual application. Reuse v0.97
scores. See `v0.100-nested-spacing-bound-provenance.md`.


### v0.101: native pass and visual checkpoint; remove temporary tracing

The user reports v0.100 mtests passing and supplies paired 37E/4.7.5 captures
for the three v0.97 controls. At the supplied resolution, AS1's final grace-to-
main gap, AG2's D-flat-to-B spacing, and AG3's down-stem group spacing look
closely aligned. The justified views also agree after accounting for capture
position. This supports the bounded AG1-AG3 milestone, not universal parity.
AG5 is absent from these controls and its compact owning-C-to-after-D gap is
still unresolved.

Remove v0.99's temporary environment guard, PRE/APPLY/POST trace, and preparation
failure logging. Keep v0.98's bounded beam preparation, v0.100's ownership fix,
all regression assertions, and numeric failure messages. No score files or
engraving algorithms change. Next address the separately bounded rest target
for AG5 before merging after- and before-grace groups or barline targets. See
`v0.101-after-grace-accepted-checkpoint.md`.


### v0.102: ordinary rest targets for after-graces

The user committed v0.101. Extend after-grace target preparation and full-system
projection to ordinary visible undotted rests in the same measure and voice.
The scoped collision policy uses the existing grace/rest padding; the system
planner reserves source-to-group clearance instead of relying on a native
interval chosen before repacking. The bounded stage excludes full-measure,
gap, cue, moved/cross-staff, dotted, beamed/tuplet, and attached rest targets.
AG4 barlines and AG6 merged before-graces still reject.

New comparisons, with both options saved on:
`v0.102-spacing-after-grace-rests.mscx` (AR1-AR4, separate systems),
`v0.102-spacing-after-grace-rests-natural.mscx` (compact overview), and
`v0.102-spacing-after-grace-rest-squeeze.mscx` (AR1 compressed).
AR1 is the original AG5 two-up group; AR2 is down, AR3 altered four-up, and AR4
single-up. Compare owning C to first D in AR1, internal grace gaps, and the last
grace to rest separately in 37E and 4.7.5. Native tests and visual confirmation
remain pending. See `v0.102-after-grace-rest-targets.md`.


### v0.103: fix the squeeze-test rest cast and comparison-score breaks

The native v0.102 report passes graceAfterRest and the earlier after-grace
cases, then aborts in graceSqueeze on a checked chord cast. The shared main-
chord search lacked an isChord guard and reached the newly added following rest.
Guard that cast without changing assertions or production engraving.

Correct the v0.102 fixtures: remove misplaced voice-level LayoutBreak elements,
retain exactly three measure-level breaks in the separate-system score, and
remove inherited measure-level breaks from the natural and squeeze scores.
Assert four separate systems versus one combined natural system in the test.
Reopen the corrected v0.102 files for visual comparison. Native completion of
the full suite and the newly genuine combined natural case remains pending.
See `v0.103-rest-squeeze-test-and-fixture-fix.md`.


### v0.104: passing rest stage; match measure stretch for visual squeezing

The user reports v0.103 tests passing. Paired captures support close agreement
for AR1/AG5's natural owning-C-to-first-D gap, internal grace intervals, and
last-grace-to-rest clearance. The static squeeze views distribute header/rhythm
space differently, but source audit finds unequal inputs: Studio 4.7.5 ignores
saved measure stretch when importing pre-4.0 scores. The old squeeze file uses
stretch 2 in 37E, while Studio retains its default 1.

Keep the stretch-2 runtime regression. Add
`v0.104-spacing-after-grace-rest-squeeze-match.mscx`, with stretch 1, narrower
page, and both experimental options on, so the imported stretch matches. Add
native checks of the file's actual static compressed plan and dynamic squeezing
under both beam modes. Fix the old squeeze title's literal font markup using a
plain title and supported size field. No engraving algorithms change. Compare
the new file in both applications before diagnosing a remaining squeeze gap.
See `v0.104-rest-squeeze-import-audit.md`.


### v0.105: grace-beam height audit and read-only anchor comparison

The user reports v0.104 tests passing. Its matched-stretch captures show close
horizontal agreement, with the two beamed grace stems slightly longer in 37E;
ordinary quarter-note stems appear comparable. Keep this vertical issue separate.

Pinned source confirms differing anchor scale conventions: 3.x grace Beam mag
already includes graceNoteMag, while Studio retains ordinary beam magnitude
and separately scales the drawn grace beam. v0.105 adds a read-only comparison
of current anchors versus the separated-scale candidate, plus default/actual
stem lengths and stem attachment metadata in the native XML report. No candidate
anchors are published, and all production layout calls retain their original
parameters. This does not claim a height fix. Reuse the v0.104 score; send the
result XML even if tests pass. See `v0.105-grace-beam-height-audit.md`.

### v0.106: use nominal anchors for ordinary grace beams

The passing v0.105 native report confirms the candidate moves both anchors of
the matched-input D5/E5 pair toward its noteheads by 0.25sp. Select Studio's
nominal magnitude for ordinary automatic grace-beam anchor solving under the
experimental beam algorithm. Keep drawn thickness/distance grace-scaled and
retain the previous handling for cue, cross-staff, edited and tremolo contexts.

Replace diagnostic logging with numeric and completed-segment regressions;
also cover up/down and altered AR groups. Reuse
`v0.104-spacing-after-grace-rest-squeeze-match.mscx` for the visual comparison.
Native tests and updated visual confirmation are pending. See
`v0.106-grace-beam-nominal-anchors.md`.

### v0.107: accepted height; barline and merged target controls

The user reports v0.106 tests passing; the paired captures show closely aligned
grace-beam heights and horizontal gaps in the matched AR1 example. Close that
specific height issue. Source audit confirms that barline targets use staff
shape/table padding, while incoming after-graces and a target's before-graces
are packed together in Studio.

New `v0.107-spacing-after-grace-targets.mscx` and
`v0.107-spacing-after-grace-targets-natural.mscx` provide B1-B3 barline and M1-M3
merged controls, with both experimental options on and stretch 1. Add atomic
fallback, ownership and layout stability tests without changing engraving.
Compare the natural file first, focusing on last-grace-to-barline and the
after-to-before group boundary. Next implement ordinary barline support before
joint merged-group support. See `v0.107-after-grace-boundary-audit.md`.

### v0.108: passing target controls; stage an ordinary barline query

The user reports v0.107 tests passing. B1-B3 clearance already looks broadly
comparable; M2 is more clearly separated in Evolution than in Studio. Preserve
that mixed-direction treatment.

Add buildGraceAfterBarlinePlan4 as a read-only target-relative query for normal,
visible, unedited, single-staff ending barlines. Reuse typed accumulated-group
packing and barline padding. Extend B/M controls to cover query success,
unsupported boundary edits and unchanged published layout. Production/system
admission remains off in this stage; M1-M3 still reject. Reuse the v0.107 files;
no new capture is needed. Next integrate validated barline geometry into system
fitting/publication. See `v0.108-after-grace-barline-plan.md`.

### v0.109: normal barline targets in live system spacing

The user reports v0.108 tests passing. Admit its bounded normal-barline plan
through production after-grace geometry, system projection and final placement.
Retain exact target-barline spacer provenance and existing unsupported-context
fallback; merged M1-M3 handling remains unchanged.

New `v0.109-spacing-after-grace-barlines.mscx` and
`v0.109-spacing-after-grace-barlines-natural.mscx` isolate B1-B3 plus a plain
closing control. `v0.109-spacing-after-grace-barline-squeeze.mscx` adds a normal
terminal-barline compression case. All options start on; stretch stays 1.
Tests cover projection multiplicity, actual solved placement, compression and
restoration. Native results and visual confirmation remain pending. Compare
the natural B cases first; preserve M2's clearer mixed-direction separation.
See `v0.109-after-grace-barline-system.md`.
