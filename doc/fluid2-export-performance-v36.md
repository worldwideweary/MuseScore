# Audio v0.36: Debug DSP optimization and export setup

The user reports v0.35 fully passes application mtests and playback works.
WAV export of C-reset-and-bend takes about ten seconds, compared with roughly
one third of a second in the online 37E build. The full timing gap is not yet
reproduced in a complete application here; do not treat the native timings
below as an end-to-end confirmation of parity.

Two concrete costs were found:

* The bundled engine inherits Debug's unoptimized C/C++ flags. Previously the
  system FluidSynth library was an independently optimized dependency. The
  adapter's isolated-channel mixing loops are also expensive without optimization.
* The WAV/OGG/FLAC format wrapper loads an unused MasterSynthesizer before
  delegating to the shared QIODevice exporter, which creates the synth that
  actually renders. The unused stack adds initialization/font-loading work.

## Changes

`MSCORE_FLUIDSYNTH2_OPTIMIZE_DEBUG` defaults ON. For GNU/Clang/AppleClang it adds
`-O2` to Debug compilation of the bundled engine and the dedicated modern
adapter target. Debug symbols and checks remain enabled; the rest of the
application retains its Debug settings. It does not enable fast-math, change
sample precision, reduce channels/voices, skip effects or alter MIDI scheduling.
Set this option OFF when instruction-level DSP debugging is needed. Other
configurations and MSVC optimization flags are unchanged.

Known pinned-upstream type-limit, sign-comparison and deprecated-wrapper
warnings are suppressed only on the bundled engine object target for those
compilers. Host/adapter diagnostics, including implicit-declaration errors,
remain enabled. Upstream source and manifest hashes are unchanged.

The unused format-wrapper synth is removed. The shared exporter retains its
state selection, sample-rate handling, normalization passes and actual renderer.

Optional launch environment `MSCORE_AUDIO_EXPORT_TIMING=1` prints one line
after a completed shared export, separating setup from rendering/writing/cleanup.
It is disabled by default and changes no output samples. Its total starts at
the shared exporter entry; it does not include the format wrapper's preliminary
MIDI rendering or file-dialog time.

## Local validation

Linux/GCC 13/CMake 3.28.3, real bundled 2.3.4, FluidR3Mono_GM.sf3, two isolated
128-channel banks, 44.1 kHz, one piano note, 512-frame blocks, 30 seconds of
rendered audio. The live font remains loaded before an export-like second
engine is constructed. Nine OpenMP decoder threads were selected for these
comparisons; these are wall times from this environment, not user-machine targets.

| Build | Setup/font timing | Render time |
| --- | --- | --- |
| Existing optimized Release engine/adapter | init 0.026 s, warm load 0.119 s | 1.073 s |
| Unoptimized Debug engine, optimized adapter | init 0.079 s, warm load 0.204 s | 3.334 s |
| Unoptimized Debug engine and adapter | init 0.114 s, warm load 0.295 s | 5.126 s |
| Corrected optimized Debug engine/adapter, two runs | init 0.042–0.078 s, warm load 0.119–0.185 s | 1.428–1.621 s |

Cold SF3 loading was separately measured at 1.258–6.671 seconds with different
decoder thread counts. Warm loading was much cheaper. No new persistent font
cache, thread limit or loading-semantics change is introduced by this patch.

* Corrected Debug engine builds with zero compiler warnings in the native build.
* All 16 bundled CTest checks pass, including source integrity, SF3/recovery,
  controllers, parameters, banks and sends. Two scratch executables initially
  lacked execute permission; restoring that bit allowed a complete 16/16 rerun.
* Configuring with optimization OFF removes `-O2` from the generated Debug
  engine flags. Corrected flags retain `-g`, `-DDEBUG` and strict diagnostics.
* The actual modified format-wrapper method was compiled with Qt 5.15.13 and
  libsndfile in a focused host bridge. A stub shared renderer wrote 512 frames
  into each real WAV/OGG/FLAC device; reopening verified stereo/44.1 kHz/frame
  count, original global sample-rate restoration, and zero unused synth factories.
  This checks delegation/container behavior, not full-score PCM correctness.

## User checkpoint

Apply on v0.35, reconfigure and rebuild. Repeat the same C-reset-and-bend WAV
export with the same font, sample rate and normalization setting, preferably
twice. The new full application mtests and export timings remain pending.
If still slow, enable `MSCORE_AUDIO_EXPORT_TIMING=1` and provide its timing line
to distinguish remaining font setup from rendering/writing. The ordinary
bend-release and D5–D7 click-fifth artifacts remain separately deferred.
