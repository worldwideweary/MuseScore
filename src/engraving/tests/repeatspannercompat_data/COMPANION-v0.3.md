# Studio 4.7 import companion v0.3 — partial-tie grip offsets

Apply this incremental git-format patch with `git am` after companion v0.2
in your MuseScore Studio 4.7 checkout. It is separate from the 37E v0.16 patch.

Diagnosis: the supplied testR04-475-37E.mscz contains a valid
`PartialTie/TieSegment/o1` offset of x=-2.5, y=0 spatium. read400 discarded
all four grip tags whenever the score version was below 4.00. The offset was
saved correctly by 37E, then deliberately skipped during legacy import.

Completed:
- Exempt PartialTieSegment from that pre-4.0 offset reset, using the segment's
  type because it is not yet attached to its owning spanner during reading.
- Preserve o1/o2/o3/o4 through the existing native unit conversion.
- Retain upstream's reset policy for ordinary legacy slurs and ties.
- Expand evolution302 regression coverage to incoming and outgoing partial
  ties with four distinct grip offsets, including R04's -2.5sp start edit.
  Check offsets after import and native save/reopen, and verify the normal
  legacy tie's offset still resets.

Files changed: read400/tread.cpp; repeatspannercompat_tests.cpp;
evolution-repeat-spanners.mscx; this checkpoint.

Validation on 2026-11-02:
- Source/fixture consistency and git diff --check passed.
- Official 4.7.5 native-path probe: a temporary copy of R04 and the regression
  fixture were declared 4.70 to bypass legacy migration, saved as native MSCZ,
  reopened and exported again. R04 retained o1=(-2.5,0); both fixture partial
  ties retained all four x/y offsets. The edited R04 was rendered and inspected.
- This probe confirms native grip compatibility; it does not execute the
  modified pre-4.0 import branch.

Outstanding: the modified 4.7 code and GoogleTest have not been compiled/run
here; this environment lacks the Qt 6.8 development SDK required by 4.7.
After building your patched 4.7, run:
  engraving_tests --gtest_filter=Engraving_RepeatSpannerCompatTests.evolution302
Then open the original, unmodified 3.02 R04 file, verify the stretched incoming
tie, save as native MSCZ and reopen. Do not change the score's version header
for the real test. Also test the remaining grip handles and Inspector reset.
