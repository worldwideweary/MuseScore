# Repeat-spanner checkpoint v0.16 — partial-tie endpoints

Apply the accompanying incremental git-format patch with `git am` to the
37E repeat-spanner v0.14 engine checkpoint (the v0.15.1 score-suite patch is
independent). This does not target stock 3.x without that backport.

Completed:
- Incoming partial ties clear start-repeat dots by the Studio 4.7 default
  0.25 spatium; after other barlines the clearance is 0.65 spatium.
- Outgoing partial ties stop 0.25 spatium before the adjacent item shape,
  including the leftmost repeat dots.
- System-start incoming ties use the header endpoint, with Studio's separate
  0.25-spatium rule for a system-start repeat.
- Manual offsets remain additive to the new default endpoints. No normal-tie
  layout path was changed; R05's ordinary outgoing ties retain that path.

Files changed: libmscore/tie.cpp; tst_repeat.cpp; six partial-tie-R*.mscx
fixtures in this directory; this checkpoint.

Validation on 2026-11-02:
- Debug application and tst_repeat/tst_spanners targets built with Qt 5.15.13.
- tst_repeat: 93 passed, 0 failed (six clearance cases and one offset case added).
- tst_spanners: 16 passed, 0 failed.
- All six fixtures exported to PNG through the built 37E application and
  visually inspected, including repeat dots, headers, chords and two staves.
- Offset test covers incoming and outgoing ties, all four grips, relayout and
  score save/reopen. The start offset reproduces the supplied R04 -2.5sp edit.

Limit: this matches Studio's default horizontal clearances, not its entire
engraving engine or custom Studio clearance styles. Existing note anchors and
vertical curve placement remain the 37E implementation.

Next: the separate Studio 4.7 companion v0.3 preserves pre-4.0 PartialTie grip
offsets during import. Rebuild each edition and compare the existing R01-R05
suite, then reopen the supplied edited R04 in the patched 4.7 application.
