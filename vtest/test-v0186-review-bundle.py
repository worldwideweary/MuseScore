#!/usr/bin/env python3
"""v0.18.6: create standalone review exports without invalidating audit state."""
import importlib.util
import json
import subprocess
from pathlib import Path
import sys
import unittest
from zipfile import ZipFile

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('test_v018', HERE / 'test-v018-isolated-compare.py')
v018 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(v018)


class ReviewBundleTests(v018.IsolatedCompareTests):
    test_combined_run_reuses_both_caches_and_refreshes_selectively = None
    test_isolated_output_and_shared_archive_and_audit_without_studio = None
    test_browser_uses_isolated_report_directory = None
    test_invalid_combinations_are_rejected = None

    def export_only(self, scope, expected=0):
        result = subprocess.run([str(HERE / 'gen'), '--export-review', scope,
                                 '--output-dir', str(self.out)],
                                env=self.env, capture_output=True, text=True, timeout=45)
        self.assertEqual(result.returncode, expected, result.stdout + '\n' + result.stderr)
        return result

    def review(self, scope):
        if scope == 'all':
            return self.out / 'html' / 'engraving-review-all.zip'
        return self.out / 'html' / 'changes' / 'engraving-review-changes.zip'

    def inspect_review(self, scope, expected_count):
        archive = self.review(scope)
        self.assertTrue(archive.is_file())
        with ZipFile(archive) as z:
            self.assertIsNone(z.testzip())
            manifest = json.loads(z.read('manifest.json'))
            comparisons = json.loads(z.read('comparisons.json'))
            self.assertIn('index.html', z.namelist())
            self.assertIn('README.txt', z.namelist())
            self.assertEqual(len(manifest['scores']), expected_count)
            self.assertEqual(len(comparisons['scores']), expected_count)
            self.assertFalse(any(x.endswith(('.mscx', '.mscz')) for x in z.namelist()))
            for score in manifest['scores']:
                for page in score['pages']:
                    for role, item in page['images'].items():
                        self.assertIn(item['file'], z.namelist())
                        self.assertTrue(z.read(item['file']).startswith(b'\x89PNG\r\n\x1a\n'))
            if expected_count:
                first_img = next(image['file'] for score in manifest['scores']
                                 for page in score['pages']
                                 for image in page['images'].values())
                self.assertIn(first_img, z.read('index.html').decode())
        return manifest

    def test_export_existing_without_renderer_calls_or_checkpoint_changes(self):
        self.run_gen('--studio-source-tag', 'v4.7.7')
        checkpoint = self.out / 'html' / 'last-completed-audit.json'
        before = checkpoint.read_bytes()
        counts = self.counts()
        self.export_only('all')
        self.assertEqual(self.counts(), counts)
        self.assertEqual(checkpoint.read_bytes(), before)
        self.inspect_review('all', 3)
        self.export_only('changes')
        self.inspect_review('changes', 3)
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.export_only('changes')
        self.inspect_review('changes', 0)
        self.export_only('all')
        self.inspect_review('all', 3)
        self.assertEqual(self.counts(), counts)

    def test_automatic_review_and_one_changed_score(self):
        self.run_gen('--bundle-review', 'all', '--studio-source-tag', 'v4.7.7')
        self.inspect_review('all', 3)
        self.run_gen('--bundle-review', 'changes', '--studio-source-tag', 'v4.7.7')
        self.inspect_review('changes', 0)
        source = self.scores / 'piece 1.mscx'
        source.write_text(source.read_text() + '<!-- change -->')
        self.run_gen('--bundle-review', 'changes', '--studio-source-tag', 'v4.7.7')
        self.inspect_review('changes', 1)

    def test_corrupt_source_png_is_rejected_but_archive_intact(self):
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.export_only('all')
        original_zip = self.review('all').read_bytes()
        source = next((self.out / 'html').glob('external-*-1.png'))
        source.write_bytes(b'corrupt')
        self.export_only('all', expected=1)
        self.assertEqual(original_zip, self.review('all').read_bytes())

    def test_archive_reference_only_and_no_scores(self):
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.export_only('invalid', expected=2)
        self.export_only('all')
        self.inspect_review('all', 3)

    def test_studio_only_second_page_has_no_fabricated_37e_diff(self):
        self.studio.write_text(self.studio.read_text().replace(
            "img.save(out.with_name(name + '-1.png'))",
            "img.save(out.with_name(name + '-1.png'))\n"
            "if 'piece-1' in name: img.save(out.with_name(name + '-2.png'))"))
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.export_only('all')
        with ZipFile(self.review('all')) as z:
            manifest = json.loads(z.read('manifest.json'))
            second_pages = [page for score in manifest['scores'] for page in score['pages']
                            if page['page'] == 2]
            self.assertTrue(second_pages)
            for page in second_pages:
                self.assertIn('studio', page['images'])
                self.assertNotIn('37e', page['images'])
                self.assertNotIn('shapes-diff', page['images'])


if __name__ == '__main__':
    suite = unittest.TestSuite(ReviewBundleTests(name) for name in (
        'test_export_existing_without_renderer_calls_or_checkpoint_changes',
        'test_automatic_review_and_one_changed_score',
        'test_corrupt_source_png_is_rejected_but_archive_intact',
        'test_archive_reference_only_and_no_scores',
        'test_studio_only_second_page_has_no_fabricated_37e_diff',
    ))
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    sys.exit(not result.wasSuccessful())
