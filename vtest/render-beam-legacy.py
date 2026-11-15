#!/usr/bin/env python3
"""Render opt-in 37E beamLayout4=0 variants without altering main/baseline renders.

Each selected score is rendered independently using the same 37E binary and
DPI as the normal run. A manifest makes inputs and output pages auditable.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from input_provenance import load_inputs
# The Studio exporter already has verified PNG metadata and hash helpers.
# A hyphenated script cannot be imported directly; share its implementation
# here without coupling this renderer to Studio-specific process flags.
import hashlib
import struct


def sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def png_dimensions(path):
    with path.open('rb') as stream:
        header = stream.read(24)
    if (len(header) != 24 or header[:8] != b'\x89PNG\r\n\x1a\n'
            or header[12:16] != b'IHDR'):
        raise ValueError(f'not a PNG with IHDR: {path}')
    width, height = struct.unpack('>II', header[16:24])
    if width <= 0 or height <= 0:
        raise ValueError(f'invalid PNG dimensions: {path}')
    return [width, height]



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mscore', type=Path, required=True)
    parser.add_argument('--html-dir', type=Path, required=True)
    parser.add_argument('--inputs-manifest', type=Path, required=True)
    parser.add_argument('--dpi', type=int, default=130)
    parser.add_argument('tests', nargs='+')
    args = parser.parse_args()
    try:
        inputs = load_inputs(args.inputs_manifest, args.tests)
        if args.dpi < 1:
            raise ValueError('DPI must be positive')
        exe = args.mscore.expanduser().resolve()
        if not exe.is_file() or not os.access(exe, os.X_OK):
            raise ValueError(f'37E executable not found or not executable: {exe}')
        out_dir = args.html_dir.resolve() / 'beam-legacy'
        out_dir.mkdir(parents=True, exist_ok=True)
        manifest = out_dir / 'manifest.json'
        # Do not expose stale A/B results if rendering fails, or if this run
        # requests tests that no longer opt in to beam A/B.
        manifest.unlink(missing_ok=True)
        selected = [name for name in args.tests if 'beam_legacy_effective' in inputs[name]]
        data = {'schema': 1, 'renderer': str(exe), 'dpi': args.dpi, 'scores': {}}
        if selected:
            with tempfile.TemporaryDirectory(prefix='.beam-legacy-', dir=args.html_dir) as temporary:
                working = Path(temporary)
                log_path = args.html_dir / 'LOG-beam-legacy'
                log_path.write_text('', encoding='utf-8')
                rendered = {}
                for name in selected:
                    output = working / f'{name}.png'
                    job = working / 'job.json'
                    job.write_text(json.dumps([{'in': inputs[name]['beam_legacy_effective'],
                                                'out': str(output)}]), encoding='utf-8')
                    proc = subprocess.run([str(exe), '-j', str(job), '-r', str(args.dpi)],
                                          cwd=working, stdout=subprocess.PIPE,
                                          stderr=subprocess.STDOUT, text=True, errors='replace')
                    with log_path.open('a', encoding='utf-8') as stream:
                        stream.write(f'=== 37E beamLayout4=0: {name} (exit {proc.returncode}) ===\n')
                        stream.write(proc.stdout)
                        if proc.stdout and not proc.stdout.endswith('\n'):
                            stream.write('\n')
                    if proc.returncode:
                        raise ValueError(f'{name}: legacy beam render failed ({proc.returncode}); '
                                         'see html/LOG-beam-legacy')
                    pages = []
                    for index in range(1, 1000):
                        page = working / f'{name}-{index}.png'
                        if not page.exists():
                            break
                        pages.append(page)
                    if not pages:
                        raise ValueError(f'{name}: no legacy beam PNGs; see html/LOG-beam-legacy')
                    rendered[name] = pages
                # Only publish after every selected render succeeded.
                for name, pages in rendered.items():
                    for old in out_dir.glob(f'{name}-[0-9]*.png'):
                        old.unlink()
                    info = []
                    for index, page in enumerate(pages, 1):
                        filename = f'{name}-{index}.png'
                        dimensions = png_dimensions(page)
                        checksum = sha256(page)
                        shutil.copy2(page, out_dir / filename)
                        info.append({'file': filename, 'sha256': checksum,
                                     'dimensions': dimensions})
                    data['scores'][name] = {
                        'source_sha256': inputs[name]['beam_legacy_source_sha256'],
                        'main_source_sha256': inputs[name]['source_sha256'],
                        'profile': inputs[name]['profile'],
                        'beamLayout4': 0,
                        'pages': info,
                    }
                    print(f'  {name}: legacy beamLayout4=0; {len(pages)} page(s)', flush=True)
        pending = out_dir / 'manifest.json.tmp'
        pending.write_text(json.dumps(data, indent=2, sort_keys=True) + '\n', encoding='utf-8')
        pending.replace(manifest)
        if not selected:
            print('  no beam A/B fixtures in this run', flush=True)
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
        parser.error(str(error))


if __name__ == '__main__':
    main()
