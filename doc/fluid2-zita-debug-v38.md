# Audio v0.38: optimize the shared Zita DSP in modern Debug builds

The user reports v0.37 mtests pass. With the same effects but normalization
off, setup/render/total are 297/1729/2026 ms. The latest mixer-send comparison
without normalization is 375/1864/2239 ms at 100%, versus 271/1133/1404 ms at
zero. The 731 ms rendering difference includes switching from compact dry to
individual send lanes, the auxiliary reverb and potentially its tail. It is
not a measurement of the reverb DSP alone. Normalization's two render passes
account for much of the earlier 3200 ms effects-on rendering time.

## Change

Zita's existing C++ DSP translation unit was still compiled unoptimized in
Debug. Both master Zita and the separate instrument return use that source.
The modern audio option `MSCORE_FLUIDSYNTH2_OPTIMIZE_DEBUG=ON` now adds `-O2`
to that file's Debug compilation on GNU/Clang/AppleClang. Debug symbols/checks
remain enabled, and no fast-math or algorithm/parameter change is introduced.
Widgets, other effects and the rest of the host keep their previous settings.

The source bypasses the shared, unoptimized host PCH because its optimization
macros differ. Required standard/Qt headers are now explicit, allowing the real
DSP file to compile independently of a PCH. With the option OFF or unsupported
compiler, normal PCH behavior is retained. Builds without the modern adapter
keep the original optimization behavior. Native tests use the same source flag.
No FluidSynth source, channel isolation, voice limit, send routing, normalization
schedule or reverb-tail cutoff is changed.

## Verification completed

* Linux/GCC 13, real production Zita DSP through the existing narrow test bridge:
  two Zita instances processing approximately 30 seconds in 512-frame blocks
  took 0.808090 seconds at `-O0` and 0.087653 seconds at `-O2`. This isolates DSP
  cost, not complete application export time.
* Raw output from the same two builds is bit-exact across 2,646,016 float samples;
  maximum difference 0. SHA-256:
  `87c09510f20da21847734f4e8abed3b3d8bbe2b3fd50c5310b74dc748fc2054f`.
* Bundled optimized Debug CMake build and all 16 CTest checks pass, including
  Zita parameters/state, auxiliary return/tails, zero sends and repeat-pass reset.
* The actual DSP translation unit compiles at C++11 with real Qt 5.15.13 headers,
  no forced prelude, and no PCH.
* A host-like CMake fixture reusing a Qt PCH builds with optimization both ON
  and OFF. A separate marker source requires the PCH, while the enabled DSP
  source compiles independently at `-O2`.

The sample comparison and benchmark do not establish universal bit-exactness
across every compiler/platform. Full application mtests, listening and matched
export timings remain the user checkpoint for this patch.

Apply on v0.37, reconfigure and rebuild normally; no new environment setting
is needed. Keep the current effects/font/rate and normalization-off settings
for the first export comparison. Retain `MSCORE_AUDIO_EXPORT_TIMING=1` while
testing. The prior 1864 ms rendering / 2239 ms total at 100% mixer send provides
the matched comparison. Afterwards check the normal effects/normalization
workflow and ordinary/LV playback. The deferred pitch artifacts remain separate.
