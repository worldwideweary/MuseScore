#!/usr/bin/env python3
"""v0.18.2: portable image-only bundle for the latest changed scores."""
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


class BundleIntegration(v018.IsolatedCompareTests):
    # Do not collect inherited v0.18 test cases when loaded normally.
    test_combined_run_reuses_both_caches_and_refreshes_selectively = None
    test_isolated_output_and_shared_archive_and_audit_without_studio = None
    test_browser_uses_isolated_report_directory = None
    test_invalid_combinations_are_rejected = None

    def bundle(self):
        return self.out / 'html' / 'changes' / 'latest-changes.zip'

    def inspect_bundle(self, expected_count):
        self.assertTrue(self.bundle().exists())
        with zipfile.ZipFile(self.bundle()) as zipped:
            self.assertIsNone(zipped.testzip())
            names = zipped.namelist()
            manifest = json.loads(zipped.read('manifest.json'))
            self.assertIn('README.txt', names)
            self.assertEqual(len(manifest['scores']), expected_count)
            self.assertFalse(any(n.endswith(('.mscx', '.mscz')) for n in names))
            for score in manifest['scores']:
                self.assertTrue(score['relative_path'])
                self.assertTrue(score['reasons'])
                self.assertTrue(score['pages'])
                for page in score['pages']:
                    imgs = page['images']
                    self.assertIn('37e', imgs)
                    self.assertIn('studio', imgs)
                    self.assertIn('shapes-diff', imgs)
                    for label, meta in imgs.items():
                        self.assertIn(meta['file'], names)
                        self.assertTrue(zipped.read(meta['file']).startswith(b'\x89PNG\r\n\x1a\n'))
        return manifest

    def test_latest_bundle_new_changed_and_empty(self):
        self.run_gen('--bundle-changes', '--studio-source-tag', 'v4.7.7')
        initial = self.inspect_bundle(3)
        self.assertIn('changes/latest-changes.zip', self.report().read_text())
        self.assertIn('href="latest-changes.zip"',
                      (self.out / 'html' / 'changes' / 'engraving.html').read_text())
        self.assertEqual(len(initial['scores']), 3)
        self.assertEqual(len(json.loads(self.report().with_name('catalogue-audit.json').read_text())['scores']), 3)
        prior_calls = self.counts()

        self.run_gen('--bundle-changes', '--studio-source-tag', 'v4.7.7')
        empty = self.inspect_bundle(0)
        self.assertEqual(self.counts(), prior_calls)
        self.assertEqual(empty['scores'], [])

        score_path = self.scores / 'piece 1.mscx'
        score_path.write_text(score_path.read_text() + '\n<!-- change -->')
        self.run_gen('--bundle-changes', '--studio-source-tag', 'v4.7.7')
        changed = self.inspect_bundle(1)
        self.assertEqual(changed['scores'][0]['relative_path'], 'piece 1.mscx')
        self.assertIn('score file changed', changed['scores'][0]['reasons'])
        self.assertEqual(len(json.loads(self.report().with_name('catalogue-audit.json').read_text())['scores']), 3)

    def test_mismatched_page_counts_and_removed_stale_zip(self):
        # A genuine extra Studio page must still be shareable, even with no 37E PNG.
        old_script = self.studio.read_text()
        self.studio.write_text(old_script.replace(
            "img.save(out.with_name(name + '-1.png'))",
            "img.save(out.with_name(name + '-1.png'))\n"
            "if 'piece-1' in name: img.save(out.with_name(name + '-2.png'))"))
        self.run_gen('--bundle-changes', '--studio-source-tag', 'v4.7.7')
        with zipfile.ZipFile(self.bundle()) as zipped:
            data = json.loads(zipped.read('manifest.json'))
            extra = [page for score in data['scores'] for page in score['pages']
                     if page['page'] == 2]
            self.assertTrue(extra)
            for page in extra:
                self.assertIn('studio', page['images'])
                self.assertNotIn('37e', page['images'])
                self.assertNotIn('shapes-diff', page['images'])
        # Deliberately disabling bundle generation must not leave an old ZIP
        # that appears to represent a different completed comparison.
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.assertFalse(self.bundle().exists())

    def test_audit_only_and_invalid_invocations(self):
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.run_gen('--bundle-changes', mode='--audit-modern')
        self.inspect_bundle(0)
        self.run_gen('--bundle-changes', mode='--cache-modern', expected=2)


if __name__ == '__main__':
    suite = unittest.TestSuite()
    for name in ('test_latest_bundle_new_changed_and_empty',
                 'test_audit_only_and_invalid_invocations',
                 'test_mismatched_page_counts_and_removed_stale_zip'):
        suite.addTest(BundleIntegration(name))
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    sys.exit(not result.wasSuccessful())
