#!/usr/bin/env python3
"""Render original MSCX scores with an optional historical MuseScore executable.

An AppImage is a valid executable here. Historical rendering uses the ORIGINAL
fixture, not a generated Studio/37E engraving profile, preserving genuine old
score-loading behavior. Each score is converted in a separate process and
outputs are published only after every score succeeds.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile

from input_provenance import load_inputs, sha256
from reference_cache import ReferenceCache, read_pages


def png_dimensions(path):
    with path.open('rb') as stream:
        header = stream.read(24)
    if len(header) != 24 or header[:8] != b'\x89PNG\r\n\x1a\n' or header[12:16] != b'IHDR':
        raise ValueError(f'not a PNG: {path}')
    width, height = struct.unpack('>II', header[16:24])
    if width < 1 or height < 1:
        raise ValueError(f'invalid image size: {path}')
    return [width, height]


def get_version(exe):
    for flag in ('--version', '-v'):
        try:
            proc = subprocess.run([str(exe), flag], stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, text=True, errors='replace',
                                  timeout=15)
        except (OSError, subprocess.TimeoutExpired):
            continue
        reported = proc.stdout.strip()
        if proc.returncode == 0 and reported:
            match = re.search(r'\bMuseScore\b[^\r\n]*?\b(\d+\.\d+\.\d+)\b', reported, re.IGNORECASE)
            version = match.group(1) if match else None
            return version, reported
    return None, 'Version unavailable (version query failed)'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mscore', required=True, type=Path)
    parser.add_argument('--inputs-manifest', required=True, type=Path)
    parser.add_argument('--html-dir', required=True, type=Path)
    parser.add_argument('--dpi', type=int, default=130)
    parser.add_argument('--refresh-cache', action='store_true')
    parser.add_argument('tests', nargs='+')
    args = parser.parse_args()
    exe = args.mscore.expanduser().resolve()
    html_dir = args.html_dir.resolve()
    if not exe.is_file() or not os.access(exe, os.X_OK):
        parser.error(f'historical renderer is not an executable file: {exe}')
    if args.dpi <= 0:
        parser.error('DPI must be positive')
    if len(set(args.tests)) != len(args.tests):
        parser.error('duplicate test names')
    try:
        inputs = load_inputs(args.inputs_manifest.resolve(), args.tests)
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
        parser.error(str(error))
    for name in args.tests:
        if name != Path(name).name or name in ('', '.', '..'):
            parser.error(f'invalid test name: {name!r}')
        if not Path(inputs[name]['origin']).is_file():
            parser.error(f'missing original score for {name}')

    refs = html_dir / 'reference-historical'
    refs.mkdir(parents=True, exist_ok=True)
    manifest_path = refs / 'manifest.json'
    # Never accidentally reuse a previous run's manifest on any failure.
    manifest_path.unlink(missing_ok=True)
    cache = ReferenceCache(html_dir / 'reference-cache' / 'historical', exe, args.dpi,
                           os.environ.get('VTEST_REFERENCE_CACHE_SALT', ''))
    version, reported = get_version(exe)
    label = f'MuseScore {version} (historical)' if version else 'MuseScore historical (version unverified)'
    print(f'Historical renderer: {label}, executable: {exe}', flush=True)
    with tempfile.TemporaryDirectory(prefix='.render-historical-', dir=html_dir) as scratch:
        working = Path(scratch)
        jobfile = working / 'job.json'
        log = html_dir / 'LOG-historical'
        log.write_text('', encoding='utf-8')
        rendered = {}
        cache_hits = set()
        for name in args.tests:
            source_sha = inputs[name]['origin_sha256']
            if cache.restore(name, source_sha, working, args.refresh_cache):
                cache_hits.add(name)
                with log.open('a', encoding='utf-8') as stream:
                    stream.write(f'=== Historical reference: {name} (verified cache hit) ===\n')
                print(f'  {name}: verified historical cache hit', flush=True)
                continue
            jobfile.write_text(json.dumps([{
                'in': inputs[name]['origin'],
                'out': str(working / f'{name}.png'),
            }], indent=2) + '\n', encoding='utf-8')
            cmd = [str(exe), '-j', str(jobfile), '-r', str(args.dpi)]
            try:
                proc = subprocess.run(cmd, cwd=str(working), stdout=subprocess.PIPE,
                                      stderr=subprocess.STDOUT, text=True, errors='replace')
            except OSError as error:
                parser.error(f'historical executable could not start: {error}')
            with log.open('a', encoding='utf-8') as stream:
                stream.write(f'=== Historical reference: {name} (exit {proc.returncode}) ===\n')
                stream.write(proc.stdout)
                if proc.stdout and not proc.stdout.endswith('\n'):
                    stream.write('\n')
            if proc.returncode:
                print(proc.stdout[-4000:], file=sys.stderr)
                parser.error(f'historical render failed for {name} ({proc.returncode}); '
                             'see html/LOG-historical')
            try:
                pages = read_pages(working, name)
                cache.store(name, source_sha, working, pages)
            except (OSError, ValueError) as error:
                parser.error(f'historical output for {name} failed validation: {error}')
            print(f'  {name}: rendered and cached ({len(pages)} page(s))', flush=True)
        try:
            for name in args.tests:
                rendered[name] = read_pages(working, name)
        except (OSError, ValueError) as error:
            parser.error(str(error))
        manifest = {'schema': 1, 'renderer': {
            'label': label, 'version': version, 'reported_version': reported,
            'executable': str(exe)}, 'dpi': args.dpi, 'scores': {}}
        for name, pages in rendered.items():
            for old in refs.glob(f'{name}-[0-9]*.png'):
                old.unlink()
            for page in pages:
                shutil.copy2(working / page['file'], refs / page['file'])
            manifest['scores'][name] = {
                'source_sha256': inputs[name]['origin_sha256'],
                'origin': inputs[name]['origin'], 'input_mode': 'original',
                'pages': pages,
            }
            print(f'  {name}: {len(pages)} historical page(s)', flush=True)
        pending = refs / 'manifest.json.tmp'
        pending.write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n', encoding='utf-8')
        pending.replace(manifest_path)
    print(f'Historical references: {manifest_path}', flush=True)


if __name__ == '__main__':
    main()
