# 37E v0.32 WIP — S from the barline before a second ending

Apply after v0.31.

The single-barline S entry point was incorrectly restricted to START_REPEAT
and END_START_REPEAT barline types. The barline before a second ending is
usually END_REPEAT and belongs to the preceding measure. Resolve its tick
to the following measure, then use the existing incoming boundary eligibility
and voice selection. Ordinary barlines without an incoming repeat/jump
transition still do nothing. Require the following measure to start exactly
at the selected barline tick; tick2measure's end-of-score fallback is not an
incoming boundary.

Extend the real-command tests with X01 (grand staff) and S02 (single staff),
selecting the preceding END_REPEAT barline and pressing the S command.
Assert a new incoming one-note slur, initial edit mode, and creation undo/redo.
Controls cover ordinary barlines and the final barline of a last measure
that is itself a repeat destination.

Validation:
- mscore, tst_slurkeyboard, tst_repeat and tst_spanners build on Qt 5.15.13.
- All 11 keyboard command checks pass, including the two new second-ending
  cases, ordinary/final barline controls, initial edit mode and undo/redo.
- Repeat and spanner regression targets pass through CTest.
- git diff --check passes.

Qt 5.9 was not available here. The change uses the existing Qt/C++ APIs.
