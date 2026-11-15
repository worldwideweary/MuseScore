#!/usr/bin/env python3
"""v0.18: isolated, single-command Studio+37E catalogue comparisons."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest
import zipfile

HERE = Path(__file__).resolve().parent
GEN = HERE / 'gen'

FAKE = '''#!/usr/bin/env python3
import json, os, pathlib, sys
from PIL import Image, ImageDraw
kind = 'studio' if 'studio' in pathlib.Path(sys.argv[0]).name else 'current'
if '--long-version' in sys.argv or '--version' in sys.argv:
    print('Unstable Prerelease for Version 4.7.7; Build test')
    sys.exit(0)
if kind == 'current' and '-f' not in sys.argv:
    raise SystemExit('37E needs forced loading in catalogue')
job = json.loads(pathlib.Path(sys.argv[sys.argv.index('-j') + 1]).read_text())[0]
out = pathlib.Path(job['out']); name = out.stem
with open(os.environ['FAKE_CALLS'], 'a') as f:
    f.write(kind + ' ' + name + '\\n')
img = Image.new('RGB', (150, 75), 'white')
painter = ImageDraw.Draw(img)
for y in (12,22,32,42,52):
    painter.line((12, y, 135, y), fill='black')
painter.ellipse((56 if kind == 'studio' else 54,25,65,35),fill='black')
img.save(out.with_name(name + '-1.png'))
'''


class IsolatedCompareTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='vtest-v018-')
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.scores = self.root / 'Sample scores'
        (self.scores / 'more').mkdir(parents=True)
        text = '<museScore version="4.00"><Score><Style/><Staff/></Score></museScore>'
        (self.scores / 'piece 1.mscx').write_text(text, encoding='utf-8')
        (self.scores / 'more' / 'piece 1.mscx').write_text(text + '<!-- 2 -->', encoding='utf-8')
        with zipfile.ZipFile(self.scores / 'more' / 'compressed.mscz', 'w') as z:
            z.writestr('score.mscx', text)
            z.writestr('Pictures/test.png', b'embedded resource')
        self.studio = self.root / 'fake-studio'
        self.current = self.root / 'fake-current'
        for exe in (self.studio, self.current):
            exe.write_text(FAKE, encoding='utf-8')
            exe.chmod(0o755)
        self.calls = self.root / 'calls.txt'
        self.out = self.root / 'reports' / 'collection-A'
        self.env = {**os.environ,
                    'FAKE_CALLS': str(self.calls),
                    'VTEST_REFERENCE_MSCORE': str(self.studio),
                    'VTEST_MSCORE': str(self.current),
                    'VTEST_OPEN_REPORT': '0'}

    def run_gen(self, *extra, env=None, expected=0, out=None, mode='--compare-modern'):
        command = [str(GEN), mode, '--score-dir', str(self.scores), '--recursive',
                   '--jobs', '2', '--output-dir', str(out or self.out), *extra]
        completed = subprocess.run(command, env=env or self.env, text=True,
                                   capture_output=True, timeout=85)
        self.assertEqual(completed.returncode, expected, completed.stdout + '\n' + completed.stderr)
        return completed

    def counts(self):
        lines = self.calls.read_text().splitlines() if self.calls.is_file() else []
        return (sum(x.startswith('studio ') for x in lines),
                sum(x.startswith('current ') for x in lines))

    def report(self, out=None):
        return (out or self.out) / 'html' / 'engraving.html'

    def test_combined_run_reuses_both_caches_and_refreshes_selectively(self):
        first = self.run_gen('--studio-source-tag', 'v4.7.7')
        self.assertEqual(self.counts(), (3, 3))
        self.assertIn('Studio reference pass complete', first.stdout)
        self.assertTrue(self.report().is_file())
        self.assertIn('Shape differences', self.report().read_text())
        manifests = list((self.out / 'references').glob('studio-*/manifest.json'))
        self.assertEqual(len(manifests), 1)
        arch = json.loads(manifests[0].read_text())
        self.assertEqual(len(arch['scores']), 3)
        self.assertEqual(arch['renderer']['source_tag_user_supplied'], 'v4.7.7')
        self.assertTrue((self.out / 'html' / '37e-cache').is_dir())
        self.assertTrue((self.out / 'html' / 'catalogue-audit.json').is_file())
        second = self.run_gen('--studio-source-tag', 'v4.7.7')
        self.assertEqual(self.counts(), (3, 3))
        self.assertIn('verified archive cache hit', second.stdout)
        self.assertIn('verified 37E cache hit', second.stdout)
        # Refresh one renderer at a time.
        self.run_gen('--studio-source-tag', 'v4.7.7', '--refresh-modern')
        self.assertEqual(self.counts(), (6, 3))
        self.run_gen('--studio-source-tag', 'v4.7.7', '--refresh-current')
        self.assertEqual(self.counts(), (6, 6))
        # Rebuilding one score changes both effective inputs.
        file = self.scores / 'piece 1.mscx'
        file.write_text(file.read_text() + '<!-- new engraving source -->')
        self.run_gen('--studio-source-tag', 'v4.7.7')
        self.assertEqual(self.counts(), (7, 7))

    def test_isolated_output_and_shared_archive_and_audit_without_studio(self):
        self.run_gen('--studio-source-tag', 'v4.7.7')
        a_manifest = next((self.out / 'references').glob('studio-*/manifest.json'))
        original_html = self.report().read_bytes()
        second = self.root / 'reports' / 'collection-B'
        other = self.run_gen('--studio-source-tag', 'v4.7.7', '--archive-root',
                             str(self.out / 'references'), out=second)
        self.assertEqual(self.counts(), (3, 6), 'different report reuses Studio, but builds separate 37E cache')
        self.assertIn('verified archive cache hit', other.stdout)
        self.assertTrue(self.report(second).exists())
        self.assertEqual(self.report().read_bytes(), original_html)
        self.assertFalse((second / 'references').exists(), 'explicit shared reference root should win')
        self.assertTrue(a_manifest.is_file())
        # Repeat an archive audit without any Studio executable configured.
        env = dict(self.env)
        env.pop('VTEST_REFERENCE_MSCORE')
        audited = self.run_gen('--archive-root', str(self.out / 'references'),
                               out=second, mode='--audit-modern', env=env)
        self.assertIn('verified 37E cache hit', audited.stdout)
        self.assertEqual(self.counts(), (3, 6))

    def test_browser_uses_isolated_report_directory(self):
        browser = self.root / 'fake-browser'
        browser_log = self.root / 'browser.log'
        browser.write_text('#!/bin/sh\nprintf "%s\\n" "$PWD/$1" > "$VTEST_BROWSER_LOG"\n')
        browser.chmod(0o755)
        env = {**self.env, 'VTEST_OPEN_REPORT': '1', 'VTEST_BROWSER': str(browser),
               'VTEST_BROWSER_LOG': str(browser_log)}
        result = self.run_gen(env=env)
        self.assertIn('Opening engraving report:', result.stdout)
        for _ in range(60):
            if browser_log.exists():
                break
            time.sleep(.05)
        self.assertEqual(browser_log.read_text().strip(), str(self.report()))
        # No default vtest/html directory should have been touched.
        self.assertEqual(self.counts(), (3, 3))

    def test_invalid_combinations_are_rejected(self):
        self.run_gen('--archive', '/tmp/fake-old-archive', expected=2)
        self.run_gen('--refresh-modern', expected=2, mode='--audit-modern')
        self.run_gen('--refresh-current', expected=2, mode='--cache-modern')
        self.run_gen(out=self.scores / 'generated', expected=2)


if __name__ == '__main__':
    unittest.main()
