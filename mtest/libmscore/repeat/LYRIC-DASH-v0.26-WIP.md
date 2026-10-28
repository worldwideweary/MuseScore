# WIP checkpoint v0.26 — Studio lyric-dash dimensions

Apply after v0.25. Lyric hyphens are painted horizontal strokes, not font
characters. Studio 4.7.5 defaults to 0.6sp maximum length and 0.1sp thickness;
3.x used 0.8sp and 0.15sp. The font and lyric font size are not the cause.

Adopt the Studio defaults for current-format/new scores and the v302 default
style. Explicit score/style values remain honored; pre-v302 legacy styles
retain their old default thickness. Remove the native-reader exception that
discarded an explicitly saved 0.1sp thickness.

Regression checks load both L02 versions, measure painted stroke dimensions,
then override both values and check custom settings through save/reopen.
Full series validation is recorded in the final checkpoint.

User confirmed the previous 37E files work in 4.7.5. That verification is no
longer deferred. The two later checkpoints address slurs and incoming ties.
