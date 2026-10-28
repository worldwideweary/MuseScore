# Bundled FluidSynth 2

Pinned upstream source: **FluidSynth 2.3.4**, tag `v2.3.4`, commit
`5ecdc4568e45123216c6888892caad07918ef127`.

Archive: <https://github.com/FluidSynth/fluidsynth/archive/refs/tags/v2.3.4.tar.gz>

Archive SHA-256:
`1529ef5bc3b9ef3adc2a7964505912f7305103e269e50cc0316f500b22053ac9`

The complete 360-file source snapshot is checked in. Configuration and builds
require no source download. Original authorship, license, examples and tests
are retained. FluidSynth is LGPL 2.1 or later; see
[`fluidsynth-2.3.4/LICENSE`](fluidsynth-2.3.4/LICENSE). The host installation also
ships that license in its resource directory's `licenses` folder.

## Local changes

Upstream's root `CMakeLists.txt` differs from the archive: when
`MSCORE_FLUIDSYNTH_EMBEDDED` is set, it skips upstream's testing/documentation
subdirectories and `enable_testing()`. This keeps their tests, `check` target,
documentation tools and installation out of the host build.

Audio v0.40 also changes five files in `src/synth`: `fluid_synth.c/.h`,
`fluid_voice.c/.h` and `fluid_mod.c`. The creation-time integer setting
`synth.mscore-release-pitch-lock` defaults to 0. When enabled, an actually
released voice retains its pitch-wheel value and sensitivity, so a channel
reset for a following note cannot retune the old release tail. Held and
pedal-held voices continue following the live wheel. Exclusive-class release
also latches; voice initialization and legato retrigger restore live input.
Other controller, generator, LFO and envelope updates remain active.

MuseScore's adapter enables the setting when available. Unmodified SYSTEM
providers lack it and retain native release behavior. External MIDI output
is unchanged and does not gain this per-voice behavior. This is the first
intentional host-specific synthesis-source change in this bundle; it is
separate from the earlier Debug optimization flags. See
`doc/fluid-release-pitch-v40.md` for tests and behavioral limits.

`MANIFEST.sha256` records every file in this snapshot **after** the local
changes. Run `cmake -P thirdparty/fluidsynth2/VerifySource.cmake` from the
repository root to verify it. The bundled native test configuration registers
the same check as `tst_fluid2_vendor`. `.gitattributes` prevents checkout-time
line-ending conversions from changing the snapshot. Its existing CRLF text
files use binary diffs so `git format-patch --binary` / ordinary `git am` also
preserve their exact bytes without needing `--keep-cr`.

The enclosing `CMakeLists.txt` is the MuseScore build wrapper. It selects a
position-independent static library, keeps dependency options local, prefers
upstream's find modules over similarly named host modules, and forwards the
dependency library directories required by the host's CMake 3.16 policies.
Neither the FluidSynth CLI nor its library/headers/pkg-config metadata is
installed by the application.

## Build profile and dependencies

The wrapper retains SF2/SF3 loading, synthesis, tuning, controllers, modulators,
parameters and threading. SF3 is required: configuration fails if libsndfile
does not advertise Ogg/Vorbis support. GLib (including GThread), libsndfile and
its codec dependencies remain external. OpenMP is used when available, as in
the upstream build; it is optional. MuseScore still owns audio-device and MIDI
I/O, exports and effects; FluidSynth's separate device/server/file-output
backends, command-line dependencies and LADSPA are disabled. Optional DLS/GIG
loading through libinstpatch is disabled; the host's supported font chooser
continues to accept SF2/SF3.

The modern adapter remains opt-in with `MSCORE_EXPERIMENTAL_FLUIDSYNTH2=ON`.
It now defaults to `MSCORE_FLUIDSYNTH2_PROVIDER=BUNDLED`. Set the provider to
`SYSTEM` to retain the previous pkg-config dependency on `fluidsynth>=2.3.3`.
The backend-disabled build and the original Fluid/Zerberus implementations do
not require this bundle. See `doc/fluid2-bundling-v34.md` for build/test commands
and the verification boundary.

For an upstream upgrade, review the new archive and license, reapply the small
CMake guard and opt-in release-pitch extension, regenerate the manifest, and
run both providers' compatibility checks. Do not replace individual DSP files
without recording that change.
