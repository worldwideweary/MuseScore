# WIP checkpoint v0.23 — partial lyric dash count

Apply after v0.22. Checkpoint 1 of three separate requested fixes.

Studio 4.7.5 defaults to half-sized gaps at the ends of a dash span and rounds
the count up beyond the maximum dash distance. The backport already used
that distribution but rounded down, producing one dash in L03 measure 4.
The partial-line count now rounds up consistently with its distribution.

Validation before interruption: application built and L03 rendered/inspected;
162 repeat tests passed. Five raster-count cases cover the actual L03 first
ending, short spans, spans beyond one/two maximum distances, and zero length.
The code and tests were reconstructed from the recorded edits after the
temporary workspace reset; final series verification is recorded in v0.25.

Next: L04 final notehead edge (v0.24), then repeat-to-note spacing (v0.25).
37E-to-4.7.5 loading remains a separate user verification pass.
