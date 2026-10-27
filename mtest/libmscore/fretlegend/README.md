# Fretboard diagram legend backport (v0.4 WIP)

The FBox implementation uses the existing 3.x vertical-frame infrastructure
and Widgets Inspector. It collects unique standard chord symbols and labelled
fretboard diagrams from the current score. Existing diagram shapes, hidden
entries and manual order remain intact during synchronization. Removing a source
chord is undoable, including restoration of its legend customization and position.

Add a legend through **Add > Frames > Insert Fretboard Diagram Legend**.
Select its frame to change scales, row/column spacing, alignment and margins,
reorder entries, or hide/show individual chords. Reset buttons restore defaults.
The normal Frames & Measures palette also contains the legend action. Existing
custom palettes may need the action added or the palette reset.

The bundled diagram database is copied unchanged from MuseScore Studio main,
`src/engraving/data/harmony_to_diagram.xml`, commit
`bb53a2d9dbdf7117e31de6a4c048e136c1a3d213` (MuseScore project's GPL source).
It contains 3,124 guitar chord shapes. Existing source diagrams take precedence;
unsupported chord names start with an empty diagram that can be edited using the
normal fretboard-diagram Inspector. Roman and Nashville symbols are excluded.
Legends are local to a score or part, and are excluded when creating parts by
default. A part can insert its own legend. Continuous view hides vertical frames,
including legends, consistently with 3.x's other vertical frames.

3.x frame margins are shown and saved in millimetres. Studio's spatium margins
are converted on import. Chord labels share a baseline within each row. Columns
allow for the label width as well as the diagram, avoiding long-label collisions
with 3.x font metrics; this can make columns wider than Studio's.

## Checks

Run the source/data checks with:

```sh
python3 mtest/libmscore/fretlegend/check_sources.py
```

Reconfigure/build your usual Qt build, then build/run the new `tst_fretlegend`
target (the CTest registration follows this repository's existing conventions).
Its six cases cover Studio-format import and layout, native save/reload, automatic
population and undo/redo, hidden/custom-order persistence, customized diagram
restoration after source deletion, and empty/continuous-view behavior.

These C++ tests and the application have NOT been compiled or run in the patch
preparation environment: Qt, qmake and CMake are unavailable. The source/data
checks and patch application checks are separate from runtime validation.
