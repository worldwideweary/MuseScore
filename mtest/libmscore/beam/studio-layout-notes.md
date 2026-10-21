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
legacy placement. Beam grouping, beamlet selection, segment construction and
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
