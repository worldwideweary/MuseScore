# Repeat-spanner checkpoint v0.21 — slur boundary validation and one-note saves

Apply with git am after v0.20.

S02's first-ending difference is a load-time validity rule in Studio 4.7.5,
not a continuation segment connecting the two systems. The original paired
files mark D incoming, but only the second ending E has a discontinuous jump
arrival. The official runtime's saved XML drops D's incoming property.

Changes:
- On load, check partial-slur anchors against the existing notation repeat
  routes. Clear an incoming/outgoing side when its anchor has no appropriate
  jump arrival/departure; keep the other valid side of BOTH.
- Remove an invalid one-note fragment rather than create a zero-length
  ordinary slur, matching Studio's rule.
- Resolve volta measure anchors before querying those routes. Before the
  initial layout, unresolved voltas would incorrectly reject A and E too.
- Write both connectors for a valid one-note partial slur. The previous
  start/else-end writer emitted only the start, losing the slur on reopening.
- Correct the S02 comparison instructions to describe the actual load result:
  A outgoing, D ordinary, E incoming. Retain the stale D flag in the original
  paired input files as a normalization control.

Validation (Qt 5.15.13, Debug):
- Repeat suite: 149 passed, 0 failed; spanner suite: 16 passed, 0 failed.
- Added S02 backport/native import rows, BOTH-side normalization, and all four
  manual grip offsets through save/reopen.
- Added four invalid one-note boundary cases and two valid round-trip
  controls, including the dashed outgoing fragment. The existing S02 header
  test also verifies D's load-time normalization before exercising a manually
  enabled incoming flag.
- The original six boundary cases plus the updated header check fail with
  v0.20. The one-note writer omission was confirmed in the saved XML: the
  corresponding same-chord end connector was absent.
- All 32 paired comparison files pass structural verification.
- Built the application; rendered S01–S04 and X01, imported native S02, and
  saved/reopened S03. S02 saved from either input now has the same directions
  as the official 4.7.5 save; valid S03 fragments survive reopening.

This checkpoint changes loading/saving behavior. It does not add automatic
slur branching when entering notes or changing the repeat structure.
