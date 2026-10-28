# Audio v0.39: reduce send-active planar mixing work

The user confirms the current local and online baseline is Qt 5.15.2.
Qt 5.9.9 compatibility is no longer a release gate for this work.

## Profile and changes

In a focused warm-font benchmark, active-send synthesis/mixing took about
0.64 seconds and the optimized auxiliary Zita about 0.04 seconds. The main
remaining cost in that workload was planar handling, rather than the effect
calculation itself. Complete application export also includes other work.

The planar buffer now records the previously written extent of each plane.
Exactly zero planes stay zero between calls and need no repeated clearing.
After rendering, byte-exact comparison with a zero plane identifies stereo
groups that need no dry summation. There is no quiet-level threshold: tiny
tails, negative zero and nonfinite data do not qualify as positive-zero bytes.
The renderer still receives all original planes and every synth channel is
rendered normally. This does not infer silence from a note/voice count.

When a request is shorter than the previous one, the entire previously written
extent is cleared, including the old tail. A failed render marks all planes
for full clearing before reuse and contributes no failed PCM. The buffers and
metadata are allocated at initialization, with no new render-time allocation.

Compact dry output selection is now per bank, based on actual routed source
gains. A bank with no possible auxiliary contribution uses the existing compact
path even if another bank has a live send. Unknown/conflicting ownership still
suppresses sends as before; controller state, 256-channel isolation, voice
limits, reset warmup and wet reverb tails are retained.

No FluidSynth/Zita DSP source, precision, normalization schedule or reverb
tail cutoff is changed. Existing dry summation ordering and the bit-exact
dry/send-transition checks are retained.

## Benchmark comparison

Linux/GCC 13, real optimized Debug FluidSynth 2.3.4, bundled
FluidR3Mono_GM.sf3, 44.1 kHz, 512-frame blocks, approximately 30 seconds of
audio, a live font held while an export-like second engine renders. Notes
release before the end; the reverb continues through the remaining blocks.
The reference is an unchanged v0.38 engine/header snapshot.

| Workload | v0.38 engine/mixing | v0.39 engine/mixing |
| --- | ---: | ---: |
| One note per bank, sends off | 0.372 s | 0.365 s |
| One note per bank, both sends active | 0.670 s | 0.416 s |
| One note per bank, only low-bank send active | 0.673 s | 0.417 s |
| 32 active MIDI lanes per bank, sends active | 1.589 s | 1.416 s |
| 128 active MIDI lanes per bank, sends active | 4.615 s | 4.297 s |

Auxiliary Zita remained about 0.040–0.044 seconds. The sparse active-send
engine case improves about 38%; the dense 256-lane case improves about 7%.
This is not a promise of the same improvement in the user's full-score export.
All dry, send and wet samples in all five workloads are bit-exact against
the v0.38 reference. There is no new render thread or change to export passes.

The benchmark's constant 512-frame workload was measured before a final
equivalent refinement tracking the written extent instead of always clearing
a full active plane; at 512 frames they perform the same clearing work.
Final tests cover short/long block changes and failed-render recovery.

## Verification and user checkpoint

Final bundled optimized Debug build/CTest passes 16/16, SYSTEM provider
build/CTest 15/15, and the dependency-disabled configuration 7/7. Focused
real Qt 5.15.13 adapter regressions pass in all three rendering modes, covering
font state/recovery, controller/parameter compatibility, pan, portamento and
the earlier seek component reproducer. A staged SYSTEM test executable first
lacked its execute bit; restoring it allowed the final full 15/15 run.

The native tests now explicitly exercise sparse 512/1/512/63-frame reuse,
failed writes and later long reads, per-bank routed/unknown/zero gains, bank
boundaries, live send changes independently in each bank, sample rates and
strict dry PCM equality against forced planar rendering.

AddressSanitizer/UndefinedBehaviorSanitizer checks cover the planar-buffer and
send tests. LeakSanitizer cannot inspect processes in this sandbox, so leak
detection was disabled for those runs; this is not a leak-check result.

Apply on v0.38 and rebuild. Keep Qt 5.15.2 and the existing options. Repeat
C with the same master effect, 100% mixer send and normalization off, with
`MSCORE_AUDIO_EXPORT_TIMING=1`. The last matched user result was setup 324 ms,
render/write/cleanup 1706 ms, total 2030 ms. Also check zero send, live send
edits, ordinary/LV playback and the normalized export after restoring settings.
Full application mtests and matched export timings remain user checkpoints.
Large-score startup/resource checks and the two pitch artifacts stay deferred.
