# 37E audio v0.23: SoundFont replacement and SF3 verification

Update: v0.24 stages sample-rate/font replacement and adds failed-initialization
recovery. See `fluid2-sample-rate-recovery-wip.md`. The explicit replacement
coverage below remains applicable; the management GUI and multiple-font support
are added by v0.25 in `fluid2-font-management-wip.md`. The one-font descriptions
below record the v0.23 checkpoint.

Apply after audio v0.22. The patch also carries the recorded v0.22 acceptance
notes: all three prior tests passed; application Save to Score/reload/reopen
and current settings in live playback/WAV were verified. Reset/default buttons,
independent live sends and large-score/startup checks remain deferred/pending.

## Completed

Explicit SoundFont replacement now rejects a loaded font with no presets before
stopping old voices or removing the active font. FluidSynth accepts a structurally
valid font containing no presets, so treating every successful sfload ID as a
usable replacement could silence all instruments. Preset validation happens in
each synth bank. Any failure unloads only the new objects, including a successfully
loaded but unusable current object, and preserves the old fonts, presets, held
notes, controllers, source ownership and send gains.

Passing an empty font list still deliberately unloads the font. Successful
replacement keeps the existing stopped-loading behavior: old voices are stopped
and drained before unloading, source ownership is cleared, and send gains remain.
The adapter updates its stored path and patch metadata only after the engine
accepts the replacement. This stage retains one logical SoundFont; the font list
GUI and multiple-font support have not yet been implemented.

Windows filenames now use QString::toUtf8 at the FluidSynth API boundary. Since
FluidSynth 2.2, that API expects UTF-8 on Windows rather than local ANSI bytes.
Other platforms retain QFile::encodeName. This branch follows the pinned upstream
API contract; a Windows runtime test has not been performed here.

## Native regression test

New target: tst_fluid2_soundfonts. It generates original minimal SF2 and SF3
fixtures, each with melodic and drum presets, and exercises both the single-synth
and isolated two-bank layouts. The SF3 contains a small, committed, already encoded
Vorbis sine sample. Neither an installed SoundFont nor FFmpeg is needed to run
the test. Its generation command and license are recorded beside the fixture.

The test covers:

- Valid SF2/SF3 loading and preset enumeration, plus initial failure cleanup.
- Missing files, a truncated RIFF file, damaged SF3 compressed samples, and a
  valid font containing no presets. Previous font IDs/counts, held LV voices,
  controllers, routing and gains survive. Dry and source-send PCM remain equal
  to an unmodified reference, including new attacks through the retained preset.
- On Linux, injected load failures in either bank and a successful second-bank
  load of an empty font. Both staged objects must roll back without leakage.
- Repeated SF2/SF3 replacement, voice cleanup and source-gain retention. Measured
  sine frequency verifies retained 442 Hz master tuning plus 50-cent note tuning.
- Short callback-style blocks and larger render requests, explicit unload with
  no cached old audio, and reload from an empty state. These are renderer checks,
  not tests of the WAV file writer.

The standard SF2 fixture remains byte-identical to earlier stages. The compressed
fixture has different amplitude and lossy encoding, so SF2 and SF3 are checked
for usable playback/tuning rather than identical PCM across formats. Deliberate
bad-font cases may log FluidSynth/libinstpatch decoder errors; test success is
determined by recovery assertions. SF3 decoding must be supported by the linked
FluidSynth installation, as needed for the application's SF3 fonts.

## Verification actually run

- The new test passes against native FluidSynth 2.3.4, with and without Linux
  linker injection. The final test fails against the previous loader on its
  initial no-preset-font check, demonstrating the regression it catches.
- The injected test also passes with AddressSanitizer/UndefinedBehaviorSanitizer
  on our code. Leak inspection is disabled for the environment restriction;
  the prebuilt FluidSynth/runtime dependencies are not sanitizer-instrumented.
- Existing tst_fluid2_banks and tst_fluid2_return pass against the updated core.
- A focused check compiles the real Fluid2 adapter with Qt 5.15.13 and native
  FluidSynth 2.3.4. It passes SF2/Unicode-path SF3 loading, failure retention of
  path/state/patch pointers/tuning/sends, legacy basename resolution, empty state,
  and playback with the repository's FluidR3Mono_GM.sf3 (197 presets).
  This uses a narrow PCH include bridge, a GUI-constructor stub and a controlled
  installed-font list. It is not a full MuseScore application/UI test.
- Strict C++11 warning checks, unchanged default-SF2 bytes and whitespace checks
  pass. No new DSP dependency, score format or license migration is introduced.

The full application build and CMake/CTest registration have not been run in this
environment. From the user's configured experimental build directory:

```sh
cmake --build . --target tst_fluid2_soundfonts tst_fluid2_banks tst_fluid2_return tst_midi
ctest --output-on-failure -R '^(tst_fluid2_soundfonts|tst_fluid2_banks|tst_fluid2_return|tst_midi)$'
```

## Outstanding and next action

This checkpoint covers explicit font replacement. Sample-rate reinitialization
still rebuilds the engine before reloading the remembered file. Its recovery
when a font has moved/disappeared needs a separate lifecycle checkpoint before
connecting the management GUI. Real third-party SF2/SF3 coverage, multi-font
ordering/state and resource measurements remain separate work. A preset's
existence is not proof that every key/velocity of every third-party preset is
audible; decoding and sample structure remain the upstream loader's job.

Next: verify this build/test checkpoint, harden the sample-rate/font lifetime
boundary, then connect the existing font list interface to atomic list updates.
The user has explicitly deferred remaining reverb checks and large-score checks.

Changed files: Fluid2 adapter and synthbanks.cpp; native test registration,
testfont.h, new soundfonts test and original Vorbis fixture/provenance; this
checkpoint and the two existing audio verification notes.

Reference: FluidSynth v2.3.4 src/synth/fluid_synth.c, sfload/sfunload API contract:
https://github.com/FluidSynth/fluidsynth/blob/v2.3.4/src/synth/fluid_synth.c
