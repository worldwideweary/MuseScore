# Audio v0.37: compact dry output without reducing channel isolation

The user verified an older Debug build without the audio changes exports C in
under one second. Debug alone therefore does not explain the remaining gap.
The v0.36 user timings were setup 329/274 ms and rendering/writing/cleanup
3371/3452 ms. With effects and normalization disabled together, setup was
330 ms and rendering/writing/cleanup 1574 ms. These last two settings were
changed together, so their individual costs have not been isolated.

## Rendering change

The two-bank isolated engine used 128 host stereo planes per bank on every
render, even when there was no requested auxiliary output or all source send
gains were zero. It cleared, copied and summed those planes unnecessarily.

FluidSynth 2.3.4's documented public `fluid_synth_process` API wraps all audio
channels into the supplied output buffers when fewer buffers are provided.
The engine now requests one stereo pair per bank when individual send lanes
are unnecessary. It retains both synth instances, all 256 logical channels,
voice limits, generators/controllers, internal channel groups and render time.
No active-voice heuristics or silent-bank skipping are used.

When any source gain is nonzero and an auxiliary output is requested, the
original separate-plane routing remains in use. The decision is evaluated on
every process call, so live gain edits take effect normally. A nonzero gain
on an unowned source conservatively keeps separate planes. Source ownership,
reset warmup and send advancement retain their existing semantics.

Both paths sum each bank locally before adding it to the host buffer. This
keeps their dry PCM bit-exact, including pre-existing host audio and output
from the other bank, across send changes. Floating-point summation relative
to pre-v0.37 builds may differ at rounding precision; existing native parity
tests retain their original tolerances. No test tolerance was relaxed.

## Empty state diagnostic

Effects with no saved parameters may return a default state group with no
name or values. MasterSynthesizer now ignores exactly that empty group.
Named unknown groups and unnamed groups containing values still follow the
existing diagnostic path. This removes the harmless `unknown <>` message
without changing real effect settings.

## Verification scope

Native export-like benchmark: real optimized Debug engine, two isolated
128-channel banks, warm FluidR3Mono_GM.sf3, 44.1 kHz, one piano note, 512-frame
blocks, 30 seconds of rendered audio. In this environment the v0.36 benchmark
took 1.428–1.621 seconds; the compact output took 0.881–1.090 seconds. These
exclude complete application export/UI/effects costs and are not a promise
of matching the older build's end-to-end timing.

The extended native bank test compares compact output against forced separate
planes at 44.1/48 kHz, channels 0/127/128/255, pre-filled dry and auxiliary
buffers, irregular 1/63/64/65/511/513-frame calls, live nonzero-send transitions,
return to zero sends and all-sounds-off. Dry parity is exact; auxiliary parity
retains the normal PCM tolerance. Existing return tests keep their strict
zero-send equality assertion.

Validation completed: bundled optimized Debug build/CTest 16/16, SYSTEM
provider build/CTest 15/15, and focused real Qt 5.15.13 adapter regressions in
all three modes (font state/recovery, controllers, parameters, pan, portamento
and the earlier seek component test). The latter also executes the production
MasterSynthesizer state loader with an empty group. No full application build
or user export timing is inferred from these focused checks.

Apply on v0.36 and rebuild. Retest the same C export with NoEffect, zero
instrument sends and normalization off to compare with the latest 1904 ms
total. Keep `MSCORE_AUDIO_EXPORT_TIMING=1` enabled while comparing. Afterwards
restore effects/normalization and verify ordinary/LV playback and WAV output.
Full application mtests and full-score speed parity remain user checkpoints.
The earlier bend-release/click-fifth artifacts remain deferred.
