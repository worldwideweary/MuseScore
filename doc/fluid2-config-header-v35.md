# Audio v0.35: isolate the bundled configuration header

The user's first full application build of v0.34 failed while compiling
FluidSynth C sources: pow, cos, sin and other math functions were undeclared.
The inherited host include paths preceded FluidSynth's private include paths,
so its `#include "config.h"` found MuseScore's generated header. That header
does not define FluidSynth's HAVE_MATH_H feature switch; consequently
fluid_sys.h omitted math.h. This is a compile-time header collision, not a
missing libm link or a reason to disable compiler diagnostics.

The dependency wrapper now clears inherited directory include paths before
adding upstream. Upstream supplies its own private/public headers and GLib /
libsndfile usage requirements. Parent include paths, the adapter's Qt includes,
the SYSTEM provider and upstream C/header/DSP files are unchanged. The pinned
360-file source manifest remains valid.

The dedicated Qt-free regression models the host's includes with a deliberately
failing config.h. It builds the actual bundled engine and probes version 2.3.4:

```sh
cmake -S mtest/audio/fluid2/host-config -B build-fluid2-host-config
cmake --build build-fluid2-host-config --parallel
ctest --test-dir build-fluid2-host-config --output-on-failure
```

Verified on Linux with GCC 13/CMake 3.28.3: the regression builds and passes
1/1; removing the wrapper fix in a disposable copy makes the same regression
fail on the poison host header. The existing bundled build and all 16 native
CTest checks also pass. The earlier host-like sibling/setup fixture builds
successfully with a poison header added to its inherited include paths too.

The v0.34 patch's whitespace warnings originate in the preserved upstream
snapshot. This follow-up does not reformat that snapshot or change its hashes.
Apply on v0.34, reconfigure the existing application build and rebuild normally.
Full application build/mtests remain the user's pending verification.
