#!/usr/bin/env python3
"""Render shared 3.x visual-test scores with a MuseScore Studio executable.

Keep the historical *-ref.png images untouched. Preserve Studio's PNG pages
under html/reference-studio/, and copy page 1 into html/NAME-ref.png for the
legacy comparison scripts. Record renderer and image provenance in a manifest.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile

from input_provenance import load_inputs
from reference_cache import ReferenceCache, read_pages

REFERENCE_DIR = 'reference-studio'


def sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def png_dimensions(path):
    # Read dimensions from PNG's mandatory IHDR chunk without new dependencies.
    with path.open('rb') as stream:
        header = stream.read(24)
    if (len(header) != 24 or header[:8] != b'\x89PNG\r\n\x1a\n'
            or header[12:16] != b'IHDR'):
        raise ValueError(f'not a PNG with an IHDR header: {path}')
    width, height = struct.unpack('>II', header[16:24])
    if width <= 0 or height <= 0:
        raise ValueError(f'invalid PNG dimensions: {path}')
    return [width, height]


def studio_version(exe):
    """Return (parsed version or None, verbatim reported version text).

    An executable built from a release tag may still report 'Unstable
    Prerelease'. Preserve that output rather than asserting release status.
    """
    for option in ('--long-version', '--version'):
        try:
            proc = subprocess.run([str(exe), option], stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, text=True,
                                  errors='replace', timeout=20)
        except (OSError, subprocess.TimeoutExpired):
            continue
        raw = proc.stdout.strip()
        if proc.returncode != 0 or not raw:
            continue
        match = re.search(r'\bVersion\s+([4-9]\.\d+\.\d+)\b', raw, re.IGNORECASE)
        if not match:
            match = re.search(r'\b([4-9]\.\d+\.\d+)\b', raw)
        return (match.group(1) if match else None), raw
    return None, 'Version information unavailable (version options failed)'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mscore', required=True, type=Path,
                        help='Path to the compiled MuseScore Studio executable')
    parser.add_argument('--inputs-manifest', type=Path,
                        help='exact prepared MSCX inputs selected by gen')
    parser.add_argument('--scores-dir', required=True, type=Path)
    parser.add_argument('--html-dir', required=True, type=Path)
    parser.add_argument('--dpi', type=int, default=130)
    parser.add_argument('--refresh-cache', action='store_true')
    parser.add_argument('tests', nargs='+', help='3.x visual-test score basenames')
    args = parser.parse_args()
    try:
        inputs = load_inputs(args.inputs_manifest, args.tests) if args.inputs_manifest else None
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
        parser.error(str(error))

    exe = args.mscore.expanduser().resolve()
    scores_dir = args.scores_dir.resolve()
    html_dir = args.html_dir.resolve()
    if not exe.is_file() or not os.access(exe, os.X_OK):
        parser.error(f'Studio executable not found or not executable: {exe}')
    if args.dpi <= 0:
        parser.error('--dpi must be positive')
    if len(set(args.tests)) != len(args.tests):
        parser.error('duplicate test names')
    for name in args.tests:
        if name != Path(name).name or name in ('', '.', '..'):
            parser.error(f'invalid test name: {name!r}')
        input_path = Path(inputs[name]['effective_studio']) if inputs else scores_dir / (name + '.mscx')
        if not input_path.is_file():
            parser.error(f'missing selected MSCX input: {input_path}')
        if not (html_dir / (name + '-1.png')).is_file():
            parser.error(f'missing current 37E PNG: {html_dir / (name + "-1.png")}')

    html_dir.mkdir(parents=True, exist_ok=True)
    refs_dir = html_dir / REFERENCE_DIR
    refs_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = refs_dir / 'manifest.json'
    # A failed render must never leave a prior run's manifest looking current.
    manifest_path.unlink(missing_ok=True)

    cache = ReferenceCache(html_dir / 'reference-cache' / 'studio', exe, args.dpi,
                           os.environ.get('VTEST_REFERENCE_CACHE_SALT', ''))
    version, reported = studio_version(exe)
    label = f'Studio {version}' if version else 'Studio (version unverified)'
    print(f'Studio reference executable: {exe}', flush=True)
    print(f'Reported version: {reported}', flush=True)
    print(f'Rendering {len(args.tests)} shared score(s) at {args.dpi} DPI', flush=True)

    with tempfile.TemporaryDirectory(prefix='.render-studio-', dir=html_dir) as temporary:
        working = Path(temporary)
        # Studio 4.7.7 debug builds may assert in kors::async when a single
        # -j invocation converts multiple scores. Keep the verified -j PNG
        # export path, but launch a fresh Studio process for EACH score so
        # callbacks and project state cannot leak into the next conversion.
        # Validate every output below before publishing any references.
        job_path = working / 'jobs.json'
        log_path = html_dir / 'LOG-studio'
        log_path.write_text('', encoding='utf-8')
        cache_hits = set()
        for name in args.tests:
            source_sha = inputs[name]['studio_source_sha256'] if inputs else sha256(scores_dir / (name + '.mscx'))
            if cache.restore(name, source_sha, working, args.refresh_cache):
                cache_hits.add(name)
                with log_path.open('a', encoding='utf-8') as stream:
                    stream.write(f'=== Studio reference: {name} (verified cache hit) ===\n')
                print(f'  {name}: verified Studio cache hit', flush=True)
                continue
            job = [{'in': inputs[name]['effective_studio'] if inputs else str(scores_dir / (name + '.mscx')),
                    'out': str(working / (name + '.png'))}]
            job_path.write_text(json.dumps(job, indent=2), encoding='utf-8')
            cmd = [str(exe), '-j', str(job_path), '-r', str(args.dpi)]
            try:
                proc = subprocess.run(cmd, cwd=str(working), stdout=subprocess.PIPE,
                                      stderr=subprocess.STDOUT, text=True, errors='replace')
            except OSError as error:
                parser.error(f'Studio executable could not start: {error}')
            with log_path.open('a', encoding='utf-8') as stream:
                stream.write(f'=== Studio reference: {name} (exit {proc.returncode}) ===\n')
                stream.write(proc.stdout)
                if proc.stdout and not proc.stdout.endswith('\n'):
                    stream.write('\n')
            if proc.returncode:
                print(proc.stdout[-4000:], file=sys.stderr)
                parser.error(f'Studio render failed for {name} ({proc.returncode}); '
                             'see html/LOG-studio')
            # Commit this successfully exported score to the disposable cache
            # immediately. An interrupted catalogue run can resume from here.
            try:
                pages = read_pages(working, name)
                cache.store(name, source_sha, working, pages)
            except (OSError, ValueError) as error:
                parser.error(f'Studio output for {name} failed validation: {error}')
            print(f'  {name}: rendered and cached ({len(pages)} page(s))', flush=True)

        # Validate ALL requested outputs before replacing ANY reference images.
        rendered = {}
        try:
            for name in args.tests:
                entries = read_pages(working, name)
                rendered[name] = [{'path': working / entry['file'],
                                   'dimensions': entry['dimensions'],
                                   'sha256': entry['sha256']} for entry in entries]
        except (OSError, ValueError) as error:
            parser.error(str(error))
        metadata = {
            'schema': 1,
            'renderer': {
                'label': label,
                'executable': str(exe),
                'version': version,
                'reported_version': reported,
            },
            'dpi': args.dpi,
            'scores': {},
        }
        for name, pages in rendered.items():
            # Generated snapshots live separately from tracked historical refs.
            for old in refs_dir.glob(f'{name}-[0-9]*.png'):
                old.unlink()
            page_info = []
            for index, page in enumerate(pages, 1):
                filename = f'{name}-{index}.png'
                shutil.copy2(page['path'], refs_dir / filename)
                page_info.append({'file': filename, 'dimensions': page['dimensions'],
                                  'sha256': page['sha256']})
            shutil.copy2(pages[0]['path'], html_dir / f'{name}-ref.png')
            metadata['scores'][name] = {
                'source_sha256': inputs[name]['studio_source_sha256'] if inputs else sha256(scores_dir / (name + '.mscx')),
                'profile': inputs[name]['profile'] if inputs else 'original',
                'origin': inputs[name]['origin'] if inputs else str(scores_dir / (name + '.mscx')),
                'origin_sha256': inputs[name]['origin_sha256'] if inputs else sha256(scores_dir / (name + '.mscx')),
                'style_overrides': inputs[name]['studio_style_overrides'] if inputs else {},
                '37e_source_sha256': inputs[name]['source_sha256'] if inputs else None,
                'pages': page_info,
            }
            print(f'  {name}: {len(pages)} page(s); {REFERENCE_DIR}/{name}-1.png', flush=True)

        # Manifest is published only after successful rendering and copying.
        pending = refs_dir / 'manifest.json.tmp'
        pending.write_text(json.dumps(metadata, indent=2, sort_keys=True) + '\n',
                           encoding='utf-8')
        pending.replace(manifest_path)

    print(f'Studio references: html/{REFERENCE_DIR}/; manifest: {manifest_path}', flush=True)
    print('Historical 3.x reference PNGs are unchanged.', flush=True)


if __name__ == '__main__':
    main()
