# Repeat-spanner checkpoint v0.17 — chord and voice placement

Apply the incremental 37E-repeat-tie-chord-placement-v0.17-format.patch with
`git am` after repeat-spanner v0.16. The Studio 4.7 companion remains v0.3.

Cause of R05's crossing incoming ties:
- layoutPartial chose the opposite stem direction for single notes, even in
  multiple voices, and used a separate top-note comparison for chords.
- It used the notehead center and a fixed vertical offset for every partial
  tie, bypassing the ordinary inside/outside attachment calculation.
- It skipped the ordinary horizontal stem, ledger-line and note clearance.

Completed:
- Partial ties use Tie::calculateDirection, including the multiple-voice
  stem-side rule and explicit up/down overrides.
- Tie::slurPos supplies both candidate anchors locally from a partial tie's
  owning note. Incoming ties select the end anchor; outgoing ties select the
  start anchor. Inside/outside placement follows the ordinary tie calculation.
- TieSegment::adjustX maps an incoming partial's owner to the end role and
  adjusts only the note-owned endpoint. The free repeat/header endpoint keeps
  the v0.16 clearance policy.
- Existing grip offsets remain additive; their save/reopen regression passes.

Files changed:
- libmscore/tie.cpp
- mtest/libmscore/repeat/tst_repeat.cpp
- this checkpoint

Validation on 2026-11-02 (Qt 5.15.13, Debug):
- Built application, tst_repeat and tst_spanners.
- Repeat suite: 99 passed, 0 failed. Six R05 parameter cases add normal/12sp
  leading space and automatic/up/down direction. Each checks all seven notes
  in both incoming/outgoing roles before and after save/reopen.
- Those six regression cases all fail against the v0.16 production source.
  Restoring v0.17 makes the complete repeat suite pass again.
- Spanner suite: 16 passed, 0 failed.
- Rendered and inspected R01-R05, R01 with a repeat at a system start, and
  R05 with 12sp extra leading space. R05's upper voices curve up and its second
  voices curve down, matching the corresponding ordinary tie landings.
- Automatic R05 note-owned x/y positions are checked against ordinary ties;
  forced directions also check x anchoring and the side of each note. Forced
  curves can use different staff-line avoidance as curve length changes.

Outstanding/next: retest R05 in the user's assembled build, including increased
leading space, direction overrides, Inspector placement options from any
separate tie-layout backport, and save/reopen. Then continue the remaining
slur/lyric comparison cases. No new serialized properties or importer changes
are needed for this layout fix.
