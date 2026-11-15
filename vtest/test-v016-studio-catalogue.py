#!/usr/bin/env python3
"""Regression tests for versioned Studio archive and failure-isolated auditing."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parent
CATALOGUE = ROOT / 'studio-catalogue.py'

FAKE = r'''#!/usr/bin/env python3
import json, os, pathlib, sys
from PIL import Image, ImageDraw
if '--long-version' in sys.argv or '--version' in sys.argv:
    print('Unstable Prerelease for Version 4.7.7; Build fake')
    sys.exit(0)
job = json.loads(pathlib.Path(sys.argv[sys.argv.index('-j') + 1]).read_text())[0]
out = pathlib.Path(job['out'])
name = out.stem
with open(os.environ['FAKE_CALLS'], 'a') as f:
    f.write(name + '\n')
if (os.environ.get('FAKE_KIND') == 'studio' and 'bad-studio' in name) or (
        os.environ.get('FAKE_KIND') == 'current' and 'bad-current' in name):
    print('fake unsupported feature detected for ' + name)
    sys.exit(27)
image = Image.new('RGB', (450, 160), 'white')
d = ImageDraw.Draw(image)
for y in range(20, 65, 10):
    d.line((30, y, 400, y), fill='black')
d.ellipse((92 if os.environ.get('FAKE_KIND') == 'current' else 102, 28,
           110 if os.environ.get('FAKE_KIND') == 'current' else 120, 38), fill='black')
image.save(out.with_name(name + '-1.png'))
print('export success')
'''


class CatalogueTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='vtest-v016-')
        root = Path(self.temp.name)
        self.scores = root / 'studio-vtest'
        (self.scores / 'scores_small').mkdir(parents=True)
        self.html = root / 'html'
        self.refs = root / 'refs'
        self.studio = root / 'studio-fake'
        self.current = root / '37e-fake'
        for executable in (self.studio, self.current):
            executable.write_text(FAKE, encoding='utf-8')
            executable.chmod(0o755)
        self.calls_studio = root / 'studio-calls'
        self.calls_current = root / 'current-calls'
        self.env_studio = {**os.environ, 'FAKE_KIND': 'studio',
                           'FAKE_CALLS': str(self.calls_studio)}
        self.env_current = {**os.environ, 'FAKE_KIND': 'current',
                            'FAKE_CALLS': str(self.calls_current)}
        score = '<museScore version="4.00"><Score><Style/><Staff/></Score></museScore>'
        for name in ('good.mscx', 'bad-studio.mscx', 'bad-current.mscx'):
            (self.scores / name).write_text(score, encoding='utf-8')
        with zipfile.ZipFile(self.scores / 'scores_small' / 'good.mscz', 'w') as z:
            z.writestr('score.mscx', score)
            z.writestr('Pictures/photo.png', b'keep-resource-bytes')

    def tearDown(self):
        self.temp.cleanup()

    def run_catalogue(self, mode, env, *extra, expected=0):
        cmd = [sys.executable, str(CATALOGUE), mode, '--score-dir', str(self.scores),
               '--archive-root', str(self.refs), '--html-dir', str(self.html),
               '--recursive', '--timeout', '10', *extra]
        if mode == '--cache-modern':
            cmd.extend(['--studio', str(self.studio)])
        else:
            cmd.extend(['--current', str(self.current)])
        result = subprocess.run(cmd, text=True, capture_output=True, env=env, timeout=35)
        self.assertEqual(result.returncode, expected, result.stdout + '\n' + result.stderr)
        return result

    def archive(self):
        folders = sorted(p for p in self.refs.glob('studio-*') if p.is_dir())
        self.assertEqual(len(folders), 1)
        return folders[0], json.loads((folders[0] / 'manifest.json').read_text())

    def test_versioned_cache_retry_preserves_manifest(self):
        self.run_catalogue('--cache-modern', self.env_studio, '--studio-source-tag', 'v4.7.7')
        folder, manifest = self.archive()
        self.assertIn('studio-v4.7.7-', folder.name)
        self.assertIn('Version 4.7.7', manifest['renderer']['reported_version'])
        self.assertEqual(manifest['renderer']['source_tag_user_supplied'], 'v4.7.7')
        self.assertEqual(manifest['summary']['total'], 4)
        self.assertEqual(manifest['summary']['ok'], 3)
        self.assertEqual(manifest['summary']['failed'], 1)
        self.assertEqual(len(self.calls_studio.read_text().splitlines()), 4)
        for name, entry in manifest['scores'].items():
            if entry['status'] == 'ok':
                self.assertEqual(entry['pages'][0]['file'], f'{name}-1.png')
                self.assertTrue((folder / 'scores' / name / f'{name}-1.png').is_file())
        content = (folder / 'manifest.json').read_bytes()
        self.run_catalogue('--cache-modern', self.env_studio, '--studio-source-tag', 'v4.7.7')
        self.assertEqual(len(self.calls_studio.read_text().splitlines()), 5,
                         'Only the originally failed score should rerender')
        self.assertEqual(content, (folder / 'manifest.json').read_bytes(),
                         'Cache hits must not dirty the Git-trackable manifest')

    def test_failure_isolated_current_audit_and_reference_without_studio(self):
        self.run_catalogue('--cache-modern', self.env_studio)
        folder, manifest = self.archive()
        self.run_catalogue('--audit-modern', self.env_current)
        audit = json.loads((self.html / 'catalogue-audit.json').read_text())
        statuses = {entry['relative_path']: entry for entry in audit['scores'].values()}
        self.assertEqual(statuses['bad-current.mscx']['37e_status'], 'failed')
        self.assertEqual(statuses['bad-current.mscx']['reference_status'], 'ok')
        self.assertEqual(statuses['bad-studio.mscx']['37e_status'], 'ok')
        self.assertEqual(statuses['bad-studio.mscx']['reference_status'], 'unavailable')
        self.assertEqual(statuses['good.mscx']['reference_status'], 'ok')
        self.assertEqual(len(self.calls_current.read_text().splitlines()), 4)
        report = (self.html / 'engraving.html').read_text()
        self.assertIn('renderer exited with status 27', report)
        self.assertIn('37E rendering failures: 1', report)
        self.assertIn('missing/stale Studio references: 1', report)
        self.assertIn('catalogue-logs/', report)
        self.assertIn('Archived Studio reference', report)
        self.assertIn('Shape differences', report)
        self.assertIn('Version 4.7.7', report)
        self.assertTrue(any((self.html / 'catalogue-logs').glob('*-37e.log')))

    def test_moved_score_root_reuses_archives_and_stale_is_not_compared(self):
        self.run_catalogue('--cache-modern', self.env_studio)
        folder, manifest = self.archive()
        target = self.scores.parent / 'moved'
        shutil.copytree(self.scores, target)
        self.scores = target
        self.run_catalogue('--audit-modern', self.env_current)
        first = json.loads((self.html / 'catalogue-audit.json').read_text())
        statuses = {e['relative_path']: e for e in first['scores'].values()}
        self.assertEqual(statuses['good.mscx']['reference_status'], 'ok')
        with (self.scores / 'good.mscx').open('a') as out:
            out.write('<!-- new score -->')
        self.run_catalogue('--audit-modern', self.env_current)
        stale = json.loads((self.html / 'catalogue-audit.json').read_text())
        statuses = {e['relative_path']: e for e in stale['scores'].values()}
        self.assertEqual(statuses['good.mscx']['reference_status'], 'unavailable')
        self.assertIn('stale reference', statuses['good.mscx']['reference_error'])
        report = (self.html / 'engraving.html').read_text()
        self.assertIn('stale reference', report)

    def test_filter_does_not_discard_old_archive_results(self):
        self.run_catalogue('--cache-modern', self.env_studio)
        folder, manifest = self.archive()
        self.run_catalogue('--cache-modern', self.env_studio, '--match', '^good\\.mscx$')
        _, updated = self.archive()
        self.assertEqual(set(manifest['scores']), set(updated['scores']))
        self.run_catalogue('--audit-modern', self.env_current, '--match', '^good\\.mscx$')
        audit = json.loads((self.html / 'catalogue-audit.json').read_text())
        self.assertEqual(len(audit['scores']), 1)


if __name__ == '__main__':
    unittest.main()
