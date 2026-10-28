# WIP checkpoint v0.28 — incoming tie spacing

Apply after v0.26 and v0.27 (the series begins on v0.25).

Studio 4.7.5 reserves horizontal room for hanging ties during spacing:
the longest incoming tie gets minHangingTieLength (default 1.5sp), and
shorter chord ties get minTieLength (default 1.0sp). Add the style value
and apply this reservation between start-repeat barlines and chords.
Reuse the existing tie landing/collision calculation with an unregistered
local TieSegment. Use the painted repeat-dot right edge for tie starts,
consistent with v0.25 barline spacing. Other repeat jump/header cases
retain their existing behavior. No new style-dialog field is added.

Validation of v0.26–v0.28:
- Full build of mscore, tst_repeat, tst_spanners and workspaces succeeds.
- Repeat suite: 176 passed, 0 failed. Spanners: 16 passed, 0 failed.
- New dash tests cover native/backport defaults, raster stroke dimensions,
  explicit custom values and save/reload.
- New one-note slur test checks displayed staff clearance and manual edits.
- Four incoming-tie rows cover X01/R05 at 1.5sp and 2.25sp hanging minimums,
  shorter chord ties, and style save/reload.
- L02/S03/X01 render successfully with both the backport and Studio 4.7.5.
  S03 SVG endpoints in both engines are 1.75sp and 1.77sp from staff top.
  Backport X01 incoming tie lengths are 1.15sp, 1.5sp and 1.5sp.
- git diff --check passes.

The user verified 37E files load in 4.7.5; that check is no longer deferred.
Keep the three patches separate for review and recovery.
