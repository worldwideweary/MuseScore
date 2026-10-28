# Repeat-spanner checkpoint v0.20 — floating slur heights

Apply with git am after v0.19. This patch changes partial-slur geometry only.

The v0.18 clearance fix supplied the correct horizontal free-end bounds but
left the ordinary slur attachment height in place. For stem-side slurs that
height sits a quarter-spatium inside the stem. Studio 4.7.5 instead uses the
first/last chord outline at the measure boundary, ignoring accidentals, with
0.4sp clearance on the notehead side. This accounts for the slight slope at
repeat dots in S01 and S03.

The backport now uses that boundary outline, the native one-note 0.25sp rule,
and the native 2.5sp limit on an outgoing end falling toward the staff. Its
horizontal-only lyric/leading-space shape entries are excluded from vertical
bounds. Chord-owned anchors and manual grip offsets retain their existing
layout paths; ordinary slurs and ties are unchanged.

Validation (Qt 5.15.13, Debug):
- Repeat suite: 141 passed, 0 failed.
- Added 24 S01–S04 height rows: three staff sizes, original and forced-down
  directions, including one-note and both-ended fragments.
- All 24 new rows fail with v0.19's slur.cpp and pass with the fix.
- Built the application; rendered and inspected S01–S04 and X01, including
  both pages where applicable. Compared against the official 4.7.5 runtime
  and the matching 4.7.5 source (3654226c).

Separate finding for the next checkpoint: S02's first ending is marked
incoming in both original comparison files. Studio 4.7.5 clears that flag on
load because there is no discontinuous repeat/jump arrival at the first
ending. Its saved XML confirms D is ordinary and E is incoming. The backport
has not yet performed this validity check; it is separate from these heights.
