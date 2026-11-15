#!/usr/bin/env python3
"""Explicitly freeze rendered 37E PNG pages for before/current visual regression.

Normal ./gen runs never change baselines. This command is called only when
VTEST_CAPTURE_BASELINE=1 is set. Historical *-ref.png files are untouched.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import tempfile

from input_provenance import load_inputs


def sha256(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def png_dimensions(path):
    with path.open('rb') as stream:
        header = stream.read(24)
    if len(header) != 24 or header[:8] != b'\x89PNG\r\n\x1a\n' or header[12:16] != b'IHDR':
        raise ValueError(f'invalid PNG: {path}')
    return list(struct.unpack('>II', header[16:24]))


def numbered_pages(directory, name):
    pattern = re.compile(re.escape(name) + r'-(\d+)\.png\Z')
    found = {int(match.group(1)): path for path in directory.iterdir()
             if (match := pattern.fullmatch(path.name)) and int(match.group(1)) > 0}
    if not found or sorted(found) != list(range(1, len(found) + 1)):
        raise ValueError(f'{name}: expected a contiguous series of rendered pages from page 1')
    return [found[index] for index in sorted(found)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--html-dir', required=True, type=Path)
    parser.add_argument('--inputs-manifest', type=Path,
                        help='exact prepared MSCX inputs selected by gen')
    parser.add_argument('--scores-dir', required=True, type=Path)
    parser.add_argument('--baseline-dir', required=True, type=Path)
    parser.add_argument('--renderer', required=True)
    parser.add_argument('--dpi', type=int, required=True)
    parser.add_argument('tests', nargs='+')
    args = parser.parse_args()
    html_dir = args.html_dir.resolve()
    scores_dir = args.scores_dir.resolve()
    baseline_dir = args.baseline_dir.resolve()
    if args.dpi <= 0:
        parser.error('DPI must be positive')
    if len(set(args.tests)) != len(args.tests):
        parser.error('duplicate test names')
    for name in args.tests:
        if name != Path(name).name or name in ('', '.', '..'):
            parser.error(f'invalid test name: {name!r}')

    baseline_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = baseline_dir / 'manifest.json'
    try:
        if manifest_path.exists():
            manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
            if manifest.get('schema') != 1 or not isinstance(manifest.get('scores'), dict):
                raise ValueError('unsupported existing baseline manifest')
            if manifest.get('dpi') != args.dpi:
                raise ValueError(f'existing baseline uses {manifest.get("dpi")} DPI, '
                                 f'but current render uses {args.dpi} DPI')
        else:
            manifest = {'schema': 1, 'dpi': args.dpi, 'scores': {}}
        inputs = load_inputs(args.inputs_manifest, args.tests) if args.inputs_manifest else None
        records = {}
        now = datetime.now(timezone.utc).isoformat(timespec='seconds')
        for name in args.tests:
            source = Path(inputs[name]['effective']) if inputs else scores_dir / (name + '.mscx')
            if not source.is_file():
                raise ValueError(f'missing input score: {source}')
            pages = numbered_pages(html_dir, name)
            records[name] = {
                'source_sha256': sha256(source),
                'profile': inputs[name]['profile'] if inputs else 'original',
                'origin': inputs[name]['origin'] if inputs else str(source),
                'origin_sha256': inputs[name]['origin_sha256'] if inputs else sha256(source),
                'captured_at': now,
                'renderer_executable': str(Path(args.renderer).expanduser().resolve()),
                'pages': [{'file': path.name, 'sha256': sha256(path),
                           'dimensions': png_dimensions(path)} for path in pages],
            }
        # Validate every selected score before publishing any new snapshots.
        with tempfile.TemporaryDirectory(prefix='.capture-', dir=baseline_dir) as temp:
            stage = Path(temp)
            for name in args.tests:
                for info in records[name]['pages']:
                    shutil.copy2(html_dir / info['file'], stage / info['file'])
            manifest['scores'].update(records)
            (stage / 'manifest.json').write_text(
                json.dumps(manifest, indent=2, sort_keys=True) + '\n', encoding='utf-8')
            for name in args.tests:
                for info in records[name]['pages']:
                    os.replace(stage / info['file'], baseline_dir / info['file'])
            # Commit the manifest last so readers can detect interrupted writes.
            os.replace(stage / 'manifest.json', manifest_path)
        # Remove unused extra pages from an earlier capture of the same score.
        # Other fixtures' pages and their baselines are never touched.
        for name in args.tests:
            active = {item['file'] for item in records[name]['pages']}
            pattern = re.compile(re.escape(name) + r'-(\d+)\.png\Z')
            for old in baseline_dir.iterdir():
                if pattern.fullmatch(old.name) and old.name not in active:
                    old.unlink()
        for name in args.tests:
            print(f'37E baseline captured: {name}, {len(records[name]["pages"])} page(s)', flush=True)
        print(f'Baseline manifest: {manifest_path}', flush=True)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        parser.error(str(error))


if __name__ == '__main__':
    main()
