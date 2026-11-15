MuseScore Visual Tests
======================

The shell script "gen" creates a subdirectory `html` with the
HTML file `vtest.html` for visual compare including all needed
image files.
Travis generates this too and then uploads to http://vtest.musescore.org/index.html

Quick command reference (Linux, v0.17.1)
---
From the 37E source root, export executable paths once per terminal session.
An exported reference path **does not launch** that renderer without its flag.

    ./vtest/gen --help                              Short command reminder
    ./vtest/gen tie-1 tie-2                         Current 37E + available bundled reference PNGs
    ./vtest/gen --modern tie-1 tie-2                Add cached Studio 4.7.x
    ./vtest/gen --historical tie-1                  Compare directly with historical 3.6.2
    ./vtest/gen --modern --historical tie-1         Compare all three executables
    ./vtest/gen --all                               All local .mscx fixtures, 37E only + bundled refs
    ./vtest/gen --all --modern                      Entire catalogue against Studio (cached)
    ./vtest/gen --score "/path/to/Score.mscz"      One external .mscx or .mscz
    ./vtest/gen --modern --score "/path/Score.mscz" External score against Studio
    ./vtest/gen --score-dir "/path/Scores"          All .mscx/.mscz in a directory
    ./vtest/gen --modern --score-dir "/path/Scores" Compare directory against Studio
    ./vtest/gen --score-dir "/path/Scores" --recursive  Include subdirectories
    VTEST_CAPTURE_BASELINE=1 ./vtest/gen tie-1       Deliberately record a Before snapshot
    VTEST_PROFILE_MODE=raw ./vtest/gen tie-1         Keep original score style; no injected modern beams
    VTEST_BEAM_AB=1 ./vtest/gen --modern beams-1     Explicit 37E legacy beam A/B diagnostic
    VTEST_REFRESH_REFERENCES=1 ./vtest/gen --modern tie-1   Force Studio re-render

Versioned Studio 4.x catalogue archives and isolated 37E diagnostics (v0.17)
---
Studio's much larger `vtest/scores/` and `vtest/scores_small/` collections
contain newer and sometimes unsupported score elements. The catalogue tool is
**separate** from the ordinary `gen --modern ...` workflow: it never filters
out scores merely because they might be incompatible with 37E.

Create a Git-trackable reference archive once (no 37E executable needed):

    VTEST_REFERENCE_MSCORE=/path/to/studio/mscore \
      ./vtest/gen --cache-modern --score-dir /path/to/Studio/vtest --recursive \
      --studio-source-tag v4.7.7 --jobs 4

Every `.mscx` and `.mscz` below the selected directory is attempted.
The renderer is started once per score; success requires an exit code of 0
and validated consecutive PNG pages. Individual failures and timeouts are
recorded and do not abort the remaining catalogue. The default per-score
timeout is 180 seconds (`--timeout 300` raises it).

Images are stored under:

    vtest/references/studio-4.7.7-<binary-sha-prefix>/
      manifest.json
      scores/<stable-relative-score-ID>/cache.json
      scores/<stable-relative-score-ID>/<ID>-1.png
      ...

The directory name includes the parsed binary-reported version (or the
explicit user-supplied source tag) and a SHA-256 prefix of the actual binary.
`manifest.json` records the raw binary-reported version, full executable SHA-256,
optional *user-supplied/unverified* source tag, export DPI, source-fixture
repository revision if Git is available, score-relative filenames, original
and prepared-score hashes, PNG hashes/dimensions and failed-score statuses.
The binary-reported version is not proof of its build's source tag.
Changing `VTEST_REFERENCE_CACHE_SALT` creates a separate archive generation
when fonts or external resources change without changing executable bytes.

The archiver saves each validated score immediately. Rerunning the same
command reuses matching entries; it retries failed entries, and successful
cache hits do not rewrite the versioned manifest. Force regeneration with:

    VTEST_REFRESH_REFERENCES=1 ./vtest/gen --cache-modern \
      --score-dir /path/to/Studio/vtest --recursive \
      --studio-source-tag v4.7.7

After reviewing the size and contents, commit the archive *explicitly*:

    du -sh vtest/references/studio-*
    git add vtest/references/studio-4.7.7-<binary-sha-prefix>

The archive is in the source tree intentionally and is NOT under `html/`.
Hundreds of 130-DPI PNGs can add substantial repository size. Consider Git LFS
or a versioned artifact store before including very large sets in a shared Git
history. Nothing is staged or committed automatically.

Audit 37E against a previously archived Studio reference collection. This
command **does not launch Studio**, so a clone with the committed archive
needs only a built 37E executable:

    VTEST_MSCORE=/path/to/37e/main/mscore \
      ./vtest/gen --audit-modern --score-dir /path/to/Studio/vtest --recursive --jobs 4

If multiple Studio archives are present, select one explicitly:

    ./vtest/gen --audit-modern --archive vtest/references/studio-4.7.7-HASH \
      --score-dir /path/to/Studio/vtest --recursive

For a small exploratory run, append a filename regular expression:

    ./vtest/gen --audit-modern --score-dir /path/to/Studio/vtest \
      --recursive --match '(^|/)(tie|slurs|beams)'

The audit starts a **separate 37E subprocess for each score**, supplying
`-f` / `--force` so version-4 scores are not rejected solely due to their
newer file format. This applies ONLY to `--audit-modern`, not ordinary GUI
opening or the bundled `gen` score tests. It logs exit status and output
(`vtest/html/catalogue-logs/`) and continues after errors.

`--jobs N` allows N independent MuseScore processes in parallel for both
`--cache-modern` and `--audit-modern`; default is 1. Start with `--jobs 2`
or `--jobs 4` and account for RAM. One job JSON and temporary directory is
used per process; the main process alone writes catalogue manifests and
final HTML. Score order in the report remains stable.

Completed **successful 37E audit renders** are now cached under
`vtest/html/37e-cache/`, independently of the Git-trackable Studio archive.
The cache validates the 37E executable's SHA-256, exact prepared score input
SHA-256, output PNG SHA-256/dimensions, DPI and the forced-import policy.
If 37E is rebuilt or a score changes, only its affected cache entries are
invalidated. Failed scores are retried on each audit. To force all 37E
scores to re-render even when valid cached images exist, use:

    ./vtest/gen --audit-modern --score-dir /path/to/Studio/vtest \
      --recursive --jobs 4 --refresh-current

`VTEST_REFRESH_37E=1` is an equivalent environment switch. Use
`VTEST_37E_CACHE_SALT` to invalidate current renders after changing external
fonts/resources without rebuilding. The versioned Studio archive is never
modified by 37E audits.

`--audit-modern` also **opens engraving.html automatically** with the browser
selected by `VTEST_BROWSER` (default follows ordinary `gen`: `firefox` on
Linux, `open` on macOS). Set `VTEST_BROWSER=xdg-open` for the system default
browser, or `VTEST_OPEN_REPORT=0` for CI/headless use.

A 37E failure with a valid Studio reference shows the Studio image instead of
pretending that no difference exists. A changed or corrupted archived reference
is marked *stale/unavailable*, never compared as if valid. The audit writes
`vtest/html/catalogue-audit.json` and the same expandable
`vtest/html/engraving.html` as other comparison modes. Unsupported elements may
be silently omitted even when rendering succeeds; visual inspection is still
necessary. The audit uses stable **score-directory-relative** identifiers so
moving a fixture checkout does not break its reference identity.

The cache-only HTML index is `vtest/html/studio-catalogue.html` and full
renderer logs are local under `vtest/html/catalogue-logs/` (not part of the
Git-tracked archive). Failed scores keep their concise diagnostic and status
in the committed manifest.

The earlier `./vtest/gen --modern tie-1` still uses its v0.15 executable-based
reference renderer; the new `--audit-modern` consumes archived Studio PNGs.
These are deliberately separate until we unify named-fixture archive lookup.

For normal tests, 37E inputs get `beamLayout4=1` even if that property is
absent from the original fixture. Raw mode preserves the original input bytes.
A recorded 37E *Before* snapshot is used when the current input hash matches.
Neither capturing a baseline nor running a comparison modifies score fixtures.

Output is always a unified `vtest/html/engraving.html`. **Expand all scores**
and **Collapse all scores** toggle top-level score groups; individual summary
links open the corresponding score. When no reference PNG exists, the report
shows the current 37E page and a matching Before snapshot if one exists,
without inventing a reference comparison percentage.

In the Tests summary, multi-page scores repeat the filename on each linked
page row instead of using a separate gray score/category header. Catalogue
audits count page-count differences by score (not by repeated HTML notices),
and distinguish 37E-extra pages, Studio-extra pages and unavailable references.
A different page count is not itself a renderer failure; compare the first
pages with care because the musical content may have reflowed.

Both Studio and historical PNG references are cached under
`vtest/html/reference-cache/`. Successful scores are cached **immediately**,
so an interrupted `--all --modern` run can resume without re-rendering its
completed Studio pages. The full comparison report is published only after
successful completion. The ordinary 37E `gen` renderer still runs every time; catalogue audits now cache successful 37E renders.


Requirements
---
In order to generate the diff between the reference
file and the generated one, *Image Magick* needs to be
installed and "compare" should be in the `PATH`.

Add a new test
---
For a Studio-reference comparison, place `my-fixture.mscx` in this directory
and run `./gen --modern my-fixture` with both `VTEST_MSCORE` and
`VTEST_REFERENCE_MSCORE` configured (see below). The SAME fixture is passed to both
renderers. The Studio PNG reference is generated automatically; there is no
need to create or commit a `my-fixture-ref.png` image. Start with a score that
both programs can load, and ensure the intended layout is reproducible.

For bundled historical PNG comparisons, a new fixture requires a
`my-fixture-ref.png`, generated from the desired historical build at 130 DPI.
Without one, the unified report shows the current 37E page (and a recorded
Before snapshot if present). Use `--historical` for an executable-based comparison.
Add a permanent test to the default `SRC` list in `gen` (and `gen.bat` for
Windows) if it should run whenever `./gen` is invoked without arguments.
You can test individual fixtures without editing the list; multiple fixtures
can be passed as separate arguments, for example `./gen tie-1 tie-2 my-fixture`.

How to use on Windows
---
- Install *Image Magick*, add it to PATH
- Run `gen.bat`. It will use msvc.install as a default folder to search MuseScore.exe. If you use mingw build, specify the path to install folder manually:
        `gen.bat win32install`


Optional engraving comparison (Linux)
---
The original `gen` comparison counts exact PNG differences. A build that
renders opaque colored page backgrounds compared with a transparent reference
can therefore appear to differ across nearly every pixel.

To add a separate background-normalized report (requires Python 3 and Pillow,
available as the `python3-pil` package on Debian-based distributions):

    VTEST_ENGRAVING_REPORT=1 VTEST_MSCORE=/path/to/build/main/mscore ./gen tie-1
    xdg-open html/engraving.html

Alternatively, use `python3 compare-engraving.py tie-1` after running `./gen`.
The comparison converts both images to grayscale ink on white; it retains
an absolute layout comparison and optionally adds an *aligned* view by
translating the current image until the first detected long staff line starts
at the reference's first-staff position. No resizing or stretching is done.
A translation does NOT remove differences in staff width, note spacing, or
engraving, and is reported numerically. Always inspect absolute differences.
You can adjust the antialiasing tolerance with `--tolerance 5` (default 5).
The tool does not modify any `*-ref.png` source images. It writes only
`html/*-normalized-*.png`, `html/*-aligned-*.png`, and `html/engraving.html`.
The normal `gen` HTML report remains available at `html/vtest.html`.

MuseScore Studio as the reference (Linux; experimental)
---
To compare the same 3.x vtest score in the current 37E build and a compiled
MuseScore Studio build (such as release tag v4.7.7), set both executable paths:

    VTEST_MSCORE=/path/to/37E/main/mscore \
    VTEST_REFERENCE_MSCORE=/path/to/studio/executable \
    ./gen --modern tie-1

The command renders both programs at the historical 130 DPI, using the same
`vtest/tie-1.mscx` input (NOT Studio's own scores). It automatically opens the
normalized `html/engraving.html` report with the renderer-reported Studio version as reference.
The original `html/vtest.html` also shows the Studio reference for that run.

Studio's exported PNGs are retained under `html/reference-studio/` (all
pages). The first page is copied to the generated `html/tie-1-ref.png` for
comparison. **Tracked `vtest/tie-1-ref.png` and every other historical
reference are untouched.** The Studio subprocess log is `html/LOG-studio`.
Each invocation with `--modern` verifies and reuses cached references
when their inputs and renderer identity are unchanged. For multiple fixtures, the renderer starts a fresh Studio process
for each cache miss (a single-item `-j` batch) to avoid Studio 4.7.7's debug-build
async assertion when converting several scores in one process. `LOG-studio`
contains a separate labeled section for each score. Images and the manifest
are published only after every requested Studio conversion succeeds.
An exported reference path alone never invokes Studio.

The generated `html/reference-studio/manifest.json` records the executable
path, its raw `--long-version` (or `--version`) output, the parsed version when
available, the selected DPI, SHA-256 of every shared input `.mscx`, and the
SHA-256 and dimensions of every exported PNG page. The HTML report uses the
manifest to label the reference automatically and verifies that the page-1
reference image matches its recorded hash. The version reported by the binary
is shown verbatim, including any "Unstable Prerelease" wording even if the
source checkout was built from a release tag. This records the binary's
claims; it does not independently certify the source tag or build revision.

The v0.3 `html/reference-475/` directory and `html/LOG-475` may remain from
older runs but are not used by this version. They can be deleted manually
if no longer needed.

Differences in notation fonts, style-version migration, and score compatibility
may remain. The report normalizes backgrounds and offers an optional staff
translation, but neither mode treats differences as proof of a layout bug.
If the image dimensions differ, both are placed at their original top-left
positions on white canvases without resizing.

Directional engraving differences (v0.5)
---
The main HTML panels now show a high-contrast *shape comparison*. Red pixels
are strong ink found only in the current build, blue pixels are strong ink
found only in the Studio (or historical) reference, and dark gray pixels are
strong ink shared by both. The default cutoff is grayscale 190/255 (where
0 is black); customize it with `--ink-cutoff 190` when running
`compare-engraving.py` directly. This is a binary diagnostic view, not a
replacement for the fine-grained pixel-difference percentage.

Both absolute and first-staff-aligned shape comparisons are shown. A collapsible
section retains the original red difference masks and new directional fine-pixel
views (red when current is darker, blue when reference is darker by more than
`--tolerance`, default 5). The fine-pixel view includes antialiasing variations
that the stronger ink view may suppress. The alignment remains purely diagnostic
and should not be used alone to judge layout parity.

A button can magnify every panel to 2× within a scrollable viewport; click an
image to open the original PNG at full resolution. When multiple tests are
requested, the report includes a linked summary table.

First-system indentation diagnostic (v0.7)
---
The engraving report also lists the first detected staff-line extent and
its start-position difference (in pixels and estimated staff spaces). This is
an image measurement, not a claim about which style property a renderer used.
An exact 5-sp difference is a useful clue when investigating first-system
indentation, but its cause still needs controlled rendering or source tracing.

Two additional experimental scores are provided, based on the same converted
3.02 `tie-1-emmentaler` score with identical music notation and explicit
Emmentaler/MScore Text fonts:

    ./gen tie-1-emmentaler-indent-on tie-1-emmentaler-indent-off

The `-on` version explicitly sets `enableIndentationOnFirstSystem` to 1 and
`firstSystemIndentationValue` to 5 sp. The `-off` version sets it to 0 while
retaining the same 5-sp value. This checks whether both renderers honor the
same saved first-system setting and isolates legacy-score migration behavior.
The original `tie-1.mscx` and the historical reference PNG remain unchanged.
Requires setting VTEST_MSCORE and VTEST_REFERENCE_MSCORE as described above.

Recorded 37E baseline and multi-page reports (v0.8)
---
Normal use remains one command from the **source root**, with both executables
already exported in the shell:

    ./vtest/gen tie-1 tie-2

The comparison now examines every page produced by both renderers, not just
page 1. Full-page and multi-page fixtures use the same command and need no
special score size or registration options. Each page is compared separately;
if 37E and Studio produce different page counts, the report explicitly lists
unmatched pages instead of silently discarding them. Absolute page coordinates
are preserved. Staff-aligned views are diagnostic and may not be meaningful
for every page. The original `vtest.html` remains a legacy, page-1-only view.
The historical `*-ref.png` fixture collection is also page-1-only; for
multi-page scores use the Studio reference renderer.

To record the current 37E output as the **before** baseline, do so deliberately
once, for the selected fixtures:

    VTEST_CAPTURE_BASELINE=1 ./vtest/gen tie-1 tie-2

Normal `./vtest/gen tie-1 tie-2` invocations then display before/current 37E
and current/Studio 4.7.7 comparisons, plus a before/Studio metric when a
matching baseline exists. Capturing the same fixture again intentionally
replaces *its* snapshot; other recorded fixtures are preserved. Nothing under
`vtest/html/` or the tracked historical `*-ref.png` files is used as a baseline.

Snapshots and their SHA-256 manifest are written to `vtest/baselines/37e/` and
ignored by Git by default: they are local worktree data, not automatically
committed to the score catalogue. Copy them somewhere safe if desired. The
manifest records the original score hash, 130 DPI, current renderer path,
capture time, page count, PNG dimensions and PNG hashes. If the source score
changes after capture, the report flags the baseline as incomparable rather
than presenting an engraving regression as though the input were unchanged.
These PNG comparisons are useful diagnostics, not automatic layout correctness
assertions. The exporter may write a different number of pages after a layout
change; unmatched pages are shown as such rather than assigned a percentage.

Existing source `.mscx` fixtures are never rewritten. Starting with v0.9,
opt-in style profiles may generate separate temporary inputs under `html/`.


Automatic modern comparison defaults and profiles (v0.12)
---
The normal Studio comparison needs no special options:

    ./vtest/gen --modern tie-1 tie-2 beams-1 slurs-1

For **every** score, including external `--score` inputs, the 37E renderer
receives `beamLayout4=1` automatically. Its score's other musical/style data
remain unchanged unless an explicitly assigned named profile sets them.
Studio receives the same original/common style information **without** the
37E-only beam setting. Optional MuseScore 3.6.2 receives the original file,
not either prepared copy. Canonical input scores are never rewritten.

The setting is defined once in `vtest/engraving-profiles.json` under
`studio_comparison_37e_overrides`. Existing named profiles such as
`legacy-emmentaler` and `modern-beams` are still available for controlled
font/indentation experiments. The default named profile is still `original`:
that now means *original style plus the Studio-comparison 37E beam override*.
It does **not** mean an unmodified 37E input.

The generated inputs, version, profile, and input hashes are recorded in
`vtest/html/input-manifest.json`. An existing 37E baseline with a different
prepared-input hash is correctly marked incomparable; to replace one,
explicitly use `VTEST_CAPTURE_BASELINE=1` once after deciding to accept the
new modern-beam default.

To compare an untouched historical fixture exactly as saved, disable all
profiles and modern beam overrides for that invocation:

    VTEST_PROFILE_MODE=raw ./vtest/gen tie-1

To compare an external `.mscx` without moving it into the catalogue:

    ./vtest/gen --score "/absolute/path/to/Score.mscx"

The external source file stays untouched. In a normal Studio comparison, its
37E *prepared copy* receives `beamLayout4=1` automatically. Use
`VTEST_PROFILE_MODE=raw` with `--score` to disable this as well.

Beam-layout A/B demonstration (explicit opt-in, v0.12)
---
The normal 37E input uses modern `beamLayout4=1`. Studio comparison
is enabled explicitly with `--modern`. To investigate the difference from
37E's legacy beaming, run:

    VTEST_BEAM_AB=1 ./vtest/gen --modern beams-1

This adds a legacy `beamLayout4=0` 37E rendering for **each requested score**
without replacing the main modern rendering or the Studio reference. The
report compares legacy versus modern and each with Studio. You can also run:

    VTEST_BEAM_AB=1 ./vtest/gen --modern slurs-1
    VTEST_BEAM_AB=1 ./vtest/gen --modern --score "/path/to/Score.mscx"

A/B is disabled in raw mode and when `--modern` is not requested. Former v0.10
`beam_layout_ab` fixture lists are no longer used; there is no need to edit a
catalogue list for an A/B demonstration. On multi-page scores, A/B
percentages are suppressed if the renderers produce different page counts.
A lower pixel-disagreement percentage alone does not establish correct
music engraving.


Optional independent historical executable (v0.11)
---
This is separate from the *committed historical `*-ref.png` files*. To compare
current 37E, Studio 4.7.7, and a locally available historical MuseScore such
as the 3.6.2 Linux AppImage **in one run**, export a third path once:

    export VTEST_HISTORICAL_MSCORE="/path/to/MuseScore-3.6.2-x86_64.AppImage"
    ./vtest/gen --modern --historical tie-1 beams-1

Use the same `VTEST_MSCORE` and `VTEST_REFERENCE_MSCORE` exports as before.
An AppImage may be invoked as a normal executable once it is executable:
`chmod +x /path/to/MuseScore-3.6.2-x86_64.AppImage`. An AppImage that requires
unavailable FUSE support must first be made runnable independently of vtest.
When both flags are present, the historical executable does **not** replace
the Studio reference or change beam A/B tests. An exported path alone does not activate it: ordinary calls
omit the third renderer. To omit it, simply run `./vtest/gen tie-1`.

The historical renderer can now run independently of Studio:
`./vtest/gen --historical tie-1`. It becomes the primary reference when used
alone, while `--modern --historical` produces the three-renderer comparison.
This feature supports any executable implementing the historical MuseScore `-j` JSON batch conversion
and `-r` PNG DPI options. Actual historical AppImage behavior should be
verified with the selected AppImage; the script does not install it.

**Input policy:** the historical renderer receives the **original MSCX/MSCZ fixture**
without 37E or Studio engraving-profile overrides. This intentionally tests
historical score-loading behavior, but is **not** a font/indentation-controlled
A/B comparison whenever the other renderers use a modern profile. The report
records all input hashes and warns when pagination differs. In particular,
reflowed pages must not be treated as direct tie/beam-shape comparisons.

All historical PNG pages live under `vtest/html/reference-historical/` with
`manifest.json` recording their input hashes, renderer version, executable,
and page hashes. `vtest/html/LOG-historical` contains the per-score logs. The
historical renderer starts a separate process for every score. Failed runs
remove the active historical manifest and do not publish a partial new one;
tracked score files and legacy `*-ref.png` files are never modified.

External `.mscx` and `.mscz` scores work with `--score` and the third renderer,
provided all selected executables support the score format.

**For CI later:** a downloaded AppImage or archived PNGs could serve as the
historical source. Studio is not assumed to be available in CI. The current
three-renderer mode remains a local diagnostic until CI dependencies and
reference provenance are configured explicitly.

Reference caching and unified bulk reports (v0.13)
---
The no-argument invocation keeps the original fixed historical catalogue
and runs 37E without launching the optional reference executables. Use
`./vtest/gen --all` to discover **every top-level `.mscx`** in `vtest/`
(including scores added since that list was written). `--all` assumes the
normal catalogue-safe filenames without whitespace; `--score` also accepts
external `.mscx` and `.mscz` files.

A Studio or opt-in historical PNG set is stored in the disposable cache at
`vtest/html/reference-cache/{studio,historical}/`. A subsequent run reuses a
score's reference only when its exact prepared/source score hash, executable
binary SHA-256, export DPI and optional `VTEST_REFERENCE_CACHE_SALT` match,
and all cached PNGs pass SHA-256 and image-size checks. A failed validation
forces re-rendering; cache files are never a baseline. Changing shared
libraries, font installations, Qt platform settings or other external assets
is not detectable from the executable SHA alone. Change the salt when those
rendering resources change, or explicitly force refresh:

    VTEST_REFRESH_REFERENCES=1 ./vtest/gen --modern tie-1 tie-2
    VTEST_REFERENCE_CACHE_SALT=fonts2026 ./vtest/gen --modern tie-1

Studio reference generation still uses a separate process for each **cache
miss**, avoiding the observed multi-score callback assertion. Every successfully rendered score is cached before proceeding to the next,
even when a later score fails or the run is interrupted. Full report manifests
are still published only after all selected scores succeed. The 37E
primary output is **always rendered afresh** to test current changes;
`VTEST_CAPTURE_BASELINE=1` is the only way to replace a recorded 37E
before-change baseline. Do not confuse the disposable cache with baselines.

All comparisons continue to appear in **one `vtest/html/engraving.html`**.
The existing top summary links to individual pages. With more than ten
scores, per-score sections start collapsed for usability; small sets start
expanded. Clicking a summary-table link opens the correct score section.
There is no split-report requirement or extra report-mode switch in v0.13.
Optional split output and measure-aware reflow comparisons remain future work.

Explicit renderer selection and resumable caches (v0.14)
---
`--modern` and `--historical` select additional executables. Both switches
work independently and can be combined; environment variables supply only
their executable paths. The default run does not start either reference.
`--all` without reference flags also works for fixtures missing the original
tracked `*-ref.png`: those appear as current-only pages in the unified report,
with compatible Before/current results where available. The comparison does
not fabricate a Studio result. Studio still uses one subprocess per cache miss
to avoid the historical multi-score crash. A completed score is cached
immediately and survives interruption of subsequent rendering.

External score directories and compressed scores (v0.15)
---
From the 37E source root, choose one score or all supported files in another
folder. This does not import scores into Git or modify any source file:

    ./vtest/gen --score "/home/user/Scores/My work.mscz"
    ./vtest/gen --modern --score-dir "/home/user/Scores"
    ./vtest/gen --modern --historical --score-dir "/home/user/Scores" --recursive

`--score-dir` selects top-level `.mscx` and `.mscz` scores, in sorted path order.
Add `--recursive` to include subdirectories. This option cannot be combined
with `--all`, named catalogue tests, or `--score` in the same invocation.
Names with spaces and repeated basenames are handled using stable IDs derived
from each score's absolute path. The report records the human-readable path.
The generated `input-manifest.json` records exact paths and SHA-256 hashes;
reference caching continues to use each selected renderer's effective input.

For `.mscz`, the score XML is updated **inside a temporary archive**, retaining
embedded images and other ZIP members. The original archive is not rewritten.
37E gets its usual modern-beam setting (`beamLayout4=1`), Studio receives the
same score without 37E-only XML properties, and the historical executable
receives the original compressed score. `VTEST_PROFILE_MODE=raw` passes the
untouched original archive to both 37E and Studio. MuseScore files requiring
features unsupported by a particular renderer may still fail to import, and
such errors are reported rather than silently converted.

A `.mscz` must be a valid MuseScore ZIP archive with a readable `.mscx` score
entry, either identified by `META-INF/container.xml` or unambiguously present.
If the archive contains external media, vtest does not remove it from the
prepared copies. For scores with different measure/system pagination between
engines, page-wide differences remain layout diagnostics, not equivalent
measure or tie-shape comparisons.

Reveal source scores from the HTML report (v0.17.3)
---
The score/page names in the Tests table still navigate to the corresponding
report section. Adjacent **Reveal file** links (also shown beside detail
headings) ask the operating-system file manager to select the *original*
`.mscx` or `.mscz` file, without opening it in MuseScore or changing it.

Browsers cannot invoke D-Bus or platform file managers from ordinary `file:`
links. Install the opt-in local custom URL handler once per machine:

    python3 vtest/reveal-score.py --install

The report uses `vtest-reveal:///absolute/path/to/score.mscz` links. The handler
accepts only existing local `.mscx`/`.mscz` files and launches no shell
commands from URL content. On Linux it uses Freedesktop FileManager1 `ShowItems`
for selection, falling back to opening the parent directory with `xdg-open` if
that service is unavailable. On macOS it invokes `open -R`; on Windows it
invokes `explorer.exe /select,`. Installation is per-user: a `.desktop` handler
on Linux, a Launch Services app on macOS, or a URL protocol registry entry on
Windows. macOS/Windows registration have not been tested on those platforms.
A browser may show an external-application confirmation prompt the first time.

No background server is required. The protocol handler operates only on the
machine where it is installed. Old generated reports without these links must
be regenerated; that does not require rerendering cached score/reference PNGs.
If the score folder moves or a clone lives on another computer, regenerate the
report to refresh its absolute source paths. To undo Linux registration,
remove `~/.local/share/applications/musescore-vtest-reveal.desktop` and change
its default x-scheme-handler association as appropriate.

One-command Studio + 37E external comparison and separate reports (v0.18)
---
From the 37E source root, with `VTEST_MSCORE` and
`VTEST_REFERENCE_MSCORE` exported, this single command prepares BOTH renderers,
reuses any valid PNG caches, and opens a self-contained report for an external
folder (including compressed `.mscz` files):

    ./vtest/gen --compare-modern --score-dir "$HOME/Scores/Backport tests" \
      --recursive --jobs 4 --studio-source-tag v4.7.7 \
      --output-dir "$HOME/vtest-reports/backport-tests"

Outputs are isolated in the selected directory:

    ~/vtest-reports/backport-tests/
      references/studio-v4.7.7-<binary-hash>/manifest.json
      references/studio-v4.7.7-<binary-hash>/scores/...
      html/engraving.html
      html/studio-catalogue.html
      html/catalogue-audit.json
      html/catalogue-logs/
      html/37e-cache/
      html/prepared-inputs/

The Studio archive is versioned by the executable's SHA-256 and records its
reported version and source tag. The 37E cache is invalidated when the 37E
binary, input, DPI or resource salt changes. Rerunning an identical command
reuses both valid caches. Failed scores are retried and listed in the report.
Visual differences are still recalculated to produce the HTML report.

To use an existing Studio archive without running Studio again:

    ./vtest/gen --audit-modern --score-dir "$HOME/Scores/Backport tests" \
      --recursive --jobs 4 --output-dir "$HOME/vtest-reports/backport-tests"

For an entirely different score collection, use a different `--score-dir` and
`--output-dir`. When `--output-dir` is omitted, the legacy defaults remain
`vtest/html` and `vtest/references`. To share Studio references explicitly
between output directories, add `--archive-root /shared/reference-root` to
both runs, or provide an individual `--archive /path/to/studio-...` to an audit.
`--output-dir` and `--archive-root` apply to the three catalogue modes only;
regular bundled test runs are unchanged.

`--refresh-modern` regenerates Studio PNGs without invalidating 37E images;
`--refresh-current` rerenders 37E without rebuilding Studio references.
`VTEST_REFRESH_REFERENCES=1` still forces both when combined. Set
`VTEST_OPEN_REPORT=0` for unattended runs; otherwise the comparison HTML is
opened in the configured `VTEST_BROWSER` after completion.

The reference archive preserves input hashes and provenance; nothing is
committed to Git automatically. Existing project-wide Studio references
remain untouched by a separately named output directory.

Latest changes report alongside the complete catalogue (v0.18.1)
---
Every successful `--compare-modern` or `--audit-modern` run now produces TWO
views without changing the command or re-rendering any extra scores:

    <output-dir>/html/engraving.html           Full current comparison
    <output-dir>/html/changes/engraving.html   New/changed since prior completed audit
    <output-dir>/html/changes/changes.json     Score paths and change reasons
    <output-dir>/html/last-completed-audit.json  Durable comparison checkpoint

The full report links to the changes report. When no score or rendering inputs
have changed, the second report explicitly says there are no changes. On the
first run without a previous completed audit, all selected scores count as new.
For existing v0.18 outputs, the last complete `catalogue-audit.json` can serve
as the first checkpoint; no rendering cache reset is necessary.

Changes are determined by score and prepared-input hashes, the 37E/Studio
executable identities, reference/result PNG hashes (where recorded), rendering
settings and statuses. A cache miss or explicit refresh alone is not a change.
Rebuilding 37E or switching the Studio reference version intentionally marks
its affected comparisons changed. Missing scores are noted as removed; when
changing the selection filter/root, the results are considered a new scope.

The changes report is a *view* of the full result: it links to images and logs
in the parent HTML directory rather than duplicating PNGs. Keep `html/changes/`
together with `html/` when moving or archiving results. Normal named-fixture
runs (for example, `./vtest/gen tie-1`) do not yet generate a changes report.


Portable newest-score image ZIP (v0.18.2)
-----------------------------------------

Add `--bundle-changes` to a directory audit or combined comparison:

    ./vtest/gen --compare-modern --score-dir "$HOME/Scores/Backport tests" \
      --recursive --jobs 4 --studio-source-tag v4.7.7 \
      --output-dir "$HOME/vtest-reports/backport-tests" --bundle-changes

This adds `<output-dir>/html/changes/latest-changes.zip` alongside the full
`engraving.html` and the existing latest-changes HTML report. The ZIP is
self-contained: `manifest.json` and `README.txt` identify scores, reasons,
renderer hashes, and original and difference PNGs, grouped per changed score
and page. It requires neither the full Studio archive nor a local web server
when shared. The original MSCX/MSCZ scores are intentionally **not** bundled.

All rendered pages of **new or changed scores** are included, not just pages
whose pixels differ. Changes are calculated against the *previous completed
audit*, not against rendering cache hits. Running the identical command again
replaces the bundle with a manifest-only empty ZIP (no new changes); it never
leaves a stale earlier ZIP labeled as this run's changes. Copy an interesting
ZIP elsewhere before another run if you wish to retain it.

`--bundle-changes` is opt-in for `--audit-modern` and `--compare-modern`; it
does not change rendering, caching, previous-completed-audit checkpoints, or
ordinary `./vtest/gen tie-1` invocations. `--cache-modern` alone cannot
create a comparison bundle.


Incremental engraving *comparison* cache (v0.18.3)
---------------------------------------------------

External catalogue audits now also reuse the expensive image analysis between
unchanged runs, independently of Studio and 37E rendering caches. Each paired
page has a verified JSON entry in `<output-dir>/html/comparison-cache/`; its
normalized PNGs, directional shape/pixel differences, and aligned images stay
in the normal HTML directory for the full report, latest-changes view, and ZIP.
No duplicate tree of derived PNGs is needed.

Cache identity includes SHA-256 of **both input PNGs**, the current
`compare-engraving.py` implementation, Pillow version, analysis tolerance, ink
cutoff, and `VTEST_COMPARISON_CACHE_SALT`. Every derived PNG is checked against
its recorded SHA-256 before reuse; deleted or modified images are rebuilt.
Unmatched 37E-only pages also cache their grayscale normalization. The cache
updates after each successfully analysed page, so interrupted reports resume.

An unchanged audit still assembles fresh full/subset HTML and change inventories,
but skips PNG normalization and pixel/staff alignment analysis. A changed *score*
which renders identical pixels still appears as changed in the latest-changes
report, but need not recalculate its image comparison.

To deliberately recompute comparisons **without** rerendering either executable:

    ./vtest/gen --audit-modern --score-dir "/path/to/Scores" --recursive \
      --output-dir "$HOME/vtest-reports/scores" --refresh-comparisons

This option also works with `--compare-modern`. The ordinary `./vtest/gen tie-1`
legacy-fixture mode is unchanged.

Opt-in unused local-cache cleanup (v0.18.4)
---------------------------------------------

After a complete, unfiltered audit of an isolated external score collection,
`--prune-unused` removes local 37E and comparison-cache entries belonging to
scores that no longer exist in that collection. It also removes obsolete
current-page PNGs and analysis products after pagination changes, plus prepared
score copies and local 37E logs for removed scores.

    ./vtest/gen --compare-modern --score-dir "/path/to/Scores" --recursive \
      --output-dir "$HOME/vtest-reports/scores" --prune-unused --prune-dry-run
    ./vtest/gen --compare-modern --score-dir "/path/to/Scores" --recursive \
      --output-dir "$HOME/vtest-reports/scores" --prune-unused

The first command previews the cleanup without advancing the completed-audit
change checkpoint; the second performs it. No deletion occurs until full
comparison and latest-changes reports have been completed.
The operation requires a previous completed, unfiltered audit of the *same*
score directory and recursion setting in the same isolated `--output-dir`.
It refuses `--match`, `--cache-modern`, or a score-directory/scope change to
avoid treating deliberately unselected files as unused.

This operation **never deletes original MSCX/MSCZ scores, any versioned Studio
reference archives (including their manifests/PNGs), recorded before baselines,
full HTML reports, latest-changes ZIPs, or checkpoints**. Studio archives are
versioned historical artifacts and are managed independently. Pruning only
removes recognizable local generated files; other files under the output root
are preserved. Removed scores remain visible as removals in `changes.json`.

Portable Studio reference verification (v0.18.5)
------------------------------------------------

Before enabling online CI, test that a Studio reference archive really works
without the Studio executable or previously generated 37E/analysis caches.
This read-only preflight does **not** render scores or create an HTML report:

    ./vtest/gen --verify-modern \
      --score-dir "/path/to/pinned/Studio/vtest" --recursive \
      --archive "$PWD/vtest/references/studio-v4.7.7-BINARYHASH" \
      --verification-json /tmp/vtest-reference-verification.json

The verifier checks the archive schema and recorded Studio version/identity,
requested DPI, selected score paths, original file SHA-256, deterministic
Studio-prepared score SHA-256, every archived PNG SHA-256 and dimensions, and
per-score cache metadata. It identifies missing/stale/corrupt/failed references
and exits nonzero if any selected score lacks a valid archived Studio output.
It checks both MSCX and MSCZ (including their preserved resources). No Studio
binary is used; its absolute path in the manifest is provenance, not a runtime
dependency. Multiple reference generations require an explicit `--archive`.

The archive also records its score-source Git revision, when available. Moving
or cloning the score directory to another path is supported: relative fixture
names and content hashes determine references. A source revision mismatch or
modified tracked source checkout emits a warning; to enforce the recorded Git
SHA for reproducible CI, add `--strict-source`. The Studio source tag supplied
when creating an archive is descriptive and is NOT proof of the source revision.

Add `--require-committed` to check that all required archive files (manifest,
cache metadata and PNGs) are present in Git `HEAD` and unchanged from the
committed version. Simply creating or staging an archive is insufficient for
fresh-clone portability. This option is appropriate for an archive stored in
37E's own Git repository; outside Git it fails. If your archives live under an
external `--output-dir`, copy the reference directory into
`vtest/references/`, explicitly review its size, and commit it before using
this stricter check. Normal verification can be run on either location.

A passing preflight proves the *reference data* can be read and matched to
available fixtures; it does not prove the 37E executable builds on CI. Once
37E has been compiled, a separate headless rendering smoke test is:

    VTEST_OPEN_REPORT=0 ./vtest/gen --audit-modern \
      --score-dir "/path/to/pinned/Studio/vtest" --recursive \
      --archive "$PWD/vtest/references/studio-v4.7.7-BINARYHASH" \
      --output-dir /tmp/37e-vtest-ci --jobs 4

No `VTEST_REFERENCE_MSCORE` is required for verification or audit-only mode.
The original Studio fixture scores are not embedded in the PNG archive: a CI
runner still needs a checkout of the exact upstream source revision noted in
the manifest. On that runner use `--strict-source --require-committed` before
rendering. `--verification-json` can be uploaded as a CI artifact.

Regression tests: `python3 vtest/test-v0185-portable-references.py`.

Portable, standalone engraving reviews (v0.18.6)
---
The archive-based catalogue audit still writes the complete HTML comparison and
`html/changes/` inventory on each successful run. The changes inventory is
saved **even without** `--bundle-changes`, so it can be exported later without
repeating renderings, comparisons, or advancing the latest-changes checkpoint.

Export **existing** completed results (no MuseScore executable or input scores
needed; the already-audited Studio archive must still be accessible):

    ./vtest/gen --export-review all --output-dir "$HOME/vtest-reports/my-scores"
    ./vtest/gen --export-review changes --output-dir "$HOME/vtest-reports/my-scores"

Or generate the review ZIP automatically during a normal catalogue audit:

    ./vtest/gen --audit-modern --score-dir "/path/to/scores" --recursive \
       --archive "/path/to/studio/archive" --output-dir "/reports/my-scores" \
       --bundle-review all

`--bundle-review changes` packages the current run's latest additions/changes.
`--export-review changes` packages the previously saved latest-changes inventory
as-is, **without recalculating it**. If you already completed another unchanged
audit, that inventory will be empty; `--export-review all` can still package the
whole current collection. No cache deletion is needed. The two portable bundles
are `html/engraving-review-all.zip` and
`html/changes/engraving-review-changes.zip`. Each ZIP contains an independent
`index.html`, `manifest.json`, `comparisons.json`, original and normalized
37E/Studio PNGs, and shape/aligned-shape images when available. It excludes
source MSCX/MSCZ files, renderer caches, and intermediate pixel diagnostics.
An audit without the corresponding bundle option discards any outdated review
ZIP in that output directory; copy a ZIP elsewhere before a new audit if you
want to preserve a particular snapshot. Export-only does not alter checkpoints.
