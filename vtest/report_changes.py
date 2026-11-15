#!/usr/bin/env python3
"""A lightweight latest-run view over an existing full engraving report.

The full report owns the PNGs; changes/ contains just HTML/JSON and links to
../*.png, so it never duplicates hundreds of cached reference images.
"""
from html import escape
import json
import os
from pathlib import Path
import re


SNAPSHOT = 'last-completed-audit.json'


def previous_completed_audit(html):
    """Return the last *finished* audit, not the partly-written active JSON.

    Import a completed v0.18 audit once, so upgrading need not make all existing
    scores appear new. Older files can have no `completed` marker; they are
    acceptable only if all references have final statuses.
    """
    for path in (html / SNAPSHOT, html / 'catalogue-audit.json'):
        try:
            data = json.loads(path.read_text(encoding='utf-8'))
        except (OSError, ValueError):
            continue
        if not isinstance(data, dict) or not isinstance(data.get('scores'), dict):
            continue
        if path.name == SNAPSHOT:
            if data.get('completed') is True:
                return data
        elif data.get('completed') is True or (
                data['scores'] and all(
                    entry.get('reference_status') in ('ok', 'unavailable')
                    for entry in data['scores'].values())):
            return data
    return None


def digest_pages(entries):
    """Stable output identity; distinguish valid pages from missing exports."""
    if entries is None:
        return None
    return [entry.get('sha256') for entry in entries]


def change_reasons(previous, current):
    """Compare semantic render inputs/results, independent of cache-hit flags.

    Do not flag files merely because they were rerendered by --refresh. Detect
    changed sources, effective inputs, outputs, renderer versions, or status.
    """
    old_scores = (previous or {}).get('scores', {})
    scores = current['scores']
    reasons = {}
    previous_selection = (previous or {}).get('selection')
    current_selection = current.get('selection')
    selection_changed = (previous_selection is not None and
                         previous_selection != current_selection)
    old_renderer = (previous or {}).get('current_renderer_sha256')
    renderer_changed = bool(old_renderer and old_renderer != current['current_renderer_sha256'])
    old_ref = (previous or {}).get('reference_renderer_sha256')
    reference_changed = bool(old_ref and old_ref != current.get('reference_renderer_sha256'))
    old_dpi = (previous or {}).get('dpi')
    old_raw = (previous or {}).get('raw')
    for name, item in scores.items():
        old = old_scores.get(name)
        why = []
        if old is None:
            why.append('new score')
        else:
            if selection_changed:
                why.append('score selection or source directory changed')
            if renderer_changed:
                why.append('37E executable changed')
            if reference_changed:
                why.append('Studio executable changed')
            if old_dpi is not None and old_dpi != current.get('dpi'):
                why.append('export DPI changed')
            if old_raw is not None and old_raw != current.get('raw'):
                why.append('input profile mode changed')
            if (old.get('origin_sha256') is not None and
                    old['origin_sha256'] != item.get('origin_sha256')):
                why.append('score file changed')
            if old.get('37e_prepared_sha256') != item.get('37e_prepared_sha256'):
                if 'score file changed' not in why:
                    why.append('prepared 37E input changed')
            if (old.get('studio_prepared_sha256') is not None and
                    old['studio_prepared_sha256'] != item.get('studio_prepared_sha256')):
                if 'score file changed' not in why:
                    why.append('prepared Studio input changed')
            if (old.get('reference_pages_sha256') is not None and
                    old['reference_pages_sha256'] != item.get('reference_pages_sha256')):
                why.append('Studio reference images changed')
            if (old.get('37e_output_sha256') is not None and
                    old['37e_output_sha256'] != item.get('37e_output_sha256')):
                why.append('37E output images changed')
            for field in ('37e_status', 'reference_status', '37e_error', 'reference_error'):
                if old.get(field) != item.get(field):
                    why.append(field.replace('_', ' ') + ' changed')
        if why:
            reasons[name] = why
    removed = sorted(set(old_scores) - set(scores)) if not selection_changed else []
    return reasons, removed


URL_ATTR = re.compile(r'(?P<attr>\b(?:href|src)=")(?P<url>[^"]+)(?P<end>")')


def nested_asset_links(html, manifest_link):
    """Rebase asset URLs, never change local #anchors or OS reveal URLs."""
    def replace(match):
        url = match.group('url')
        if (not url or url.startswith('#') or url == 'catalogue-audit.json' or
                url == manifest_link or re.match(r'^[a-zA-Z][a-zA-Z0-9+.-]*:', url) or
                url.startswith('//')):
            return match.group(0)
        return match.group('attr') + '../' + url + match.group('end')
    return URL_ATTR.sub(replace, html)


def make_changes_report(html, manifest, archive, audit, results, previous, comparison,
                        atomic_json, bundle_enabled=False, review_mode=None):
    """Produce a fresh per-run subset without ever touching the full PNG cache."""
    changed, removed = change_reasons(previous, audit)
    change_dir = html / 'changes'
    change_dir.mkdir(parents=True, exist_ok=True)
    subset_audit = {**audit, 'scores': {name: audit['scores'][name] for name in changed}}
    selected = [entry for entry in results if entry['name'] in changed]
    manifest_link = os.path.relpath(manifest, change_dir)
    report = comparison.page(
        selected, 5, archive.get('renderer', {}).get('label', 'Studio archive'),
        archive={'path': manifest_link, 'renderer': archive['renderer'], 'audit': subset_audit})
    report = nested_asset_links(report, manifest_link)
    if previous is None:
        baseline_note = ('No prior completed audit was found. All selected scores are considered new. '
                         'Subsequent runs will show only changes since the previous completed audit.')
    else:
        baseline_note = 'Compared against the previous completed audit in this output directory.'
    zip_link = (' · <a href="latest-changes.zip" download>Share changed PNGs (ZIP)</a>'
                if bundle_enabled else '')
    if review_mode == 'changes':
        zip_link += ' · <a href="engraving-review-changes.zip" download>Standalone review ZIP</a>'
    detail = ('<p><a href="../engraving.html">Full engraving report</a> · '
              '<a href="changes.json">Changed-score inventory</a>'
              + zip_link + '</p>'
              f'<p>{len(changed)} new or changed score(s); {len(removed)} removed. '
              f'{escape(baseline_note)}</p>')
    if not selected:
        detail += '<p><strong>No new or changed scores in this run.</strong></p>'
    elif changed:
        detail += ('<details><summary>Why were these scores included?</summary><ul>' +
                   ''.join('<li><code>' + escape(audit['scores'][name]['relative_path']) +
                           '</code>: ' + escape('; '.join(reason)) + '</li>'
                           for name, reason in changed.items()) + '</ul></details>')
    if removed:
        detail += ('<details><summary>Removed scores (not rendered)</summary><ul>' +
                   ''.join('<li><code>' + escape(previous['scores'][name].get('relative_path', name)) +
                           '</code></li>' for name in removed) + '</ul></details>')
    report = report.replace('<h1>MuseScore visual comparisons</h1>',
                            '<h1>Latest changes · MuseScore comparisons</h1>' + detail, 1)
    (change_dir / 'engraving.html').write_text(report, encoding='utf-8')
    atomic_json(change_dir / 'catalogue-audit.json', subset_audit)
    atomic_json(change_dir / 'changes.json', {
        'schema': 1, 'relative_to': 'previous completed audit',
        'new_or_changed': {audit['scores'][name]['relative_path']: reason
                           for name, reason in changed.items()},
        'removed': [previous['scores'][name].get('relative_path', name) for name in removed],
        'changed_count': len(changed), 'removed_count': len(removed),
    })
    print(f'Changes report: {change_dir / "engraving.html"} '
          f'({len(changed)} new/changed scores, {len(removed)} removed)', flush=True)
    return changed
