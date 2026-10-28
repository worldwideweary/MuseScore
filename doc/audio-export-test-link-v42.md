# Audio v0.42: isolate the export regression from application bootstrap

Apply on delivered v0.41. The user reports `tst_midi` fails to link with
duplicate test/application globals, Qt metadata and score command definitions.

v0.41 put the shared renderer in the same object as the main-window export
wrappers. The new MIDI test pulled that object, which referenced the
application synth factory and MuseScore methods. Linking those pulled the
application bootstrap alongside `testutils`' intentionally separate globals.
The component/syntax checks did not catch that full archive dependency.

The renderer now compiles as `mscore/renderaudio.cpp`, with no main-window
include or application factory reference. A required factory callback transfers
ownership of the created rendering synth to the common renderer. Application
wrappers pass their existing `synthesizerFactory`; `tst_midi` supplies its own
factory with the real Fluid/Fluid2 engines and NoEffect stages. Test GUI objects
are owned by the test master; legacy Fluid uses a basic GUI without application
icons. The light test environment and normal linker checks are retained.

There is no change to the render loop, file writer, routing, normalization,
note events or application synth/effect selection. v0.41's export error and
resource cleanup remain intact. The unrelated migration implementation is not
changed; its duplicate command definitions were reached through the unwanted
application-bootstrap dependency.

## Checks

- Full MIDI test source syntax passes with the modern backend enabled/disabled.
- The actual new renderer source compiles against real Qt/core headers in both
  configurations. Its undefined symbols contain no MuseScore methods or
  application factory reference.
- A focused static-archive probe uses the production renderer body, narrow
  score/factory declarations, actual engine DSP, separate test globals and a
  conflicting application-bootstrap object. The v0.41 dependency shape fails
  with duplicate globals; v0.42 links and renders while leaving that object
  unlinked. This establishes the archive boundary, not a full app link result.
- Focused production renderer/file-wrapper checks still pass: real original
  Fluid/bundled FluidSynth, normalized/raw 44.1/48 kHz, WAV/OGG/FLAC metadata and
  pitch, cancellation, open/short-write/write/close errors, empty input,
  restored rate, resource cleanup and existing-destination preservation.

Rebuild normally so CMake adds the new source, then rerun mtests. Full
application/test linking remains for the user's complete build environment.
The v0.41 normal-export, normalized-export and cancel/retry smoke check remains
the application acceptance check; no launch setting changes are needed.
