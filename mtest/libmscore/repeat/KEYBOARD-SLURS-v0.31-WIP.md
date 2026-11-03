# 37E v0.31 WIP — keyboard entry and repeat-boundary navigation

Apply after v0.30. This is a 37E keyboard workflow improvement.

Behavior:
- Select a start-repeat barline (mouse or Alt+Left/Right), then press S.
  Create an incoming one-note partial slur and enter editing on its end grip.
  Use the selected staff, preferring the current voice when available.
- Select the last note before a repeat/jump/skipped-volta boundary, then S.
  Create an outgoing one-note partial slur and enter editing.
- Shift+Right at an outgoing boundary moves to the next written note and
  clears the outgoing flag. Shift+Left returns to that boundary. Another
  contraction returns to the owning note if a nonzero slur remains.
- Start-grip navigation is symmetric. Valid repeat boundaries are stops
  between notes, rather than a partial flag following every moved note.
- An incoming slur can extend from its first note and contract back to it.
  Ordinary slurs cannot collapse to zero length or cross opposite endpoints.
- Eligible boundaries come from the existing notation repeat roadmap, also
  covering skipped endings and jumps. A plain system break does not itself
  create a repeat boundary. Ordinary note entry and list selection remain
  available; Shift+Alt+Left/Right retains the explicit partial toggle.

Implementation:
- Slur::boundaryAnchor shares boundary eligibility between S and editing,
  with the same onset/end conditions as partial-slur load validation.
- Endpoint navigation updates partial flags and note anchors in the same
  undo command. Existing changeAnchor segment handoff follows system breaks.
- Single-note partial creation enters the same editing path as regular S.

Validation:
- Application and all three test targets build with Qt 5.15.13.
- tst_repeat: 188 passed, 0 failed. New rows cover one-note contraction,
  system crossings, both grip directions, repeat and skipped-volta stops,
  endpoint ordering, undo/redo, and save/reload.
- tst_spanners: 16 passed, 0 failed.
- tst_slurkeyboard: 7 passed, 0 failed. Calls the real S command entry point
  for a selected start-repeat barline, last note before a repeat, last note
  before a skipped ending, ordinary-note control, and the existing two-item
  list selection. Checks initial editing and creation undo/redo.
- The new application test uses temporary settings and copies the built
  workspaces; its target depends on workspaces and needs no installed score
  application. CTest registers tst_slurkeyboard.
- git diff --check passes.

Build only the affected targets if desired:
    cmake --build BUILD_DIR --target tst_repeat tst_spanners tst_slurkeyboard
    ctest --test-dir BUILD_DIR/mtest -R '^tst_(repeat|spanners|slurkeyboard)$' --output-on-failure

Qt 5.9 was not available here; the implementation uses the existing C++11
and Qt APIs. No Studio 4.7 code changes are part of this patch.
