# Audio v0.34: pinned FluidSynth source

The experimental modern adapter now defaults to a bundled, statically linked
FluidSynth **2.3.4**. This is the version used for the earlier native compatibility
checks. Its full source is under `thirdparty/fluidsynth2/fluidsynth-2.3.4`;
configuration does not fetch a dependency from the network. The original
Fluid/Zerberus backends and the runtime opt-in remain available.

This stage changes build integration, not synthesis or playback event handling.
The bundled upstream C sources and headers are unchanged. Its sole upstream
edit is a CMake guard excluding standalone tests/documentation from the host
build. Provenance, archive hash, file manifest, license and build profile are
recorded in `thirdparty/fluidsynth2/README.md`.

## Application configuration

Retain the existing Qt Creator build directory and set:

```text
MSCORE_EXPERIMENTAL_FLUIDSYNTH2=ON
MSCORE_FLUIDSYNTH2_PROVIDER=BUNDLED
```

The second setting defaults to `BUNDLED` when absent. Reconfigure and build
normally. The configure output must report `MuseScore FluidSynth 2 provider:
BUNDLED` and `Support for SF3 files: yes`. No installed FluidSynth library or
headers are required for this provider. GLib/GThread and libsndfile development
packages, with Ogg/Vorbis support and codec dependencies, are still required.
On Debian/Ubuntu these are provided by `libglib2.0-dev` and `libsndfile1-dev`
and their dependencies. OpenMP remains optional when the compiler supports it.
The static FluidSynth archive is linked into the application; these remaining
dependencies retain their normal platform linkage.

For the previous system-library arrangement, select
`MSCORE_FLUIDSYNTH2_PROVIDER=SYSTEM`; it still requires pkg-config and
`fluidsynth>=2.3.3`. A build with `MSCORE_EXPERIMENTAL_FLUIDSYNTH2=OFF` does not
configure the bundled dependency. The provider does not select the runtime
backend. Keep the existing launch environment for modern isolated playback:

```sh
MSCORE_USE_FLUIDSYNTH2=1 MSCORE_FLUID2_ISOLATED_CHANNELS=1 ./mscore
```

Normally leave `MSCORE_FLUID2_SOUNDFONT` unset; it is an optional file-path
override, not a Boolean. Font discovery, default loading, chooser, ordering,
saved state, dry/send routing and master effects use the existing adapter code.

## Standalone automated checks

From the repository root, without a Qt SDK:

```sh
cmake -S mtest/audio/fluid2 -B build-fluid2-bundled \
  -DMSCORE_EXPERIMENTAL_FLUIDSYNTH2=ON \
  -DMSCORE_FLUIDSYNTH2_PROVIDER=BUNDLED
cmake --build build-fluid2-bundled --parallel
ctest --test-dir build-fluid2-bundled --output-on-failure
```

Use another build directory with provider `SYSTEM` for comparison. To exercise
the seven pure audio/routing checks without FluidSynth or pkg-config, use a
third directory with:

```sh
cmake -S mtest/audio/fluid2 -B build-fluid2-off \
  -DMSCORE_EXPERIMENTAL_FLUIDSYNTH2=OFF \
  -DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=TRUE
cmake --build build-fluid2-off --parallel
ctest --test-dir build-fluid2-off --output-on-failure
```

Without that last setting, an OFF standalone build retains the older optional
discovery of an installed FluidSynth for native tests. The full experimental
application mtests use the same selected provider as `audio`.

## Verification performed November 6, 2026

Linux x86-64, GCC 13, CMake 3.28.3; private dependency headers and system GLib /
libsndfile runtimes were used, without installing build tools system-wide.

| Check | Result |
| --- | --- |
| Bundled configure, static library build and CTest | 16/16 passed, including source integrity, SF2/SF3, failure recovery, banks, sends, return, controllers, modulators and parameters |
| SYSTEM configure/build with real FluidSynth 2.3.4 shared library | 15/15 passed |
| Backend OFF with pkg-config discovery disabled | 7/7 passed; no bundled dependency configured |
| Real adapter regression bodies with Qt 5.15.13 and a narrow host bridge, linked to bundled archive | Passed in all three rendering modes: fonts/XML, sample-rate failure/recovery, controllers, parameters, pan and portamento; prior seek component reproducer also passed |
| Actual legacy SF2 renderer versus bundled modern adapter | 84 level/PCM cases passed; modern/legacy RMS ratio 0.954586–0.955912, matching v0.32's system-library result |
| Host-like parent project, repeated setup from sibling directories, conflicting host find modules and pre-existing shared-library/ALSA cache options | Configure/build passed; parent options and C++11/automoc settings remained intact, library stayed static, host probe reported 2.3.4 |
| Host-like default installation | Installed host marker and FluidSynth license only; no FluidSynth CLI/library/headers/docs |
| Invalid provider; missing SF3 dependency; changed vendor source | Each correctly rejected with an explicit error |
| Runtime linkage of bundled native/adapter probes | No dependency on `libfluidsynth.so`; GLib, libsndfile and normal codec/system libraries remain |
| Pristine archive comparison | Same 360-file inventory; only upstream root CMake differs; archive matches independent GitHub/codeload/Ubuntu orig downloads |

An inherited scratch test executable initially lacked its executable bit; after
restoring it, the final complete bundled CTest run passed 16/16. No assertion or
production code was changed for that environment issue.

The binary format patch was replayed with ordinary `git am`. Existing upstream
CRLF text files use binary diffs to survive mailinfo's default line handling;
the replay must reproduce the exact committed tree and pass the source manifest.

These are focused checks, not a full application build or full Qt mtest run.
The user's application build/full mtests and a brief font-load/playback/WAV
smoke check remain pending for v0.34. Qt 5.9.9, non-Linux packaging, and
large-score startup/CPU/memory/Piano Roll measurements remain unverified or
explicitly deferred. No new listening fixtures are required; use the accepted
A/B/C/D audio compatibility scores and ordinary/LV repeat checks.

The persistent click-fifth on D5–D7 remains unresolved and explicitly deferred;
D1–D4 were reported unaffected. The earlier normal-playback bend-release
artifact remains separately deferred. Bundling does not claim to fix either.

Once this build/packaging checkpoint is accepted, preserve the compatibility
checkpoint and consider extracting LV playback onto the original synth before
layering the modern engine. Extra master effects and Piano Roll automation
remain later enhancements.
