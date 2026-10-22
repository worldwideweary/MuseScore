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
