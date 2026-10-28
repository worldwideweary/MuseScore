# 37E audio v0.15: source send gains and auxiliary signal generation

Apply after audio v0.14. No additional user build/test result was reported in
this stage. Audio/WAV and large-score startup checks remain explicitly deferred.

## Completed stage

The isolated FluidSynth core now generates two independent additive stereo
signals: the original dry mix and an auxiliary effect input. For each physical
output it looks up the logical source owner and applies that source's send gain
only to the auxiliary signal. Its input already includes FluidSynth channel
volume and pan. The eventual shared return will be mixed before host master
Effect A/B and gain.

All gains start at zero and accept finite values from 0 to 1. Invalid source
indices, NaN/infinity and out-of-range values are rejected without changing a
valid gain. Bend lanes use their parent source's gain even when their own MIDI
channel has a different stored gain. Unknown or conflicting owners contribute
no auxiliary audio. Sources in different banks remain independent.

The dry mix is retained exactly as before, subject to the same floating-point
mixing tolerance. Both buffers are additive: the caller clears them once when
needed, so another synth's contribution is retained. Overlapping buffers and
auxiliary requests in the non-isolated core are rejected before rendering or
advancing synth time. Failed blocks contribute to neither dry nor auxiliary.
The production send path uses fixed state and the existing planar scratch
buffer; it adds no allocation in rendering.

## Adapter seam and current boundary

`Synthesizer::processWithAux` is an optional new hook. Its default delegates to
the existing process method, preserving legacy backends. Fluid2 implements it
and falls back to dry-only playback when its engine is not isolated. Existing
calls to Fluid2::process still request only the dry signal.

The runtime API `setSourceSend(source, gain)` / `sourceSend(source)` exposes
gain state through Synthesizer/Fluid2. The Fluid2 setter requires isolated mode,
selected using v0.14's `MSCORE_FLUID2_ISOLATED_CHANNELS=1`. The adapter serializes
updates with its existing mutex. Ownership state now lives in the core beside
the sends, so the real-engine tests exercise the same routing used by playback.

MasterSynthesizer still calls the existing dry process method in this increment.
It does not yet allocate/consume a shared auxiliary bus or run a reverb return.
There is no new audible effect, saved score property, mixer control or additional
environment variable for send amounts. These gains are runtime state for the
next host-integration stage. MIDI-out CC91 and SoundFont reverb generators are
separate and unchanged. No GPL3 DSP or license changes are included.

## Reset and lifetime policy

Stop/all-sounds-off resets ownership while retaining source gain settings.
All-notes-off retains ownership for release tails. Successful font replacement
retains gains and clears ownership after the existing old-voice teardown;
failed replacement retains both. Core reinitialization clears gain state to
zero. A future saved-state/UI layer must reapply gains when reinitializing or
switching scores; persistence is not implemented here.

FluidSynth can have cached samples remaining after an all-sounds-off request.
Following a hard reset or CC120, the affected channel's auxiliary contribution
is suppressed for two internal DSP blocks, including after a new owner binds.
This avoids sending old cached PCM through that owner's gain. The dry signal is
not muted by this policy. The countdown advances once per successful complete
engine render, regardless of the number of banks or lanes, and includes renders
without an auxiliary buffer. A failed render conservatively retains the delay.
With 2.3.4's 64-frame blocks this is 128 frames, about 2.9 ms at 44.1 kHz.

Gains currently change at render-call boundaries. Smoothing, saved send state,
GUI controls and host-return reset/tail policy remain subsequent work.

## Verification

The new dependency-free `tst_fluid2_sends` compiles the production send mixer
and buffer code. It checks zero defaults, range validation, independent bank
owners, bend inheritance, stereo summing and output offsets, unchanged dry
planes, unknown/mixed suppression, reset delays and gains, and a failed scratch
block reaching neither output. Existing buffer tests exercise the new optional
successful-block callback through the original three-argument interface.

The native `tst_fluid2_banks` now renders a three-channel mixture and three
independent reference stems using actual FluidSynth 2.3.4 and the generated SF2
fixture. It compares dry PCM to the stem sum and auxiliary PCM to their weighted
sum across 1/63/513/2048-frame calls, with volume, pan and note tuning applied.
It checks shared source ownership across banks, a deliberately different
bend-lane gain, gain updates, conflict suppression, preserved existing buffer
contents, rejected aliases not advancing time, lifecycle state and unsupported
non-isolated requests.

A native reset regression renders one frame to leave cached audio, kills that
channel, assigns a different source and starts a new note. The auxiliary stays
silent during reset warmup, then resumes with the new gain. The full v0.14
PCM/tuning/sostenuto/font transaction checks remain in that native test.

Strict GCC C++11 runs pass. AddressSanitizer/UndefinedBehaviorSanitizer checks
of our send/core code also pass against the uninstrumented system 2.3.4 library;
leak detection is disabled for the environment's process-inspection restriction.
These are native core checks, not a full Qt application build, listening test,
or large-score performance result. CMake integration remains pending the user's
build in this environment.

```sh
ctest --output-on-failure -R '^(tst_midi|tst_fluid2_audio|tst_fluid2_routing|tst_fluid2_sends|tst_fluid2_banks)$'
```

## Next action

Connect a dedicated cleared auxiliary bus in MasterSynthesizer, then process
one compatible wet-only reverb return and add it to the dry mix before master
Effect A/B. Check zero-send neutrality, tails, stop/seek and export through that
host path. Add saved source controls and mixer UI after the return is testable;
keep their meaning distinct from MIDI-out reverb settings.
