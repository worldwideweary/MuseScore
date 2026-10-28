# WIP checkpoint v0.25 — note spacing after start-repeat dots

Apply after v0.23 and v0.24 (both apply incrementally after v0.22).
This is the third of the user's three separate WIP checkpoints.

Studio 4.7.5 treats barlines as NON_KERNING against notes and applies the
bar-to-note padding from the right edge. The legacy 3.x conditional addition
of repeat-barline width could omit that clearance, especially for notes below
the staff. In S01 this left only about 0.093sp after the painted repeat dots.

Changes:
- Use the painted start-repeat right edge as a minimum spacing boundary for
  following visible noteheads, with the existing barNoteDistance style value.
- Respect the separate barAccidentalDistance for preceding accidentals.
- Share the scaled repeat-dot/tip geometry from v0.22 between line placement
  and note spacing through the new barlinegeometry.h utility. Drawing and
  general barline layout widths are unchanged.

The lower bound is scoped to start-repeat-to-chord spacing. Rest spacing and
other segment pairs retain the existing implementation. Existing collision
requirements can still demand a larger gap. This is not a wholesale port of
Studio's spacing engine or a change to the user's spacing style values.

Validation after reconstruction (Qt 5.15.13, Debug):
- Application rebuilt; 169 repeat tests and 16 spanner tests pass.
- Five spacing rows cover C3 below, C4 ledger, C5 on staff, C6 above, and C#4
  with an accidental. They measure painted ink, not the barline layout box.
- Restoring the previous lyric/spacing algorithms produces seven focused
  failures: three dash counts, two melisma endpoints, and two repeat gaps.
  The final source was restored, rebuilt, and the tests passed again.
- Rendered L03, L04 page 2, and S01 alongside official 4.7.5 reference files.
- S01 SVG gap from dot RHS to first notehead is 1.2999sp in 37E, matching its
  default 1.30sp style. Native 4.7.5 is 1.2497sp with its 1.25sp default. To
  compare exact gap sizes, use the same bar-to-note style value in both.
- v0.23 and v0.24 reconstruct the previously tested lyric changes, each in
  its own git-format patch. This checkpoint records verification of the full
  restored series after the temporary workspace reset.

Resume/user verification:
1. Apply v0.23, v0.24, v0.25 in order after v0.22; each includes its new files.
2. Rebuild and compare L03 measure 4, L04 measure 3, and S01 repeat spacing.
3. Loading 37E saves into 4.7.5 remains a separate, explicitly deferred pass.
