# Audio v0.41: export cleanup and compatibility checkpoint

Apply directly on delivered v0.40. The patch also includes the previously
unpublished v0.40 user acceptance notes. `audio-backport.md` is the consolidated
configuration/status guide; earlier stage documents remain historical.

## Export behavior

The shared float-stereo exporter previously leaked its synth and left its
device open if headless rendering produced no events. It also ignored
QIODevice short/failed writes, while the soundfile wrapper reported every
requested frame as written regardless of libsndfile's result. File-close
errors were logged but did not change success.

The rendering synth now has scoped ownership, every opened output is closed
on exit, and scoped sample-rate restoration covers success, cancellation and
failure. Short or failed writes return false. The soundfile wrapper propagates
actual frame counts and close failure; unsuccessful file exports remove the
incomplete output only if this attempt successfully opened the file. A failed
open leaves an existing destination intact and does not close a device
owned/opened elsewhere.
Invalid score/device input fails before opening output.

`renderAudio` is the common renderer used by the existing MuseScore methods.
It receives an explicit optional live SynthesizerState, enabling the same code
to be tested without constructing a main window. GUI exports still use live
settings; headless exports still use the saved score state and expressive-patch
preparation. Public application saveAudio signatures are unchanged. There is
no change to note events, normalization passes/gain, effect routing, tail
cutoffs, provider selection or accepted release-pitch behavior.

## Verification boundary

`TestMidi::audioExportLifecycle` uses the new one-measure `audio-export.mscx`
fixture and an original generated/padded SF2. It renders 44.1/48 kHz normalized
and unnormalized streams, checks finite/nonzero PCM and normalized peak,
closed output and restored host rate, then injects failed/short writes,
open failure and progress cancellation. It covers original Fluid in both
build configurations, plus isolated modern FluidSynth when built. Environment
and preference overrides are restored. It uses no installed SoundFont.

Patch-environment checks use real Qt 5.15.13, libsndfile and rebuilt original
Fluid / bundled FluidSynth DSP. The actual production renderer and soundfile
wrapper bodies run with narrow score/factory declarations. Checks exercise
the same mtest method body, both state-selection branches, 44.1/48 kHz
normalization, WAV/OGG/FLAC metadata and pitch, cancellation/open/short-write/
write failures, empty input, injected libsndfile write/close failure,
restored rate, no retained synth and removal of failed output. This is not
the full score loader, complete MasterSynthesizer effect chain or application.

All eight successful PCM comparisons (two engines, two rates, normalization
on/off) are bit-exact against the v0.40 production renderer in this focused
setup. Injected file-open failure preserves an existing destination. The
previous renderer fails the negative-control assertion because it reports a
failed output write as successful.

AddressSanitizer/UndefinedBehaviorSanitizer pass for the focused exporter,
adapters and failure-injection checks. Linked engine/runtime libraries are
regular builds. LeakSanitizer is disabled because process inspection is not
supported here; the explicit factory lifetime checks are not a general leak
audit.

The full MIDI test source and actual exporter translation unit (with a small
main-window declaration bridge) are syntax-checked against real Qt/core
headers, with the modern backend enabled/disabled and audiofile support
enabled/disabled. Full application build/mtests and application export/cancel
acceptance remain user checks. Engine sources and the already-passing v0.40
native suite are unchanged by v0.41.

## Application check

Rebuild normally and run full mtests. Export the existing C score as WAV, with
normalization on and off; verify accepted playback/bend behavior is retained.
Cancel an export, then export again to confirm recovery. The focused tests
inject storage failures automatically; there is no need to fill a disk or
change permissions manually. Keep the existing launch settings.
