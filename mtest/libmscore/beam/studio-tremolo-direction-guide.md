# Tremolo direction comparisons — v0.23

Open `studio-tremolo-direction.mscx` in Evolution and Studio 4.7.5. Evolution's
experimental beam style starts enabled. This checkpoint adds a diagnostic
score only; it does not yet change stem-direction behavior. Apply its patch
after v0.22 to keep the inputs and notes in your source tree, or open the score
directly in the current build. There are twelve 1/4 measures, two per system.
Each contains a standalone two-note tremolo with written quarter notes and
actual 1/8 durations. U = Up, D = Down, A = Auto in the saved stem properties.

## What the Studio source says

Studio v4.7.5 TremoloLayout::calcIsUp() unifies ordinary unmodified same-staff
tremolo stems. If the first chord has an explicit direction, that direction
wins. Otherwise the second explicit direction wins. When both are Auto and
there are no other voices, the combined sorted notes determine direction by
canceling balanced outer pairs and moving inward. Complete balance gives Down.
ChordLayout::computeUpTremoloCase() also returns this common direction for
unmodified non-cross-staff pairs. These are source-derived expectations;
confirm them in the application before adapting Evolution's behavior.

| Case | Stored first/second directions | Pitches | Expected Studio stems |
|---|---|---|---|
| O1 | Up / Down | C4 → C5 | Both Up |
| O2 | Down / Up | C4 → C5 | Both Down |
| O3 | Up / Down | C5 → C4 | Both Up |
| O4 | Down / Up | C5 → C4 | Both Down |
| O5 | Auto / Auto | C4 → C5 | Both Up |
| O6 | Auto / Auto | C5 → C4 | Both Up |
| O7 | Auto / Up | C5 → C4 | Both Up |
| O8 | Auto / Down | C4 → C5 | Both Down |
| O9 | Up / Auto | C5 → C4 | Both Up |
| O10 | Down / Auto | C4 → C5 | Both Down |
| O11 | Auto / Auto | Low three-note chords rising | Both Up |
| O12 | Auto / Auto | C4 → A5, balanced around B4 | Both Down |

Start with O1–O6, then O7–O12. Keep labels visible. Compare actual stem
directions first, then tremolo tilt, stack side, notehead clearance and stem
lengths. Evolution may still show opposing stems and legacy stroke geometry
where its modern adapter declines the pair. Those differences are expected
diagnostic results at this stage. O11 is a same-direction chord control.

The intended next adaptation will derive layout direction without rewriting
the stored stem-direction properties, so disabling the score style can restore
Evolution's legacy choices. That adaptation is not part of this patch. Manual
tremolo edits, multiple voices and cross-staff pairs need separate cases;
Studio uses additional branches for them. TAB remains deferred.

## Validation

XML parsing, measure sums, pair connections, written/actual duration matching,
all twelve direction combinations, pitch spellings and the balanced control
were checked locally. No application test or engraving code changes here.
The score has not been loaded or rendered in either application locally.
