# Repeat-spanner checkpoint v0.22 — incoming repeat-dot clearance

Apply with git am after v0.21.

Incoming slurs could nearly touch start-repeat dots while outgoing ends had
the intended gap. Two details contributed: 3.x's barline bounding box uses
an unscaled dot width, and the open-line start was capped at the first note's
x-position. That cap could consume the clearance even at the default size.

The shared open-line start helper now follows BarLine::draw's dot position
and scaled glyph bounds for start repeats, including repeat tips where
enabled. It retains the requested 0.25sp gap instead of shortening it at the
first-note cap. Other headers retain their existing cap and clearance.
This also corrects incoming lyric-line clearance at these repeat headers.
General barline layout widths and outgoing endpoint calculations are not
changed by this patch.

Validation (Qt 5.15.13, Debug):
- Repeat suite: 157 passed, 0 failed; spanner suite: 16 passed, 0 failed.
- Eight new raster checks measure the painted barline rather than its layout
  box: four score sizes, each with normal and 0.7 staff magnification. They
  check both incoming S03 fragments and positive one-note slur length.
- Six of those eight rows fail with v0.21's measure.cpp; all eight pass with
  the corrected helper.
- Rendered and inspected S03. SVG coordinates confirm the incoming gap is
  6.25 units for a 25.002-unit staff space (0.25sp), previously about 0.094sp.
  The outgoing-only S03 segment's SVG path is unchanged.
