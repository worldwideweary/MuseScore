# 37E audio v0.16: shared auxiliary reverb return

Apply after audio v0.15. The user reports the v0.15 application build is running;
completion has not been reported. Earlier audio/WAV and large-score startup
checks remain explicitly deferred. This checkpoint does not replace those checks.

## Completed

MasterSynthesizer now has a dedicated cleared stereo send bus and return scratch
buffer. Active synths contribute their usual dry audio plus optional source sends.
One separate existing Zita instance processes the summed sends at 100% wet. The
host adds its return to dry audio before existing master Effect A/B and gain.
The existing master effect instances, controls and state remain independent.
No new DSP library, GPL3 code or license migration is involved.

The source core already supports different send gains per source, including bend
lanes inheriting the canonical source's gain. This stage connects that signal to
an audible effect. Until saved source controls exist, a temporary environment
setting applies the same gain to all 256 source slots. This is a routing and
listening aid, not a replacement for per-instrument mixer controls or MIDI CC91.
It is read when each live/export MasterSynthesizer is constructed and is not
stored in the score or synthesizer settings.

The return is off by default. With no setting (or a zero setting), the host calls
the original synth process path, allocates no additional Zita instance, and adds
no return audio. An enabled bus with unused/zero sends also skips reverb until
an input signal arrives, preserving dry samples exactly despite Zita's tiny
internal denormal-prevention offsets. After a signal, silent input continues
processing its natural decay. There is no tail cutoff/gating in this stage.

## Try the optional return

Build with `MSCORE_EXPERIMENTAL_FLUIDSYNTH2=ON`, then launch the application with:

```sh
MSCORE_USE_FLUIDSYNTH2=1 \
MSCORE_FLUID2_ISOLATED_CHANNELS=1 \
MSCORE_FLUID2_REVERB_SEND=0.20 /path/to/mscore
```

A send must be a finite number between 0 and 1. Invalid values disable the return
with a warning. A nonzero request on a backend without isolated send support
warns and retains the normal dry renderer. Legacy Fluid and Zerberus do not
contribute to this new bus. The optional return uses fixed default Zita room
parameters; its separate controls/preset persistence are not implemented yet.
The original isolated banks' 256/256 voice split and resource caveats still apply.

For an initial listening comparison, set master Effect A/B to NoEffect, compare
with the send unset or zero, then enable 0.20. Re-enable the original master
reverb afterwards to check both together. They are two separate instances when
both are used; the shared return also passes through the master effects.

## Lifecycle and bounded processing

The bus uses two fixed buffers of 4096 stereo frames (64 KiB total). Its process
and reset callbacks allocate nothing. Zita is created/replaced outside rendering
when the sample rate is configured; a fresh instance avoids reinitializing its
existing delay allocations. The host uses its existing process exclusion while
reconfiguring DSP. An oversized/empty/null process request now returns before
taking the process guard, fixing the old oversized-request guard leak.

`ZitaReverb::clear()` clears the allocated delay, diffuser, filter and EQ histories
without changing its controls. The host requests this through an atomic flag;
only the next valid render clears the DSP history. Global all-sounds-off, score
reset, synthesizer-state changes and live seeks request a clear. Export's existing
global all-sounds-off starts each normalization pass with a cleared return.
Ordinary all-notes-off preserves natural reverb decay, including the export tail.
A channel-only stop does not clear other instruments' shared reverberation.

Seeking clears the old return, but the existing 3.x note-off/release path can still
feed residual dry voice releases into the new return. This stage does not change
voice release policy. FluidSynth can retain cached dry samples across hard reset;
new auxiliary owner warmup remains governed by v0.15. The native restart check
compares the return to a fresh effect given identical input, not a claim of
bit-identical whole-score export passes. WAV/MP3 retain their existing decay
threshold and roughly three-second export hard limit; long tails can be clipped.
No new score rendering or file-load render prepass was added.

## Verification actually run

- Strict C++11 bus checks: additive dry/return, cleared send/return buffers,
  zero-send neutrality, tails, bounded/null/empty requests and deferred reset.
- Actual production Zita DSP: wet-only impulses, nonzero decay, clear/reuse,
  no process/reset allocations, and parity across 1/513/4096-frame calls at
  44.1/48/96 kHz.
- Native FluidSynth 2.3.4 plus production Zita and bus: exact zero-send dry PCM,
  nonzero source/bend return against independently rendered weighted stems,
  release tails, and repeated hard-reset return comparisons to fresh effects.
- AddressSanitizer/UndefinedBehaviorSanitizer checks passed for all three new
  tests. System FluidSynth itself is not sanitizer-instrumented; leak detection
  is disabled because the test environment restricts process inspection.

The two Zita test targets compile the production DSP against a narrow test-only
Qt bridge, without widgets, moc or Qt state serialization. They do not certify
an application build or full MasterSynthesizer/Seq/export integration. No Qt SDK
or CMake executable is available in this environment. The host call order and
live/WAV/MP3 reset paths were reviewed in source; actual score tests remain pending.

The standalone/full experimental mtest project includes these tests:

```sh
ctest --output-on-failure -R '^(tst_midi|tst_fluid2_.*|tst_auxiliary_.*)$'
```

The native return target requires system FluidSynth development headers. Its
original generated SF2 fixture is written to a private test-build path and removed.

## Outstanding and next action

Build the application and run the new tests, then check optional return playback
and WAV (normalization off/on), live stop/seek/repeats, muted instruments, original
bends/LV and at least two instruments with different future sends. Saved controls
are not present yet, so different gains currently require the existing source API
or the native tests. The previously deferred repeat/jump audio and large-score
startup checks in `audio-verification.md` remain pending.

Next: save independent source send values with a clear migration/default policy
and add mixer controls that are distinct from the existing MIDI-out reverb field.
Keep the temporary uniform diagnostic setting documented until those controls
are testable. SoundFont UI and broader SF3/resource checks remain separate work.

## Files changed

- `audio/CMakeLists.txt`, `audio/midi/midi.cmake`: effect dependency/header registration.
- `audio/midi/auxiliarybus.h`: bounded bus and deferred reset.
- `audio/midi/msynthesizer.h`, `audio/midi/msynthesizer.cpp`: opt-in return and lifecycle.
- `effects/zita1/zita.h`, `effects/zita1/zita.cpp`: allocation-free history clear.
- `mscore/seq.cpp`: request return clear on live seek.
- `mtest/audio/fluid2/CMakeLists.txt`: test registration.
- `mtest/audio/fluid2/tst_auxiliary_bus.cpp`, `tst_auxiliary_zita.cpp`,
  `tst_fluid2_return.cpp`, `zita-standalone/effects/effect.h`: checks and narrow Qt bridge.
- `doc/audio-verification.md`, this checkpoint: verification status and next action.
