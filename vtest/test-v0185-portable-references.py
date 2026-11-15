#!/usr/bin/env python3
"""Portable Studio reference preflight without either MuseScore executable."""
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
GEN = ROOT / 'gen'
CATALOGUE = ROOT / 'studio-catalogue.py'
FAKE = '''#!/usr/bin/env python3
import json, pathlib, sys
from PIL import Image
if '--version' in sys.argv or '--long-version' in sys.argv:
    print('Studio Test 4.7.7')
    sys.exit(0)
job = json.loads(pathlib.Path(sys.argv[sys.argv.index('-j')+1]).read_text())[0]
p = pathlib.Path(job['out']); Image.new('RGB',(101,75),'white').save(p.with_name(p.stem+'-1.png'))
'''


class PortableReferences(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='vtest-v0185-')
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.scores = self.root / 'sources' / 'scores'
        self.scores.mkdir(parents=True)
        self.original = '<museScore version="4.00"><Score><Style/><Staff/></Score></museScore>'
        (self.scores / 'one.mscx').write_text(self.original)
        with zipfile.ZipFile(self.scores / 'two.mscz', 'w') as z:
            z.writestr('score.mscx', self.original)
            z.writestr('Pictures/picture.png', b'dummy-resource')
        self.studio = self.root / 'fake-studio'
        self.studio.write_text(FAKE)
        self.studio.chmod(0o755)
        self.refs = self.root / 'references'
        self.html = self.root / 'original-html'
        self.run_cmd(sys.executable, str(CATALOGUE), '--cache-modern',
                     '--score-dir', str(self.scores), '--studio', str(self.studio),
                     '--archive-root', str(self.refs), '--html-dir', str(self.html))
        self.archive = next(self.refs.glob('studio-*'))

    def run_cmd(self, *args, expected=0, env=None):
        result = subprocess.run(args, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                env=env, timeout=60)
        self.assertEqual(result.returncode, expected, result.stdout + '\n' + result.stderr)
        return result

    def verify(self, expected=0, *flags, scores=None, archive=None):
        target = self.root / 'check.json'
        env = {**os.environ, 'VTEST_MSCORE': '', 'VTEST_REFERENCE_MSCORE': '',
               'VTEST_OPEN_REPORT': '0'}
        cmd = [str(GEN), '--verify-modern', '--score-dir', str(scores or self.scores),
               '--archive', str(archive or self.archive), '--verification-json', str(target),
               *flags]
        result = self.run_cmd(*cmd, expected=expected, env=env)
        return json.loads(target.read_text()), result

    def test_archive_relocates_and_verifies_without_renderer(self):
        original_manifest = (self.archive / 'manifest.json').read_bytes()
        new_scores = self.root / 'new-machine' / 'scores'
        new_refs = self.root / 'new-machine' / 'refs'
        shutil.copytree(self.scores, new_scores)
        shutil.copytree(self.archive, new_refs)
        self.studio.unlink()  # binary doesn't exist at its recorded absolute path
        report, output = self.verify(scores=new_scores, archive=new_refs)
        self.assertTrue(report['complete'])
        self.assertEqual(report['counts']['verified'], 2)
        self.assertIn('Studio Test 4.7.7', output.stdout)
        self.assertEqual((self.archive / 'manifest.json').read_bytes(), original_manifest)
        self.assertFalse((new_refs / 'html').exists())

    def test_stale_score_and_corrupt_png_fail_separately(self):
        (self.scores / 'one.mscx').write_text(self.original + '<!-- change -->')
        report, _ = self.verify(expected=1)
        self.assertEqual(report['counts']['stale'], 1)
        self.assertEqual(report['counts']['verified'], 1)
        (self.scores / 'one.mscx').write_text(self.original)
        entry = next((name for name, e in json.loads((self.archive/'manifest.json').read_text())['scores'].items()
                      if e['relative_path'] == 'two.mscz'))
        image = self.archive / 'scores' / entry / (entry + '-1.png')
        image.write_bytes(image.read_bytes() + b'bad')
        report, _ = self.verify(expected=1)
        self.assertEqual(report['counts']['corrupt'], 1)
        self.assertEqual(report['counts']['verified'], 1)

    def test_missing_reference_reports_failure(self):
        (self.scores / 'new.mscx').write_text(self.original)
        report, _ = self.verify(expected=1)
        self.assertEqual(report['counts']['missing'], 1)
        self.assertEqual(report['selected_scores'], 3)

    def test_strict_source_revision_requires_matching_checkout(self):
        # This archive wasn't originally created from a Git checkout. The strict
        # check therefore rejects it, even though file contents are valid.
        report, _ = self.verify(1, '--strict-source')
        self.assertFalse(report['complete'])
        self.assertTrue(report['metadata_errors'])
        # Non-strict verification stays portable based on original score bytes.
        report, _ = self.verify()
        self.assertTrue(report['complete'])

    def test_strict_source_positive_and_dirty(self):
        subprocess.run(['git', 'init', '-q', str(self.scores)], check=True)
        subprocess.run(['git', '-C', str(self.scores), 'config', 'user.name', 'Test'], check=True)
        subprocess.run(['git', '-C', str(self.scores), 'config', 'user.email', 'test@example.org'], check=True)
        subprocess.run(['git', '-C', str(self.scores), 'add', '.'], check=True)
        subprocess.run(['git', '-C', str(self.scores), 'commit', '-qm', 'test fixtures'], check=True)
        # Rebuild the archive from a pinned fixture checkout.
        shutil.rmtree(self.archive)
        self.run_cmd(sys.executable, str(CATALOGUE), '--cache-modern',
                     '--score-dir', str(self.scores), '--studio', str(self.studio),
                     '--archive-root', str(self.refs), '--html-dir', str(self.html))
        report, _ = self.verify(0, '--strict-source')
        self.assertTrue(report['complete'])
        (self.scores / 'one.mscx').write_text(self.original + '<!-- dirty -->')
        report, _ = self.verify(1, '--strict-source')
        self.assertFalse(report['complete'])
        self.assertEqual(report['counts']['stale'], 1)


    def test_git_publication_check(self):
        report, _ = self.verify(1, '--require-committed')
        self.assertGreater(report['uncommitted_archive_files'], 0)
        subprocess.run(['git', 'init', '-q', str(self.archive)], check=True)
        subprocess.run(['git', '-C', str(self.archive), 'add', '.'], check=True)
        report, _ = self.verify(1, '--require-committed')  # staging is not publication
        subprocess.run(['git', '-C', str(self.archive), 'config', 'user.name', 'Test'], check=True)
        subprocess.run(['git', '-C', str(self.archive), 'config', 'user.email', 'test@example.org'], check=True)
        subprocess.run(['git', '-C', str(self.archive), 'commit', '-qm', 'publish archive'], check=True)
        report, _ = self.verify(0, '--require-committed')
        self.assertEqual(report['uncommitted_archive_files'], 0)
        entry = next((name for name, e in json.loads((self.archive/'manifest.json').read_text())['scores'].items()
                      if e['relative_path'] == 'two.mscz'))
        # A staged or unstaged edit differs from the committed archive.
        image = self.archive / 'scores' / entry / (entry + '-1.png')
        image.write_bytes(image.read_bytes() + b'local-edit')
        report, _ = self.verify(1, '--require-committed')
        self.assertTrue(report['modified_archive_relative_to_head'])



if __name__ == '__main__':
    unittest.main(verbosity=2)
