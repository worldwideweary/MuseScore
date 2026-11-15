#!/usr/bin/env python3
"""Select a directory of MSCX/MSCZ scores for one unified vtest report.

Prints safe, stable report IDs separated by newlines and atomically writes
an ID-to-absolute-path JSON mapping consumed by prepare-inputs.py.
"""
import argparse
import json
from pathlib import Path

from score_archive import external_name


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--score-dir', required=True, type=Path)
    parser.add_argument('--manifest', required=True, type=Path)
    parser.add_argument('--recursive', action='store_true')
    args = parser.parse_args()
    directory = args.score_dir.expanduser().resolve()
    if not directory.is_dir():
        parser.error(f'not a score directory: {directory}')
    files = directory.rglob('*') if args.recursive else directory.iterdir()
    paths = sorted({p.resolve() for p in files if p.is_file() and p.suffix.lower() in ('.mscx', '.mscz')},
                   key=lambda path: str(path))
    if not paths:
        parser.error(f'no .mscx or .mscz scores in {directory}' +
                     (' or its subdirectories' if args.recursive else ' (use --recursive for subdirectories)'))
    mapping = {}
    for path in paths:
        key = external_name(path)
        if key in mapping:
            parser.error(f'external score identifier collision: {path} and {mapping[key]}')
        mapping[key] = str(path)
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    temp_path = args.manifest.with_name(args.manifest.name + '.tmp')
    temp_path.write_text(json.dumps({'schema': 1, 'scores': mapping},
                                    sort_keys=True, indent=2) + '\n', encoding='utf-8')
    temp_path.replace(args.manifest)
    for key in mapping:
        print(key)


if __name__ == '__main__':
    main()
