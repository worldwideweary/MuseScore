#!/usr/bin/env python3
"""v0.15 external directories and compressed-score integration tests.

Uses simulated MuseScore executables; needs Pillow and ImageMagick compare.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('v014', ROOT / 'test-v014-renderers.py')
v014 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(v014)


@unittest.skipUnless(shutil.which('compare'), 'ImageMagick compare is required')
class ExternalScoreTests(v014.RendererSelectionTests):
    def setUp(self):
        super().setUp()
        self.external = self.root / 'External scores'
        self.external.mkdir()
        self.fixture = (self.tests / 'tie-1.mscx').read_bytes()
        self.mscx = self.external / 'Same Name.mscx'
        self.mscx.write_bytes(self.fixture)
        self.mscz = self.external / 'Same Name.mscz'
        self.create_mscz(self.mscz, self.fixture)
        nested = self.external / 'More scores'
        nested.mkdir()
        self.nested = nested / 'Another score.mscz'
        self.create_mscz(self.nested, self.fixture)

    @staticmethod
    def create_mscz(path, xml):
        container = ('<?xml version="1.0"?>\n'
                     '<container xmlns="urn:oasis:names:tc:opendocument:xmlns:container">'
                     '<rootfiles><rootfile full-path="music.mscx"/></rootfiles></container>')
        with zipfile.ZipFile(path, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
            archive.writestr('META-INF/container.xml', container)
            archive.writestr('music.mscx', xml)
            archive.writestr('Pictures/logo.bin', b'\x00asset bytes\xf0' * 30)

    def report_scores(self):
        return json.loads((self.tests / 'html' / 'input-manifest.json').read_text())['scores']

    def test_directory_with_spaces_and_duplicate_stems(self):
        original_sha = hashlib.sha256(self.mscz.read_bytes()).hexdigest()
        self.run_gen('--modern', '--score-dir', str(self.external))
        records = self.report_scores()
        self.assertEqual(len(records), 2)
        self.assertEqual({Path(r['origin']).suffix for r in records.values()}, {'.mscx', '.mscz'})
        self.assertEqual(len(set(records)), 2)
        self.assertEqual(original_sha, hashlib.sha256(self.mscz.read_bytes()).hexdigest())
        for record in records.values():
            self.assertIn('beamLayout4', record['37e_only_overrides'])
            self.assertTrue(Path(record['effective']).exists())
            self.assertTrue(Path(record['effective_studio']).exists())
            self.assertEqual(record['origin_sha256'], hashlib.sha256(Path(record['origin']).read_bytes()).hexdigest())
        compressed = next(r for r in records.values() if r['origin'].endswith('.mscz'))
        with zipfile.ZipFile(compressed['effective']) as current, zipfile.ZipFile(compressed['effective_studio']) as studio:
            self.assertEqual(current.read('Pictures/logo.bin'), b'\x00asset bytes\xf0' * 30)
            self.assertEqual(studio.read('Pictures/logo.bin'), b'\x00asset bytes\xf0' * 30)
            self.assertEqual(current.read('META-INF/container.xml'), studio.read('META-INF/container.xml'))
            self.assertIn(b'<beamLayout4>1</beamLayout4>', current.read('music.mscx'))
            self.assertNotIn(b'<beamLayout4>', studio.read('music.mscx'))
        html = (self.tests / 'html' / 'engraving.html').read_text()
        self.assertIn('Same Name.mscx (External scores)', html)
        self.assertIn('Same Name.mscz (External scores)', html)

    def test_recursive_and_raw(self):
        self.run_gen('--score-dir', str(self.external))
        self.assertEqual(len(self.report_scores()), 2)
        self.run_gen('--score-dir', str(self.external), '--recursive', env={'VTEST_PROFILE_MODE': 'raw'})
        records = self.report_scores()
        self.assertEqual(len(records), 3)
        for record in records.values():
            self.assertEqual(record['source_sha256'], record['origin_sha256'])
            self.assertEqual(record['studio_source_sha256'], record['origin_sha256'])
            self.assertEqual(record['effective'], record['origin'])
        self.assertFalse(any('studio ' in line for line in self.logs()))

    def test_single_compressed_score_with_historical_and_cache(self):
        self.run_gen('--modern', '--historical', '--score', str(self.mscz))
        record = next(iter(self.report_scores().values()))
        self.assertTrue(record['effective'].endswith('.mscz'))
        self.assertEqual(sum(x.startswith('studio ') for x in self.logs()), 1)
        self.assertEqual(sum(x.startswith('historical ') for x in self.logs()), 1)
        self.run_gen('--modern', '--historical', '--score', str(self.mscz))
        self.assertEqual(sum(x.startswith('studio ') for x in self.logs()), 1)
        self.assertEqual(sum(x.startswith('historical ') for x in self.logs()), 1)

    def test_invalid_archive_and_missing_directory_rejected(self):
        (self.external / 'broken.mscz').write_bytes(b'not a zip')
        bad = subprocess.run([str(self.tests / 'gen'), '--score', str(self.external / 'broken.mscz')],
                             cwd=self.root, env=self.env, text=True, capture_output=True)
        self.assertNotEqual(bad.returncode, 0)
        self.assertIn('invalid MuseScore MSCZ archive', bad.stderr + bad.stdout)
        bad = subprocess.run([str(self.tests / 'gen'), '--score-dir', '/directory-that-does-not-exist'],
                             cwd=self.root, env=self.env, text=True, capture_output=True)
        self.assertNotEqual(bad.returncode, 0)
        self.assertIn('not a score directory', bad.stderr + bad.stdout)


def load_tests(loader, existing, pattern):
    selected = unittest.TestSuite()
    for name in ('test_directory_with_spaces_and_duplicate_stems',
                 'test_recursive_and_raw',
                 'test_single_compressed_score_with_historical_and_cache',
                 'test_invalid_archive_and_missing_directory_rejected'):
        selected.addTest(ExternalScoreTests(name))
    return selected


if __name__ == '__main__':
    unittest.main()
