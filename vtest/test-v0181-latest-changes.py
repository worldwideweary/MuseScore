#!/usr/bin/env python3
"""v0.18.1: full report plus a per-run changes view, without extra rendering."""
import json
from pathlib import Path
import re
import sys
from urllib.parse import unquote
import unittest

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from report_changes import change_reasons, nested_asset_links

# Reuse the exact fake renderers/fixtures from the tested v0.18 integration.
import importlib.util
spec = importlib.util.spec_from_file_location('test_v018', HERE / 'test-v018-isolated-compare.py')
v018 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(v018)


class ChangeReportIntegration(v018.IsolatedCompareTests):
    """Run only new cases, not the inherited v0.18 cases on discovery."""
    test_combined_run_reuses_both_caches_and_refreshes_selectively = None
    test_isolated_output_and_shared_archive_and_audit_without_studio = None
    test_browser_uses_isolated_report_directory = None
    test_invalid_combinations_are_rejected = None

    def changes(self):
        return self.out / 'html' / 'changes'

    def inventory(self):
        return json.loads((self.changes() / 'changes.json').read_text())

    def verify_links(self):
        html = (self.changes() / 'engraving.html').read_text()
        # Images and nested links must resolve on disk without duplicating PNGs.
        for url in re.findall(r'(?:src|href)="([^"]+)"', html):
            if not url or url.startswith(('#', 'vtest-reveal:')):
                continue
            if url.startswith(('http:', 'https:', 'mailto:')):
                continue
            dest = (self.changes() / unquote(url)).resolve()
            self.assertTrue(dest.exists(), f'Broken changes-report link: {url}')
        self.assertFalse(list(self.changes().glob('*.png')))

    def test_new_unchanged_edit_add_and_remove(self):
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.assertEqual(self.inventory()['changed_count'], 3)
        self.assertIn('Latest changes', (self.changes() / 'engraving.html').read_text())
        self.assertIn('changes/engraving.html', self.report().read_text())
        self.verify_links()
        calls = self.counts()
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.assertEqual(self.counts(), calls)
        self.assertEqual(self.inventory()['changed_count'], 0)
        self.assertIn('No new or changed scores', (self.changes() / 'engraving.html').read_text())
        changed_score = self.scores / 'piece 1.mscx'
        changed_score.write_text(changed_score.read_text() + '<!-- edit -->')
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.assertEqual(self.inventory()['changed_count'], 1)
        self.assertIn('piece 1.mscx', self.inventory()['new_or_changed'])
        self.assertIn('score file changed', self.inventory()['new_or_changed']['piece 1.mscx'])
        self.assertEqual(len(json.loads((self.out / 'html' / 'catalogue-audit.json').read_text())['scores']), 3)
        self.verify_links()
        (self.scores / 'added.mscx').write_text('<museScore version="4.00"><Score/></museScore>')
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.assertEqual(self.inventory()['changed_count'], 1)
        self.assertIn('added.mscx', self.inventory()['new_or_changed'])
        self.assertEqual(len(json.loads((self.out / 'html' / 'catalogue-audit.json').read_text())['scores']), 4)
        self.verify_links()
        (self.scores / 'added.mscx').unlink()
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.assertEqual(self.inventory()['changed_count'], 0)
        self.assertEqual(self.inventory()['removed'], ['added.mscx'])
        self.verify_links()

    def test_renderer_changes_and_completed_snapshot_survive_partial_audit(self):
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.assertEqual(self.inventory()['changed_count'], 0)
        # The incremental active audit is deliberately incomplete after an
        # interruption; the durable completed checkpoint must win.
        (self.out / 'html' / 'catalogue-audit.json').write_text(json.dumps({
            'schema': 1, 'scores': {'x': {'reference_status': 'pending'}}}))
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.assertEqual(self.inventory()['changed_count'], 0)
        # A 37E rebuild changes every comparison, even if inputs are identical.
        self.current.write_text(self.current.read_text() + '\n# pretend rebuild\n')
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.assertEqual(self.inventory()['changed_count'], 3)
        self.assertTrue(all('37E executable changed' in reasons for reasons
                            in self.inventory()['new_or_changed'].values()))
        self.verify_links()


class TinyUnitTests(unittest.TestCase):
    def test_refresh_alone_is_not_change(self):
        score = {'37e_prepared_sha256': 'a', '37e_output_sha256': ['a'],
                 'reference_status': 'ok', '37e_status': 'ok', '37e_error': None,
                 'reference_error': None}
        doc = {'current_renderer_sha256': 'hash', 'scores': {'score': score}}
        self.assertEqual(change_reasons(doc, doc), ({}, []))

    def test_rebased_links_preserve_intra_report_anchor_and_manifest(self):
        html = ('<a href="#a">test</a><a href="vtest-reveal:///tmp/test.mscx">reveal</a>'
                '<a href="../../references/studio/manifest.json">manifest</a>'
                '<img src="name-normalized-current.png"><a href="catalogue-audit.json">audit</a>')
        result = nested_asset_links(html, '../../references/studio/manifest.json')
        self.assertIn('href="#a"', result)
        self.assertIn('href="vtest-reveal:///tmp/test.mscx"', result)
        self.assertIn('href="../../references/studio/manifest.json"', result)
        self.assertIn('src="../name-normalized-current.png"', result)
        self.assertIn('href="catalogue-audit.json"', result)


if __name__ == '__main__':
    # Do not inherit/re-run v0.18 cases automatically; only v0.18.1 tests.
    suite = unittest.TestSuite()
    for name in ('test_new_unchanged_edit_add_and_remove',
                 'test_renderer_changes_and_completed_snapshot_survive_partial_audit'):
        suite.addTest(ChangeReportIntegration(name))
    suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(TinyUnitTests))
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    sys.exit(not result.wasSuccessful())
