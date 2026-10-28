# v0.30 WIP — incoming tie spacing at all boundaries

Apply after v0.29 (v0.29 follows v0.28).

Move the v0.28 minimum-length reservation out of the start-repeat-only
segment case into measure minimum-width layout. Every visible incoming
partial tie can request room. This covers alternate endings after an
end-repeat barline, ordinary barlines, system headers and start repeats.
Keep the 1.5sp hanging minimum for the longest tie and the ordinary tie
minimum for shorter chord ties. Hidden notes/staves and tablature with
back ties suppressed do not request room.

Factor the free endpoint into Tie::incomingStartOffset(), shared by drawing
and spacing. It preserves the existing barline/header margins without
clamping to the chord. For the preceding measure, work relative to its right
edge, using its actual trailing shape. Measure width already contains
rhythmic stretch before the segment positions are stretched, so subtracting
that width would under-reserve room after ordinary/end-repeat barlines.
The note landing still uses the v0.28 collision calculation. Outgoing ties
retain their behavior.

Regression coverage extends incomingTieMinimumSpace to all incoming ties
in X01/R05, including X01's second ending, and variants after ordinary
barlines and at system headers. Test both 1.5sp and 2.25sp hanging minima.

Validation on the final tree:
- mscore, tst_repeat, tst_spanners, tst_mxml_io and workspaces build.
- Repeat: 184 passed, 0 failed. Spanners: 16 passed, 0 failed.
- MusicXML: 313 passed, 9 skipped, 0 failed (all 14 reported failures fixed).
- X01 PNG/SVG exports succeed. Second-ending incoming ties measure
  1.15sp, 1.5sp and 1.5sp in the SVG; the start-repeat ties retain those lengths.
- git diff --check passes. Manual endpoint offsets are retained.

The existing test skips were not changed. The full combined custom fork's
other test suites remain for the user's normal integration run.
