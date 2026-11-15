#!/usr/bin/env python3
"""Regression tests for per-page score labels and accurate page-count notices."""
import importlib.util
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
spec = importlib.util.spec_from_file_location('vtest_comparison', ROOT / 'compare-engraving.py')
comparison = importlib.util.module_from_spec(spec)
spec.loader.exec_module(comparison)


class SummaryPageLabels(unittest.TestCase):
    def test_names_on_every_page_and_unique_score_count(self):
        def result(score, n, total, **more):
            return {'name': score, 'key': score if n == 1 else f'{score}-p{n}',
                    'page': n, 'pages_total': total,
                    'input': {'origin': '/upstream/vtest/scores/' + score.removeprefix('external-') + '.mscz',
                              'profile': 'external-original', 'source_sha256': '123'},
                    **more}

        # A Studio archive with one 37E-extra score, one Studio-extra score,
        # and one failed score. Failure must not count as a pagination mismatch.
        statuses = {
            'a': {'37e_status': 'ok', 'reference_status': 'ok',
                  '37e_cached': True, '37e_pages': 2, 'reference_pages': 1},
            'b': {'37e_status': 'ok', 'reference_status': 'ok',
                  '37e_cached': False, '37e_pages': 1, 'reference_pages': 2},
            'c': {'37e_status': 'failed', 'reference_status': 'ok',
                  '37e_cached': False, '37e_pages': 0, 'reference_pages': 2}}
        archive = {'path': 'references/studio/manifest.json',
                   'renderer': {'reported_version': '4.7.7', 'executable_sha256': 'abc'},
                   'audit': {'scores': statuses, 'jobs': 2}}
        results = [
            result('external-chord-layout-19', 1, 2, current_only=True),
            result('external-chord-layout-19', 2, 2, current_only=True,
                   current_only_note='No corresponding Studio page — 37E has an additional page (page counts differ)'),
            result('external-accidental-10', 1, 2, current_only=True),
            result('external-accidental-10', 2, 2, missing='No corresponding 37E page — Studio has an additional page (page counts differ)'),
            result('external-other', 1, 1, current_only=True),
        ]
        html = comparison.page(results, 5, 'Studio 4.7.7', archive=archive)
        self.assertIn('>chord-layout-19.mscz (scores) — Page 1</a>', html)
        self.assertIn('>chord-layout-19.mscz (scores) — Page 2</a>', html)
        self.assertIn('>accidental-10.mscz (scores) — Page 1</a>', html)
        self.assertIn('>accidental-10.mscz (scores) — Page 2</a>', html)
        self.assertIn('>other.mscz (scores)</a>', html)
        self.assertNotIn('class="group-row"', html)
        self.assertIn('Page-count differences: 2 scores (37E has extra pages: 1; Studio has extra pages: 1)', html)
        self.assertIn('No corresponding Studio page — 37E has an additional page', html)
        self.assertIn('No corresponding 37E page — Studio has an additional page', html)
        self.assertIn('data-score-group="score-group-1"', html)
        self.assertIn('id="score-group-1"', html)
        self.assertIn('setScoreGroupsOpen(true)', html)
        self.assertIn('No reference-difference percentage was calculated', html)


if __name__ == '__main__':
    unittest.main()
