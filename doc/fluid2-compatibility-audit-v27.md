# Audio v0.27: legacy SoundFont choices and comparison scores

Apply after audio v0.26. Its application build, startup, mtests, default font,
installed/user discovery, ordering and load/reload/default behavior are now
accepted. This is the first compatibility-audit correction, not certification
of full legacy equivalence. Large-score/resource checks remain deferred.

Accepted November 5: the user reports mtests pass, both comparison scores play
well, and lower-slot preset selection survives save/reload. v0.28 addresses
controller initialization/reset and bank sequencing; see
`fluid2-controller-compatibility-v28.md` for that checkpoint and remaining limits.

## Confirmed gap and correction

Legacy `Fluid::updatePatchList()` gives each SoundFont a disjoint bank range.
The first font has offset zero; each following offset advances by the preceding
font's largest native bank plus one. `SFont::get_preset()` subtracts the offset
when selecting that font's native preset. Consequently, lower-font copies of
the same instrument remain selectable and their bank numbers are saved in the
score. Mixer also uses the zero-based font-list index to group those choices.

v0.25/v0.26 instead exposed only the winning native bank/program. That hid
lower-font duplicates, broke legacy saved offset-bank selections, and omitted
legacy `Fluid::program_change()` fallback. A failed selection could leave the
previous instrument active. v0.27 restores the legacy bank ranges, font indices,
and every Mixer preset. Native drum bank 128 stays classified as percussion even
when its exposed bank includes a font offset. The first font still owns the
ordinary bank range. As in legacy Fluid, a missing request tries bank zero's
same program, then bank zero/program zero. It does not implicitly choose a
lower font's native bank zero. If neither fallback exists, the old selection
is cleared rather than retained.

The core retains requested host bank/program numbers across sample-rate
replacement; native bank numbers alone lose the lower-font identity. Font
staging/rollback, source-send gains and stopped-time replacement remain intact.
Single-font bank numbers do not change. Experimental multi-font choices saved
using v0.25/v0.26's mapping may need reselection in Mixer; legacy 37E offset-bank
choices use their original meaning again. Reordering fonts changes bank ranges
as it does in the old backend.

## Audit status

| Area | Finding / next check |
| --- | --- |
| Font discovery, defaults, list/state operations | v0.26 application acceptance; retain regression checks |
| Multi-font Mixer choices and saved banks | Confirmed regression corrected here; native PCM, metadata and score-save regression coverage |
| Missing bank/program | Legacy fallback restored here; verify exact native preset and audible result |
| Notes, bend, pressure, ordinary CC dispatch | New adapter has API routes; broad controller parity is not yet certified |
| Pitch-wheel initialization/reset | Legacy channels default to 12 semitones; modern library defaults differ. LV bend lanes explicitly send RPN 12, which does not establish all ordinary bend paths. Next controller checkpoint |
| Default volume/pan/expression/key pressure | Legacy has custom initial values and a forced pan modulator; compare initialization, CC121 and actual PCM before calling equivalent |
| Bank MSB sequencing | Legacy MSB clears the low bank bits; adapter currently retains them until LSB arrives. Ordinary init sends both; isolated-MSB compatibility remains a follow-up |
| Portamento/legato, RPN/NRPN | Upstream handles these controllers, but legacy behavior and renderer assumptions require targeted comparisons |
| Tuning, LV, repeats, stop/seek, WAV, source sends | Earlier user acceptance stands; do not infer untested combinations |
| Reverb/chorus and SoundFont modulators | Host effects retained; compare audible controller/modulator behavior separately from the new independent-send feature |
| Performance and large scores | Explicitly deferred; no speed/memory claim |
| Source bundling and platform packaging | After compatibility checks; current optional system-library adapter remains |

The legacy backend remains selectable. No LV event timing, master-effect
algorithm, notation layout or third-party library source is changed here.

## Listening scores

New scores are in `doc/fixtures/audio-compatibility-v27/` and use ordinary 3.x
XML so the same files can be opened by either backend:

* `A-controllers.mscx`: volume, expression, left/right pan, modulation, sustain
  on/off and program changes. Measure labels describe the expected relative
  change. The modulation response depends on the SoundFont. MIDI actions are
  embedded in the instrument and triggered by staff text; no manual CC entry.
* `B-instruments-percussion.mscx`: piano, flute and GM percussion separately,
  then together; piano-only +50-cent tuning, then normal tuning. Test Mixer solo,
  mute, volume, pan and choosing another preset. Save/reopen after Mixer changes.

Use exactly the same SoundFont files in the same order, master tuning 440,
both master effects set to NoEffect and Audio reverb sends at zero. First run
legacy Fluid (`MSCORE_USE_FLUIDSYNTH2=0`), then restart with the modern backend
(`MSCORE_USE_FLUIDSYNTH2=1`, `MSCORE_FLUID2_ISOLATED_CHANNELS=1`). Compare relative
behavior and routing, not byte-identical audio or equal absolute loudness.
Re-enable normal effects afterward. Test live playback and WAV. Stop during a
held/sustained note, start at a later bar, then replay from the beginning.

For multi-font verification, load two familiar fonts, choose a lower-font sound
in Mixer (including a duplicate of one in the first font), Save to Score,
reopen and play it. Confirm the lower-font choice remains selectable with the
same ordered font list. Repeat for a drum kit if both fonts contain one.

Retain the existing `doc/fixtures/fluid2-note-tuning-v1.mscx` and previously
accepted LV/bend/repeat fixtures; they need not be regenerated. These two new
scores do not yet cover aftertouch, portamento or every reset/RPN combination.

## Automated verification

`tst_fluid2_fontstack` now checks both fonts' actual pitched PCM, native preset
identity, exposed drum-bank mapping, missing-bank/program fallback, rollback,
removed-font fallback and lower-font selection across rate changes, in stereo,
raw-group and isolated modes. `tst_midi::fluid2SoundFontManagement` checks all
Mixer entries, font indices/drum flags, bank-select playback in both synth banks,
XML list persistence and full-score save/reopen of a lower-font bank/program.

`tst_midi::audioCompatibilityScores` loads and renders both listening scores,
checks attack counts, CC/program timing and percussion/tuning content with
either backend build configuration. These tests verify the score inputs;
listening is still required to assess each SoundFont's response.

From the configured main build directory:

```sh
cmake --build . --target tst_midi tst_fluid2_fontstack
ctest --test-dir mtest --output-on-failure -R '^(tst_midi|tst_fluid2_fontstack)$'
```

Full application/CMake/CTest and score rendering remain for the user's build.
Patch-environment checks passed:

* Expanded native font-stack test, including actual PCM, rollback/failure
  injection and three rendering modes, against FluidSynth 2.3.4; also ASan/UBSan
  with dependency leak inspection disabled (runtime libraries not instrumented).
* Existing native banks, lifecycle, SF2/SF3 loading and shared Zita return tests.
* Updated real adapter/core/GUI management and recovery method bodies using the
  existing narrow host/XML/UI bridges, in all three modes. This is not the full
  application or full-score save/reopen check.
* Negative control with the previous adapter fails the five-preset metadata
  assertion, demonstrating the hidden-duplicate regression.
* Full MIDI test source syntax with the backend enabled/disabled, and actual
  adapter/core/GUI syntax with real GUI headers.
* Listening-score XML, action references, exact measure durations and expected
  attack totals checked structurally. The new score-render tests themselves
  still require the user's mtest build; no application rendering/listening was
  performed in this environment.
