# 37E audio v0.06: Studio 4.7 routing and event audit

Source audit, October 29, 2026. No audio behavior changes in this increment.
Reference: official MuseScore `4.7`, commit
`96480d58445e3b24b230a3fe7d4725d639aa2eb3`. Paths below refer to that revision
unless marked 37E. This is a targeted audit of reverb sends, LV and the adapter
boundaries, not certification of the entire playback engine or its licensing.

User verification through audio v0.05: piano playback and WAV export, rapid
repeated notes, basic stop/seek, and individual note tuning in playback/export.
Repeats and broader instrument/controller combinations still need tests.

## Findings

| Concern | 4.7 implementation | Implication for 37E |
| --- | --- | --- |
| Default reverb | Shared Aux 1 with Muse Reverb; independent instrument send levels | Add audio routing and saved send state, not a reverb field on every note |
| Send control | AudioOutputParams::auxSends, amount plus enabled flag | Keep separate from ordinary MIDI note events |
| Internal synth effects | FluidSynth reverb/chorus disabled | MIDI CC91 alone cannot control the host's aux reverb |
| LV | Articulation metadata converted to CC66 sostenuto | Generate timed controller NPlayEvents; no new event format is needed |
| Individual tuning | 4.7 collects pitches into tuning tables; current 37E adapter adjusts newly started voices | Preserve the verified voice-based implementation; no reason to replace it solely for similarity |
| Master processing | Aux returns enter mix before master FX and final gain | Keep 37E's Effect A/B after the dry-plus-aux mix |

## 1. Reverb is an audio route, not a note articulation

4.7 routes score events through MPE playback data, EventAudioSource and
FluidSequencer, rather than using 37E's NPlayEvent class. FluidSequencer creates
timed MIDI events; the FluidSynth adapter renders stereo sample buffers. Track
output parameters and their aux sends are a different layer from note events.

The inspected routing order is:

1. Each instrument track has an audio source and MixerChannel.
2. MixerChannel processes the source, its insert FX, then its output gain and
   balance. The send therefore receives the processed, post-fader/post-balance
   track audio, not an unprocessed note or a pre-fader feed.
3. Mixer adds this buffer to the dry output and adds a scaled copy to each
   active aux buffer. Each send is independent; Aux 1 does not feed Aux 2 in
   the inspected loop.
4. Each aux channel processes its summed input through its FX chain and output
   gain/balance, then adds its return to the mix.
5. Master FX process that combined mix, followed by master output gain/balance.

Default Aux 1 contains one Muse Reverb instance shared by the instrument sends.
Explicitly adding reverb as an instrument insert creates a separate processor
for that insert. The native effect is not FluidSynth's own reverb.

For Fluid/SoundFont sources the inspected default send amount is 0.30, in
linear signal amplitude units. This is not a promise of a 30-percent audible
wet/dry ratio: the effect and aux return gains also matter. Empty sends are
initialized when adding a non-metronome track. Aux 2 initially has no effect,
so its configured send does not itself create reverb.

Muse Reverb initializes its dry level at the muted endpoint. That fits an aux
return; a new aux adapter must not inadvertently add another dry copy. When
using an existing 37E effect for the prototype, create an independent wet-only
instance rather than sharing the master Zita object's delay state/settings.

AudioOutputParams stores a vector of AuxSendParams { signalAmount, active }.
ProjectAudioSettings serializes these under auxSends in project audio JSON.
Mixer UI edits update output parameters; they do not rewrite all note events
or embed a CC91 event in every attack.

Evidence anchors:

- `src/framework/audio/common/audiotypes.h`: AuxSendParams, AudioOutputParams.
- `src/playback/internal/playbackconfiguration.cpp`: defaultAuxSendValue.
- `src/playback/internal/playbackcontroller.cpp`: makeReverbOutputParams,
  addTrack send initialization, addAuxTrack.
- `src/project/internal/projectaudiosettings.cpp`: auxSendsFromJson,
  auxSendParamsFromJson, auxSendsToJson, auxSendParamsToJson.
- `src/framework/audio/engine/internal/tracksequence.cpp`: addTrack/addAuxTrack.
- `src/framework/audio/engine/internal/mixerchannel.cpp`: process/completeOutput.
- `src/framework/audio/engine/internal/mixer.cpp`: process,
  writeTrackToAuxBuffers, processAuxChannels, processMasterFx, completeOutput.
- `src/framework/audio/engine/internal/fx/musefxresolver.cpp`: createFxProcessor.
- `src/framework/audio/engine/internal/fx/reverb/reverbprocessor.cpp`:
  ReverbProcessor initialization and output mix.

## 2. LV uses a different controller and timing path

NoteArticulationsParser::parseLaissezVibrer checks that the mark plays and that
the instrument profile supplies an LV pattern. It attaches LV metadata whose
duration reaches the following written measure's end, or the current measure's
end when it is last, with the playback-position offset applied.

FluidSequencer::addNoteEvent emits ordinary note-on/note-off events and examines
articulation metadata separately. LV belongs to SOSTENUTO_PEDAL_CC_SUPPORTED_TYPES,
not to the sustain-pedal set. It schedules CC66-on after 10 percent of the
rendered note duration. Its corresponding off time is that on time plus the
metadata duration. The resulting release is therefore slightly later than the
measure boundary used to construct the metadata; it is not an indefinite hold.

addSostenutoEvents groups requests by MIDI channel and avoids some intervening
off events for overlapping requests. Do not copy this short loop as a complete
overlap specification: test nested durations, equal timestamps and a shorter
last request, and retain the longest required hold. Repeated CC66-on messages
also matter. FluidSynth 2.3.3 updates its sostenuto note-ID threshold on each
on message, allowing later sounding notes to join the held set. Reducing a set
of requests to just one on and one off would change that behavior.

CC66 captures sounding notes on that MIDI channel. It is not automatically
isolated to the one note carrying the LV mark; mixed marked/unmarked chords
and simultaneous voices require explicit comparison with 4.7. Reverb can
extend the audible decay after release but does not establish the LV hold.

4.7's doFlushSound clears note activity, sustain, sostenuto and pitch bend.
37E's Seq stop path explicitly clears sustain and sends all-notes-off, but it
does not explicitly clear sostenuto there. The current adapter's allSoundsOff
clears CC66; that alone does not cover every sequencer/MIDI-output stop path.
Check every allocated channel rather than copying loop bounds mechanically.

37E implementation plan: use ordinary NPlayEvent(ME_CONTROLLER, channel,
CTRL_SOSTENUTO, value), timestamped on the unfolded playback timeline. Generate
the same event map for live playback/export; handle repeats/jumps, selection
playback and seeking into a hold. Add/reset CC66 in common stop/seek handling,
including external MIDI, and define a fallback for the old synth backend.
The notation patch remains independently usable without modern FluidSynth.

Evidence anchors:

- `src/engraving/playback/metaparsers/notearticulationsparser.cpp`:
  parseLaissezVibrer.
- `src/framework/audio/engine/internal/synthesizers/fluidsynth/soundmapping.h`:
  SOSTENUTO_PEDAL_CC_SUPPORTED_TYPES.
- Same directory, `fluidsequencer.cpp`: addNoteEvent/addSostenutoEvents.
- Same directory, `fluidsynth.cpp`: createFluidInstance/processSequence,
  doFlushSound; internal reverb and chorus are disabled there.
- FluidSynth v2.3.3 `src/synth/fluid_synth.c`: SOSTENUTO_SWITCH handling.

## 3. Mapping back to 37E

37E NPlayEvent already represents timed controllers, channel, tuning and note
provenance. Seq::putEvent chooses a synthesizer from score MIDI mappings;
MasterSynthesizer::play passes a PlayEvent to that synth. The current modern
adapter then renders all channels into one stereo mix. Once summed, that mix
cannot supply independent instrument sends.

37E already has Channel reverb/chorus values and initializes CC91/CC93. Its
legacy Fluid voice code also contains separate effect-buffer writes. However,
MasterSynthesizer::process applies Effect A/B to the full mixed output and
reuses those buffers as processing scratch. Those interfaces do not constitute
the post-fader host aux buses described above. The modern adapter forwards
CC91/CC93 but disables internal FluidSynth effects and has no host aux bus yet.

Use an explicit logical instrument/track-to-audio-bus map. Fold auxiliary bend
channels into their source instrument. Do not infer identity solely from pitch,
written staff index or the Note pointer, and do not query mutable score objects
from the audio callback. Supply mapping/control snapshots from the owning
thread and use the same routing setup in live playback and export.

A reverb knob should update persistent mixer/send state and audio-thread
control state, not regenerate thousands of note events. Preserve external MIDI
CC91/CC93 behavior. Do not silently reinterpret old saved MIDI reverb values
as enabled host sends; new sends should default to zero for old scores unless
the user explicitly enables/migrates them. No new NPlayEvent fields are proven
necessary for static send controls. Timed send automation would be a separate
feature and is not supplied by this proposal.

For audio separation, compare two prototypes before choosing:

- 4.7's track-oriented audio-source/synth architecture is a useful model, but
  per-instrument synth instances need memory/load/CPU measurement on older
  systems. Do not transplant the whole MPE/framework stack just to obtain sends.
- FluidSynth's fluid_synth_process supports multiple stereo output groups in
  one instance. In v2.3.3, normal voices route by MIDI channel modulo the audio
  group count. Clear its output buffers before rendering: this API accumulates
  samples, unlike the adapter's current write_float path. Group-count limits,
  channel collisions, and 37E's 256 mapped channels/bend lanes require a real
  test; never assume every MIDI channel maps to a unique output group.

The second prototype may suit the older-system goal, but that is an engineering
hypothesis, not a measured conclusion. Prototype two instruments plus one bend
lane, verify separation, then test resource cost at a realistic score size.

37E evidence: audio/midi/event.h, audio/midi/msynthesizer.cpp,
audio/midi/fluid2/fluid2.cpp, audio/midi/fluid/voice.cpp,
libmscore/instrument.cpp (channel init controllers), mscore/seq.cpp,
mscore/exportaudio.cpp, mscore/mixer/mixerdetails.cpp.

## 4. Next recoverable checkpoints and acceptance criteria

1. LV event generation/reset: tied-chain endpoints; late and early marks in
   chords; overlapping and nested holds; stop/pause/seek/repeat/volta; WAV and
   live output; external MIDI; modern versus old-backend fallback.
2. Controller and instrument compatibility: percussion, patch changes,
   master/individual tuning plus bends, dynamics, channel counts, sample-rate
   recreation and multi-font priority. User tuning checks are recorded as
   passed, not a substitute for the remaining combinations.
3. Routing prototype: two instruments with deliberately different sends; bend
   lane follows its instrument; mute/solo and volume/pan behavior; render export
   using the same map; no allocations or GUI/score access in the audio callback.
4. Shared reverb return: independent wet-only effect instance; preserve master
   A/B; audible tail after notes/rests/end of score; bounded stop/reset behavior;
   silence and clipping checks; zero-send backward compatibility.
5. Mixer UI and persistence: update/send controls without rerendering notes,
   save/reopen, undo where appropriate, and original-backend behavior defined.
6. Optional new reverb algorithm: separate DSP adapter, dependency/platform
   checks, and distribution-license audit. Retain existing effects. Private
   experiments and redistributable builds must be distinguished; no relicensing
   is performed by this document and no GPLv3 DSP source is imported.

The current audit verifies source paths and designs. It does not verify audio
quality, resource budgets, licensing of every dependency, or runtime parity.
