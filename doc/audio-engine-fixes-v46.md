# Audio v0.46: first-run selector, original-engine fonts and unsupported sends

Apply after v0.45. The user reports v0.45 mtests pass, but the first Preferences
opening can show duplicated engines and Original Fluid starts without a font.

Engine choices were appended in PreferenceDialog::start(), which may run more
than once on the same dialog. Populate the empty combo once in the constructor;
repeatable setup only binds/loads the saved selection. Restart semantics and
explicit environment overrides are retained.

Original Fluid did not override Synthesizer::loadDefaultSoundFonts(), so the
master's empty-state recovery hook could not load its default. It now requests
MuseScore_General.sf3 when empty, using the existing shared resolver, including
the source-tree FluidR3Mono_GM.sf3 fallback. Saved-font loading also uses that
resolver, honoring existing absolute paths outside the discovery directories.
An already loaded font is preserved. Master recovery still does not replace a
usable Zerberus-only setup. Missing/unreadable default files still fail normally;
this does not download or install fonts.

The Audio reverb send label, slider and spinbox are hidden when the focused
channel's synthesizer does not support source sends. They return for a supported
modern Fluid channel; Zerberus and explicit modern stereo routing also hide them.
Saved channel send values are retained across engine switches.

Verification: a focused harness using rebuilt production original Fluid code
loads a generated default through the General-to-FluidR3 alias, repeats recovery
without duplication, loads an absolute font outside discovery folders and
preserves it on subsequent recovery. Full MIDI mtest header checks cover both
backend configurations. The new audioLegacyDefaultFont mtest covers those same
font recovery/selection cases. Source audit checks no combo population remains
in repeatable start(). Full application mtests/UI and actual installed SF3
playback remain user checks; no full GUI execution is claimed here.

Check a fresh launch on Original Fluid, default font/playback, repeat Preferences
open/close, selecting either engine and restarting, Load Default, saved fonts,
and Mixer send visibility for Fluid2/original/Zerberus. No layout work or deferred
click-fifth/LV extraction changes are included.
