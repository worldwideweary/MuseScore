#!/usr/bin/env python3
"""Verified, incremental cache for catalogue *image analysis*, not rendering.

The derived PNGs already live in html/ for the full/subset HTML reports. Keep
those files in place and store per-page hashes plus analysis results alongside
in html/comparison-cache/. No duplicate PNG tree is required. A cache hit means
both source PNGs, the analyzer implementation, settings, and every derived PNG
are verified. Interrupted or corrupt entries are recalculated automatically.
"""
import hashlib
import json
import os
from pathlib import Path

import PIL


SCHEMA = 1
COMMON = ('normalized-current', 'normalized-reference', 'normalized-diff',
          'ink-diff', 'directional-diff')
ALIGNED = ('aligned-current', 'aligned-diff', 'aligned-ink-diff',
           'aligned-directional-diff')
ALL = COMMON + ALIGNED


def file_hash(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as src:
        for data in iter(lambda: src.read(1024 * 1024), b''):
            h.update(data)
    return h.hexdigest()


class ComparisonCache:
    def __init__(self, html, comparison, tolerance=5, ink_cutoff=190, refresh=False):
        self.html = Path(html)
        self.comparison = comparison
        self.tolerance = tolerance
        self.ink_cutoff = ink_cutoff
        self.root = self.html / 'comparison-cache'
        self.refresh = refresh
        self.identity = {
            'analysis_sha256': file_hash(comparison.__file__),
            'pillow_version': PIL.__version__,
            'tolerance': tolerance, 'ink_cutoff': ink_cutoff,
            'resource_salt': os.environ.get('VTEST_COMPARISON_CACHE_SALT', ''),
        }
        self.hits = 0
        self.misses = 0
        self.current_only_hits = 0
        self.current_only_misses = 0

    @staticmethod
    def key(name, page):
        return name if page == 1 else f'{name}-p{page}'

    def paths(self, key):
        return {suffix: self.html / f'{key}-{suffix}.png' for suffix in ALL}

    def metadata_path(self, name, page):
        return self.root / name / f'page-{page:03}.json'

    def expected(self, name, page, current, reference, mode):
        return {
            'mode': mode, 'score': name, 'page': page,
            'current_sha256': file_hash(current),
            'reference_sha256': file_hash(reference) if reference is not None else None,
            'analysis': self.identity,
        }

    def restored(self, name, page, identity, required):
        if self.refresh:
            return None
        try:
            data = json.loads(self.metadata_path(name, page).read_text(encoding='utf-8'))
            if data.get('schema') != SCHEMA or data.get('identity') != identity:
                return None
            files = data.get('products')
            if not isinstance(files, dict) or not set(required).issubset(files):
                return None
            if not set(files).issubset(ALL):
                return None
            if identity['mode'] == 'paired':
                record = data.get('result')
                fields = ('raw_count', 'aligned_count', 'shape_counts', 'aligned_shape_counts',
                          'shift', 'anchors', 'spacings', 'backgrounds', 'size', 'original_sizes')
                if (not isinstance(record, dict) or record.get('name') != name or
                        record.get('page') != page or any(field not in record for field in fields)):
                    return None
                allowed = set(COMMON + (ALIGNED if record.get('shift') is not None else ()))
                if set(files) != allowed:
                    return None
            elif set(files) != {'normalized-current'}:
                return None
            targets = self.paths(self.key(name, page))
            if any(not targets[suffix].is_file() or file_hash(targets[suffix]) != digest
                   for suffix, digest in files.items()):
                return None
            self.remove_unused(name, page, set(files))
            if identity['mode'] == 'paired':
                # Preserve tuple-valued diagnostics exactly as analyse() returns
                # them, so HTML is identical on fresh and cached runs.
                r = data['result']
                for field in ('backgrounds', 'size', 'original_sizes', 'shape_counts',
                              'aligned_shape_counts', 'shift', 'spacings'):
                    if r[field] is not None:
                        r[field] = tuple(r[field])
                for field in ('backgrounds', 'original_sizes'):
                    if r[field] is not None:
                        r[field] = tuple(tuple(part) if part is not None else None
                                         for part in r[field])
                r['anchors'] = tuple(tuple(a) if a is not None else None
                                     for a in r['anchors'])
            return data
        except (OSError, ValueError, TypeError, KeyError):
            return None

    def remove_unused(self, name, page, keep):
        for suffix, path in self.paths(self.key(name, page)).items():
            if suffix not in keep:
                path.unlink(missing_ok=True)

    def save(self, name, page, identity, suffixes, result=None):
        products = {}
        paths = self.paths(self.key(name, page))
        for suffix in suffixes:
            products[suffix] = file_hash(paths[suffix])
        dest = self.metadata_path(name, page)
        dest.parent.mkdir(parents=True, exist_ok=True)
        meta = {'schema': SCHEMA, 'identity': identity,
                'products': products, 'result': result}
        temp = dest.with_suffix('.json.tmp')
        temp.write_text(json.dumps(meta, sort_keys=True, indent=2) + '\n', encoding='utf-8')
        temp.replace(dest)

    def analyse(self, name, page, reference):
        """Return (numerical analysis result, cache hit)."""
        current = self.html / f'{name}-{page}.png'
        identity = self.expected(name, page, current, reference, 'paired')
        hit = self.restored(name, page, identity, COMMON)
        if hit is not None:
            self.hits += 1
            return hit['result'], True
        self.remove_unused(name, page, set())
        result = self.comparison.analyse(
            self.html, name, self.tolerance, self.ink_cutoff,
            page, reference_path=reference)
        suffixes = COMMON + (ALIGNED if result['shift'] is not None else ())
        self.save(name, page, identity, suffixes, result)
        self.misses += 1
        return result, False

    def normalize_current(self, name, page):
        """Normalize unmatched 37E pages without repeating work on cache hits."""
        current = self.html / f'{name}-{page}.png'
        identity = self.expected(name, page, current, None, 'current-only')
        hit = self.restored(name, page, identity, ('normalized-current',))
        if hit is not None:
            self.current_only_hits += 1
            return True
        self.remove_unused(name, page, set())
        normalized, _ = self.comparison.normalize_ink(current)
        normalized.save(self.paths(self.key(name, page))['normalized-current'])
        self.save(name, page, identity, ('normalized-current',))
        self.current_only_misses += 1
        return False
