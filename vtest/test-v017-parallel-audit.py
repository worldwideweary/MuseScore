#!/usr/bin/env python3
"""Tests v0.17: forced newer-score loading, parallel jobs, cache, browser."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parent
CATALOGUE = ROOT / 'studio-catalogue.py'
GEN = ROOT / 'gen'

FAKE = '''#!/usr/bin/env python3
import json, os, pathlib, sys, time
from PIL import Image, ImageDraw
if '--long-version' in sys.argv or '--version' in sys.argv:
    print('Unstable Prerelease for Version 4.7.7; Build fake')
    sys.exit(0)
kind = os.environ['FAKE_KIND']
if kind == 'current' and '-f' not in sys.argv:
    print('This score was saved using a newer version of MuseScore')
    sys.exit(66)
job = json.loads(pathlib.Path(sys.argv[sys.argv.index('-j') + 1]).read_text())[0]
out = pathlib.Path(job['out'])
name = out.stem
with open(os.environ['FAKE_CALLS'], 'a') as f:
    f.write(kind + ' ' + name + ' ' + ' '.join(sys.argv[1:]) + '\\n')
time.sleep(0.12)
if kind == 'current' and 'broken-' in name:
    print('unsupported element caused a simulated renderer failure')
    sys.exit(27)
image = Image.new('RGB', (300, 130), 'white')
canvas = ImageDraw.Draw(image)
for y in (30, 40, 50, 60, 70):
    canvas.line((15, y, 260, y), fill='black')
canvas.ellipse((70, 38, 84, 48), fill='black')
image.save(out.with_name(name + '-1.png'))
'''

class ParallelAuditTests(unittest.TestCase):
    def setUp(self):
        self.t = tempfile.TemporaryDirectory(prefix='vtest-v017-')
        base = Path(self.t.name)
        self.scores = base / 'scores'
        self.scores.mkdir()
        self.html = base / 'html'
        self.refs = base / 'refs'
        self.studio = base / 'fake-studio'
        self.current = base / 'fake-current'
        self.calls = base / 'calls.txt'
        for exe in (self.studio, self.current):
            exe.write_text(FAKE)
            exe.chmod(0o755)
        self.browser = base / 'fake-browser'
        self.browser_calls = base / 'browser.txt'
        self.browser.write_text('#!/bin/sh\nprintf "%s\\n" "$PWD/$1" >> "$BROWSER_LOG"\n')
        self.browser.chmod(0o755)
        self.env = {**os.environ, 'FAKE_CALLS': str(self.calls),
                    'BROWSER_LOG': str(self.browser_calls)}
        for name in ('a.mscx', 'b.mscx', 'broken-case.mscx'):
            (self.scores / name).write_text('<museScore version="4.00"><Score><Style/><Staff/></Score></museScore>')
        import zipfile
        with zipfile.ZipFile(self.scores / 'd.mscz', 'w') as z:
            z.writestr('score.mscx', '<museScore version="4.00"><Score><Style/><Staff/></Score></museScore>')
            z.writestr('Pictures/still-here.png', b'embedded resource retained')

    def tearDown(self):
        self.t.cleanup()

    def run_mode(self, mode, *extra):
        env = dict(self.env)
        env['FAKE_KIND'] = 'studio' if mode == '--cache-modern' else 'current'
        command = [sys.executable, str(CATALOGUE), mode, '--score-dir', str(self.scores),
                   '--html-dir', str(self.html), '--archive-root', str(self.refs),
                   '--jobs', '3', '--timeout', '20', *extra]
        command += ['--studio', str(self.studio)] if mode == '--cache-modern' else ['--current', str(self.current)]
        proc = subprocess.run(command, env=env, capture_output=True, text=True, timeout=60)
        self.assertEqual(proc.returncode, 0, proc.stdout + '\n' + proc.stderr)
        return proc

    def calls_of(self, kind):
        return [line for line in self.calls.read_text().splitlines() if line.startswith(kind + ' ')]

    def test_concurrent_force_cache_resume_invalidation_and_refresh(self):
        self.run_mode('--cache-modern')
        self.assertEqual(len(self.calls_of('studio')), 4)
        first_audit = self.run_mode('--audit-modern')
        self.assertIn('37E rendering/cache checks complete: 4 scores', first_audit.stdout)
        self.assertIn('Generating normalized images, page differences', first_audit.stdout)
        self.assertIn('[report 1/4]', first_audit.stdout)
        self.assertIn('[report 4/4]', first_audit.stdout)
        self.assertIn('Image comparisons complete:', first_audit.stdout)
        self.assertIn('Assembling HTML summary and navigation...', first_audit.stdout)
        self.assertIn('Writing engraving.html...', first_audit.stdout)
        self.assertEqual(len(self.calls_of('current')), 4)
        self.assertTrue(all(' -f ' in line for line in self.calls_of('current')))
        audited = json.loads((self.html / 'catalogue-audit.json').read_text())
        values = list(audited['scores'].values())
        self.assertEqual(sum(v['37e_status'] == 'ok' for v in values), 3)
        self.assertEqual(sum(v['37e_status'] == 'failed' for v in values), 1)
        self.assertEqual(audited['jobs'], 3)
        self.assertTrue(audited['force_newer_scores'])
        self.run_mode('--audit-modern')
        self.assertEqual(len(self.calls_of('current')), 5, 'only the failed score should retry')
        audited = json.loads((self.html / 'catalogue-audit.json').read_text())
        self.assertEqual(sum(v['37e_cached'] for v in audited['scores'].values()), 3)
        self.assertEqual(sum(v['37e_status'] == 'failed' for v in audited['scores'].values()), 1)
        self.assertIn('37E cache hits: 3', (self.html / 'engraving.html').read_text())
        self.assertIn('Shape differences', (self.html / 'engraving.html').read_text())
        self.run_mode('--audit-modern', '--refresh-current')
        self.assertEqual(len(self.calls_of('current')), 9, '--refresh-current must rerender all')
        (self.scores / 'a.mscx').write_text('<museScore version="4.00"><Score/></museScore>')
        self.run_mode('--audit-modern')
        self.assertEqual(len(self.calls_of('current')), 11, 'modified score plus failed score should rerender')
        audited = json.loads((self.html / 'catalogue-audit.json').read_text())
        self.assertTrue(any('stale reference' in str(s.get('reference_error'))
                            for s in audited['scores'].values()))
        # An executable rebuild must invalidate all previously valid 37E pages.
        self.current.write_text(FAKE + '\n# rebuild changes binary SHA\n')
        self.run_mode('--audit-modern')
        self.assertEqual(len(self.calls_of('current')), 15,
                         'rebuilding 37E must rerender all four scores')

    def test_browser_open_on_success_and_disable_for_headless(self):
        self.run_mode('--cache-modern')
        env = {**self.env, 'FAKE_KIND': 'current',
               'VTEST_MSCORE': str(self.current),
               'VTEST_BROWSER': str(self.browser)}
        archive = next(self.refs.glob('studio-*'))
        cmd = [str(GEN), '--audit-modern', '--score-dir', str(self.scores),
               '--archive', str(archive), '--jobs', '3']
        # gen uses source-root vtest/html; we do not want to write there during
        # these tests. The real gen has no --html-dir; test browser with a
        # disposable copy of the script/modules using an isolated directory.
        import shutil
        local = Path(self.t.name) / 'copy' / 'vtest'
        local.mkdir(parents=True)
        for src in ('gen','studio-catalogue.py','prepare-inputs.py','score_archive.py',
                    'compare-engraving.py','reference_cache.py','render-studio-references.py',
                    'input_provenance.py','engraving-profiles.json'):
            shutil.copy2(ROOT / src, local / src)
        (local / 'gen').chmod(0o755)
        cmd[0] = str(local / 'gen')
        proc = subprocess.run(cmd, env=env, text=True, capture_output=True, timeout=60)
        self.assertEqual(proc.returncode, 0, proc.stdout + '\n' + proc.stderr)
        for _ in range(30):
            if self.browser_calls.exists():
                break
            time.sleep(0.05)
        self.assertTrue(self.browser_calls.is_file(), 'browser did not start')
        self.assertIn('engraving.html', self.browser_calls.read_text())
        self.assertIn('Opening engraving report:', proc.stdout)
        self.browser_calls.unlink()
        env['VTEST_OPEN_REPORT'] = '0'
        proc = subprocess.run(cmd, env=env, text=True, capture_output=True, timeout=60)
        self.assertEqual(proc.returncode, 0, proc.stdout + '\n' + proc.stderr)
        time.sleep(0.12)
        self.assertFalse(self.browser_calls.exists(), 'headless should skip browser')

if __name__ == '__main__':
    unittest.main()
