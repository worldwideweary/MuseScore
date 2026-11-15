#!/usr/bin/env python3
"""v0.18.3: verify cached analyses, invalidation, and full/subset reports."""
import importlib.util
import json
from pathlib import Path
import sys
import unittest
import zipfile

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('test_v018', HERE / 'test-v018-isolated-compare.py')
v018 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(v018)


class CachedComparisons(v018.IsolatedCompareTests):
    test_combined_run_reuses_both_caches_and_refreshes_selectively = None
    test_isolated_output_and_shared_archive_and_audit_without_studio = None
    test_browser_uses_isolated_report_directory = None
    test_invalid_combinations_are_rejected = None

    def cache_entries(self):
        return sorted((self.out / 'html' / 'comparison-cache').rglob('*.json'))

    def test_repeat_and_bundle_reuse_analyses(self):
        first = self.run_gen('--bundle-changes', '--studio-source-tag', 'v4.7.7')
        self.assertIn('0 reused, 3 recalculated', first.stdout)
        cache_entries = self.cache_entries()
        self.assertEqual(len(cache_entries), 3)
        for entry in cache_entries:
            record = json.loads(entry.read_text())
            self.assertEqual(record['schema'], 1)
            self.assertEqual(record['identity']['mode'], 'paired')
            self.assertIn('ink-diff', record['products'])
            self.assertIn('raw_count', record['result'])
        original_timestamps = {e: e.stat().st_mtime_ns for e in cache_entries}
        original_calls = self.counts()
        repeat = self.run_gen('--bundle-changes', '--studio-source-tag', 'v4.7.7')
        self.assertIn('3 reused, 0 recalculated', repeat.stdout)
        self.assertEqual(self.counts(), original_calls)
        self.assertEqual(original_timestamps, {e: e.stat().st_mtime_ns for e in cache_entries})
        changes = json.loads((self.out / 'html' / 'changes' / 'changes.json').read_text())
        self.assertEqual(changes['changed_count'], 0)
        with zipfile.ZipFile(self.out / 'html' / 'changes' / 'latest-changes.zip') as z:
            self.assertFalse(any(n.endswith('.png') for n in z.namelist()))
        self.assertIn('Shape differences', self.report().read_text())
        # The first run reports fresh 37E exports, subsequent runs report
        # renderer cache hits; the image analysis itself must match.

    def test_deleted_or_corrupted_product_recalculates_one_score(self):
        self.run_gen()
        entries = self.cache_entries()
        self.assertEqual(len(entries), 3)
        target = entries[0]
        report = self.out / 'html'
        metadata = json.loads(target.read_text())
        key = metadata['result']['key']
        product = report / (key + '-ink-diff.png')
        product.write_bytes(b'not a valid png')
        check = self.run_gen()
        self.assertIn('2 reused, 1 recalculated', check.stdout)
        self.assertTrue(product.read_bytes().startswith(b'\x89PNG\r\n\x1a\n'))
        self.assertEqual(self.counts(), (3, 3), 'corrupt analysis products must not rerender score inputs')
        # Any implementation/resource revision must invalidate even intact products.
        environment = {**self.env, 'VTEST_COMPARISON_CACHE_SALT': 'new-ink-algorithm-v2'}
        revised = self.run_gen(env=environment)
        self.assertIn('0 reused, 3 recalculated', revised.stdout)
        again = self.run_gen(env=environment)
        self.assertIn('3 reused, 0 recalculated', again.stdout)
        forced = self.run_gen('--refresh-comparisons', env=environment)
        self.assertIn('0 reused, 3 recalculated', forced.stdout)
        self.assertEqual(self.counts(), (3, 3), 'analysis refresh must preserve renderer caches')

    def test_current_only_normalization_is_cached(self):
        import tempfile
        from PIL import Image
        from comparison_cache import ComparisonCache
        comp_spec = importlib.util.spec_from_file_location('vtest_compare', HERE / 'compare-engraving.py')
        comp = importlib.util.module_from_spec(comp_spec)
        comp_spec.loader.exec_module(comp)
        with tempfile.TemporaryDirectory() as temp:
            html = Path(temp)
            Image.new('RGB', (25, 25), 'white').save(html / 'unmatched-1.png')
            cache = ComparisonCache(html, comp)
            self.assertFalse(cache.normalize_current('unmatched', 1))
            self.assertTrue(cache.normalize_current('unmatched', 1))
            self.assertEqual((cache.current_only_hits, cache.current_only_misses), (1, 1))
            normalized = html / 'unmatched-normalized-current.png'
            normalized.write_bytes(b'bad image')
            self.assertFalse(cache.normalize_current('unmatched', 1))
            self.assertTrue(normalized.read_bytes().startswith(b'\x89PNG\r\n\x1a\n'))

    def test_analysis_round_trip_preserves_tuple_diagnostics(self):
        import tempfile
        from PIL import Image, ImageDraw
        from comparison_cache import ComparisonCache
        comp_spec = importlib.util.spec_from_file_location('vtest_compare', HERE / 'compare-engraving.py')
        comp = importlib.util.module_from_spec(comp_spec)
        comp_spec.loader.exec_module(comp)
        with tempfile.TemporaryDirectory() as temp:
            html = Path(temp)
            for suffix, offset in (('unit-1.png', 10), ('reference.png', 12)):
                im = Image.new('RGB', (100, 75), 'white')
                draw = ImageDraw.Draw(im)
                for row in (12, 22, 32, 42, 52):
                    draw.line((offset, row, 90, row), fill='black')
                im.save(html / suffix)
            cache = ComparisonCache(html, comp)
            fresh, hit = cache.analyse('unit', 1, html / 'reference.png')
            self.assertFalse(hit)
            restored, hit = cache.analyse('unit', 1, html / 'reference.png')
            self.assertTrue(hit)
            self.assertEqual(fresh, restored)
            self.assertTrue(all(isinstance(v, tuple) for v in
                                (restored['size'], restored['anchors'], restored['spacings'])))

    def test_changed_score_same_pixels_and_changed_reference_pixels(self):
        self.run_gen()
        score = self.scores / 'piece 1.mscx'
        score.write_text(score.read_text() + '\n<!-- changed, but fake renderer draws same pixels -->')
        unchanged = self.run_gen()
        self.assertEqual(self.counts(), (4, 4))
        self.assertIn('3 reused, 0 recalculated', unchanged.stdout)
        changes = json.loads((self.out / 'html' / 'changes' / 'changes.json').read_text())
        self.assertEqual(changes['changed_count'], 1, 'semantic score changes still appear in latest-changes report')
        # Force new Studio ink and therefore a different SHA-256 for each rendered reference.
        self.studio.write_text(self.studio.read_text().replace(
            "56 if kind == 'studio'", "60 if kind == 'studio'"))
        altered = self.run_gen()
        self.assertIn('0 reused, 3 recalculated', altered.stdout)
        self.assertEqual(self.counts(), (7, 4), 'changing Studio must not rerender 37E')


if __name__ == '__main__':
    suite = unittest.TestSuite(CachedComparisons(name) for name in (
        'test_repeat_and_bundle_reuse_analyses',
        'test_deleted_or_corrupted_product_recalculates_one_score',
        'test_changed_score_same_pixels_and_changed_reference_pixels',
        'test_current_only_normalization_is_cached',
        'test_analysis_round_trip_preserves_tuple_diagnostics',
    ))
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    sys.exit(not result.wasSuccessful())
