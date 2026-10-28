# Audio v0.33: finish old voices before seek controller replay

## Current status: unresolved and deferred

After v0.33, the user still hears a brief fifth when repeatedly clicking notes
in D5, D6 and D7 of the D comparison score. D1-D4 are unaffected. The user has
also observed this behavior in 3.6.2 and explicitly requests setting it aside.
Do not spend further time investigating until requested. This is not an
accepted application fix. No new v0.33 mtest result was supplied with this
report.

The release/controller replay mechanism below reproduces a fifth in focused
tests, but it has not established the complete cause of the user's persistent
click behavior. Preserve that distinction when resuming investigation. The
ordinary bend-release pitch-wheel artifact remains a separate deferred issue.

Apply after v0.32, whose tests the user reports look good and whose mtests pass.
The user also confirms this click/seek artifact occurs in 3.6.2. This is a
small shared-sequencer correction, without FluidSynth-library, adapter or
controller-default changes.

The video's moving green cursor identifies clicks during playback as seeks.
Its C5 is briefly joined by G5, matching a fifth. `Seq::setPos()` first called
`stopNotes()`, which starts release tails, then replayed controller history.
On a backward jump through D4/D5 this includes NRPN coarse tuning +7 followed
by CC121. Released voices still receive parameter updates; CC121 clears the
channel offset without necessarily clearing offsets already stored in voices.
An old C5 release can therefore retain +7 while the new C5 plays normally.
This ordering reproduces without GUI heartbeat timing or inserted delays.

An isolated replay through bundled SF3 strings changed the release's spectral
peak from about 521 Hz to 781 Hz. The generated sine SF2 changed from 523.2 Hz
to 783.9 Hz. These reproduce the interval heard in the supplied video.

The seek now sends All Sounds Off (CC120) to each MIDI mapping and calls the
host-wide `allSoundsOff(-1)` before replaying controller state. The latter also
clears the shared auxiliary return, replacing the earlier explicit auxiliary
clear. The global call covers internal voices outside the visible mappings.
The playback clock and target iterator use the original seek logic.

This intentionally finishes dry release tails when seeking. Regular note-off,
stopped note audition, ordinary transport stop, score events and WAV/MIDI
export scheduling retain their existing behavior. The older normal-playback
bend-release pitch-wheel reset issue remains deferred; this is not its general
fix. Source bundling and effects enhancements stay outside this correction.

## Verification

`fluid2SeekReleasePitch` repeatedly releases notes, finishes old voices and
replays +7/reset history before a new attack. It checks silence from old voices
and correct A4/C5 attacks for D5/D7, in stereo, raw groups and both isolated
banks including logical 255. A voice outside the target mapping exercises the
host-wide stop requirement.

Focused real-adapter PCM checks pass against FluidSynth2.3.4, along with the
earlier portamento, parameter, modulation, controller, font/XML and recovery
bodies using host/UI bridges. A separate check executes the exact changed
`Seq::setPos` method with narrow host stubs: all external/internal stops precede
controller replay, the null-score guard holds, and clock/iterator results are
unchanged. Restoring the old method fails the ordering/old-voice assertion.
Full MIDI-test source syntax passes enabled and disabled. Full application,
CMake/CTest and listening remain for the user's build.

The original verification request was repeated playback clicks on D7/D5 plus
normal stop/release and D6-to-D7 portamento. User verification now shows the
fifth persists on D5-D7, as recorded above. Component tests do not supersede
that listening result. No further changes are requested for this issue now.
