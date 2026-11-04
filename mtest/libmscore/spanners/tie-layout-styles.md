# Slur/tie properties: 37E-v0.11

This patch is rebased onto worldwideweary/MuseScore branch
`3.7-Evolution/backport/slurTieProperties`, commit
`ae8ce1fe85b01ef435fc123f4c98c5bf2c692c95`. Apply it directly to that
clean commit, replacing the previous properties patch rather than stacking it.
The supplied original diff includes notation follow-ups through v0.10; they
are included here. Audio-engine/playback patches are a separate series.

The existing repeat backport remains authoritative for partial slurs:
Normal=-1, Incoming=0, Outgoing=1, Incoming and outgoing=2. The Inspector
uses explicit combo-box item data, and XML continues to use the words
`incoming`, `outgoing`, and `both`. Existing repeat destinations, partial
lyrics, partial-tie ownership, partial-slur anchor extension, chord-symbol
styles, and time-signature styles are retained.

The combined Tie reader accepts partial direction and placement together.
Partial ties use the new placement setting through the existing repeat anchors.
The baseline's minimum hanging length, shared spacing endpoints and shorter
inner chord ties are preserved. Its layoutPartial, incomingStartOffset,
incomingEndOffset and calculateDirection implementations are retained.
The repeat-destination panel is added after the other Inspector rows to avoid
overlapping the new controls.

Rebuild and run `tst_element`, `tst_spanners`, `tst_repeat` and
`tst_repeatcourtesy`:

```bash
ctest -j1 --output-on-failure -R '^(tst_element|tst_spanners|tst_repeat|tst_repeatcourtesy)$'
```

Existing `testPartialSlur` and `testPartialLyrics` remain intact. Added tests
cover segment property delegation with the established numbering, partial-tie
placement import, tie placement, release-mark ownership, style import, and the
full test-score save/reload. Build and mtests were not run in the patch-preparation
environment because Qt development tools are unavailable.

For visual testing, open `tie-layout-styles.mscx` in this directory. Its new
options are already enabled. Check score-wide styles, Inspector overrides and
reset, repeat destinations, partial slurs between voltas, and save/reload. The
initial fixture from v0.04 still applies: its enum values are serialized as
words, not numeric ordinals. Native 4.7 comparison scores and the reported C/D/LV
visual discrepancies are the next verification stage after this rebase.

## 37E-v0.06: curved laissez-vibrer profile

The initial release-mark drawing used a 1.0 sp shoulder at a 3.0 sp length
and control points at 35%/65% of the length. Studio 4.7's normal tie profile
uses a shoulder of approximately 0.724264 sp at that length, with control
points at 20%/80%. The release-mark drawing now uses that profile, Studio's
short-tie thickness interpolation, and the styled end-width stroke. Ordinary
ties, articulation symbols, and release-mark placement are not changed by
this correction. Studio's later collision adjustments can still affect the
final appearance in a chord.

The `testLaissezVibCurveProfile` element test checks the unadjusted curve bounds
at 2, 3, and 4 sp in both directions. The patch also adds the required
`notedot.h` include to `chord.cpp`. If that include is already staged or
committed locally, apply this increment with `git apply --3way --index` so
Git can retain the identical change. Rebuild and run the same element and
spanner tests, then compare the 3.0 sp release marks visually with Studio.

## 37E-v0.07: apply l.v. to selected tie-chain ends

Applying the l.v. palette element now visits `Selection::noteList()` instead
of the generic palette range loop, which stops after one note per chord.
Every selected chord note is considered, with outgoing normal ties and
outgoing repeat partial ties skipped. An incoming tie does not exclude a
note. A chain that continues beyond the selected range is still skipped;
its unselected destination is not modified.

Like Studio's command, applying l.v. again toggles existing release marks off.
The range remains selected, and one undo restores the whole operation. Direct
drops also reject outgoing-tied notes and duplicate release marks. The palette
prototype's custom properties are retained for marks added to eligible notes.

`laissez-vibrer-range.mscx` reproduces the reported chord/rest/tied-pair example.
`TestSpanners::laissezVibRangeSelection` checks all notes of the chord, only the
last tied note, range preservation, undo/redo, repeated application, ties that
leave the range, and incoming/outgoing repeat partial ties. Run the usual
`tst_spanners` test, then select the fixture's measure and apply l.v. visually.
The expected result is three release marks on the first chord and one on the
last note. Build and mtests were not run in the patch-preparation environment.

## 37E-v0.09: linked staves and TAB fret anchors

Laissez-vibrer marks now use the existing linked-note attachment path in
`Score::undoAddElement`, so adding or removing a mark on either notation or
linked TAB updates its partner. The palette command processes each linked-note
group once, including when a range contains both staves. Direct drops use the
same linked addition path.

TAB anchoring now uses the right edge of the note's rendered fret-string
bounding box, with 0.2 sp padding. It does not use music-font notehead width or
the outside-tie midpoint overlap. This covers multi-digit frets and different
fret-font sizes; a negative SMuFL glyph bearing is also accounted for. Pitched
note anchors and curve shapes are unchanged.

`TestSpanners::laissezVibLinkedTab` checks linked additions/removals from either
staff, ranges covering both, undo/redo, fret 12 with Auto/Inside/Outside and
curved/SMuFL rendering, and increased fret-font size. Build and mtests remain
unrun in the patch-preparation environment. Run `tst_spanners`, then repeat
the notation/TAB visual comparison. Existing independently added marks are not
retroactively linked; remove the manually added marks from both staves before
checking fresh linked application.

## 37E-v0.10: replace the legacy LV palette item

The Articulations palette and bundled Advanced workspace now offer
**Laissez vibrer (l.v.)**, the functional release mark. Saved workspace and
palette cells containing either old LV articulation glyph are upgraded when
loaded. Cells from earlier versions of this backport get the searchable name
as well. This changes palettes only; old articulations already in scores are
preserved. The SMuFL glyphs remain available for rendering the new mark.

Manual verification after rebuilding:

1. Open Advanced and search for `l.v.` and `Laissez vibrer`. Apply the result
   to the range fixture: expect three marks on the first chord, none on the
   outgoing-tied note, and one on the last note.
2. Load an existing workspace or custom palette with an old above/below LV
   articulation cell. Its item should now show the new name and create a
   functional release mark with the LV inspector properties.
3. Restart with the saved workspace and repeat the search. Also open a score
   containing an old LV articulation: that score element must stay intact.

XML parsing, source checks, whitespace checks, and incremental patch
application were verified. Compilation and GUI checks were not run in the
patch-preparation environment.
