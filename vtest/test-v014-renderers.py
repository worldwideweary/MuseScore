#!/usr/bin/env python3
"""v0.14 renderer-selection and interrupted-reference-cache integration checks.

Uses small fake -j renderer executables. No MuseScore binary is needed.
Requires Python Pillow and ImageMagick 'compare' (the normal vtest dependencies).
"""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent

FAKE = '''#!/usr/bin/env python3
import json, os, sys
from pathlib import Path
from PIL import Image, ImageDraw
kind = KIND
args = sys.argv[1:]
if '--version' in args or '--long-version' in args or '-v' in args:
    print('MuseScore: Music Score Editor for Version 4.7.7; Build 1' if kind == 'studio'
          else 'MuseScore 3.6.2')
    sys.exit(0)
if '-j' not in args:
    sys.exit(4)
for job in json.loads(Path(args[args.index('-j')+1]).read_text()):
    source = Path(job['in'])
    if (os.environ.get('FAKE_FAIL_NAME') and os.environ.get('FAKE_FAIL_NAME') in source.name
            and kind == os.environ.get('FAKE_FAIL_KIND')):
        sys.exit(9)
    target = Path(job['out'])
    target.parent.mkdir(parents=True, exist_ok=True)
    image = Image.new('RGB', (512, 256), (230, 242, 255) if kind == '37e' else 'white')
    draw = ImageDraw.Draw(image)
    for y in (60, 69, 78, 87, 96):
        draw.line((20, y, 500, y), fill='black')
    draw.ellipse((112, 66, 120, 74), fill='black')
    image.save(target.with_name(target.stem + '-1.png'))
    with Path(os.environ['FAKE_RENDER_LOG']).open('a') as stream:
        stream.write(kind + ' ' + source.name + '\\n')
'''


@unittest.skipUnless(shutil.which('compare'), 'ImageMagick compare is required')
class RendererSelectionTests(unittest.TestCase):
    def setUp(self):
        self.workspace = tempfile.TemporaryDirectory(prefix='vtest-v014-')
        self.addCleanup(self.workspace.cleanup)
        self.root = Path(self.workspace.name)
        self.tests = self.root / 'vtest'
        self.tests.mkdir()
        for path in ROOT.iterdir():
            if path.suffix == '.py' or path.name in ('gen', 'README.md', 'engraving-profiles.json'):
                shutil.copy2(path, self.tests / path.name)
        (self.tests / 'style.css').write_text('body { font-family: sans-serif; }\n')
        fixture = ('<?xml version="1.0" encoding="UTF-8"?>\n'
                   '<museScore version="3.02"><Score><Style></Style></Score></museScore>\n')
        (self.tests / 'tie-1.mscx').write_text(fixture)
        (self.tests / 'novel-1.mscx').write_text(fixture)
        for name, kind in [('renderer37', '37e'), ('rendererStudio', 'studio'),
                           ('rendererOld', 'historical')]:
            f = self.root / name
            f.write_text(FAKE.replace('kind = KIND', f'kind = {kind!r}'))
            f.chmod(0o755)
        self.log = self.root / 'renders.log'
        self.log.write_text('')
        self.env = dict(os.environ, VTEST_MSCORE=str(self.root / 'renderer37'),
                        VTEST_REFERENCE_MSCORE=str(self.root / 'rendererStudio'),
                        VTEST_HISTORICAL_MSCORE=str(self.root / 'rendererOld'),
                        VTEST_BROWSER='/bin/true', FAKE_RENDER_LOG=str(self.log))

    def run_gen(self, *args, env=None, expected=0):
        proc = subprocess.run([str(self.tests / 'gen'), *args], cwd=self.root,
                              env={**self.env, **(env or {})}, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True)
        self.assertEqual(proc.returncode, expected, proc.stdout[-4500:])
        return proc.stdout

    def logs(self):
        return self.log.read_text().splitlines()

    def test_default_is_current_only_where_reference_missing(self):
        self.run_gen('novel-1')
        self.assertFalse(any('studio ' in line or 'historical ' in line for line in self.logs()))
        html = (self.tests / 'html' / 'engraving.html').read_text()
        self.assertIn('no reference PNG', html)
        self.assertNotIn('0.00%</td>', html)  # No fabricated parity percentage
        self.run_gen('novel-1', env={'VTEST_CAPTURE_BASELINE': '1'})
        self.run_gen('novel-1')
        html = (self.tests / 'html' / 'engraving.html').read_text()
        self.assertIn('Before/current differing pixels', html)

    def test_reference_selection_is_independent(self):
        self.run_gen('--modern', 'tie-1')
        self.assertEqual(sum(x.startswith('studio ') for x in self.logs()), 1)
        self.assertEqual(sum(x.startswith('historical ') for x in self.logs()), 0)
        self.run_gen('--historical', 'tie-1')
        self.assertEqual(sum(x.startswith('historical ') for x in self.logs()), 1)
        html = (self.tests / 'html' / 'engraving.html').read_text()
        self.assertIn('MuseScore 3.6.2', html)
        self.run_gen('--modern', '--historical', 'tie-1')
        html = (self.tests / 'html' / 'engraving.html').read_text()
        self.assertIn('Additional historical MuseScore comparison', html)

    def test_partial_reference_cache_survives_failure(self):
        self.run_gen('--modern', 'tie-1', 'novel-1',
                     env={'FAKE_FAIL_NAME': 'novel-1', 'FAKE_FAIL_KIND': 'studio'},
                     expected=1)  # gen propagates the failed reference script
        cache = self.tests / 'html' / 'reference-cache' / 'studio'
        self.assertTrue((cache / 'tie-1' / 'cache.json').is_file())
        self.assertFalse((cache / 'novel-1' / 'cache.json').exists())
        before = sum(x.startswith('studio ') for x in self.logs())
        self.run_gen('--modern', 'tie-1', 'novel-1')
        after = sum(x.startswith('studio ') for x in self.logs())
        self.assertEqual(after - before, 1)
        self.assertTrue((cache / 'novel-1' / 'cache.json').is_file())

    def test_all_without_studio(self):
        self.run_gen('--all')
        html = (self.tests / 'html' / 'engraving.html').read_text()
        self.assertIn('tie-1', html)
        self.assertIn('novel-1', html)
        self.assertFalse(any(x.startswith('studio ') for x in self.logs()))


if __name__ == '__main__':
    unittest.main()
