# Repeat-spanner checkpoint v0.19 — native Studio lyric-line import

Apply with git am after repeat-spanner v0.18. Keep Studio companion v0.3.

Diagnosis: the user confirmed X01-475 / a Studio-saved score was being opened
in 37E. X01-37E already displayed all three extenders. Native Studio stores
incoming lines as PartialLyricsLine connector records, and outgoing repeat
melismas as ordinary Lyrics (often with a one-tick temporary marker). The
backport previously handled only its own lyric-owner representation.

Completed:
- Defer native PartialLyricsLine records during ordinary connector reading.
  The caller skips each complete record once, preserving the surrounding score.
- After a successful native 4.x file load, read the original XML metadata and
  convert incoming spans to existing 37E lyric owners. The XML is not rewritten.
- Resolve locations using the score's actual chord/rest times, verse and
  placement. Convert native exclusive melisma/next-syllable ends to the 37E
  inclusive final-chord duration, including both-ended repeat lines.
- Convert outgoing melismas/dashes at repeat, jump and volta boundaries;
  preserve the temporary marker on ordinary lyrics without a repeat boundary.
- Handle both MSCX and MSCZ input through the existing compatibility load path.
- Use the v0.18 header/barline clearance helpers for partial lyric lines and
  stop dashes before an immediately following syllable.
- Save the converted data with the existing 37E properties for normal reload
  and the existing Studio companion. No new score property IDs are introduced.

Files: new libmscore/lyricscompat.cpp/.h, libmscore/CMakeLists.txt,
connector.cpp, scorefile.cpp, lyricsline.cpp; tst_repeat.cpp; twelve lyric
fixtures (37E/native MSCX and two native MSCZ containers); this checkpoint.

Validation (Qt 5.15.13, Debug):
- Application, tst_repeat and tst_spanners built successfully.
- Complete repeat suite: 117 passed, 0 failed. Spanners: 16 passed, 0 failed.
- L01-L04 and X01 tested in both representations. Native MSCZ fixtures include
  separate score_style.mss files, with above-staff L04 and grand-staff X01.
- Compared measures, note/rest onsets, durations and pitches to the paired
  37E scores. Compared converted lyric track, verse, placement, direction,
  melisma/dash type, span and text before and after 37E save/reopen.
- Checked system rendering enumeration, positive spans and actual painted ink
  for every partial lyric segment, rather than only its bounding rectangle.
- Rendered and inspected all five native reference cases, including all pages.
  Native X01 now shows the outgoing hold line and both incoming extenders.
  Before this fix, native X01 exported zero partial lyric owners; now it has 3.
- The accepted tie cases and v0.18 slur regressions remain passing.

Scope: chord/rest-anchored master-score lines on one track. Studio line-specific
segment edits and cross-track partial lyric spans are not mapped here.

Next: retest S01-S04 and native X01 in the assembled build, then continue the
L01-L04 comparison and Inspector checks.
