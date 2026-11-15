#!/usr/bin/env python3
"""v0.18.4: cache pruning is opt-in, scoped, and never touches Studio archives."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('vtest_v018', HERE / 'test-v018-isolated-compare.py')
v018 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(v018)


class PruneIntegration(v018.IsolatedCompareTests):
    test_combined_run_reuses_both_caches_and_refreshes_selectively = None
    test_isolated_output_and_shared_archive_and_audit_without_studio = None
    test_browser_uses_isolated_report_directory = None
    test_invalid_combinations_are_rejected = None

    def test_prune_removed_score_preserves_studio_and_reports(self):
        self.run_gen('--studio-source-tag', 'v4.7.7')
        html = self.out / 'html'
        previous = json.loads((html / 'last-completed-audit.json').read_text())
        target_rel = 'piece 1.mscx'
        old_id = next(k for k, v in previous['scores'].items()
                      if v['relative_path'] == target_rel)
        page = html / (old_id + '-1.png')
        differences = html / (old_id + '-ink-diff.png')
        current_cache = html / '37e-cache' / old_id
        comparison_cache = html / 'comparison-cache' / old_id
        self.assertTrue(all(p.exists() for p in (page, differences, current_cache, comparison_cache)))
        studio_cache = next((self.out / 'references').glob('studio-*/scores')) / old_id
        self.assertTrue(studio_cache.exists())
        self.scores.joinpath(target_rel).unlink()
        before_calls = self.counts()
        preview = self.run_gen('--studio-source-tag', 'v4.7.7', '--prune-unused', '--prune-dry-run')
        self.assertIn('Would prune:', preview.stdout)
        self.assertTrue(current_cache.exists(), 'preview may not remove cached scores')
        self.assertTrue(page.exists())
        self.assertEqual(self.counts(), before_calls)
        executed = self.run_gen('--studio-source-tag', 'v4.7.7', '--prune-unused')
        self.assertIn('Pruned:', executed.stdout)
        for path in (page, differences, current_cache, comparison_cache):
            self.assertFalse(path.exists(), str(path))
        self.assertTrue(studio_cache.exists(), 'versioned Studio archive must remain available')
        updated_audit = json.loads((html / 'catalogue-audit.json').read_text())
        self.assertNotIn(old_id, updated_audit['scores'])
        self.assertNotIn('Sample%20scores/piece%201.mscx', (html / 'engraving.html').read_text())
        changes = json.loads((html / 'changes' / 'changes.json').read_text())
        # Preview must not advance the previous-completed-audit checkpoint.
        self.assertEqual(changes['removed_count'], 1)
        self.assertTrue((html / 'last-completed-audit.json').is_file())
        self.assertEqual(self.counts(), before_calls)
        again = self.run_gen('--studio-source-tag', 'v4.7.7', '--prune-unused')
        self.assertIn('0 unused score IDs', again.stdout)
        self.assertEqual(self.counts(), before_calls)

    def test_scope_refusals_and_unrelated_files(self):
        self.run_gen()
        html = self.out / 'html'
        unrelated = html / 'custom-export.png'
        unrelated.write_bytes(b'keep this user file')
        report = html / 'engraving.html'
        original = report.read_bytes()
        failed = self.run_gen('--match', 'piece 1', '--prune-unused', expected=2)
        self.assertIn('--prune-unused cannot be used with --match', failed.stderr)
        self.assertEqual(original, report.read_bytes())
        # Changing --recursive would unintentionally treat subdirectories as
        # orphaned; reject before any render or deletion.
        command = [str(v018.GEN), '--audit-modern', '--score-dir', str(self.scores),
                   '--output-dir', str(self.out), '--prune-unused']
        import subprocess
        proc = subprocess.run(command, env=self.env, capture_output=True, text=True, timeout=20)
        self.assertEqual(proc.returncode, 2)
        self.assertIn('same --score-dir and --recursive', proc.stderr)
        self.assertEqual(unrelated.read_bytes(), b'keep this user file')
        self.assertTrue(report.exists())

    def test_prune_old_extra_page_without_removing_active_score(self):
        self.run_gen()
        html = self.out / 'html'
        audit = json.loads((html / 'last-completed-audit.json').read_text())
        name = next(iter(audit['scores']))
        # Simulate outdated page-2 artifacts left by a prior 2-page render.
        extra_current = html / f'{name}-2.png'
        extra_current.write_bytes(b'extra old page')
        extra_diff = html / f'{name}-p2-ink-diff.png'
        extra_diff.write_bytes(b'extra old derived image')
        meta = html / 'comparison-cache' / name / 'page-002.json'
        meta.write_text(json.dumps({'schema': 1, 'identity': {'mode': 'paired'},
                                    'products': {'ink-diff': 'old'}}))
        run = self.run_gen('--prune-unused')
        self.assertIn('Pruned:', run.stdout)
        for path in (extra_current, extra_diff, meta):
            self.assertFalse(path.exists(), str(path))
        self.assertTrue((html / '37e-cache' / name).exists())
        self.assertTrue((html / f'{name}-1.png').exists())
        self.assertEqual(self.counts(), (3, 3))


if __name__ == '__main__':
    suite = unittest.TestSuite(PruneIntegration(name) for name in (
        'test_prune_removed_score_preserves_studio_and_reports',
        'test_scope_refusals_and_unrelated_files',
        'test_prune_old_extra_page_without_removing_active_score'))
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    raise SystemExit(not result.wasSuccessful())
