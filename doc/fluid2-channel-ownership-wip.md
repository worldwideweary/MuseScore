# 37E audio v0.13: source-channel ownership handoff

Apply after audio v0.12. The user reported that v0.12 built successfully.
Audio/WAV and large-score startup testing remain explicitly deferred.

Update: v0.14 implements the opt-in two-bank output route and its explicit
voice/font policies. See `fluid2-isolated-banks-wip.md`. The user reported
v0.13 applied; a separate build/test result has not been reported.

## Implemented checkpoint

Live playback already knows the source mixer channel of an isolated bend
lane, but previously only the lane's MIDI channel reached the synth. WAV/other
audio export and the separate MP3 loop also have that source in MidiMapping.
All three dispatch paths now pass it as a separate integer alongside the
original event. Init events identify their own normal source channel.

MasterSynthesizer forwards ownership through a new Synthesizer::playWithSource
hook. Its default implementation calls the existing play method, preserving
the old Fluid and Zerberus implementations. Existing two-argument callers of
MasterSynthesizer::play still work and default ownership to the event channel.
Fluid2 records the metadata only when optional planar audio groups are enabled.
Direct Fluid2::play calls likewise default ownership to their event channel.

The original event, channel, pitch, tuning and controllers are retained. No
additional MIDI messages or score properties are generated. External MIDI
continues using its existing mapping. Export mute/preset selection is retained;
the extra source identifier is audio ownership metadata only.

Ownership is the logical mixer/articulation channel, not a staff number or
MIDI port/channel pair. Linked staves already sharing a playback channel share
that owner. Isolated bend lanes keep distinct MIDI channels/controllers while
inheriting the source mixer channel's eventual send amount. This increment
does not merge different articulation channels of a part into one owner.

## Collision and lifetime rules

Fluid2ChannelRouting uses fixed storage for all 256 MIDI channels. Its group
query returns one source owner only when all registered members of the raw
modulo group agree. Empty groups are Unknown; differing or invalid owners are
Mixed. In particular, channels 0 and 128 must not silently share a send just
because both map to group 0 with 128 audio groups.

Changing an observed channel's owner marks it Mixed until an all-sounds-off
reset. This avoids claiming that release tails from the prior owner belong to
the new one. Channel-specific all-sounds-off clears only that channel;
all-sounds-off(-1), successful font replacement and synth reinitialization clear
the complete table. MIDI CC120 also clears its channel after successful
delivery. All-notes-off retains owners because notes can still have release
tails. The table is conservative: it records observed channel ownership, not
an active-voice census, and Mixed is not itself a playback error.

This is the ownership foundation, not completed instrument isolation. The
current audio buffers are still FluidSynth's raw modulo groups, and no send
gain is applied to them. Their groupOwner check is ready for later routing
validation; it is not used to change audio in this checkpoint. Existing master
Effect A/B and the default stereo renderer are retained.

## Verification

The new standalone `tst_fluid2_routing` checks all 256 source indices, every
group count from 1 to 128, high-numbered bend lanes, shared owners across
groups, same-owner groups, modulo collisions, invalid sources, sticky owner
changes, channel-specific resets and whole-table resets. It joins
`tst_fluid2_audio` in the experimental backend's mtest configuration and in the
standalone CMake project from v0.12. Tests use explicit failures in Release.

Both production-header tests pass under GCC C++11 with strict warnings. The
routing test also passes AddressSanitizer/UndefinedBehaviorSanitizer with leak
detection disabled for the environment's process-inspection restriction. The
existing v0.11 standalone controller/tie harness still passes.

A separate syntax/runtime harness compiles the actual new default hook and
MasterSynthesizer::play function against minimal synth/event stubs. It checks
legacy fallback dispatch once, activation, explicit owner 5 with event channel
255, the default owner, invalid synth indices, and retained event identity and
tuning. This does not compile Qt or the live/export loops. Source inspection
checked the ownership arguments at all five live/export/init call sites.
Full application compilation, Qt mtests and audio integration are pending the
user's build. CMake is unavailable in the patch environment.

When convenient, build and run:

```sh
ctest --output-on-failure -R '^(tst_midi|tst_fluid2_audio|tst_fluid2_routing)$'
```

The audio and large-score checks in `audio-verification.md` remain deferred;
this checkpoint does not mark them accepted.

## Next action

Prototype an explicit output mapping for the entire 256-channel range without
modulo collisions. Two synth banks with at most 128 separate dry outputs each
are a bounded candidate. Preserve the logical ownership table across those
physical outputs so bend lanes inherit their source's sends.

Resolve SoundFont and voice-budget lifetimes before enabling that route:
official FluidSynth v2.3.3 documents that fluid_synth_add_sfont transfers
ownership to the receiving synth. Reusing one loaded SoundFont pointer in two
instances is therefore not a safe sharing implementation. Independently loaded
fonts, the default loader's sample-cache behavior and teardown need evaluation.
Likewise, two instances each configured for 512 voices would change the current
512-voice budget; that must be an explicit tested choice, not an incidental
result of splitting outputs.

Reference inspected locally:
https://github.com/FluidSynth/fluidsynth/blob/v2.3.3/src/synth/fluid_synth.c
(`fluid_synth_add_sfont` and `fluid_synth_remove_sfont`). No source from the
GPL3 Studio reverb implementation is included. A compatible shared reverb
return, saved send controls and mixer UI follow after output isolation.
