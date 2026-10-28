# Audio v0.43: quiet cancellation of file export

Apply on delivered v0.42. The user reports mtests pass and the audio tests seem
okay. Canceling file export works, but the caller displays "Cannot write into"
because it handles every false return as an I/O failure.

The file exporter now supplies cancellation separately through an overload.
The existing two-argument method retains its signature and false-on-cancel
result. WAV/OGG/FLAC export from Save As suppresses the generic write-error
dialog only for explicit cancellation. Actual write/open/close failures still
return false and retain the error dialog. Other export formats are unchanged.
Canceled exports remain unsuccessful and their incomplete output is removed.
The flag is reset at the start of each attempt, including retries and errors.

Focused checks run the actual file-wrapper body with real Qt progress dialogs,
engine DSP and narrow score/factory/window declarations. A queued Cancel gives
false plus the cancellation flag, removes output and restores the host rate;
the next export succeeds and clears the flag. Injected write failure returns
false with cancellation clear, preserving the caller's error condition.
Existing focused normalized/raw export, metadata and failure-cleanup checks
also pass. Wrapper syntax is checked with backend/audiofile support on/off.
The full Save As main-window interaction remains an application check.

Rebuild/run mtests normally. Cancel a WAV export: the progress window should
close without the write-error popup. Then export successfully to verify retry.
No engine, MIDI, DSP, normalization or launch-setting changes are included.

Acceptance, November 6, 2026: the user confirms cancellation remains quiet and
mtests pass. This accepts the v0.43 cancellation correction and closes this
audio cleanup phase. Audio/export behavior was accepted through v0.42; the
separately deferred checks in `audio-backport.md` remain deferred. The user did
not separately report a new retry check in this confirmation.
