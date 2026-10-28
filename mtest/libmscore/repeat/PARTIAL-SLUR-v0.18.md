# Repeat-spanner checkpoint v0.18 — partial-slur clearances

Apply with git am after repeat-spanner v0.17.

Completed:
- Incoming slurs start after the measure's visible header/start-repeat shape,
  using Studio 4.7's default clearance and first-note cap.
- Outgoing slurs stop before the end-barline segment, including repeat dots.
- One-note partial slurs use the 4.7 quarter-spatium free-end vertical offset.
- The incoming S02 first-ending curve no longer crosses the clef/header. This
  removes the collision that inflated and lifted the curve in the backport.
- Added measure-level endpoint helpers, shared with the forthcoming lyric fix.

Files changed: libmscore/measure.h, measure.cpp, slur.cpp; tst_repeat.cpp;
four partial-slur-S*.mscx fixtures; this checkpoint.

Validation (Qt 5.15.13, Debug):
- Repeat suite: 104 passed, 0 failed; spanner suite: 16 passed, 0 failed.
- The four new S01-S04 clearance rows and S02 header-shape regression all fail
  with v0.17's slur.cpp and pass with the fix restored.
- Rendered S01-S04 and X01 through the built application and inspected both
  pages where applicable. S02's first-ending shape matches the ordinary-slur
  control rather than ballooning above the clef.
- The accepted 99 tie/repeat tests remain passing.

Next/outstanding: native 4.7 lyric-line import is a separate v0.19 checkpoint.
The X01-37E fixture already renders its three extenders; the user confirmed
that the missing lines occur with X01-475 / a native 4.7 save. Conversion of
PartialLyricsLine records and outgoing repeat melisma markers is required.
