#!/usr/bin/env python3
"""Produce a self-contained, image-focused ZIP for the most recent audit changes.

Only scores changed relative to the previous completed audit are included;
full audit PNG caches and the original score files are never bundled.
"""
import hashlib
import json
from pathlib import Path
import re
from zipfile import ZIP_DEFLATED, ZipFile


# All images are derivatives of verified page renders; these are kept in the
# bundle to make it possible to inspect the comparisons without the HTML report.
DERIVATIVES = (
    ('37e-normalized', 'normalized-current'),
    ('studio-normalized', 'normalized-reference'),
    ('shapes-diff', 'ink-diff'),
    ('aligned-shapes-diff', 'aligned-ink-diff'),
    ('pixels-diff', 'directional-diff'),
    ('aligned-pixels-diff', 'aligned-directional-diff'),
)


def file_sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as src:
        for chunk in iter(lambda: src.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def create_bundle(html, archive_dir, archive, audit, results, changed, removed):
    """Package only this completed run's changed-score images in a portable ZIP.

    'changed' is the mapping returned by make_changes_report. Do not infer
    changes from cache hits or file timestamps. The archive is created with
    an atomic replacement, so an interrupted bundle never masquerades as
    a complete current result.
    """
    destination = html / 'changes' / 'latest-changes.zip'
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_suffix('.zip.tmp')
    doc = {
        'schema': 1,
        'description': 'Images from changes since the previous completed vtest audit',
        'studio_version': audit.get('reference_version'),
        'studio_renderer_sha256': audit.get('reference_renderer_sha256'),
        '37e_renderer_sha256': audit.get('current_renderer_sha256'),
        'dpi': audit.get('dpi'),
        'scores': [],
        'removed': list(removed),
    }
    report_pages = {(r['name'], r['page']): r for r in results}
    lines = [
        'MuseScore vtest - latest new/changed score images',
        '',
        'This ZIP contains PNGs only for scores changed since the previous',
        'completed audit. For each affected score, all rendered pages are',
        'included (not only the page whose pixels may have changed).',
        'Input MSCX/MSCZ score files and the full catalogue are not included.',
        '',
        '37e.png / studio.png: original exported pages',
        'shapes-diff.png: red = 37E-only ink; blue = Studio-only ink',
        'aligned-shapes-diff.png: difference after staff alignment',
        'pixels-diff.png: original fine-pixel directional difference',
        '',
        'Consult manifest.json for filenames, identities and SHA-256 values.',
        '',
    ]
    try:
        with ZipFile(temporary, 'w', compression=ZIP_DEFLATED, compresslevel=6) as zipped:
            for name in sorted(changed, key=lambda n: audit['scores'][n]['relative_path']):
                if not re.fullmatch(r'[A-Za-z0-9_-]+', name):
                    raise ValueError(f'Unsafe score identifier for ZIP: {name!r}')
                record = audit['scores'][name]
                entry = {
                    'score_id': name,
                    'relative_path': record['relative_path'],
                    'reasons': list(changed[name]),
                    '37e_status': record.get('37e_status'),
                    'studio_status': record.get('reference_status'),
                    'pages': [],
                }
                lines.append(f"{record['relative_path']}: {', '.join(changed[name])}")
                studio_pages = (archive.get('scores', {}).get(name, {}).get('pages') or []
                                if record.get('reference_status') == 'ok' else [])
                current_pages = record.get('37e_output_sha256') or []
                page_count = max(len(studio_pages), len(current_pages))
                for index in range(1, page_count + 1):
                    key = name if index == 1 else f'{name}-p{index}'
                    compare_page = report_pages.get((name, index), {})
                    folder = f'scores/{name}/page-{index:02d}/'
                    images = {}

                    def insert(role, path, expected_sha=None):
                        if not path.is_file():
                            raise FileNotFoundError(f'{record["relative_path"]}: missing expected image {path}')
                        digest = file_sha256(path)
                        if expected_sha and digest != expected_sha:
                            raise ValueError(f'{record["relative_path"]}: stale or changed image {path}')
                        target = folder + role + '.png'
                        zipped.write(path, target)
                        images[role] = {'file': target, 'sha256': digest}

                    if index <= len(current_pages):
                        insert('37e', html / f'{name}-{index}.png', current_pages[index - 1])
                    if index <= len(studio_pages):
                        meta = studio_pages[index - 1]
                        insert('studio', archive_dir / 'scores' / name / f'{name}-{index}.png',
                               meta['sha256'])
                    for role, suffix in DERIVATIVES:
                        # Optional aligned artifacts may remain on disk from an
                        # older audit; only package them if this run aligned.
                        if role.startswith('aligned-') and compare_page.get('shift') is None:
                            continue
                        image_path = html / f'{key}-{suffix}.png'
                        if image_path.is_file() and (
                                (role.startswith('37e') and index <= len(current_pages)) or
                                (role.startswith('studio') and index <= len(studio_pages)) or
                                (not role.startswith(('37e', 'studio')) and
                                 index <= len(current_pages) and index <= len(studio_pages))):
                            insert(role, image_path)
                    entry['pages'].append({'page': index, 'images': images})
                doc['scores'].append(entry)
            zipped.writestr('manifest.json', json.dumps(doc, indent=2, sort_keys=True, ensure_ascii=False) + '\n')
            if not changed:
                lines.append('No new or changed scores in this run.')
            if removed:
                lines.append('Removed scores (no images): ' + ', '.join(removed))
            zipped.writestr('README.txt', '\n'.join(lines) + '\n')
        temporary.replace(destination)
    except BaseException:
        temporary.unlink(missing_ok=True)
        raise
    print(f'Portable changes bundle: {destination} ({len(changed)} changed scores)', flush=True)
    return destination
