# Horizontal spacing baseline — v0.29

## v0.175 WIP: beam-end slur anchor selection and insets

User verification: v0.174 native mtests pass and other notation remains stable
while editing. Slow nudge/undo behavior is deferred at the user's request until
more visual work is complete. The supplied P2 captures still show a vertical
curve/attachment difference. No claim of identical saved properties between
those two captures is made; compare explicitly UP anchors in the next check.

The earlier apparent F-natural direction regression is explained: the uploaded
score stores that slur as AUTO with no manual grip offsets. Ctrl+R resets its
explicit UP request to AUTO without an immediate visible change; moving into
the destination's before-grace group can then select DOWN. X makes the request
explicit. The pinned Studio rules agree; do not change direction policy here.

Source: Studio v4.7.5 slurPos and stemSideForBeam, verified reference blob
cc4d7659bffd339ebf6b757d4d70b284812d1fe1. When both endpoint stems agree with
the slur side, select the stem inset independently at each endpoint except the
beam-side exemptions: non-final start and non-first end. Evolution's older
selection could leave the final start/first end near a notehead merely because
one partner was beamed. Use the source selection for pitched, same-effective-
staff endpoints without tremolos or cross-staff beams under spacingLayout4.
Retain the existing continued-slur handling outside that scope.

For selected beamed stem anchors, port the source's inset: start vertical
inset -0.5 times chord magnification, end -0.5 (or 0.75 when not the first beam
member), with horizontal stem offset max(0.35 times magnification, 0.2) spaces.
Keep established unbeamed stem offsets and the first-after/main leading-edge
adapter. This isolates the beam-end change and preserves the recently verified
fixed endpoint behavior. Disabled-style layout retains its original formulas.

Add beamEndSlurAnchors4 using the existing v0.113 natural UP/DOWN input. Probe
last-beamed starts and first-beamed ends with a matching unbeamed partner;
check physical stem-tip inset, horizontal stem offset, explicit side, user
offset preservation, style-off restoration and unchanged published notation.
Require nonempty UP/DOWN start/end coverage. Existing first/interior beam and
cross-system/sibling tests remain active. No additional score is required.

Changed files: libmscore/slur.cpp, mtest/libmscore/layout/tst_spacing.cpp and
this guide. Local validation: Debug/NDEBUG C++11 syntax checks with regenerated
moc and warning flags; whitespace and exact patch replay. Native mtests and
visual verification remain pending. Apply after v0.174, run the usual beam/
spacing pair, then repeat P2's explicitly UP start traversal, especially the
last note of each beam and F-natural. Review endpoint height separately from
the remaining legacy curve solver. Performance work stays deferred.

## v0.174 WIP: vertical clearance in the general beam-side slur branches

User verification: v0.173 native mtests pass. Pulling a start anchor into the
preceding system and the second forward-extension demonstration now keep the
fixed local endpoint steady. The lower sibling slur also stays unchanged.
Close those endpoint/sibling issues, retaining the existing regression checks.

Next source-backed step: outside the validated first-after-grace/main adapter,
Evolution's beam-side start and forced stem-side end still use stem height plus
one full spatium. Studio v4.7.5 slurPos uses actual stem length, half its source
beam-width term, and max(0.35 * intrinsic magnification, 0.2) staff spaces of
clearance. Its start beam term includes beam magS(); its end term does not.
Reference blob remains cc4d7659bffd339ebf6b757d4d70b284812d1fe1.

Apply those vertical metrics in the existing pitched, same-effective-staff
beam-side branches under spacingLayout4, excluding tremolo endpoints. Preserve
the branch's horizontal coordinate and final native articulation adjustment.
The existing first-after-grace adapter continues to supply its established
leading-edge alignment. Cross-staff beams, tablature, disabled style and branch
selection at beam ends retain their existing handling. This is a clearance
increment, not a complete slur-renderer replacement. No new spacing query,
layout pass, persistent cache or edit batching is added.

Add beamSlurAnchorHeights4 with ordinary H1-H8 and existing v0.113 UP/DOWN grace
inputs. Query first/interior starts through the last member of each usable
beam; both anchors are inside that beam, so the first-after/main adapter cannot
mask the general branch. Measure clearance beyond actual stem tips, retain
horizontal coordinates, verify style-off restoration and unchanged published
notation. Existing sibling/cross-system tests remain active. No new score is
required: v0.167 P2 already supplies interior grace starts for visual checking.

Changed files: libmscore/slur.cpp, mtest/libmscore/layout/tst_spacing.cpp and
this guide. Local validation: Debug/NDEBUG C++11 syntax checks, regenerated moc,
warning/whitespace checks and exact patch replay. Native mtests and comparison
against Studio remain pending. Apply after v0.173, run the usual spacing/beam
pair, then compare P2's upper slur while its start visits the interior grace
notes. Retain 200 percent zoom for captures. Beam-end branch selection and
measured nudge latency remain subsequent work.

## v0.173 WIP: independent stem anchors for continued slurs

User verification: v0.172 beam and spacing mtests pass, spacing in 79.16 s.
The new MS37E/MS475 clips confirm that the unedited lower P2 slur now stays
fixed. Close that sibling-curve issue. A separate continued-slur difference
remains: while its start moves within P1, Evolution's upper P2 endpoint changes
height even though its end chord stays fixed. Studio's P2 endpoint stays put.

Re-fetched the pinned Studio v4.7.5 slurtielayout.cpp and verified git blob
cc4d7659bffd339ebf6b757d4d70b284812d1fe1. In slurPos, the continued-slur stem
selection anchors each unbeamed endpoint on the slur side to its own stem,
including when the remote stem direction differs. Evolution's older selection
can depend on both endpoints being unbeamed or on the start's beam branch;
otherwise its pitch-dependent floating branch can move the opposite endpoint.

Under spacingLayout4, apply the source's continued-slur unbeamed stem-side
selection in both directions. Retain existing stem-offset metrics for this
increment. Keep same-system slurs, tablature and tremolo-containing endpoints
on their existing branches. No new retained state, spacing query, reflow pass,
manual offset reset or historical-layout shortcut is introduced.

Add continuedSlurStemAnchors4 using v0.167-grace-policy-systems.mscx. Probe all
main and grace chords across both systems, with UP and DOWN slurs. Require
both the raw fixed anchor and rendered fixed endpoint to stay constant when
only the remote anchor changes. Verify same-system anchor positions remain
unchanged and toggling the style off restores the captured legacy coordinates
for every probe. Require published note/beam geometry to remain unchanged.

Changed files: libmscore/slur.cpp, mtest/libmscore/layout/tst_spacing.cpp and
this guide. Local validation: Debug/NDEBUG C++11 syntax checks with regenerated
moc and warning flags, whitespace check and exact patch replay. Native mtests
and visual verification remain pending. Apply after v0.172, rerun the usual
spacing/beam pair, then repeat the supplied clip's P2 start-anchor traversal.
Also check the reverse case, extending an end anchor into the next system.
Broader beam-side vertical clearance and measured nudge latency remain open.

## v0.172 correction: preserve the regression command's end anchor

The user's v0.171 tst_spacing XML reports one failing case,
graceSlurSiblingAnchors4, at the command/fresh-layout comparison. Its preceding
fixed-position obstacle, prepared-policy and lower-curve assertions pass.
The new test's command setup reread upper->endElement() after changing
SPANNER_TICK/SPANNER_TICKS. These property setters invalidate the end pointer;
the subsequent ChangeSpannerElements therefore received a null endpoint.
This does not reproduce native changeAnchor, which preserves ecr in advance.

Retain the original end identity before any property changes and pass that
pointer into ChangeSpannerElements. Assert both endpoints and the fixed end
tick before and after command completion. Preserve all existing geometry,
fresh-layout, undo and redo checks; distinguish fresh-layout and redo failures
in their messages. Files changed: tst_spacing.cpp and this guide. Production
layout code is unchanged from v0.171.

Local validation: C++11 Debug/NDEBUG syntax checks with regenerated moc,
whitespace and exact patch replay. Native tests remain pending. Apply after
v0.171 and rerun the usual spacing/beam pair, then repeat the P2 visual edit.

## v0.171 WIP: keep sibling grace-slur curve admission on prepared inputs

User verification after v0.170: all 63 native mtests pass. The resource/catalog
detour is closed. Blue paper identifies Evolution; white identifies Studio
4.7.5. Compare captures at 200 percent zoom, using pieces when necessary.
The v0.167 natural and system-boundary captures show close horizontal agreement,
and save/reopen preserves layout. Editing P2's upper start anchor can subtly
reshape its unedited lower slur, especially when entering the preceding system
or moving between grace members. Nudge responsiveness remains slower than the
legacy layout; no new timing measurements are available.

Source finding: GraceMergedSlurQuery4 still rebuilt its geometry with the
COMPATIBILITY policy after horizontal publication had switched to PREPARED.
The compatibility consumer admits particular sibling anchor patterns. Changing
the upper anchor can reject that late rebuild and disable the lower slur's
existing collision fitter even when its published obstacles have not moved.
Route experimental late curve geometry through a fresh prepared capture of the
owner's already-produced measure. Root directions are resolved read-only, with
external raw endpoint identities retained. Both shared and standalone anchor/
obstacle calls use this query. Reject prepared failures without a compatibility
retry. The query's attachment owner/cache end before any curve writes; successful
geometry sharing remains limited to one unchanged slur-layout operation.

Add graceSlurSiblingAnchors4 using the existing v0.167 systems input. At fixed
published notation, visit the preceding system, owner main, interior/final
after-graces, and both before-graces: require prepared admission, identical
physical obstacles, and an unchanged lower curve. Separately exercise an
undoable cross-system start edit, compare incremental versus fresh layout,
and check undo/redo restoration. Do not freeze horizontal layout across a real
endpoint-side constraint change. No extra score is required.

Changed files: libmscore/spacinginput4.{h,cpp}, gracespacing4.{h,cpp},
graceslurlayout4.cpp; mtest/libmscore/layout/tst_spacing.cpp and this guide.
Local validation: Debug/NDEBUG C++11 syntax checks with unused-parameter and
signed-comparison diagnostics, regenerated moc, whitespace and exact patch
replay. Native compilation/GUI tests are pending on the user's machine.

Next: rerun spacing/beam mtests and the same P2 upper-start edit. Check the
unedited lower slur and note positions separately. The beam-end/interior-anchor
vertical clearance difference versus Studio is recorded for its own slur-anchor
audit; this patch does not change those clearance constants. Time remaining
nudge latency after this visual check; no performance improvement is claimed.

## v0.170 detour: US English palette labels (independent of v0.169)

Installing the current build resolved the user's startup-resource abort. The
fresh palette result now reaches the duplicate-name assertion: Advanced reports
Unfingered in Articulations and Text. Both are intentional Open entries already
allowed by the test. The US English catalog incorrectly translated Open to
Unfingered in Palette and symUserNames; this catalog was untouched by the beam/
spacing patch series. Correct both entries to Open, preserving the test and
workspace content. No duplicate is added or removed and no test exception is
broadened. Files: share/locale/mscore_en_US.ts and this guide.

This patch applies over either v0.168 or optional resource-staging v0.169. The
latter is not required when using installed resources, as the user now does.
Regenerate catalogs using cmake --build . --target lrelease from the build root,
then cmake --install . so the tests load the corrected installed catalog.
Rebuilding only the C++ targets does not run the explicit lrelease target.
Rerun the palette/workspaces pair before returning to the visual gate. Native
Qt GUI test execution remains on the user's machine.

Local validation: parse the two source-context entries, compile the corrected
catalog with Qt 5.15 lrelease and round-trip the QM to confirm both labels are
Open. Compiling the original catalog reproduces Unfingered in both contexts.
The patch is also verified to apply over both v0.168 and v0.169. No native GUI
mtest execution is claimed locally.


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

### v0.110: accepted barlines; read-only combined-group query

The user reports v0.109 tests passing. Paired natural B1-B3 captures show close
agreement in note/grace/barline gaps. Add a separate read-only combined plan for
same-direction incoming after-graces and target before-graces, packed together
against the target's own components while retaining each original parent.

M1/M3 query coverage uses the existing v0.107 matrix; M2 rejects mixed directions.
Production/system admission remains unchanged, as do original after-only and
default collision scopes. Check ownership, ordering, boundary clearance, stale
position independence, edited-stem rejection and unchanged score snapshots.
No new capture is needed. Native tests remain pending; next stage joint typed
projection and both-beam preparation before live placement. See
`v0.110-merged-grace-readonly-plan.md`.

### v0.111: joint geometry and isolated preparation of both beams

The user reports v0.110 native tests passing. Add read-only joint projections
that remove old source grace footprints and place after/before members exactly
once in the target's temporary geometry. Add an isolated helper that prepares
both independent beams at planned internal distances, restores member x values
and requires stable geometry within eight attempts.

Only tests invoke preparation in this checkpoint; production admission and live
placement remain unchanged. Reuse v0.107 M1/M3 for native geometry, ownership,
atomic rejection, repeat-preparation and native-relayout restoration checks;
M2 remains rejected. No new capture is needed. Next integrate validated joint
geometry/preparation with system spacing and per-parent publication. See
`v0.111-merged-grace-geometry-beams.md`.

### v0.112: same-direction combined groups in live spacing

The user reports v0.111 tests passing. Connect joint geometry and both-beam
preparation to system fitting and final per-parent placement. Retain merged
plans through compression/justification and skip independent before preparation
on their targets. M2 remains on its existing mixed-direction path.

New `v0.112-spacing-merged-graces.mscx` and
`v0.112-spacing-merged-graces-natural.mscx` cover M1/M3 up and M4/M5 down, plain
and altered. `v0.112-spacing-merged-grace-squeeze.mscx` adds actual compression
coverage. Both options start on; stretch is 1. Compare the natural file first,
checking the group boundary, last-before-to-main gap and accidentals separately.
Native tests/visual confirmation remain pending. See
`v0.112-merged-grace-system-spacing.md`.

### v0.113: slurred combined groups exposed a spacing fallback

v0.112 passes native tests and its slur-free captures agree closely, but the
user's paired-slur captures reveal a missing eligibility case. The earlier
acceptance did not establish slurred-grace parity. Add a validated combined-pair
scope for up to two automatic first-after-to-target slurs, retaining the local
before anchor and strict rejection of unsupported slurs. Keep M2 unchanged.

New `v0.113-spacing-slurred-merged-graces.mscx`,
`v0.113-spacing-slurred-merged-graces-natural.mscx` and
`v0.113-spacing-slurred-merged-grace-squeeze.mscx` recreate paired above/below
slurs. Tests verify their parsed anchors, compare horizontal layout with plain
v0.112 inputs, exercise compression/toggling and reject edits/third slurs.
Both options start on. Native results remain pending. Curve placement still
uses the legacy renderer and needs a separate attachment/collision audit. See
`v0.113-merged-grace-slur-admission.md`.


### v0.114: correct the slurred/plain comparison boundary

The six new v0.113 rows compared a replaced native target spacing cache.
Compare published horizontal positions and full-system replacement plans
instead. Preserve all eligibility, edit rejection, compression, repeat and
style restoration checks. Production layout is unchanged; reuse the v0.113
scores. See `v0.114-slurred-grace-snapshot-fix.md` for the failure values and
source explanation. Native verification and the separate curve audit remain
pending.


### v0.115: passing slurred spacing; curve source audit

User confirms v0.114 native tests pass. New blue/white overview captures show
closely aligned horizontal spacing; M4's upper curve still crowds the grace
noteheads in Evolution. `v0.115-slurred-grace-source-audit.md` records the
endpoint-segment collision omission, source/target shape ownership, missing
item identity in native shapes, and the distinct attachment/solver changes.
Next build and verify physical obstacle collection before changing curves.
Documentation only; v0.114 remains the tested runtime baseline. Reuse the
v0.113 comparison scores.


### v0.116: read-only grace-slur physical obstacles

Add the bounded combined-grace obstacle query and six native test rows using
the v0.113 inputs. Collect current physical components from both musical
parents, exclude the true start anchor, ledgers and spacing extents, and check
coordinate changes without publishing positions. See
`v0.116-grace-slur-obstacle-collector.md` for scope and validation. No rendering
call yet, so screenshots should remain unchanged. Native tests are pending;
next integrate clearance and curve adjustment separately.


### v0.117: fit combined-grace slur curves against physical obstacles

After the user's passing v0.116 native tests, connect the collector to a
bounded adaptation of Studio's localized curve/end-point collision adjustment.
Only automatic single combined-grace slurs with experimental spacing enabled
enter; failures and disabled/manual contexts retain legacy handling. Add
synthetic and rendered-clearance/repeat/toggle native checks. Reuse v0.113
natural, filled and squeeze inputs; inspect the upper M4 arc and both lower
arcs first. Initial anchors, shoulder geometry and stroke remain Evolution's.
See `v0.117-grace-slur-curve-clearance.md`; native tests and visuals pending.


### v0.118: grace slur beam-side attachment branch

User confirms v0.117 native tests pass; M4 upper clearance is improved.
Adapt the pinned source's first-grace beam-side attachment and forced
same-direction unbeamed main endpoint, within the existing validated joint
scope and spacing style. Preserve opposite-side anchors and the source's
beamWidthSp/magS metric rather than substituting drawn beam width. Add six
native anchor-query rows; keep all rendered-clearance/repeat/toggle checks.
Reuse v0.113 controls, inspecting M1/M3 upper and M4/M5 lower arcs. See
`v0.118-grace-slur-beam-anchors.md`; native and visual verification pending.


### v0.119: user-preferred upper slur / beam-edge alignment

User confirms v0.118 native tests pass, but prefers the upper slur's visible
start directly above the leading beam edge rather than either application's
observed offset. Reference the actual primary beam edge and account for the
rounded slur-end stroke. Preserve lower starts and all vertical calculations.
Update both anchor and final fitted-curve checks; reuse the v0.113 natural
score and inspect M1/M3 closely. Other differences remain deferred. See
`v0.119-upper-grace-slur-edge-alignment.md`; native and visual checks pending.


### v0.120: repair v0.119's stale beam-edge input

v0.119 failed six final rendered-alignment rows and visibly moved the upper
starts too far left. Its direct anchor queries passed, exposing a cached-
beam versus final-layout mismatch. Derive the same leading edge as
Beam::layout2 from current first-stem/page coordinates; do not read the cached
primary segment during slur layout. Add cache-poisoning and moved-grace
regressions; retain the failing final-edge assertion with diagnostic values.
See `v0.120-upper-grace-slur-live-edge-fix.md`. Apply after v0.119; native
mtests and M1/M3 close-up validation are pending. Other differences deferred.


### v0.121: keep note spacing and the modern base during slur edits

User confirms v0.120 tests pass and upper alignment is improved, but moving a
slur grip rejects the experimental spacing plan. Separate visual-edit
admission from strict automatic-only queries. Fit a fresh unedited modern base
and apply user grip offsets afterward, preserving note spacing and requested
movement. Extend native tests for direct previews, every left grip, control/
endpoint edits, object translations, repeated layout and restoring the base.
See `v0.121-grace-slur-edit-spacing-independence.md`; apply after v0.120 and
repeat M3's upper-left grip edit in the existing v0.113 natural input. Native
verification pending; other differences deferred.


### v0.122: reduce redundant work during grace-slur editing

User confirms the no-jump behavior but reports slow interaction. Reuse already-
validated joint component rectangles in the obstacle collector and raw anchors
supplied by the current layoutSegment frame, eliminating a repeated component
extraction round and raw-anchor query. Preserve fitting/clearance and edit
behavior. Add raw-frame refresh coverage; old/new collector outputs agree in
70 local mock cases. See `v0.122-grace-slur-repeated-work-reduction.md`.
Native mtests and editing-speed comparison remain pending; this is a first
performance increment, with no claimed native speed ratio.

### v0.123: measure the remaining held-arrow slowdown

v0.122 did not resolve held-arrow latency. Add opt-in command and nested-stage
timings with `MS_LAYOUT4_PROFILE=1`, including counts for item-context checks,
grace planning, anchors, obstacles and fitting. No layout behavior changes.
See `v0.123-grace-slur-edit-timing.md` for the short on/off reproduction and
log command. Native tests and timing results are pending; this checkpoint
provides diagnosis rather than another unmeasured optimization.

### v0.124: reuse eligibility within grace component queries

Native timings identify repeated item-context validation as the main measured
cost, with about 19,700 checks per arrow step and only about 0.23 ms in curve
fitting. Reuse chord eligibility within each read-only component/distance
query, clearing on scope changes and retaining all item and pair checks.
The local old/new harness agrees in 43,200 cases and reduces spanner scans;
add native policy-equivalence coverage. See
`v0.124-grace-query-context-reuse.md`. Apply after v0.123, run both mtests,
and repeat the existing timing/edit reproduction. Native speed gain remains
unmeasured; no new score or engraving adjustment is included.

### v0.125: extend reuse to the complete read-only grace query

v0.124 native mtests pass. Held-arrow latency remains, though the new log's
median experimental command is about 112 ms rather than 202 ms. Share the
existing eligibility cache across complete packing and merged projection,
discarding it before any later query, beam preparation or edit. Add native
query-boundary invalidation coverage. See
`v0.125-grace-geometry-query-reuse.md`. Apply after v0.124; repeat both mtests
and the existing timing reproduction. Native v0.125 performance is pending.

### v0.126: remove the second combined-component extraction round

v0.125 native mtests pass; the new log shows about 96 ms median experimental
commands and continuing latency. Packing and projection currently extract
the same target/member shapes twice. Reuse packing's validated physical
snapshot within that geometry query, preserving projection order and fresh
inputs for every later query. Add native independent fresh-extraction
comparison. See `v0.126-grace-packed-component-reuse.md`. Apply after v0.125;
native mtests and latency validation are pending, using the same score.

### v0.127: reuse completed preparation during system-prefix fitting

v0.126 native mtests pass, but latency remains and Studio 4.7.5 is responsive.
The latest log shows about 85 ms median commands and unchanged preparation
counts. Keep completed combined-grace beam preparation only within one
collectSystem call; skip it after a fresh validated geometry query matches
every recorded input and beam identity exactly. Add successful final-output
and failure-atomicity checks plus a `beamReuse` timing counter. See
`v0.127-same-system-grace-beam-reuse.md`. Apply after v0.126; native mtests
and the existing timing/edit reproduction remain pending.

### v0.128: reuse context checks in system collision queries

The v0.127 log confirms ten preparation reuses, beam preparation calls down
from 14 to four, geometry calls from 73 to 53, and about 69 ms median commands.
Extend the existing query-local eligibility cache to generic system shape
distances and collision projections; preserve item/pair/provenance checks.
Expand native cached/uncached policy, distance and projection comparisons.
See `v0.128-system-collision-context-reuse.md`. Apply after v0.127; both
native mtests and the existing timing reproduction remain pending.

### v0.129: detail the remaining grace-planning timing

v0.128 native mtests pass. The latest log shows about 57 ms median commands,
context time down to about seven ms, and unchanged geometry/beam counts.
Split the remaining planning time into component extraction, distance work,
padding setup, native shape extraction and structural chord checks before
choosing another optimization. See `v0.129-grace-planning-detail-timing.md`.
Apply after v0.128 and repeat the same timing test. No engraving changes or
new score; this checkpoint adds opt-in diagnostics, not a speed claim.

### v0.130: reuse native child shapes within grace queries

v0.129 native mtests pass. Detailed timings identify about 20 ms in native
shape extraction, with 1,544 calls, and about 13 ms in grace distances; padding
and structural checks are small. Reuse resolved native child snapshots within
one immutable-input grace query, preserving all geometry and provenance.
Extend native cached/uncached snapshot checks and retain independent final
projection comparisons. See `v0.130-grace-native-shape-reuse.md`. Apply after
v0.129 and repeat both native mtests and the existing timing test. The new
`shapeReuse` counter reports avoided extractions; native gains are unmeasured.

### v0.131: prepare the merged component-pair scope once

v0.130 native mtests pass. The latest log shows about 56 ms median commands,
1,024 element extractions and 260 shape reuses. Grace distance remains about
12.5 ms; avoid rebuilding its three-vector merged context for every pair.
Reuse it within the existing query cache and invalidate by root identity.
Also report compiler optimization/assertions/Qt once per profiling process;
current build comparability with Studio has not been established. See
`v0.131-grace-pair-context-preparation.md`. Apply after v0.130, run both
native mtests and repeat the same timing test, retaining the complete log.

### v0.132: reuse eligibility across a whole system plan

User reports ordinary notation is also slow and retains Debug as the normal
workflow. Share eligibility across each complete read-only system plan instead
of restarting at every projection/distance query. Preserve all pair and scope
checks and discard the cache at query return. Extend local/shared-cache native
comparisons. See `v0.132-system-plan-context-reuse.md`, including the requested
measured percentages: 4.455 ms legacy versus 51.9915 ms experimental, about
1,067% more elapsed time; Studio has not been timed. Apply after v0.131 and
repeat both native mtests and the same Debug timing test. Native gains pending.

### v0.133: isolate ordinary slur and ordinary accepted spacing

v0.132 native mtests pass, but its median command improvement is only about
0.52 ms. Audit the shared arrow/edit/full-system layout path and ordinary
slur fallback. Supply matched ordinary slur and hairpin fixtures with no grace
notes, both experimental settings enabled. See
`v0.133-general-edit-layout-audit.md` for the source findings and timing steps.
No engraving code change or rebuild: open the new v0.133 files in the current
Debug application and measure on/off before another general optimization.

### v0.134: ordinary edit controls are responsive

User logs confirm ordinary slur median 5.2755 ms with spacing on versus
4.612 ms off (14.4% more elapsed time), and hairpin median 11.8145 ms on;
no hairpin-off layout samples were supplied. Neither exercises merged-grace
geometry or grace-distance work. The visible N2 beam change after editing N1
remains an unresolved repeat-layout discrepancy, not demonstrated fallback.
See `v0.134-ordinary-edit-timing-results.md`. Documentation checkpoint only;
no rebuild, engraving change or additional user test requested.

### v0.135: reuse repeated native pair intersections

Memoize the exact ordered native shape-intersection result in the existing
read-only query cache. Keep per-pair eligibility and padding behavior, and
discard all results before geometry changes. Add native comparison coverage
and the pairOverlap timing stage. See v0.135-pair-shape-intersection-reuse.md.
The consolidated patch applies after v0.132 and includes the v0.133/v0.134
diagnostic files. Run native tst_spacing and tst_beam, then repeat the original
merged-grace edit timing. Speed gains and the separate N2 beam issue remain
unverified; Debug responsiveness is still the target.

### v0.136: share validation during one slur layout call

The v0.135 user mtests pass, but its M1 timing shows no clear speed gain.
Share a successful complete merged-geometry query between beam-anchor and
obstacle helpers within one Slur::layout/layoutSystem call. A scoped guard
restores the borrowed pointer on return; no result survives edits or later
layout. Add independent/shared native comparisons and slurGeometryReuse timing.
See v0.136-slur-operation-geometry-reuse.md. Apply after corrected v0.135,
run native spacing/beam mtests, then repeat M1 timing. The separate N2 beam
discrepancy remains unresolved; no measured performance gain is claimed yet.

### v0.137: preparation-to-plan geometry handoff

v0.136 native mtests pass and remove four geometry builds, leaving 49 per M1
command. Hand complete validated geometry from beam preparation into the
immediately following read-only plan. Clear the entire handoff before every
possible preparation write and start each fitting/final call empty. Missing
entries still query fresh. Add full/partial/empty handoff plan comparisons and
planGeometryReuse timing. See v0.137-preparation-to-plan-geometry-handoff.md.
Apply after v0.136, run native spacing/beam mtests and repeat M1 timing.
Appearance and N2's separate discrepancy remain to be checked; no speed gain
is claimed before native measurement.

### v0.138 WIP: classify offset-only slur edits and preserve horizontal layout

v0.137 native mtests pass and M1 reaches 46.152 ms, still about ten times
default elapsed time. Classify known slur-offset commits from captured anchors,
ticks, tracks, property IDs and a command generation; cancel on other layout
requests. For the initial single-staff/single-music-system/one-page Page-view
scope, preserve horizontal layout while rerunning native system elements,
skylines, staff distances and page placement. Fall back for other scopes and
possible vertical overflow. Keep normal undo commands and final updates.
See v0.138-slur-offset-layout.md. Native full-layout equivalence, undo/redo,
UI and timing checks are pending. Apply after v0.137, run spacing/beam mtests,
then repeat M1. Successful shortcut commands show slurOffsetLayout without
plan/system-collection counters; no native speed claim is made yet.

### v0.139 WIP: independent two-staff offset updates

v0.138 native interaction is smooth: 35 shortcut commands at median 9.180 ms,
80.1% below v0.137. All avoid system collection/spacing plans. See its updated
result note; native mtest status still needs a separate reported result.
Extend the same bounded update to two visible pitched staves without any
cross-staff notes/graces/beams/tuplets/spanners. Keep native skyline, staff-
distance and page layout. Add six native equivalence/undo rows across both
beam styles, snapshot geometry on every staff and test cross-staff rejection.
See v0.139-two-staff-slur-offset-layout.md. Apply after v0.138, run spacing/beam
mtests and test v0.139-spacing-two-staff-slur-offsets.mscx on both staves.
Multiple systems/pages and general note-changing edits remain future work;
no two-staff performance or native equivalence result is claimed yet.

### v0.140 WIP: forward grace-slur anchor admission

v0.139 native mtests pass; 252 two-staff offset edits use the shortcut at median
16.7385 ms, with no plan/system collection counters. Shift+Right screenshots
expose a separate anchor-change spacing fallback. Admit one explicit Up/Down
after-grace span to the next measure's first chord on the same track. Keep
native curve/range layout and adapt Studio's endpoint-side note kerning for
this span. Automatic/cross-track/multi-segment/farther cases retain fallback.
Add actual anchor-command/plan acceptance, fresh-layout, undo/redo, style-
roundtrip and cached/fresh kerning tests. See v0.140-forward-grace-slur-spacing.md.
Apply after v0.139, run spacing/beam mtests, then use
v0.140-spacing-slur-anchor-changes.mscx to repeat M1 end-grip Shift+Right.
Native correctness and visual results are pending; no speed claim is made.

### v0.141 WIP: offset-only undo/redo updates

v0.140 native mtests pass, but recordings expose slow undo despite improved
direct arrows. Inspect every next history operation: exact leaf ChangeProperty
objects, one unlinked slur segment, offsets plus GENERATED only. Verify actual
index movement and unchanged anchors after native replay, then use the existing
bounded update. Reject anchors/mixed/subclass/nested/multiple-target commands.
Add undoRedo profiling and native history-admission/replay assertions.
See v0.141-slur-offset-undo-layout.md. Apply after v0.140, run spacing/beam
mtests, then repeat held-arrow offsets followed by held Undo/Redo on the same
v0.140 score. Native results are pending. Longer Shift+Arrow spans retain the
known admission fallback; this version does not expand their scope.

### v0.142: pre-system kerning coordinate safety

The v0.141 native-pass report was corrected: undo/redo feels better but
spacing aborts at graceBeforePlan:legacy-beams. Source audit finds an
unconditional v0.140 page-coordinate query; ledger-line pagePos requires a
system that can be absent during grace preparation. Skip it for zero slur
restriction, validate frames/staves/finiteness otherwise, and reject atomically
when unavailable. Add pre-system ledger and restricted-frame regressions.
See v0.142-grace-kerning-coordinate-guard.md. Apply after v0.141 and rerun
spacing/beam mtests before resuming broader reanchor work. No native crash
reproduction or confirmed fix is claimed before that rerun.

### v0.143 WIP: explicit forward after-grace traversal

v0.142 native mtests pass. Remove v0.140's next-measure/first-chord restriction
for qualified explicit after-grace slurs ending later than the owner's end.
Combine multiple forward-span endpoint flags without an arbitrary count cap;
keep original adjacent/local behavior and unsupported categories unchanged.
Add eight actual Shift+Right traversal/plan/fresh-layout/undo/redo rows.
See v0.143-forward-grace-slur-traversal.md. Apply after v0.142, run spacing/beam
mtests and use v0.143-spacing-slur-anchor-traversal.mscx. This fixes the shown
admission switch; reanchors still use range layout, with no new speed claim.
The separately planned performance stage must prove horizontal geometry is
unchanged before skipping fitting/preparation work.

### v0.144: grace-endpoint traversal correction

The v0.143 test's main-chord-only traversal model was incorrect: native
Shift+Arrow visits grace notes too. Its eight pointer failures expose that
mistake, while screenshots expose the independent rejection of grace endpoints.
Admit later before/after-grace stops and later members of the source owner's
after-group, validating parent/membership/track/tick metadata. Preserve the
original main-pair contract and native navigation. Test forward and backward
grace stops, published modern plans, fresh layout and undo/redo.
See v0.144-grace-endpoint-admission-fix.md. Apply after v0.143, run spacing/beam
mtests and reuse the v0.143 visual score. Native correctness remains pending;
this correction does not implement the reanchor speed optimization.

### v0.145 WIP: cross-system logical endpoint admission

v0.144 native mtests pass. Remove rendered-segment-count admission gates from
explicit forward after-grace spacing and logically same-chord/measure pair
validators, including stale two-segment state during return to M1. Keep
logical/membership/track/direction guards and existing curve formulas.
Add eight two-system endpoint/tick mutation rows with published-plan checks,
justified first-system/natural last-system comparison, fresh layout, undo/redo
and original curve restoration. See v0.145-cross-system-slur-admission.md.
Apply after v0.144 and run spacing/beam mtests. Use the normal-width
v0.145-spacing-slur-anchor-systems.mscx for actual GUI grip handoff across
the forced break. Native results are pending. General start-height matching
and reanchor speed remain separate work; this version claims neither.

### v0.146: cross-system test history and bounded traversal repetition

The v0.145 GUI is reported stable, but all eight cross-system test rows fail
undo geometry. Correct the synthetic command sequence to include the native
anchor-preservation prefix before duration invalidation. Keep the failing
assertions. Retain the full single-step matrix, use two complete same-system
walks and four pairwise cross-system walks to reduce repeated full layouts.
See v0.146-slur-test-undo-and-runtime.md. Apply after v0.145; run spacing/beam
mtests and note elapsed spacing-suite time. No engraving algorithm changes.


### v0.147: fresh-plan proof for bounded end-anchor updates

v0.146 native mtests pass; spacing suite is approximately 75 seconds. Add a
separate anchor-only command candidate and reuse the bounded native system/
page update only when a fresh natural-width plan matches all active horizontal
positions, widths, stretch, offsets and planned after/merged grace positions.
No cross-command plan cache. Normal range layout handles mismatches and
unsupported contexts. Offset behavior and reanchor undo commands are retained.
See v0.147-slur-anchor-layout-proof.md. Apply after v0.146, run both mtests,
and use the new natural-width score with MS_LAYOUT4_PROFILE=1 for an actual
Debug reanchor comparison. slurAnchorProof/slurAnchorLayout identify the new
path. Start-height matching and broader layout-policy coverage remain pending.

### v0.148: preserve modern spacing with a second main-start slur

v0.147 native mtests pass. The additional-slur screenshots/log use the
v0.145 forced-break fixture, so they do not exercise the single-system shortcut.
Admit bounded same-measure forward main-start slurs, validate normal/grace
endpoints and apply endpoint-side kerning flags. AUTO requires a single-voice
span where read-only Evolution and pinned Studio direction rules agree.
Keep native curves and the existing efficiency scope. New four-row native
test covers addition, X twice, before/after-grace traversal, published modern
plans, fresh layout and undo/redo. Compare the v0.148 before/added scores.
See v0.148-main-start-slur-admission.md. Native results remain pending.

### v0.149: architectural audit; pause case-specific expansion

v0.148 native tests and requested end-anchor/addition visual sequence pass;
start-anchor traversal exposes another admission boundary. Audit finds a hybrid
fallthrough: modern rhythm/local packing can remain when final modern planning
rejects. Full-prefix rebuilding, inferred rectangle identity and attachment/
curve-scope coupling are structural problems. Preserve arithmetic, kernels,
position stream, beam work and fixtures; consolidate prepared producer/attachment
inputs and collection/publication lifecycle. The audit maps all 54 pinned
HorizontalSpacing functions, inventories 101 named diagnostic sites and defines
six mechanism-based work packages and acceptance gates. This is documentation
only; no build, mtest or new screenshot is requested. See
v0.149-spacing-architecture-audit.md and its three CSV inventories.


### v0.150: pass-scoped musical-input prototype, no runtime selection change

Implement the first package-A seam from the architecture audit: exact root
slur endpoints, per-track before/main/after musical order, reverse anchor
attachment indices and note tie/spanner metadata in a read-only view. It does
not consult curve eligibility, first-grace/count/direction whitelists or lazy
anchor completion. No runtime planner consumes it yet. Four focused native
rows cover independent navigation order, immutable query behavior, unresolved
anchors and actual START-grip movement with equal ticks/different identities.
See v0.150-prepared-spacing-inputs.md. After CMake rebuild, run
`tst_spacing spacingInputSnapshot4`; no new visual comparison is needed.
Package-A geometry/direction/consumer integration and package-B lifecycle work
remain outstanding. This stage makes no appearance or speed claim.


### v0.151: differential indexed-attachment consumer

v0.150 native combined tests pass. Extend the prepared view with a separate
compatibility relevance index (exact endpoints OR existing track/tick aliases),
then allow collision-query caches and buildSpacingPlan4 to borrow that view.
Do not collect whole scores in pair/prefix loops. Existing policy evaluates the
same indexed roots; default runtime callers remain on their current lookup.
The same four native rows compare roots, policies and complete plans before/
after actual start-anchor movement and undo; no visual run is needed. See
v0.151-indexed-attachment-consumer.md. General endpoint/direction policy and
shared runtime preparation ownership remain the next architecture steps.


### v0.152: general exact-endpoint slur-side preparation

v0.151 native tst_spacing and tst_beam pass. Build the general slur attachment
flags from the pass-scoped input in one linear preparation, independently of
curve admission, grace-member position, slur count, visibility and line style.
Match pinned Studio ChordLayout::checkStartEndSlurs and the slur portion of
Chord::computeKerningExceptions. Exact note endpoints map to their captured
chord owner; equal-tick grace neighbors receive no attachment flags.

Explicit directions are authoritative. A shared preparation producer can supply
one resolved direction per root; unresolved AUTO directions remain counted as
pending, and unmapped endpoints are counted separately. Never read cachedUp or
live layout data to guess readiness. Validation is atomic, including direction
count, enum values and contradictions with explicit directions. The side mask
may be used as a complete policy only when its pending directions are resolved.
This is the slur portion of kerning preparation; articulation policy is separate.

Files changed: libmscore/spacinginput4.h/.cpp, tst_spacing.cpp and this guide.
The existing four snapshot rows compare flags with an independent exact-root
scan, before/after START reanchoring and undo. A new synthetic-root test covers
multiple simultaneous slurs on a non-first grace, stale cached directions,
hidden/dashed roots, producer-supplied directions and atomic failure. No new
score file or visual comparison is needed. This patch includes its checkpoint
here; no separate checkpoint file or ZIP is required.

Runtime callers still use the verified existing policy. This preparation kernel
does not yet change appearance or performance. Remaining problems: connect
shared authoritative direction/stem preparation and replace curve-based admission;
then address collection/publication ownership and hybrid fallback (audit A/B).
Apply after v0.151, rebuild and run the usual combined spacing/beam CTest command.
Local verification: eight C++11 syntax checks pass (Debug/NDEBUG for the
collector, collision queries, planner and regenerated-moc spacing test), with
unused-parameter and signedness warnings enabled. The extracted actual collector
and attachment kernel pass 960 mock-DOM cases, including 768 direction-table
comparisons against an independent exact-endpoint scan. Native application and
mtests cannot run locally; their results remain pending. Kernel WIP checkpoint:
47fdaa1. The final incremental patch is based on delivered v0.151 29b76cc.


### v0.153: prepared Studio slur-direction rules

v0.152 native mtests pass; tst_spacing took 79 seconds in the user's build.
Continue architecture package A with the pinned Studio 4.7.5 computeUp and
isDirectionMixture rules. Capture cross-staff beam/stem presence and the exact
multi-voice span query as musical metadata. A read-only resolver consumes one
caller-prepared chord/rest direction per exact anchor and emits one direction
per root, directly usable by v0.152's attachment preparation.

Explicit directions remain authoritative. AUTO handles cross-staff start beams,
voice priority, stem mixtures, grace endpoints, owner stems and normal chords
across measures/systems. Index normal anchors once per track and binary-search
the span start. Do not inspect cached Slur::up, captured anchor.up, curve
eligibility or rendered segment count. A definite mixture resolves UP even if
another stem is pending; otherwise required unprepared directions stay AUTO.
Raw NOTE/missing/unmapped AUTO anchors stay pending rather than invoking the
renderer's lazy anchor repair. Invalid direction arrays fail atomically.

Files changed: libmscore/spacinginput4.h/.cpp, tst_spacing.cpp and this guide.
Existing snapshot rows compare prepared results to an independent DOM traversal
of the source rules before/after START reanchoring and undo. Three small existing
fixtures additionally cover synthetic roots from every grace/main/rest/voice
anchor, across systems and cross-staff beams. They test four fully prepared
stem patterns, pending inputs, direction-to-attachment handoff and unchanged
state on validation failure. They do not lay out synthetic slur curves or add
new visual test scores. No extra screenshot or profile run is requested.

Local checks: four C++11 syntax passes (Debug/NDEBUG collector and regenerated-moc
spacing test), with unused-parameter/signedness warnings enabled. The actual
collector, attachment and direction kernels pass 960 mock-DOM cases, including
1,536 direction-table comparisons with full/partial pending stem inputs and
independent endpoint/range expectations. Native application/mtests cannot run
locally. WIP checkpoint ccf76aa preserves the direction kernel; the delivered
incremental patch is based on v0.152 7294353.

Runtime selection and rendered directions are unchanged in this stage. A
prepared stem input must come from the shared producer; the source rule's
cross-system note-line/stem refresh cannot be replaced with cached observations.
Next: connect a shared preparation owner and renderer-consistent directions,
then replace curve-based horizontal admission and resolve collection/publication
ownership and hybrid fallback (audit A/B). Rebuild and run the usual combined
spacing/beam CTest command to include all new rows.


### v0.154: one owner for attachment preparation and fitting queries

v0.153 native mtests pass; tst_spacing took 75.5 seconds in the user's build.
Compose the pass-scoped musical view, caller-prepared chord/rest directions,
resolved slur directions and exact attachment-side tables into one move-only
SpacingAttachmentPreparation4. Successful preparation consumes/resets the input;
validation failure preserves both the input and the previous owner. Moved-from
owners are empty and cannot report complete preparation. Const queries expose
per-anchor sides and per-root directions without new DOM scans.

Bind optional collision caches and natural/squeeze planner overloads to that
owner. Every squeeze pass borrows the same musical/direction/attachment input;
query-local shape/eligibility memoization still starts fresh. No score-wide
collection or direction resolution occurs inside plan, pair or prefix loops.
The existing grace projection preparation is read-only, so it does not invalidate
this lifetime. Discard all borrowers before moving/replacing the owner, editing
the score or preparing geometry that writes to native items.

Files changed: spacinginput4.h/.cpp, spacingcollision4.h/.cpp, spacingplan4.h/.cpp,
tst_spacing.cpp and this guide. Existing snapshot rows compare old/indexed/owned
pair decisions and complete natural plans. They also compare fitting outcomes
and all planned fields at natural and 95% target widths, before/after actual
START reanchoring and undo. Owner tests cover lookup identities, kernel handoff,
move/reset semantics, atomic failed replacement and invalid-owner plan/fitting
rejection. No new fixture or screenshot is required.

Local validation: twelve C++11 syntax passes (Debug/NDEBUG collector, collision,
planner, grace spacing, layout and regenerated-moc test). Unused-parameter and
signedness checks are enabled. The actual collector/direction/attachment/owner
code passes 960 mock-DOM cases, including 1,536 preparation, lookup, move and
failure-atomicity comparisons with fully/partially pending producer inputs.
Native application/mtests cannot run locally. WIP checkpoint fcd840a saves the
owner/planner seam; the incremental patch starts at delivered v0.153 9a22195.

Runtime selection remains unchanged. The optional owned path preserves current
curve-based admission; it does not yet substitute the general flags for that
policy. Shared ownership establishes lifetime, not stem readiness: production
must still refresh stems/note lines on later systems and use the same direction
rules as the renderer. That renderer-consistent producer connection is next,
followed by removing the curve-based horizontal gates and addressing collection/
publication and hybrid fallback (audit A/B). Rebuild and run the usual combined
spacing/beam CTest command; no visual or performance run is requested here.


### v0.155: shared direction priority in both native slur render paths

v0.154 native mtests pass; tst_spacing took 76.16 seconds in the user's build.
Connect the prepared direction rules to native rendering under spacingLayout4.
The original layout() and layoutSystem() direction blocks remain byte-for-byte
unchanged for the setting-off path. A common pure priority kernel now serves
both the indexed prepared-input resolver and a native Studio direction producer.
Both render entry points use that producer when experimental spacing is on.

The producer follows pinned 4.7.5 computeUp/isDirectionMixture: cross-staff start
beam priority, multiple voices before mixture, exact grace endpoint/owner-stem
handling, same-track main chords across measures and later-system stem refresh.
No whole-score musical snapshot is collected in Slur::layout/layoutSystem or a
collision loop. Pitch-staff end chords on another system have note lines and
stem direction refreshed; intervening pitched chords refresh only when both
systems exist and differ. Percussion skips that refresh. Tablature stays on
Evolution's original renderer until its deferred audit. Explicit directions are
literal; missing/non-chordrest AUTO raw anchors retain normal native handling.

This producer is a write phase: Evolution's Note::updateLine updates derived
note y as well as line, and Chord::computeUp updates stem direction. Run it
before capturing immutable musical/geometry inputs, not inside a spacing
query or while an attachment owner is borrowed. It does not lay out entire
scores, pack grace groups, change slur anchor properties or add a new cache.

Files changed: new slurdirection4.h/.cpp and libmscore CMake source registration,
spacinginput4.h/.cpp, slur.cpp, tst_spacing.cpp and this guide. Native tests add
432 priority combinations and three small existing fixtures checking native
full-layout and immediate-layout AUTO directions against the prepared owner,
then full layout again. Fixtures cover grace/two-system, multiple-voice and
cross-staff spans; no backward synthetic curves are rendered. Existing plan,
reanchor and undo tests remain active. No new visual test score is added.

Local validation: twelve final C++11 syntax passes (Debug/NDEBUG producer,
collector, slur renderer, planner, grace spacing and regenerated-moc test), with
unused-parameter/signedness checks. Actual extracted collector, preparation,
shared rule and producer bodies pass 960 mock-DOM cases, 1,536 prepared-input
comparisons, 432 priority checks and explicit later-system refresh, percussion
and tablature checks. Removing only the new opt-in calls/include reconstructs
the previous slur.cpp exactly. Native application/mtests cannot run locally.
WIP checkpoint 2582ab8 preserves the renderer bridge; apply the incremental
patch after delivered v0.154 a50237d. Reconfigure/rebuild for the new source file
and run the usual combined spacing/beam CTest command.

This stage can change AUTO slur direction with experimental spacing enabled.
The planner still uses existing curve-based admission; this is not yet a
production-wide activation of the general attachment-side policy. Next: order
the native producer and immutable owner in the horizontal preparation lifetime,
then replace curve-based horizontal gates and address collection/publication
and hybrid fallback (audit A/B). Native results are required before that next
runtime change. No new screenshot/profile run is required for this checkpoint.


### v0.156: producer/query boundary and identity-based capture

v0.155 native mtests pass; tst_spacing took 75.89 seconds in the user's build.
Separate native direction production from a const prepared-direction query.
prepareSlurDirection4 retains the v0.155 renderer behavior and later-system
note-line/stem refresh. resolvePreparedSlurDirection4 uses the same rule adapter
without any refresh or raw-anchor repair. It requires already prepared inputs;
calling a read-only query does not certify that cached stems are current.

Add captureSpacingAttachments4 as an explicit post-producer handoff. Callers
supply prepared chord/rest directions by exact identity rather than snapshot
indices. The capture collects one musical view, maps those declarations and
composes the existing direction/attachment owner. Undeclared anchors remain
AUTO; live/captured up() is never substituted for a readiness declaration.
Null/foreign/unmapped anchors, duplicates, invalid directions and missing scores
fail without replacing the prior owner. Pending readiness is retained normally.

Files changed: slurdirection4.h/.cpp, spacinginput4.h/.cpp, tst_spacing.cpp and
this guide. Existing snapshot rows use reversed-order declarations, validate
failure atomicity and pending omissions, then borrow the captured owner through
natural/fitting plans, START reanchoring and undo. Native renderer tests compare
the const query with the captured owner and verify unchanged note lines/y,
stem direction, horizontal layout, cached slur direction and command/history
state across the query. Producer writes occur outside that read-only interval.
No new test score or new fixture load is introduced.

Local checks: ten C++11 syntax passes (Debug/NDEBUG direction adapter, collector,
slur renderer, planner and regenerated-moc test), with unused-parameter and
signedness warnings enabled. Extracted actual implementation passes 960 mock-DOM
cases, the existing 1,536 prepared-input and 432 priority comparisons, identity
capture/atomicity checks and an explicit stale later-system test: const query
leaves refresh counters and stale stems unchanged, native production refreshes
them, then const query agrees without further writes. Percussion/TAB checks
remain active. Native application/mtests cannot run locally. WIP checkpoint
6363a2a saves the producer/query boundary; the incremental patch starts at
v0.155 4c6d4e9. Rebuild and run the usual combined spacing/beam CTest command.

No additional renderer or admission behavior is selected in this stage. Capture
is deliberately not installed in plan/pair/prefix loops: it is score-wide and
belongs outside those loops. Next: bind the producer, readiness declarations
and immutable owner to horizontal layout lifetime, then replace curve-specific
admission and resolve collection/publication and hybrid fallback (audit A/B).
No new screenshot or profiling run is requested at this checkpoint.


### v0.157: finalized-system capture enters horizontal fitting

v0.156 native mtests pass; tst_spacing took 75.95 seconds in the user's build.
Install the prepared-input owner in finalized-system horizontal fitting. Capture
one musical domain after grace beam/shape preparation, borrow it throughout the
natural/squeeze plan attempts, then destroy it before publishing segment/measure
positions. Prefix fitting is unchanged; no whole-score snapshot is collected
for each candidate prefix or each finalized system.

Add a shared collector for explicit contiguous measure/root domains. The full
score prototype delegates to it. System capture visits that system's measures
and gets intersecting roots from the native interval index. It preserves raw
external endpoint identity/kind without pretending they have local anchor
indices. Ordinals are local to the captured track/domain. Local pending flags
and global complete() reflect partial coverage; external endpoints do not make
the captured view invalid. Readiness declarations remain omitted/AUTO in this
first runtime binding: existing admission/kerning policy consumes the identity
index; general side flags are not activated and stale up() is not certified.

Native map order must be preserved even when setTick dirties the interval index
without moving an old insertion key. Add lazy map-order ranks to SpannerMap's
ordered interval query. Build them only on demand and invalidate them with the
existing add/remove/clear/tick/length dirty operations. This indexes topology,
not geometry, positions or layout admission. Ordinary interval callers do not
build ranks. First ordered use after invalidation may scan roots/rebuild the
native interval tree; subsequent systems reuse those indices. There is no
unconditional whole-root-map scan per system and no whole-score anchor scan.

Files changed: spacinginput4.h/.cpp, spannermap.h/.cpp, layout.cpp,
tst_spacing.cpp and this guide. Native tests compare local anchors/root order,
pair decisions, complete natural plans and fitting outcomes against full/native
queries before/after START reanchoring and undo. They check scope/capture failure
atomicity, outside-domain declarations, map-key/current-tick differences and
remove invalidation. Existing full/range/edit/undo geometry tests now exercise
the production finalized-system binding. No new test fixture is loaded.

Local verification: twelve C++11 syntax passes (Debug/NDEBUG root index,
collector, layout, planner, collision and regenerated-moc test), with unused-
parameter/signedness checks. Actual collector/direction/owner code passes the
960 mock-DOM suite plus multi-measure partial domains, external endpoints and
contiguity/atomicity checks. The actual native ordered interval implementation
passes 3,200 independent ordering checks across initial queries, moved ticks,
removal, reinsertion and clear. Prior preparation/priority/producer checks stay
active. Native application/mtests cannot run locally. WIP 6321c5d saves the lazy
ordered query; WIP 7803361 saves scoped capture and fitting integration. The
incremental patch begins at delivered v0.156 4bea04e.

Rebuild and run the usual combined spacing/beam CTest command. No intended visual
change, new screenshot or profiling run is requested for this checkpoint. Next:
provide authoritative readiness/direction inputs within the captured system
lifetime and replace curve-specific horizontal admission. Collection/prefix
rebuilding, coherent fallback and final publication remain audit package B;
this stage does not claim those problems are resolved.


### v0.158: native measure-production readiness reaches scoped capture

v0.157 native tst_spacing passes in 78.04 seconds in the user's build. Supply
explicit direction readiness from the native preparation lifetime instead of
leaving every finalized-system anchor pending. LayoutContext owns the records
only when spacingLayout4 is enabled; no record survives a layout command.

Invalidate a measure's identities before getNextMeasure reprocessing, including
its line-mode early-return path. Mark them only after native note/line, initial
stem/beam membership, chord/grace layout and local shape production finish.
These are completion certificates for that phase, not cached up() values. After
final grace-beam preparation, emit current directions only for live anchors with
those certificates and capture the system owner. Missing/unprocessed measures
remain AUTO. Current beam writes can change values without making a stored
direction snapshot stale; removed/replaced identities are not dereferenced.

Files changed: spacinginput4.h/.cpp, layout.h/.cpp, tst_spacing.cpp and this
guide. The context allocates readiness records only for the experimental path.
The owner still ends before horizontal publication. Prefix fitting is unchanged.
The current initial-direction phase is certified here; this does not claim all
later cross-staff/page-pass stem constraints are final.

Native snapshot tests check that already existing geometry does not imply a
certificate, partial-measure readiness stays partial, invalidation removes it,
new passes start empty and emitted identities retain native order. Captured
ready inputs feed the same local-root direction, pair-policy, complete natural
plan and fitting parity checks before/after START reanchoring and undo. Context
allocation is checked against the actual enabled/disabled style. No new test
fixture or score load is added.

Local validation: fourteen C++11 syntax passes (Debug/NDEBUG collector, layout,
planner, collision, slur renderer, regenerated-moc spacing test and score.cpp's
incomplete-owner header integration), with unused-parameter/signedness checks.
Actual extracted code passes the 960-case mock-DOM suite with readiness ordering,
partial domains, invalidation/fresh-pass and post-beam direction-change checks;
existing direction, interval and owner checks remain applicable. Native app and
mtests cannot run locally. WIP checkpoint efcf083 saves the producer readiness
integration; the incremental patch starts at delivered v0.157 23e514e.

No new direction/side-policy consumer is activated: admission and kerning still
use the previously verified rules. Next: use prepared direction/endpoint-side
inputs to remove the remaining curve-specific horizontal gates, including the
internal grace preparation queries. Prefix collection and coherent fallback
remain audit package B. Rebuild and run the usual combined spacing/beam CTest
command. No new screenshot or profiling run is requested at this checkpoint.


### v0.159: retain prepared context through grace queries; test general slur policy

v0.158 native tst_spacing passes in 83.27 seconds in the user's build. Thread
one read-only context cache through before/after/merged grace packing and source,
target and member projections. Construct the planner's cache before grace
validation, not afterward. The same attachment input owner now reaches those
nested queries as well as final note/segment constraints. Standalone null-cache
callers retain their existing behavior. Mutable beam preparation and publication
still end/discard read-only caches before writing native geometry.

Add an explicit PREPARED slur-policy mode to the query cache. It uses exact
attachment sides and declared stem directions, independently of curve scopes,
first-member tests or slur counts. Known tick aliases do not create attachments;
unknown raw endpoints at an alias remain ineligible rather than silently absent.
Pending directions refuse that query without falling back to compatibility.
Articulation/tie/note-spanner and physical group restrictions are unchanged.
This mode is exercised only by tests: production constructors still select
COMPATIBILITY. There is no new user setting and no general-policy activation.

Files changed: gracespacing4.h/.cpp, spacingcollision4.h/.cpp, spacingplan4.cpp,
tst_spacing.cpp and this guide. Existing snapshot rows compare standalone/shared
packing and full source/target rectangle identity, ownership sets, offsets and
admission, then natural/fitting plans before/after START reanchoring and undo.
The borrower in the scope test now ends before owner replacement; subsequent
geometry queries bind a fresh cache, respecting the documented lifetime.

Extend the existing synthetic-root test: twelve simultaneous slurs, covering
AUTO/UP/DOWN, visibility and line styles, start on a non-first after-grace.
Compatibility rejects the merged query; the prepared mode packs and projects
it without moving native positions. A pending owner still rejects atomically.
Unresolved raw-anchor alias tests remain conservative. No new fixture, score
load or visual comparison is introduced.

Local checks: fourteen final C++11 syntax passes (Debug/NDEBUG grace spacing,
collision, planner, layout, chord/slur renderer and regenerated-moc test), with
unused-parameter/signedness checks. The 960-case actual-code mock suite also
executes the new prepared readiness and side-mask consumers over the existing
1,536 direction/owner combinations, with a compatibility fallback stub that
aborts if reached. Cached side reads and unknown aliases are checked separately.
Native physical packing/projection and application/mtests cannot run locally.
WIP 95009f1 saves the shared grace-query seam; WIP 2f61d8e saves the prepared
policy consumer. The incremental patch starts at delivered v0.158 4de92b3.

Rebuild and run the usual combined spacing/beam CTest command. No intended
visual change or profiling run is requested. Next: make early mutable grace
production and post-spacing publication use matching attachment policy before
activating PREPARED in production. In particular, compatibility-packed geometry
handed off from beam preparation cannot be reused under changed packing rules
without rebuilding or proving matching policy provenance. Prefix collection,
coherent fallback and publication ownership remain audit A/B work.


### v0.160: fresh grace-query scopes and packing-policy provenance

v0.159 native tst_spacing passes in 79.56 seconds in the user's build.
This is the first bounded implementation stage following the remaining-stage
estimate: establish the mutable producer/publication boundary before binding
and activating the general prepared policy.

A pass-local factory can now be borrowed by a Score through a nested RAII
scope. Each before/after/merged query captures a fresh attachment owner and
constructs its own physical query cache. Both die before control returns to
code that writes beam, shape or position data. A failed capture refuses the
query without silently using compatibility inputs. No production factory is
bound yet; existing unbound calls retain their previous compatibility policy.
Capture callbacks must be read-only and limited to the active musical domain.

Before-grace application and beam preparation use these query boundaries.
After-grace publication completes planning and all clearance checks with the
same cache, then destroys it before publishing positions. Before/after/merged
plans carry the policy used to pack them. The system planner rebuilds a handed
off merged result when its policy differs; beam convergence also compares
policy provenance. These tags are internal, not serialized score settings.

Files changed: libmscore/gracespacing4.h/.cpp, chord.cpp, layout.cpp, score.h,
spacingplan4.cpp, tst_spacing.cpp and this guide. Tests cover nested and disabled
bindings, fresh capture around native beam writes, capture-failure atomicity,
publication idempotence, and rebuilding mismatched packing-policy handoffs.
The full-fixture capture helper exists only in tests; production integration
must use scoped collection and explicit native producer readiness.

Local checks: sixteen C++11 syntax checks covering Debug/NDEBUG grace spacing,
collision, planner, layout, chord, slur, regenerated-moc tests and Score header
integration, with unused-parameter/signedness checks. An extracted actual-code
factory/scope/query-runner harness with instrumented mock owners and query
backends passes 1,600 fresh-query lifetime checks plus nested scope, failed
capture and exception-unwind checks. These are not native engraving tests;
the application and mtests cannot run locally. WIP 41ad187 saves the core seam.

Rebuild and run the usual combined tst_spacing/tst_beam CTest command. No new
screenshot or profiling run is requested for this boundary-only checkpoint.
Next: bind narrow native preparation and activate matching policy across early
packing, final planning and publication, then run consistency and visual checks.
Cross-boundary root direction and early coordinate-frame readiness must be
resolved at that binding, not hidden by new grace-member/count exceptions.
The broader prefix/fallback audit remains open and is not a reason to postpone
all supported-path visual verification indefinitely.


### v0.161 WIP: local slur-side coordinates before system placement

Restored the delivered v0.160 source by replaying saved patches against the
original upstream base in a temporary Git index. Native v0.160 results remain
unverified; the user has authorized deferred aggregate testing.

Prepared-policy side comparisons now use same-measure, same-visual-staff
coordinates when available. Native grace-chord and ledger-line origin rules
are retained; a missing system no longer makes these local queries fail merely
because pagePos is unavailable. Cross-staff and other frames still require the
existing ready-system check. Compatibility mode is unchanged. No prepared
production binding is activated by this patch.

Files: spacingcollision4.h/.cpp, tst_spacing.cpp and this guide. The existing
attachment fixture compares local side with native page coordinates, detaches
the measure, then repeats and restores it before assertions. Native tests are
deferred. Next: share scoped capture between early measure and final system
queries, without full-score collection in inner loops.


### v0.162 WIP: shared early-measure/final-system capture

Factor native interval-root selection, snapshot construction and atomic owner
capture into one domain helper. Add a measure-scoped entry point that does not
require a system. Direction readiness declarations can now be requested for a
measure; the system declaration path concatenates those same results. No score-
wide collection or producer writes are introduced in these capture functions.
External root endpoints remain pending, rather than borrowing cached up().

Files: spacinginput4.h/.cpp, tst_spacing.cpp and this guide. Extend the existing
attachment fixture to capture a certified measure, detach it from its system,
capture again, and check domain membership, null-capture atomicity and readiness
invalidation. Restore the parent before assertions. No additional fixture load.

Native build/mtests are deferred at the user's request. No production prepared
factory is bound yet. Outstanding integration is explicit: produce root directions
across capture boundaries, bind the measure/system factories at their native
write boundaries, and activate one policy consistently through fitting and
publication. Do not infer those readiness guarantees from existing shapes.
Workspace recovery and rebuilding local QA consumed this batch; the earlier
remaining-stage estimate is not yet met. These WIPs are not a visual-test-ready
activation and do not claim Studio parity.

Batch local validation: restored the Qt header/moc syntax environment, then ran
six C++11 syntax checks (Debug/NDEBUG collision, input capture and regenerated-
moc spacing tests) with unused-parameter/signedness checks. The actual local-side
helper also passes 9,826 coordinate cases in an instrumented mock DOM, including
staff offsets, normal/grace chord origins, ledger-line origins and atomic
failure. These checks do not execute native engraving or replace the deferred
spacing/beam mtests. Cumulative patches start at delivered v0.160; apply either
the incremental sequence or the cumulative patch, never both. A separate full-
series recovery patch is for rebuilding the original base, not for an existing
patched checkout.


### v0.163 WIP: root-direction declarations across capture domains

Add identity-validated producer direction declarations for roots whose endpoints
can lie outside a measure/system snapshot. Local endpoint membership remains
unchanged. Duplicate, foreign, invalid or explicitly contradictory declarations
fail before consuming input or replacing an owner. AUTO remains pending.
Existing callers keep their previous behavior when no declarations are supplied.

Files: spacinginput4.h/.cpp, tst_spacing.cpp and this guide. Extend the existing
synthetic-root fixture with authoritative AUTO-root direction, duplicate/invalid
rejection and owner/input preservation. Native tests remain deferred. Next bind
native producer certificates and fresh root resolution at mutable boundaries;
this API does not read cached Slur::up() or activate the policy itself.


### v0.164 WIP: bind native producer and fresh query scopes

Bind bounded measure factories after native beam directions/mirroring and before
recursive chord layout. Bind system factories around prefix preparation and
final fitting/publication. Capture callbacks remain read-only, create fresh
owners per query and never retain physical memos across writes. Root producers
run outside callbacks, at most once per LayoutContext; root directions are
resolved again from current inputs rather than cached Slur::up(). The early
root producer runs before createBeams so its stem refresh cannot override that
measure's subsequent native beam direction production.

Files: layout.cpp, spacinginput4.h/.cpp and this guide. Direction readiness now
has a separate root-production certificate; existing measure certificates are
established before recursive packing as well as at the completed-measure boundary.
All bindings still request COMPATIBILITY pending the next activation checkpoint.
Production does not collect whole-score views. Native testing remains deferred.
Next thread explicit policy through natural/squeeze plans and activate matching
prepared policy at all bound phases together.


### v0.165 WIP: activate matching prepared policy through production

Switch the bounded native factories to PREPARED and explicitly carry their mode
into prefix natural planning and final natural/squeeze passes. Plan options now
name the policy; prepared mode without an attachment owner rejects atomically.
The no-owner/standalone compatibility APIs retain their defaults. Every squeeze
retry preserves the selected policy. Policy provenance continues to prevent
reuse of merged packing from a different mode.

Files: spacingplan4.h/.cpp, layout.cpp and this guide. Early packing, mutable
beam preparation, final planning and post-spacing publication are now connected
under one prepared policy when spacingLayout4 is enabled. No new setting or
serialization is introduced. Legacy style-off factories remain unbound. General
articulation/tie/note-spanner, unsupported item and stronger-compression limits
remain; this is not completion of the broader spacing audit. Native tests are
deferred. Next add production-route consistency assertions and a compact visual
score for internal/endpoint slurs, manual edits and boundary crossings.


### v0.166 WIP: retained-layout proof uses the activated policy

Update the anchor-edit horizontal proof to snapshot settled native inputs and
request PREPARED explicitly. This avoids proving a newly prepared layout with
compatibility constraints and unnecessarily taking the slower full-layout path.
The snapshot API requires completed producers for every intersecting root span;
it performs no refresh, lazy anchor resolution or geometry writes and is not
used as a shortcut during mutable collection. Offset-only edits remain on their
existing fast path without this extra proof.

Files: spacinginput4.h/.cpp, layout.cpp, tst_spacing.cpp and this guide. Add one
production-route test using the existing two-staff fixture: native publication
must match a fresh prepared plan, all grace plans retain prepared provenance,
no-owner prepared requests fail atomically, natural-width squeezing agrees with
natural planning, queries preserve geometry and no factory binding escapes.
Native test execution is deferred. Next prepare the visual gate and complete
local syntax/protocol validation of the aggregate changes.


### v0.167 WIP: aggregate production checks and visual gate

Add v0.167-grace-policy-natural.mscx and v0.167-grace-policy-systems.mscx.
Both enable beamLayout4 and spacingLayout4. They derive from the existing
v0.145 fixture: two pitched staves, before/after merged groups, explicit upper
and lower slurs, plus a new main-note root crossing the barline (AUTO on the
upper staff, forced DOWN below). The second score forces that barline onto the
next system. P1/P2 labels and the versioned title identify the comparison.

Extend preparedSpacingProduction4 with both scores. Compare actual publication
against prepared natural/squeeze/justification rules, not a compatibility plan
or an assumption that every system is natural. Check policy provenance, query
read-only behavior, atomic rejection, absence of escaped factory bindings and
style OFF/ON round-trip geometry. Native test execution remains deferred.
Files: tst_spacing.cpp, these two scores and this guide.

This batch reaches the supported-path visual gate after aggregate native tests.
Apply the cumulative-since-v0.162 patch OR incremental v0.163 through v0.167.
Run ctest -j1 --output-on-failure -R '^(tst_spacing|tst_beam)$' once. Only after
it passes, open the two v0.167 files in Evolution and Studio 4.7.5. Compare P1/P2
main-to-grace gaps, internal grace spacing, beam heights and slope; the cross-
barline AUTO/forced slurs should not change spacing according to whether the
barline is a system break. Slur curve engraving itself is still a separate
known difference: evaluate its effect on spacing, not exact curve equality.

In Evolution, move an inner slur grip vertically and undo/redo it; note spacing
must remain fixed. Shift+Arrow through first/second grace and main anchors,
then back; a spacing change must follow actual endpoint-side constraints, not
revert wholesale to compatibility packing. Preserve the existing manual
control-point edits and one-command-per-arrow undo behavior. Toggle spacing
OFF/ON and reopen to check persistence. Please provide one overview per engine
for each score plus close-ups of any differing P1/P2 gap; no broad new corpus
is requested at this gate.

Outstanding beyond this gate: unsupported engraving/item coverage, the broader
hybrid-fallback and full-prefix audit, and Debug performance profiling of the
new root/query preparation. No claim is made that this completes all Studio
spacing, fixes cross-staff inset tremolo behavior or ports slur curve layout.


### v0.168 WIP: first-layout AUTO root look-ahead production

A native AUTO root can reach a measure that has not yet run getNextMeasure.
Before certifying that root, explicitly refresh native note lines/stem direction
for its unproduced endpoint/span chords. Use the same updateLine/computeUp
operations as the native slur direction producer; skip TAB/percussion and leave
already certified native anchors untouched. Do not certify an entire future
measure merely because some root inputs were refreshed. This runs only in the
mutable producer phase, never in a query callback, pair loop or snapshot.

Certify the current measure's initial native directions before this look-ahead,
so it preserves the current native grace/voice choices; createBeams afterward
can still update directions and subsequent declarations read those live values.
Root production remains once per layout context and root direction resolution
remains fresh after beam writes. Files: spacinginput4.cpp, layout.cpp and guide.

The visual gate remains the two v0.167 scores and aggregate spacing/beam CTest
command documented above. Native execution is deferred to the user's machine;
local checks cannot establish rendered Studio parity or Debug responsiveness.


Extend the production-route fixture test with both endpoint measures detached:
corrupt an unproduced end chord's line/direction, certify only the start measure,
run root production, restore the parents, then require corrected end inputs,
unchanged start direction and no false certificate for the future measure. Both
new v0.167 files must exercise this check. No assertion runs with detached
measure parents. This adds tst_spacing.cpp to the v0.168 files.


Normalize AUTO producer declarations on explicitly directed roots back to that
literal direction, keeping the owner's direction vector consistent with its
attachment flags. Add an assertion for that case to the existing declaration
validation test. A pending AUTO request still remains pending.


Local aggregate validation: fourteen Debug/NDEBUG C++11 syntax checks across
input/collision/grace spacing, direction resolver, planner, layout and regenerated-
moc spacing tests, with unused-parameter and signedness warnings enabled. Final
changed input/layout/test files were checked again after the look-ahead and
literal-direction fixes. The actual preparation function passes 3,600 extracted
root-declaration validation/atomicity cases with mocked direction/flag kernels;
the actual local-side helper passes 9,826 mock-DOM coordinate cases. XML fixture
checks validate structure, note totals, connector balance and enabled settings.
These are local syntax/protocol checks, not native rendering or mtest results.

Saved checkpoints in this batch: v0.163 root declarations, v0.164 native query
factories, v0.165 prepared-policy activation, v0.166 retained-layout proof,
v0.167 visual/production fixtures, v0.168 first-layout producer look-ahead and
literal-direction normalization. Incremental patches apply in that order from
v0.162; the cumulative-since-v0.162 patch replaces the whole sequence. A separate
cumulative-since-v0.160 option includes v0.161/v0.162 for a checkout still there.
Do not apply both cumulative and incremental forms. No intermediate native test
result is assumed in these WIPs; the next gate is one aggregate native build and
spacing/beam CTest run, then the two visual score comparisons if it passes.
