# 37E audio v0.12: optional separate dry-output buffers

Apply after audio v0.11. The user's v0.11 MIDI test passed; audio/WAV and
large-score startup checks are deferred in `audio-verification.md`.

Update: the user reported that v0.12 built successfully. v0.13 adds source
ownership metadata and collision checks; see `fluid2-channel-ownership-wip.md`.

## Purpose and behavior

The current modern adapter obtains one interleaved stereo mix through
`fluid_synth_write_float`. That combined audio cannot supply independent
instrument sends. This stage adds an opt-in `fluid_synth_process` path that
obtains separate planar dry stereo groups before summing them into the existing
host output. It does not add send controls or a reverb return yet.

Leave `MSCORE_FLUID2_AUDIO_GROUPS` unset to retain the existing stereo renderer.
With the experimental backend selected, set it to an integer from 1 to 128 to
exercise the new path, for example:

```sh
MSCORE_USE_FLUIDSYNTH2=1 MSCORE_FLUID2_AUDIO_GROUPS=16 /path/to/mscore
```

Use 1 first to compare the rendering API without changing the grouping; then
try 16. Invalid values log a diagnostic and retain the stereo renderer. This
choice is read during synth initialization and is not saved into scores.
The backend's 256 MIDI channels and 512-voice limit are unchanged. Internal
FluidSynth reverb and chorus remain disabled; the summed dry output still
passes through the existing host master Effect A/B and gain.

The buffers allocate once during initialization and render in blocks of at
most 512 frames, with no allocation in the production buffer callback. Each
plane is cleared before FluidSynth adds samples. Dry output is added to the
host buffer, preserving other synths. A failed block contributes nothing and
is not retried, since synth time may already have advanced. Storage is about
4 KiB per stereo group, up to 512 KiB at 128 groups, plus pointer storage.
More groups can increase rendering/mixing cost; they are optional.

## Routing constraints and next stage

These are raw audio groups, not instrument tracks. Stock FluidSynth maps MIDI
channel `c` to group `c % groupCount`. Its maximum of 128 groups cannot uniquely
separate all 256 MIDI channels in the current adapter. No send gain is applied
to groups in this increment, so sharing a group does not accidentally share a
new instrument control. It is essential to solve ownership before enabling
independent sends. A bend lane must inherit its logical source's send.

The next stage is an explicit mapping from score instruments and their bend
lanes to output ownership, including channels 128-255. Evaluate two bounded
synth banks or per-instrument instances with shared font data; do not silently
treat modulo groups as tracks. Initialization/controller restoration, tuning,
polyphony, font memory and stop/seek behavior must be preserved by that choice.
After that, add saved send amounts and one shared compatible reverb return
before the existing master effects. Existing score sends must remain opt-in
until compatibility and audio tests establish their behavior.

The mixer already serializes per-channel CC91 reverb amounts and sends them
to playback, including copied controls for isolated bend lanes. Those values
currently do not activate a host reverb send in the modern adapter. Review
whether a distinct host-send property is needed before changing the meaning
of the existing MIDI-out control. No additional DSP or license change occurs
in this increment.

## Source basis and verification

Reviewed official FluidSynth v2.3.3:

- `src/synth/fluid_synth.c`: `fluid_synth_process`, paired planar ordering,
  caller-owned zeroing, additive output, zero effects outputs, and 128-group
  limit. Source: https://github.com/FluidSynth/fluidsynth/blob/v2.3.3/src/synth/fluid_synth.c
- `src/synth/fluid_voice.c`: channel-modulo audio-group routing.
  Source: https://github.com/FluidSynth/fluidsynth/blob/v2.3.3/src/synth/fluid_voice.c
- `include/fluidsynth/synth.h`: public process and group-count APIs.
  Source: https://github.com/FluidSynth/fluidsynth/blob/v2.3.3/include/fluidsynth/synth.h

`tst_fluid2_audio` compiles the actual buffer class without Qt/FluidSynth. A
synthetic additive renderer checks 1/2/16/128 groups, stereo ordering, host
accumulation, zeroing, stable buffer addresses, block boundaries from 0 to
8193 frames, reuse after a shorter request, and failed-block isolation.
Checks remain active in Release builds. This verifies buffer handling, not
FluidSynth's PCM or integration with the application.

The test is included in mtests when the experimental backend is built. It can
also run as a standalone CMake project without Qt:

```sh
cmake -S mtest/audio/fluid2 -B /tmp/37e-fluid2-buffer-test
cmake --build /tmp/37e-fluid2-buffer-test
ctest --test-dir /tmp/37e-fluid2-buffer-test --output-on-failure
```

Standalone GCC C++11 tests pass, including AddressSanitizer/UndefinedBehaviorSanitizer
with leak detection disabled because this environment blocks LeakSanitizer's
process inspection. A syntax-only check of the new process/group API calls
against the official v2.3.3 public headers also passes; it does not compile the
Qt adapter. CMake is unavailable here, so CMake integration is reviewed only.
Full Qt/application build and real FluidSynth output remain unverified here. When audio testing
resumes, compare the same SoundFont and score with the variable unset, then 1,
then 16; include stereo pan, percussion, channels above 15 and 127, tuning,
LV/bend lanes, repeats, stop/seek and WAV export. Small floating-point summation
differences with multiple groups are possible; check levels and listen rather
than requiring byte-identical multigroup PCM.
