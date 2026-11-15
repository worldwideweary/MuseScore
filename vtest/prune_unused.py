#!/usr/bin/env python3
"""Conservative, opt-in garbage collection for an isolated catalogue audit.

The user-owned score directory and immutable Studio reference archives are never
pruned. Cleanup is restricted to recognizable generated products under the
selected report's html/ directory, after the full audit succeeds.
"""
import json
from pathlib import Path
import re
import shutil

from comparison_cache import ALL as ANALYSIS_PRODUCTS

SCORE_ID = re.compile(r'^external-[A-Za-z0-9_-]+-[0-9a-f]{12}$')
PAGE_META = re.compile(r'^page-([0-9]+)\.json$')
PRODUCT = re.compile(r'^(?P<page>p[0-9]+-)?(?P<suffix>' +
                     '|'.join(re.escape(x) for x in ANALYSIS_PRODUCTS) + r')$')


def require_safe_scope(previous, root, recursive, match, html):
    """Require a completed, unfiltered audit of the very same collection."""
    if match is not None:
        raise ValueError('--prune-unused requires an unfiltered audit (omit --match)')
    if html == root or root in html.parents:
        raise ValueError('--prune-unused requires --output-dir outside the score directory')
    old = (previous or {}).get('selection') or {}
    if (old.get('root') != str(root) or old.get('recursive') != recursive or
            old.get('match') is not None or (previous or {}).get('completed') is not True):
        raise ValueError('--prune-unused requires a previous completed, unfiltered audit '
                         'of the same --score-dir and --recursive selection in this output directory; '
                         'run one normal audit first')


def cached_ids(directory):
    if not directory.exists():
        return set()
    if directory.is_symlink() or not directory.is_dir():
        raise ValueError(f'unsafe cache root, refusing to prune: {directory}')
    return {p.name for p in directory.iterdir()
            if p.is_dir() and not p.is_symlink() and SCORE_ID.fullmatch(p.name)}


def prune_unused(html, selected, audit, previous, dry_run=False):
    """Delete obsolete locally generated files; return totals for diagnostics.

    selected is the exact, unfiltered score-ID set. Caller checks that both
    audit and previous audit are complete and refer to the same score scope.
    """
    html = Path(html)
    active = set(selected)
    old = set((previous or {}).get('scores', {}))
    renderer_root = html / '37e-cache'
    comparison_root = html / 'comparison-cache'
    prepared_root = html / 'prepared-inputs'
    log_root = html / 'catalogue-logs'
    stale = ((old | cached_ids(renderer_root) | cached_ids(comparison_root)) - active)
    stale = {name for name in stale if SCORE_ID.fullmatch(name)}
    for directory in (prepared_root, log_root):
        if directory.is_symlink():
            raise ValueError(f'unsafe generated directory, refusing to prune: {directory}')

    files = set()
    directories = set()
    # Only complete, score-owned cache directories are candidates.
    for name in stale:
        for parent in (renderer_root, comparison_root):
            directory = parent / name
            if directory.is_dir() and not directory.is_symlink():
                directories.add(directory)
        for extension in ('.mscx', '.mscz'):
            for renderer in ('37e', 'studio'):
                file = prepared_root / f'{name}-{renderer}{extension}'
                if file.is_file() or file.is_symlink():
                    files.add(file)
        current_log = log_root / f'{name}-37e.log'
        if current_log.is_file() or current_log.is_symlink():
            files.add(current_log)

    # Page products for active scores are determined by the just-completed
    # audit, not by old cache metadata or a file's modification time.
    expected = {}
    for name in active:
        expected[name] = {}
        for meta in (comparison_root / name).glob('page-*.json'):
            match = PAGE_META.fullmatch(meta.name)
            if not match:
                continue
            number = int(match.group(1))
            try:
                data = json.loads(meta.read_text(encoding='utf-8'))
                identity = data.get('identity') or {}
                products = data.get('products') or {}
                # Keep metadata only for current analysis of an actual page,
                # confirmed by the latest audit and generated results.
                count_37e = audit['scores'][name]['37e_pages']
                count_ref = audit['scores'][name]['reference_pages']
                useful = ((number <= count_37e and number <= count_ref and
                           identity.get('mode') == 'paired') or
                          (number <= count_37e and number > count_ref and
                           identity.get('mode') == 'current-only'))
                if useful and isinstance(products, dict):
                    expected[name][number] = set(products) & set(ANALYSIS_PRODUCTS)
                else:
                    files.add(meta)
            except (OSError, ValueError, TypeError, AttributeError):
                # Never delete metadata we cannot classify confidently.
                continue
    for name in stale | active:
        # Score IDs have a terminal 12-digit hash; prefix matches are filtered
        # again with the exact score ID to avoid removing adjacent fixtures.
        for image in html.glob(name + '-*.png'):
            if image.is_dir():
                continue
            tail = image.name[len(name)+1:-4]
            score_page = re.fullmatch(r'[0-9]+', tail)
            derived = PRODUCT.fullmatch(tail)
            if not score_page and not derived:
                continue
            if name in stale:
                files.add(image)
            elif score_page:
                if int(tail) > audit['scores'][name]['37e_pages']:
                    files.add(image)
            elif derived:
                number = int(derived.group('page')[1:-1]) if derived.group('page') else 1
                if derived.group('suffix') not in expected[name].get(number, set()):
                    files.add(image)
    # The remaining stale metadata for active scores is not referenced by any
    # completed comparison. Avoid removing other JSON files in the cache.
    for name in active:
        for meta in (comparison_root / name).glob('page-*.json'):
            match = PAGE_META.fullmatch(meta.name)
            if match and int(match.group(1)) not in expected[name]:
                files.add(meta)

    files = {p for p in files if not any(parent in p.parents for parent in directories)}
    file_count = len(files)
    dir_count = len(directories)
    total_bytes = sum(p.stat(follow_symlinks=False).st_size for p in files if p.exists() or p.is_symlink())
    for directory in directories:
        total_bytes += sum(f.stat(follow_symlinks=False).st_size for f in directory.rglob('*') if f.is_file())
    if not dry_run:
        for file in sorted(files):
            file.unlink(missing_ok=True)
        for directory in sorted(directories):
            shutil.rmtree(directory)
    action = 'Would prune' if dry_run else 'Pruned'
    print(f'{action}: {len(stale)} unused score IDs, {dir_count} local cache dirs, '
          f'{file_count} generated files, {total_bytes / (1024*1024):.2f} MiB; '
          'Studio reference archives preserved.', flush=True)
    return {'unused_score_ids': len(stale), 'cache_directories': dir_count,
            'generated_files': file_count, 'bytes': total_bytes, 'dry_run': dry_run}
