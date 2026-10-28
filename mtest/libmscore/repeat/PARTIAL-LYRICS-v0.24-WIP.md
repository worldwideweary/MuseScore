# WIP checkpoint v0.24 — partial melisma final notehead edge

Apply after v0.23. Checkpoint 2 of three separate requested fixes.

L04's continued melisma used the overall chord width, ending too early.
Studio 4.7.5 selects the bottom note for up stems or top note for down stems
and ends at that note's x-position plus its head width. The backport now
uses the same rule for a partial line's note-owned final endpoint.

Validation before interruption: application built; L04 page 2 rendered and
inspected with L03; 164 repeat tests passed. New endpoint checks cover L04
across system/page breaks and combined X01 in page coordinates. The restored
series is reverified in the v0.25 checkpoint after workspace recovery.
Outgoing lines and endpoints before the next lyric retain their own rules.

Next: normal bar-to-note clearance after start-repeat dots (v0.25).
37E-to-4.7.5 loading remains a separate user verification pass.
