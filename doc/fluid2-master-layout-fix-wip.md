# 37E audio v0.19: stable MasterSynthesizer allocation layout

Apply after audio v0.18. The user verified the mixer label is on the correct row.
The MIDI executable now aborts with `malloc(): invalid next size (unsorted)` and
an empty result XML. This checkpoint fixes a definite startup heap-corruption
path; the user build still needs to confirm the full test run afterwards.

## Cause

v0.16 put the optional auxiliary buffers and reverb state directly inside the
public MasterSynthesizer class under MSCORE_EXPERIMENTAL_FLUIDSYNTH2. The Audio
target exports that define, but the ordinary testutils static target links only
Freetype and does not receive it. MTest::initMTest() allocates MasterSynthesizer
from testutils; its out-of-line constructor is supplied by Audio. The allocator
and constructor therefore disagreed about the object size. The constructor can
write its enabled-build fields past the smaller allocation before any MIDI case
runs. Rebuilding the same sources without correcting the layout cannot fix that
configuration mismatch.

A regression using the actual public header with a narrow Qt layout bridge
measured 131240 versus 65680 bytes before the fix: a 65560-byte disagreement.
A separate mixed-define allocator/production-constructor harness reproduced an
AddressSanitizer heap-buffer-overflow with the pre-fix source. These values are
from the bridge build, not measurements of the user's Qt executable. This is a
concrete defect matching the reported allocator failure; no user backtrace was
available to independently establish the abort site.

## Correction

Move the optional state into a private implementation structure owned by one
unconditional unique_ptr in MasterSynthesizer. Its public header no longer has
backend-conditional fields or includes. Allocators agree regardless of whether
the backend define is present in their translation unit. The constructor,
implementation state and destructor are compiled together in Audio. Reverb
ownership also uses unique_ptr. Nonexperimental builds leave the state empty;
they do not allocate the auxiliary buffers.

The source-send, return DSP, gain and reset behavior is unchanged. This fixes all
consumers of the header, rather than supplying an extra define only to tst_midi
or relying on every future allocator inheriting an Audio build option.

## Verification actually run

- Added tst_master_layout: separate translation units explicitly enable/disable
  the backend define and compare sizeof/alignof of the actual production class.
  It fails on v0.18 and passes after this change (65696 bytes in both bridge TUs).
- Strict C++11, AddressSanitizer and UndefinedBehaviorSanitizer checks passed for
  the fixed layout regression.
- Focused harness compiled the production constructor, destructor, auxiliary
  implementation structure, reset and gain conversion with narrow Qt/effect
  doubles. Allocating from a TU without the define and constructing from one
  with it passes 100 construction/reset/destruction cycles under ASan/UBSan.
  The reverse define combination passes too. The pre-fix version reproduced
  an ASan heap-buffer-overflow.
- Source review confirms no backend-conditional fields remain in the public
  MasterSynthesizer header. Whitespace checks passed. No DSP testing was repeated
  for this ownership/layout change.

The maintained layout test uses real Qt Core in the full application mtest build.
Standalone builds without a Qt target use the small layout-only bridge. Neither
local harness is a full Qt startup/MIDI test. Qt SDK/CMake execution remains
unavailable here; leak inspection is disabled under the environment's process
inspection restriction.

## Rebuild and next action

Rebuild both targets so Audio, testutils and test consumers all see the new
header. From the configured build directory:

```sh
cmake --build . --target tst_midi tst_master_layout
ctest --output-on-failure -R '^(tst_midi|tst_master_layout)$'
```

For readable output independent of the XML reporter, the new case can also be
run directly from its executable directory:

```sh
./tst_midi audioReverbChannels -v1
```

The confirmed mixer-row fix remains accepted. Saved sends, undo/remapping,
independent live/export levels and earlier deferred audio/startup checks remain
pending until actually reported. Do not advance return-control features until
the startup/MIDI regression is resolved on the user build.

Changed files: audio/midi/msynthesizer.h/.cpp, mtest/audio/fluid2/CMakeLists.txt,
the new layout test and its two translation units/protocol header/standalone
bridge headers, doc/audio-verification.md and this checkpoint.
