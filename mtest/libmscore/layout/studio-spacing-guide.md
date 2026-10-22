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
