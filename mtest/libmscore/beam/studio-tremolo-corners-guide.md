# Tremolo corner comparisons — v0.20

Apply the v0.20 patch after v0.19, or open the included scores directly in your
existing v0.19 build. This checkpoint changes no engraving code or application
tests. The user reports mtest passing through v0.19, and the V3/V4/V5 up/down
captures looked broadly consistent with Studio 4.7.5.

Open the same original files in Evolution and Studio 4.7.5. These new files
include `beamLayout4=1`, so Evolution's experimental beam style starts enabled.
Verify it under Format → Style → Beams. Studio uses its own layout. Turn the
setting off in Evolution when checking legacy behavior, then restore it before
capturing comparisons. The final-system fill limit is zero in both files to
avoid the compressed final system seen in the earlier anchor score.

## K1–K12: stroke counts, chords, and ledger lines

File: `studio-tremolo-corners.mscx`. Twenty-four 1/4 measures, two per system.
Measures 1–12 use up stems; 13–24 repeat the same pitches with down stems.
Labels deliberately stay short. The chord cases use three notes per chord.

| Case | Up measure | Down measure | Contents | Inspect |
|---|---:|---:|---|---|
| K1 | 1 | 13 | Standalone rising, one stroke | Endpoint gaps and slope |
| K2 | 2 | 14 | Standalone falling, two strokes | Stroke grid and stem tips |
| K3 | 3 | 15 | Standalone rising, three strokes | Compare with the V3 baseline |
| K4 | 4 | 16 | Standalone falling, four strokes | Stack height and notehead clearance |
| K5 | 5 | 17 | Standalone rising low chords, three strokes | Stem length includes the complete chord |
| K6 | 6 | 18 | Standalone falling high chords, three strokes | Chord clearance and staff placement |
| K7 | 7 | 19 | Standalone low ledger notes, three strokes | Shortening and staff overlap |
| K8 | 8 | 20 | Standalone high ledger notes, four strokes | Four-stroke stem-length exceptions |
| K9 | 9 | 21 | Rising inset chords, two strokes | Parent beam and inset stack clearance |
| K10 | 10 | 22 | Falling inset chords, three strokes | Anchor interpolation and complete chord heights |
| K11 | 11 | 23 | Rising inset notes, four strokes | Ordinary beam plus four tremolo strokes |
| K12 | 12 | 24 | Falling inset notes, one stroke | Thin stack control |

Start with K4/K5/K7/K8, then K9–K12. Compare vertical distances in staff spaces,
not pixels: horizontal spacing still differs between Evolution and Studio.
Keep the case labels and staff visible in captures. Check both directions.
No particular slope is forced flat by this fixture.

## Q1/Q2: dense inset pairs

File: `studio-tremolo-tight.mscx`. Two 6/4 measures, one per system: Q1 up and
Q2 down. Each has 48 written sixteenth-note chords with actual 1/32 durations,
connected into 24 two-note tremolos under one ordinary beam group. Stroke counts
cycle 1, 2, 3, 4 six times from left to right. The short-note sequence changes
pitch direction to exercise the parent beam's contour rules too.

Inspect narrow stroke spans, reduced endpoint gaps, collisions with stems or
neighboring tremolos, and clearance from the ordinary beams. The default gap
is 1sp; the adapter reduces it when anchor distance is below 2.6sp, toward a
0.4sp minimum. Actual gaps depend on each application's horizontal layout, so
they need not match if note spacing differs. A very short stroke can be correct;
a reversed or nonfinite path is not. Compare the first four pairs in close-up
if the whole-system capture is too small to read.

## Validation and next step

XML parsing, exact duration sums, natural-pitch spellings, pair connections,
case counts and the enabled style were checked locally. These scores have not
been loaded or rendered in either application here; this is a diagnostic pack,
not a claim that all new cases match. No new application test was added because
this checkpoint changes only fixture inputs and documentation.

After the visual pass, isolate any differences into a small regression case
before changing the algorithm. Opposite stems, cross-staff tremolos, flags,
small/grace notes, stemless pairs and manual adjustments remain separate work.
