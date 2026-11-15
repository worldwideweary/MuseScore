#!/usr/bin/env python3
"""Portable, image-focused review exports from completed catalogue audits.

Exports do not modify renderer, comparison or latest-changes caches. Archive
and local PNG hashes are checked against the completed audit and per-page
comparison metadata before a ZIP is published atomically.
"""
import hashlib
from html import escape
import json
from pathlib import Path
import re
from zipfile import ZipFile, ZIP_DEFLATED


ROLES = (
    ('37e-normalized', 'normalized-current'),
    ('studio-normalized', 'normalized-reference'),
    ('shapes-diff', 'ink-diff'),
    ('aligned-shapes-diff', 'aligned-ink-diff'),
)
SAFE_ID = re.compile(r'[A-Za-z0-9_-]+\Z')


def digest(path):
    d = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            d.update(chunk)
    return d.hexdigest()


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8'))


def selected_names(html, audit, mode):
    if mode == 'all':
        return {name: ['selected score'] for name in audit['scores']}
    inventory = read_json(html / 'changes' / 'changes.json')
    relatives = inventory.get('new_or_changed', {})
    if not isinstance(relatives, dict):
        raise ValueError('Invalid latest-changes inventory')
    names = {record['relative_path']: name for name, record in audit['scores'].items()}
    unknown = set(relatives) - set(names)
    if unknown:
        raise ValueError('Changes inventory does not match completed audit: ' + ', '.join(sorted(unknown)[:3]))
    return {names[relative]: reasons for relative, reasons in relatives.items()}


def load_completed(html, archive_override=None):
    audit = read_json(html / 'catalogue-audit.json')
    if audit.get('completed') is not True or not isinstance(audit.get('scores'), dict):
        raise ValueError('A completed catalogue audit is required before exporting a review')
    manifest = Path(archive_override).expanduser().resolve() / 'manifest.json' if archive_override else Path(audit['reference_manifest'])
    archive = read_json(manifest)
    if archive.get('schema') != 1 or not isinstance(archive.get('scores'), dict):
        raise ValueError(f'Invalid Studio reference archive: {manifest}')
    if archive.get('renderer', {}).get('executable_sha256') != audit.get('reference_renderer_sha256'):
        raise ValueError('Selected Studio archive differs from the completed audit')
    return audit, manifest.parent, archive


def verified_page_inputs(html, archive_dir, archive, name, record, page):
    """Resolve only PNGs recorded by a completed audit, verifying their hashes."""
    paths = {}
    current_hashes = record.get('37e_output_sha256') or []
    studio_entry = archive['scores'].get(name, {})
    studio_pages = (studio_entry.get('pages') or []) if record.get('reference_status') == 'ok' else []
    if page <= len(current_hashes):
        current = html / f'{name}-{page}.png'
        paths['37e'] = (current, current_hashes[page - 1])
    if page <= len(studio_pages):
        studio = archive_dir / 'scores' / name / f'{name}-{page}.png'
        paths['studio'] = (studio, studio_pages[page-1]['sha256'])
    return paths


def metrics_for_page(html, name, page, has_both):
    """Load the recorded comparison; never calculate or alter it during export."""
    metadata = html / 'comparison-cache' / name / f'page-{page:03}.json'
    if not metadata.is_file():
        if has_both:
            raise ValueError(f'Missing comparison metadata for {name} page {page}; rerun audit')
        return None, {}
    data = read_json(metadata)
    if data.get('schema') != 1 or data.get('identity', {}).get('score') != name or data.get('identity', {}).get('page') != page:
        raise ValueError(f'Stale comparison metadata: {metadata}')
    if has_both:
        if data['identity'].get('mode') != 'paired':
            raise ValueError(f'Comparison metadata not paired: {metadata}')
        required = {'normalized-current', 'normalized-reference', 'ink-diff'}
        products = data.get('products') or {}
        if not isinstance(data.get('result'), dict) or not required.issubset(products):
            raise ValueError(f'Incomplete comparison products: {metadata}; rerun audit')
        if data['result'].get('shift') is not None and 'aligned-ink-diff' not in products:
            raise ValueError(f'Missing aligned comparison product: {metadata}; rerun audit')
    return data.get('result'), data


def page_view_html(score, page):
    parts = [f'<h3>Page {page["page"]}</h3>']
    stats = page.get('metrics') or {}
    if stats.get('raw_count') is not None and stats.get('size'):
        total = stats['size'][0] * stats['size'][1]
        parts.append(f'<p>Absolute pixel difference: {100 * stats["raw_count"] / total:.2f}%')
        if stats.get('aligned_count') is not None:
            parts.append(f' · Aligned: {100 * stats["aligned_count"] / total:.2f}%')
        parts.append('</p>')
    panels = [('37e', '37E original'), ('studio', 'Studio original'),
              ('37e-normalized', '37E normalized'), ('studio-normalized', 'Studio normalized'),
              ('shapes-diff', 'Shape differences'), ('aligned-shapes-diff', 'Aligned shape differences')]
    parts.append('<div class="panels">')
    for role, label in panels:
        image = page['images'].get(role)
        if image:
            src = escape(image['file'], quote=True)
            parts.append(f'<div class="panel"><div class="label">{label}</div>'
                         f'<a href="{src}"><img src="{src}" alt="{label}" loading="lazy"></a></div>')
    parts.append('</div>')
    if not page['images']:
        parts.append('<p>No rendered PNGs were available for this page.</p>')
    return ''.join(parts)


def build_html(manifest):
    score_count = len(manifest['scores'])
    title = f'MuseScore engraving review · {manifest["mode"]}'
    s = ['<!doctype html><html lang="en"><head><meta charset="utf-8">',
         '<meta name="viewport" content="width=device-width, initial-scale=1">',
         f'<title>{escape(title)}</title>',
         '<style>body{font:15px/1.5 system-ui,sans-serif;margin:20px;max-width:1800px}'
         'h1{margin:0 0 8px}h3{margin:12px 0 4px}.meta{color:#555}'
         'details{border:1px solid #cdd4df;margin:12px 0;padding:8px 12px;border-radius:6px}'
         'summary{cursor:pointer;font-weight:600;padding:7px}.panels{display:flex;gap:12px;flex-wrap:wrap}'
         '.panel{flex:1 1 320px;min-width:250px;max-width:550px}'
         '.panel img{max-width:100%;max-height:460px;object-fit:contain;object-position:top left;'
         'border:1px solid #ccc;background:white}.label{font-weight:600;margin:8px 0}'
         'button{padding:7px 12px;cursor:pointer;margin-right:8px}'
         'code{overflow-wrap:anywhere}</style></head><body>',
         f'<h1>{escape(title)}</h1>',
         f'<p>{score_count} score(s), {sum(len(x["pages"]) for x in manifest["scores"])} page sections. '
         'All images are included in this ZIP. No renderer executable or PNG cache is required.</p>',
         '<p class="meta">Original rendering: 37E vs Studio · red indicates 37E-only ink; '
         'blue indicates Studio-only ink. Staff-aligned differences are supplemental. '
         'Numerical diagnostics: <a href="comparisons.json">comparisons.json</a>; '
         'image hashes and provenance: <a href="manifest.json">manifest.json</a>.</p>',
         '<button type="button" onclick="for(const d of document.querySelectorAll(\'details\'))d.open=true">Expand all</button>',
         '<button type="button" onclick="for(const d of document.querySelectorAll(\'details\'))d.open=false">Collapse all</button>']
    if not manifest['scores']:
        s.append('<p>No scores are included in this review.</p>')
    for score in manifest['scores']:
        relative = escape(score['relative_path'])
        reason = escape(', '.join(score['reasons']))
        s.append(f'<details><summary>{relative} <small class="meta">({len(score["pages"])} pages; {reason})</small></summary>')
        s.append(f'<p class="meta">37E: {escape(str(score["37e_status"]))} · '
                 f'Studio: {escape(str(score["studio_status"]))}</p>')
        for page in score['pages']:
            s.append(page_view_html(score, page))
        s.append('</details>')
    s.append('</body></html>')
    return '\n'.join(s) + '\n'


def export_review(html, mode, archive_override=None):
    html = Path(html).expanduser().resolve()
    if mode not in ('all', 'changes'):
        raise ValueError('Review mode must be all or changes')
    audit, archive_dir, archive = load_completed(html, archive_override)
    names = selected_names(html, audit, mode)
    target = html / ('engraving-review-all.zip' if mode == 'all' else 'changes/engraving-review-changes.zip')
    target.parent.mkdir(parents=True, exist_ok=True)
    temp = target.with_name(target.name + '.tmp')
    inventory = (read_json(html / 'changes' / 'changes.json') if mode == 'changes' else {})
    manifest = {'schema': 1, 'mode': mode, 'dpi': audit.get('dpi'),
                'removed': inventory.get('removed', []),
                '37e_renderer_sha256': audit.get('current_renderer_sha256'),
                'studio_renderer_sha256': audit.get('reference_renderer_sha256'),
                'studio_version': audit.get('reference_version'),
                'scores': []}
    comparison = {'schema': 1, 'mode': mode, 'scores': []}
    try:
        with ZipFile(temp, 'w', ZIP_DEFLATED, compresslevel=6) as z:
            for name in sorted(names, key=lambda name: audit['scores'][name]['relative_path']):
                if not SAFE_ID.fullmatch(name):
                    raise ValueError(f'Unsafe score identifier: {name!r}')
                record = audit['scores'][name]
                entry = {'score_id': name, 'relative_path': record['relative_path'],
                         'reasons': names[name], '37e_status': record['37e_status'],
                         'studio_status': record['reference_status'], 'pages': []}
                scores_metrics = {'relative_path': record['relative_path'], 'score_id': name,
                                  '37e_status': record['37e_status'], 'studio_status': record['reference_status'],
                                  'pages': []}
                pages = max(record.get('37e_pages', 0), record.get('reference_pages', 0), 1)
                for page in range(1, pages + 1):
                    inputs = verified_page_inputs(html, archive_dir, archive, name, record, page)
                    result, analysis = metrics_for_page(html, name, page, len(inputs) == 2)
                    if len(inputs) < 2:
                        # Old paired results can survive on disk after reflow.
                        # Never export those derivatives for an unmatched page.
                        wanted = 'current-only' if '37e' in inputs else None
                        if analysis.get('identity', {}).get('mode') != wanted:
                            result, analysis = None, {}
                    identity = analysis.get('identity', {})
                    if (identity.get('mode') == 'current-only' and '37e' in inputs and
                            identity.get('current_sha256') != inputs['37e'][1]):
                        raise ValueError(f'{name} page {page}: current-only normalization is stale; rerun audit')
                    if len(inputs) == 2 and (identity.get('current_sha256') != inputs['37e'][1] or
                                             identity.get('reference_sha256') != inputs['studio'][1]):
                        raise ValueError(f'{name} page {page}: comparison inputs changed; rerun audit')
                    images = {}
                    folder = f'scores/{name}/page-{page:02d}/'
                    def add(role, path, expected_hash):
                        if not path.is_file() or digest(path) != expected_hash:
                            raise ValueError(f'{name} page {page}: missing/stale {role} PNG: {path}')
                        member = folder + role + '.png'
                        z.write(path, member)
                        images[role] = {'file': member, 'sha256': expected_hash}
                    for role, (path, hash_value) in inputs.items():
                        add(role, path, hash_value)
                    products = analysis.get('products', {})
                    key = name if page == 1 else f'{name}-p{page}'
                    for role, suffix in ROLES:
                        if suffix in products:
                            if (role.startswith('aligned-') and not result) or (role.startswith('studio-') and 'studio' not in inputs):
                                continue
                            add(role, html / f'{key}-{suffix}.png', products[suffix])
                    page_entry = {'page': page, 'images': images, 'metrics': result}
                    entry['pages'].append(page_entry)
                    scores_metrics['pages'].append({'page': page, 'metrics': result,
                                                    'image_roles': sorted(images)})
                manifest['scores'].append(entry)
                comparison['scores'].append(scores_metrics)
            z.writestr('manifest.json', json.dumps(manifest, indent=2, ensure_ascii=False) + '\n')
            z.writestr('comparisons.json', json.dumps(comparison, indent=2, ensure_ascii=False) + '\n')
            z.writestr('index.html', build_html(manifest))
            z.writestr('README.txt', 'MuseScore engraving review\n\nExtract this ZIP and open index.html.\n'
                       'No executable, renderer cache, or original score files are included.\n'
                       'This archive contains original and normalized 37E/Studio PNGs, shape differences,\n'
                       'and comparison metrics. See manifest.json for page identities and SHA-256.\n')
        temp.replace(target)
    except BaseException:
        temp.unlink(missing_ok=True)
        raise
    print(f'Portable {mode} review: {target} ({len(names)} scores)', flush=True)
    return target
