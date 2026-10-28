# 37E audio v0.18: mixer row collision and test linkage

Apply after audio v0.17. This is a narrow corrective checkpoint; source sends,
score format and playback behavior are unchanged.

## Corrections

The Audio reverb send widgets were added to row 7 of MixerDetails' main controls
grid, but Track color already spanned all three columns of that row. Move Track
color to row 8, below the new send. A structural check found exactly those three
cell collisions before the edit and no overlapping item spans in any of the
five mixer grid layouts afterwards. Actual widget sizing/appearance remains to
be checked in the application.

The reported tst_midi command linked AWL before Effects. Effects' CompressorGui
uses Awl::Knob, but the effects static target did not declare that dependency.
The newer Audio -> Effects dependency exposes this missing edge in the link
graph. Declare `target_link_libraries(effects awl)` so CMake carries AWL after
its users for tests and other consumers. Do not fix this by changing individual
test link commands or relying on a manually repeated archive in tst_midi.
AWL is already a project target; no new package is required.

## Attached XML interpretation

The user's `result(20261029-220340).xml` records 24 testcase results, all marked
pass, with no failure/error elements. Its `errors="360"` corresponds to 360
qdebug message entries. It does not include `audioReverbChannels`, so it is not
verification of the new test. A failed link cannot generate a fresh test run;
the XML may be from an earlier executable/run. Do not count the v0.17 case as
passed based on this attachment.

## Verification and remaining work

UI XML parsing/span checks and patch whitespace checks passed. The CMake
source dependency was checked against the reported link order and CompressorGui's
actual Awl::Knob usage. No Qt SDK or CMake executable is available here, so a
full application build or generated-link-command verification was not run.
No playback/DSP code changed; native DSP testing was not repeated for this fix.

Reconfigure/rebuild in Qt Creator (Ninja should regenerate CMake after the edit),
then run the new case or full target:

```sh
./tst_midi audioReverbChannels
ctest --output-on-failure -R '^tst_midi$'
```

Confirm Track color appears below Audio reverb send without overlap. Then resume
v0.17's independent send, save/reopen, undo/remapping and playback/export checks.
Keep `MSCORE_FLUID2_REVERB_SEND` removed; retain the modern and isolated-channel
backend options. Earlier audio/export/startup checks remain pending/deferred.
Return controls remain the next feature stage after these corrections are
verified.

Changed files: `effects/CMakeLists.txt`, `mscore/mixer/mixerdetails.ui`,
`doc/audio-verification.md`, and this checkpoint.
