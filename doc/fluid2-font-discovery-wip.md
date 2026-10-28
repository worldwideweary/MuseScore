# 37E audio v0.26: restore SoundFont discovery and defaults

Apply after audio v0.25. The user reports mtests pass but found that Add opened
a generic file picker in the build directory, that normal default loading left
Fluid empty, and that Load Default did not recover it. The user also confirms
basic Zerberus playback; its SFZ chooser and playback implementation are unchanged.
Large-score/resource checks remain explicitly deferred.

Accepted November 5: the user reports mtests pass, application build/startup
works, the default font appears, installed /usr/local and configured user fonts
are found, and ordering, load/reload and defaults work correctly.

## Behavior

Fluid's Add action again opens a **SoundFont Files** list with Load and Cancel,
supporting multiple selection and double-click loading. It uses the existing
Fluid discovery entry point, not the working directory or a file-navigation
dialog. The list includes files from Preferences > General > Folders >
SoundFonts, their visible subfolders, bundled resources and extensions. Paths
are available as tooltips. Active-list canonical deduplication and priority,
atomic loading, sends, tuning and v0.24 rate recovery remain intact.

The shared discovery helper joins the application resource folder with `sound`
through QDir, independent of a trailing separator. Empty preference components
are skipped, avoiding an accidental recursive scan of the current build folder.
Canonical directory visitation handles repeated roots and symlink cycles; file
aliases retain their names for existing basename references. Hidden subfolders
and non-SoundFont extensions are excluded as in the legacy chooser. SoundFont
filename extensions remain case-insensitive.

Experimental development builds additionally search the source `share/sound`
directory after installed, user and extension roots. This needs no installation
or extra font copy. General is preferred by its ordinary basename; if it is
absent, the bundled `FluidR3Mono_GM.sf3` placeholder can supply the default,
matching the repository's installation fallback. Missing arbitrary requested
fonts are still errors; they do not silently become the default font.

Startup and Load Default can now recover the normal Fluid default when no
synthesizer has any font loaded, including an empty saved-default configuration.
An empty saved font group recovers without resetting its master/reverb settings.
An existing SFZ-only configuration prevents that fallback; it does not
force Fluid on top of a working Zerberus setup. The optional backend recovery
hook does nothing for legacy Fluid/Zerberus themselves. Explicitly removing all
Fluid fonts still unloads them during the session; a completely fontless saved
default will recover the normal Fluid font at the next startup/Load Default.

Keep only the existing `MSCORE_USE_FLUIDSYNTH2=1` and
`MSCORE_FLUID2_ISOLATED_CHANNELS=1` switches for normal testing. No additional
environment setting is required. The optional `MSCORE_FLUID2_SOUNDFONT` path
override remains supported and is not enabled by this patch.

## Verification

New MIDI cases cover shared roots with/without a trailing separator, preference
subdirectories, extension/build roots, aliases/cycles, hidden files and empty
folder components; default-name/placeholder resolution; startup without stored
settings and with an empty saved Fluid group; preserving reverb settings and an
existing SFZ configuration; and Load/Cancel in the discovered-font GUI.

Focused checks run in the patch environment:

- The actual discovery/resolve code, Fluid2 adapter/core, production Master
  init/setState/ensure/has-font method bodies and state XML methods pass against
  Qt 5.15.13 and native FluidSynth 2.3.4. Host preferences/extensions/effects and
  base-GUI signals use narrow test bridges; this is not the full application.
- Real headless Qt list-dialog selection and Cancel pass; the dialog was rendered
  and inspected. The real bundled 197-preset SF3 loads via General's fallback
  name and produces audio in isolated mode. The prior v0.25 loader fails this
  bundled-default regression with the same discovered files.
- The focused default/dialog check also passes ASan/UBSan on our code, with leak
  inspection disabled; prebuilt runtime dependencies are not instrumented.
- Earlier management/XML and sample-rate recovery method bodies pass in stereo,
  raw-group and isolated modes. Complete changed Fluid/master sources, adapter/
  GUI sources and MIDI tests pass syntax checks, including MIDI with the backend
  disabled. Full application/CMake/MOC/MTest execution remains for the user's build.

Rebuild the application normally, then from the configured main build directory:

```sh
cmake --build . --target tst_midi
ctest --test-dir mtest --output-on-failure -R '^tst_midi$'
```

On restart, confirm Fluid has its normal default. Open Add and check fonts from
the configured SoundFonts folder and subfolders appear with Load/Cancel. Load a
font, cancel another selection, and recheck order and saved defaults. Confirm
existing Zerberus playback/SFZ selection remains normal. The source fallback is
only a development convenience; installed/user fonts retain priority.
