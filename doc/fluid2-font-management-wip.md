# 37E audio v0.25: ordered SoundFont management

Update: v0.25 mtests pass, and the user confirms Zerberus playback. v0.26
restores the original discovered-font Add dialog and normal preference/default
lookup after the reported application gaps; see `fluid2-font-discovery-wip.md`.
The v0.25 file-picker verification below records that earlier implementation.

Compatibility correction: v0.27 restores legacy per-font bank offsets and all
Mixer choices, including duplicate programs in lower fonts, plus the old
missing-preset fallback. The effective-preset-only mapping described below is
the v0.25/v0.26 implementation, superseded by that correction. See
`fluid2-compatibility-audit-v27.md`.

Apply after audio v0.24. The user reports that v0.24 builds and mtests pass.
Large-score/resource behavior remains explicitly deferred. This stage adds
the SoundFont list and its controls; it changes no notation or LV event timing.

## Application behavior

The Synthesizer window's Fluid tab now contains a SoundFont list with Add,
Remove, Top, Up, Down and Bottom controls. Add accepts existing SF2/SF3 files,
including multiple selections. Newly added fonts go to the top in selection
order. Full paths are available as tooltips; Remove removes a list entry, not
the file. The top font supplies each matching bank/program, and lower fonts
supply programs absent from fonts above them. Reordering therefore changes
which sound wins. Mixer patch choices include each effective bank/program
once, with its winning font's name/ID, rather than listing overridden choices
which 37E's bank/program channel setting cannot independently select.

Use the existing Save to Score and Set as Default / Load Default controls.
The ordered list uses the existing Fluid synthesizer state group, one
`val id="0"` per file, highest priority first. No new score format is required.
Single-font states still work. Canonical paths identify files; duplicate paths
and symlink aliases collapse to one entry. Legacy basename lookup through the
configured SoundFont search paths remains available. Two different files with
the same basename remain distinct; basename-only removal refuses ambiguity.

Keep the experimental build and existing launch options:

```sh
MSCORE_USE_FLUIDSYNTH2=1 MSCORE_FLUID2_ISOLATED_CHANNELS=1 /path/to/mscore
```

Unset `MSCORE_FLUID2_SOUNDFONT` to edit the list. When that existing diagnostic
override is set, it still forces one font for playback/state loading; the UI
explains why editing is disabled. Add/remove APIs reject edits under the
override instead of reporting an edit that the override would immediately undo.
No new launch option is introduced.

## Loading, failure and lifecycle

All requested fonts are staged and validated in every active synth bank before
old voices/fonts or accepted metadata are changed. A missing, malformed or
no-preset member rejects the entire list. Rollback unloads every staged object,
retaining old presets, held voices, controllers, sends and source ownership in
the core. The highest-priority lookup is explicit and is independent of font
IDs; reverse loading also keeps FluidSynth's own font stack in the same order.

GUI edits first request that SynthControl stop transport, before loading can
take the synth lock. Loading runs on a worker with an application-modal progress
window; list controls cannot start overlapping edits. It waits for completion
even if that progress window is closed, because an upstream font load cannot
be canceled safely. There is no automatic playback restart. Failed GUI edits
show the accepted list again and emit neither a changed-font nor dirty-state
notification. They have already stopped the old performance as part of the GUI
operation. Successful edits refresh the Mixer and mark synthesizer settings
dirty through the existing signals.

A successful list change reloads the complete stack, stops old voices, drains
old dry/send PCM and clears lane ownership. Tuning and send gains remain.
This is not a gapless live preset replacement. The old and new font objects
coexist temporarily; sample-cache sharing remains FluidSynth's responsibility.
Large-font memory/CPU acceptance is not implied by the small fixtures.

v0.24 sample-rate recovery now reloads the full ordered list. A failed rate
change retains that list for retry and mutes mismatched-rate output. Returning
to the retained rate, restoring/reloading the files, or explicitly unloading
still recovers. Its isolated mapping and single/raw-synth physical-channel-255
limitation are unchanged. The bundled Fluid/Zerberus management implementation
and master Effect A/B behavior are unchanged.

## Tests and verification

New native `tst_fluid2_fontstack` uses an original SF2 base and SF3 overlay with
different pitch and an extra program. It checks top priority in actual tuned
PCM, lower-font program lookup, melodic/drum selection, isolated logical 255,
reordering, removal/unload/reload, source sends and full-stack rate changes.
Missing/no-preset fonts at every list position and Linux-injected failures in
every bank/member must retain old held-LV dry/send PCM with no leaked fonts.

New `tst_midi::fluid2SoundFontManagement` has stereo, raw-group and isolated rows.
It checks effective patch metadata, duplicate paths/aliases, ambiguous basenames,
add/remove APIs, rate retention, state XML, real GUI reorder/remove/rollback and
change/dirty/stop-request signals, plus the launch override. The isolated row
also saves/reopens a real score and restores both fonts in order. The existing
scoped test-mode override permits normal synthesizer-state serialization.

Verification actually run in the patch environment:

- Native stack tests and rebuilt banks, SoundFont, lifecycle and shared-return
  tests pass against FluidSynth 2.3.4. The new stack test also passes ASan/UBSan
  on our code, with leak inspection disabled; runtime dependencies are prebuilt
  and are not sanitizer-instrumented.
- A focused Qt 5.15.13 check executes the management regression body against the
  real adapter/core and real headless Qt widgets in all three modes, including
  Add's file picker, reorder/remove, failure recovery and the override. It uses
  a narrow base-GUI constructor/signal-counter bridge. Production synthesizer
  state write/read/group methods are exercised with narrow XML adapters; the
  focused GUI check also passes ASan/UBSan. The full-score save/reopen case still
  requires the user's MTest/application run.
- The new panel was rendered and inspected for fit. Updated MIDI tests compile
  in syntax checks with the backend enabled and disabled; the host stop-request
  connection and both adapter/GUI sources with real base-GUI headers were
  separately syntax checked. Original default SF2 fixture bytes
  remain identical. Full application/CMake/MOC execution was not run here.

From the configured main experimental build directory:

```sh
cmake --build . --target tst_midi tst_fluid2_fontstack tst_fluid2_lifecycle tst_fluid2_soundfonts tst_fluid2_banks tst_fluid2_return
ctest --test-dir mtest --output-on-failure -R '^(tst_midi|tst_fluid2_(fontstack|lifecycle|soundfonts|banks|return))$'
```

Rebuild the application too. In Fluid, add two real fonts and exercise every
reorder control and Remove. Compare an overlapping program and one supplied
only by a lower font. Verify Save to Score/save/reopen and Set as Default/Load
Default retain order, then compare live/WAV output with tuning, LV and independent
sends. Check a failed font addition leaves the accepted list intact, and confirm
editing during playback stops transport before loading. Master A/B remain
independent. MP3 has not been separately accepted; large-score/startup/resource
checks remain deferred. EQ, stereo width, extra master effects, regular slur/tie
refinement and original-synth LV extraction remain later topics.
