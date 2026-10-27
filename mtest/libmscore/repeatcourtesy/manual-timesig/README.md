# Time-signature drawing comparison scores — v0.40

Apply the format patch once in the 37E checkout, then open these same files in
both 37E and Studio 4.7.5. No rebuild is needed for these added score files.
This set does not change engine code or register additional automated tests.

Start with the untouched files. Use separate copies for save/reopen checks;
the originals contain no element position overrides or saved courtesy offsets.
The scores use the 4.50 XML format already supported by the backport.

## Shared material

Each score has two piano parts, four staves, eight measures, and a line break
after measure 4. Above/across placement uses display staves 1 and 3: staves
1–2 form one group and staves 3–4 form another. Both builds should use Leland.

The sequence is 4/4, 3/4, 7/8, common time, 5/8, cut time, additive (3+2)/8,
and 3/8. The underlying additive meter is 5/8. The key changes from one flat
to two sharps at measure 5. A note followed by rests fills every measure.

The change to 5/8 should also appear as a courtesy at the end of the first
system. This is the main place to compare end-of-system margin policies.
Landscape A4 page dimensions and staff distances are supplied explicitly.
Automatic horizontal justification can differ between engines; compare the
meters against their nearby staff lines, key signatures, and barlines.

## Files

| Score | Main comparison |
| --- | --- |
| `TS-01-normal-standard.mscx` | Normal placement, standard glyphs, 1 × 1 scale. Baseline. |
| `TS-02-normal-narrow.mscx` | Normal placement, narrow glyphs, 1 × 1 scale. |
| `TS-03-normal-large-scaled.mscx` | Large glyphs, 1.2 × 1.5 scale, 0.35sp numeral distance, Y = −0.25sp. |
| `TS-04-above-hang-centered.mscx` | Above staves, narrow glyphs, 2 × 2 scale; centered on barlines; courtesy hangs into the right margin. |
| `TS-05-above-inset-centered.mscx` | Above staves, standard glyphs, 2 × 2 scale; the courtesy's right edge aligns to the final barline. |
| `TS-06-above-create-space-left.mscx` | Above staves, large glyphs, 2 × 2.2 scale; left alignment, create-space margin policy, 0.3sp numeral distance, Y = +0.25sp. |
| `TS-07-across-centered-groups.mscx` | Across staves, narrow glyphs, 4 × 4 scale; group centering enabled for both two-staff groups. |
| `TS-08-across-offset-groups.mscx` | Across staves, large glyphs, 2.5 × 3 scale; group centering disabled, 0.25sp numeral distance, Y = −0.5sp. |

In score 03, measure 7 additionally has an explicit per-element scale of
0.9 × 1.1 on every staff. It should override the score-wide normal scale.
The scale lock is off in scores 03, 06, and 08; its effect is checked while
editing width/height, not by opening the score alone.

## Quick pass

1. Compare scores 01–03 for numeric, common/cut, and additive glyphs, scale,
   numeral separation, vertical offsets, and the measure-7 scale override.
2. Compare scores 04–06 at the final barline of the first system. Confirm
   hanging, right-edge alignment, and reserved space respectively. Above-staff
   meters should appear on staves 1 and 3, not all four staves.
3. In score 06, inspect the lowered meter where it overlaps a vertical barline.
   The barline should clear the meter. Horizontal staff lines remain visible.
4. Compare scores 07–08 for display groups, group-centering behavior, scale,
   and the explicit vertical offset. Compare placement relative to both staves
   in each group, not just the visible glyph's bounding-box midpoint.
5. On copies, change placement/glyph/scale settings live, undo, save, close,
   and reopen. Also save a copy in 37E and open that saved copy in 4.7.5.
6. Optionally disable the second display staff and restore it: the second
   group's large meter should disappear and return without changing the meter
   or rhythm of the underlying staves.

XML structure and all 256 staff-measure durations were checked when generating
this set. Full rendering and save/reopen behavior are the manual checks above.
These comparison scores were newly composed for this set (CC0-1.0).
