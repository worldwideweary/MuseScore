# Repeat-spanner comparison scores v0.15

Patch packaging revision: **v0.15.1**. The fixture directory remains v0.15.

This is an additive visual/behavior test suite for MuseScore 3.7-Evolution
with the repeat-spanner backport and MuseScore Studio 4.7.x. It installs no
application code and changes no CMake test registration.

## Open the right member of each pair

- Open `ID-37E.mscx` in your current **37E** build (repeat-spanner v0.14 or
  the equivalent merged changes).
- Open `ID-475.mscx` in **Studio 4.7 / 4.7.5**. These are native 4.70 files;
  the stock 4.7.5 reference requires no unofficial importer.
- To test **migration**, separately open the **37E** member in your 4.7 build
  with the **repeat-spanner import companion v0.2**. Compare it against the
  native 475 member, then save a new `.mscz` copy and reopen it.
- Use `.mscz` for saved comparison copies so Studio retains its separate
  score-style data. Do not overwrite the pristine fixtures.
- Do not use a 475 file in 37E as the principal test: native
  `PartialLyricsLine` and the legacy lyric-owner representation differ.

All measures are 4/4, with quarter notes at 96 BPM. Repeated C notes are
intentional: they make missing reattacks and unexpectedly broken ties audible.
R05 uses distinct pitches for each voice/chord tone. A4 paper, Bravura,
FreeSerif text, and explicit system/page breaks make comparisons manageable.
Exact horizontal spacing and skyline choices may differ between engines;
compare the note anchors, continuation edges, verse baselines, and routes.

## Suggested order

1. **R01, R02:** basic ties, alternate endings, and Inspector destination
   enable/disable.
2. **R03, R04:** D.C., D.S., Fine, and Coda routing.
3. **R05, R06:** chords, voices, two staves, disabled endpoints, rests, and
   mismatched pitches.
4. **S01-S04:** explicit incoming/outgoing/both slurs, single-note anchors,
   and system/page breaks.
5. **L01-L04:** extensions, dashes, multiple verses, score-wide Above
   placement, and long continuations.
6. **X01:** all three families together on a grand staff.
7. **E01:** create the objects yourself from the clean entry laboratory.

## Case index and expected playback order

Numbers refer to written measure numbers, starting at 1. Beat labels in the
checklist are also one-based. The expected order below is for **Play Repeats
enabled**. With it disabled, the written measures should play in ascending
order; incoming partial ties must not silence an unrelated first attack.

| ID | Focus | Written bars | Play Repeats order |
| --- | --- | ---: | --- |
| R01 | Ties: ordinary repeat and linear continuation | 6 | 1 2 3 4 2 3 4 5 6 |
| R02 | Ties: first and second endings | 6 | 1 2 3 4 2 3 5 6 |
| R03 | Ties: D.C. al Fine | 6 | 1 2 3 4 5 6 1 2 3 |
| R04 | Ties: D.S. al Coda | 8 | 1 2 3 4 5 6 2 3 4 7 8 |
| R05 | Ties: chords, voices, and two staves | 6 | 1 2 3 4 2 3 4 5 6 |
| R06 | Ties: disabled, rest, and pitch controls | 12 | 1 2 3 2 3 4 5 6 7 6 7 8 9 10 11 10 11 12 |
| S01 | Slurs: incoming and outgoing at a repeat | 6 | 1 2 3 4 2 3 4 5 6 |
| S02 | Slurs: alternate endings | 6 | 1 2 3 4 2 3 5 6 |
| S03 | Slurs: one-note anchors and both ends | 6 | 1 2 2 3 4 4 5 6 |
| S04 | Slurs: system and page breaks | 6 | 1 2 3 4 2 3 4 5 6 |
| L01 | Lyrics: melisma across a repeat | 6 | 1 2 3 4 2 3 4 5 6 |
| L02 | Lyrics: dashed continuation across a repeat | 6 | 1 2 3 4 2 3 4 5 6 |
| L03 | Lyrics: two verses and voltas | 6 | 1 2 3 4 2 3 5 6 |
| L04 | Lyrics: above staff, page break, and both ends | 8 | 1 2 3 4 5 2 3 4 5 6 7 7 8 |
| X01 | Combined: grand staff, endings, ties, slurs, lyrics | 8 | 1 2 3 4 5 2 3 4 6 7 8 |
| E01 | Entry lab: create ties, slurs, and lyric continuations | 6 | 1 2 3 4 2 3 5 6 |

## Repeatable comparison procedure

For each ID:

1. Load the 37E and 475 members side by side at the same zoom. Locate the
   letter labels on the score. Check the first system, interior barlines,
   second-ending entry, system headers, and page-break continuations.
2. Play from the beginning with repeats on, following the printed measure
   numbers. Ties sustain the matching note only on a pass which arrives from
   their source. Slurs and lyric lines are explicit notation; lyrics do not
   synthesize sung words.
3. Repeat with Play Repeats off. Do not judge the printed partial shapes by
   their presence alone; judge whether an unrelated attack is silenced.
4. Select each source tie and change one repeat destination. Check the
   incoming marker, then Undo, Redo, save an edited copy, close, and reopen.
   Repeat with a normal tie plus branch and an outgoing-only tie.
5. Select slur fragments separately. Test direction/line style and their
   partial direction. Move a grip, reset it, Undo, and reopen. In 37E, the
   Partial slur control and Shift+Alt+Left/Right endpoint editing are useful
   checks. Studio's interface need not use the same wording.
6. Select a lyric continuation. Test verse, placement, extension versus
   dashes, Delete, and Undo. Deleting a line attached to a word should retain
   the word; undoing deletion of an empty incoming owner should restore the
   line. Check edited copies after reopening.
7. For tie sources, remove the source tie or source chord, then Undo.
   Dependent incoming markers should not remain orphaned.
8. In the companion-patched 4.7 build, repeat steps 1-6 after importing the
   37E member. Save as native Studio `.mscz`, close, and reopen. Compare
   against the native 475 reference.

Record results in `RESULTS.csv`. Prefer a small edited score plus the ID,
application/version, measure/beat, and exact action when reporting a difference.

## E01 entry laboratory

Use a fresh copy for each exercise:

- **Tie outside note entry:** select A (m3 beat 4), press T. D (m4 beat 1)
  and E (m5 beat 1) should receive the corresponding ties.
- **Tie after typing:** enter the matching note at A in note entry, then
  press T. Check the generated continuation and that the input cursor
  advances once. Repeat with F (m4 beat 4): its jump is back to B (m2 beat 1),
  and the written second-ending note at E must remain unchanged.
- **Inspector:** select A's source tie, disable E, Undo/Redo, and save/reopen.
  Repeat with F's outgoing-only tie and B's incoming marker.
- **Slur:** Ctrl-select a note and its relevant boundary barline, then S.
  Use the 37E partial-direction Inspector control to verify incoming versus
  outgoing. Build separate first/second-ending fragments; slur branching is
  explicit, unlike automatic repeat ties.
- **Lyrics:** select B, Ctrl+L, then underscore for an incoming extension or
  hyphen for incoming dashes. Repeated presses extend the continuation.
  Separately enter a word near a jump boundary and continue with underscore
  or hyphen. Check Delete/Undo and save/reopen.

## Reference observations and scope

- The final native scores were opened by the released **Studio 4.7.5** CLI,
  saved to native `.mscz`, reopened, rendered, and exported to MIDI.
  Their note/rhythm and spanner counts survived that roundtrip.
  This is **not** a claim that the 37E GUI or the patched 4.7 importer was
  exercised here: those are the comparisons this suite is intended for.
- **S04:** a draft outgoing partial slur spanning multiple systems caused
  a stock 4.7.5 crash during rendering. A normal slur and an incoming partial
  over the same span rendered. The final S04 therefore has a long incoming
  fragment, a separate short outgoing fragment, and a normal control. Avoid
  extending its outgoing fragment across a system break during this baseline
  comparison. This observation does not identify a 37E regression.
- **L03/L04:** Studio 4.7's native PartialLyricsLine reader/writer does not
  persist an individual `placement` tag like a regular lyric does. L03 uses
  two Below verses; L04 uses the **score-wide Above style** for a stable
  Above reference. Per-line placement changes and migration save/reopen are
  still useful diagnostic checks, but do not assume stock 4.7 serializes
  every override supported by 37E.
- **MIDI baseline:** stock 4.7.5 MIDI export omitted the first-visit attacks
  on the incoming tied notes in R01-R05 and X01. The other attacks and routes
  matched the explicit musical model. This concerns the exported MIDI files;
  live audio should be tested separately on your build. The intended 37E
  behavior remains pass-aware: the first visit attacks, and the tied return
  sustains. Do not change 37E just to imitate a missing reference attack.
- Guitar TAB, grace-note destinations, tuplets, linked-part editing, and
  large-score performance are later focused checks, not claims of this set.

## Regeneration and verification

The files are deliberately plain-text `.mscx` for review in Git.

```bash
python3 test/repeat-spanners/v0.15/generate_scores.py
python3 test/repeat-spanners/v0.15/verify_scores.py
```

Generation overwrites the 32 pristine scores and `manifest.json`; do not run
it over edits you want to retain. The verifier checks paired music, 4/4 voice
durations, bidirectional spanner connectors, and expected element counts.
It does not claim to replace MuseScore layout or playback.

For optional native export verification, provide a directory containing
`ID-475.mscx` and `ID-475.mid` emitted by Studio:

```bash
python3 test/repeat-spanners/v0.15/verify_scores.py --studio-output /path/to/exports
```

The script reports MIDI differences rather than hiding the reference
observations above. No application binary or soundfont is included.

## Detailed case checks

### R01 — Ties: ordinary repeat and linear continuation

- m1 beats 1-2 are an ordinary tie control.
- A ties to B on the repeat and to C on the last pass.
- B attacks on its first visit; it is sustained only when reached from A.

### R02 — Ties: first and second endings

- A has a normal tie to D and an incoming branch at E.
- F has an outgoing partial to B; there is no full F-to-E tie.
- Changing a repeat destination in the Inspector must survive undo/redo and save/reopen.

### R03 — Ties: D.C. al Fine

- No initial tie-in attack is suppressed on the first visit to m1.
- The jump from m6 into m1 is sustained.
- Playback ends at Fine on the return, not at the first Fine visit.

### R04 — Ties: D.S. al Coda

- A sustains into B after D.S.
- C ties linearly into m5 on pass 1 and into D on the Coda pass.
- The first visits to B and D must follow the actual route; do not mute an unrelated attack.

### R05 — Ties: chords, voices, and two staves

- Each chord tone has its own incoming repeat tie.
- Voice 2 and the lower staff retain separate ties and attacks.
- Compare system-header clearance, stem direction, and stacked tie separation.

### R06 — Ties: disabled, rest, and pitch controls

- A has a valid repeat destination B, deliberately unchecked: B must reattack.
- C has no repeat tie into a rest, even though a C note follows later in m6.
- E has no repeat tie into the differently pitched F; all three linear ties remain.

### S01 — Slurs: incoming and outgoing at a repeat

- B hangs from the beginning of m2 and ends on beat 3.
- A begins on beat 2 and hangs to the repeat barline.
- C is an ordinary slur; Inspector direction and line style edits must remain independent.

### S02 — Slurs: alternate endings

- A is outgoing, D becomes ordinary on load (no jump arrives at the first ending), and E stays incoming.
- Changing E must not reshape D.
- Partial slurs are entered explicitly; automatic tie branching is not an automatic-slur promise.

### S03 — Slurs: one-note anchors and both ends

- A hangs at both barlines, spanning the complete repeat bar.
- B and C have one-note anchors and positive visual length.
- C is below and dashed: compare direction/style persistence after save/reopen.

### S04 — Slurs: system and page breaks

- A is an incoming partial spanning three systems and a forced page break.
- B is a separate outgoing fragment on the final system; C is a normal slur.
- A draft outgoing partial across several systems crashed stock 4.7.5; do not extend B across a system break during this baseline comparison.

### L01 — Lyrics: melisma across a repeat

- A continues to the repeat barline; B begins at the repeat landing.
- B extends through beat 2, ending before the lyric on beat 3.
- There is no printed empty syllable at B; selecting/deleting the line is undoable.

### L02 — Lyrics: dashed continuation across a repeat

- A and B show dashes rather than solid melisma lines.
- The incoming dashed span leads to turn on beat 3.
- Toggle Extension line in the 37E Inspector, undo, save, and reopen.

### L03 — Lyrics: two verses and voltas

- Verse 1 is a melisma below; verse 2 is dashed on the next verse baseline below.
- Both endings carry incoming lines for both verses.
- Edit verse independently, then undo and save/reopen without moving another verse. L04 supplies the above-staff style control.

### L04 — Lyrics: above staff, page break, and both ends

- The score-wide lyric placement is Above. A starts at m2 and spans the page break through the last note of m3.
- B is a manually prepared empty continuation hanging at both boundaries of a one-bar repeat.
- B is a geometric stress case; lyrics do not synthesize sung words in playback.

### X01 — Combined: grand staff, endings, ties, slurs, lyrics

- All three feature families coexist over a forced page break.
- Chords branch to the correct ending, while the lower staff remains a rhythmic reference.
- Save modified copies in each application, reopen, then repeat the route and Inspector checks.

### E01 — Entry lab: create ties, slurs, and lyric continuations

- Start from a fresh copy for each exercise; this score intentionally contains no ties, slurs, or lyrics.
- Select A and press T outside note entry: D and E should receive ties.
- Repeat in note entry and verify that the input cursor moves forward exactly once.
- Select F and press T: create outgoing/incoming repeat ties without changing E.
- Create explicit note+barline partial slurs and partial lyric lines; use the README step sequence.
