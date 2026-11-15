#!/usr/bin/env python3
"""Regression coverage: file-manager URL links and local URI dispatcher."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
from unittest import TestCase, main, mock

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))


def load(name, label):
    spec = importlib.util.spec_from_file_location(label, ROOT / name)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


reveal = load('reveal-score.py', 'vtest_reveal')
comparison = load('compare-engraving.py', 'vtest_comparison')


class RevealURLTests(TestCase):
    def test_path_encoding_unsafe_schemes_and_extensions(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'My score #5 % & ü.mscz'
            path.write_bytes(b'test')
            url = reveal.score_uri(path)
            self.assertTrue(url.startswith('vtest-reveal:///'))
            self.assertIn('%23', url)
            self.assertIn('%25', url)
            self.assertEqual(reveal.parse_score_uri(url), path)
            for bad in ('https://example.com/score.mscz',
                        'vtest-reveal://evil.test/etc/score.mscz',
                        'vtest-reveal:///etc/passwd',
                        url + '?x=1', url + '#anything'):
                with self.subTest(bad=bad), self.assertRaises(ValueError):
                    reveal.parse_score_uri(bad)

    def test_report_preserves_navigation_and_offers_reveal_links(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'Score & other.mscx'
            path.touch()
            record = {'name': 'external-a', 'key': 'external-a', 'page': 1,
                      'pages_total': 1, 'current_only': True,
                      'input': {'origin': str(path), 'profile': 'raw',
                                'source_sha256': 'hash'}}
            markup = comparison.page([record, dict(record, name='external-b', key='external-b')],
                                     5, 'Studio')
            self.assertIn('href="#external-a"', markup)
            self.assertIn('class="reveal-link"', markup)
            self.assertIn('vtest-reveal:', markup)
            self.assertIn('%26', markup)
            self.assertIn('Reveal file', markup)
            self.assertIn('Reveal source score', markup)
            self.assertEqual(markup.count('class="reveal-link"'), 4)
            self.assertNotIn('href="file:', markup)

    def test_linux_reveal_calls_dbus_then_parent_fallback(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'a & b.mscx'
            path.touch()
            with mock.patch.object(reveal.sys, 'platform', 'linux'), mock.patch.object(reveal.os, 'name', 'posix'):
                with mock.patch.object(reveal.subprocess, 'run') as run:
                    reveal.reveal(path)
                    cmd = run.call_args.args[0]
                    self.assertIn('org.freedesktop.FileManager1.ShowItems', cmd)
                    self.assertIn('array:string:' + path.as_uri(), cmd)
                with mock.patch.object(reveal.subprocess, 'run', side_effect=[subprocess.CalledProcessError(1, 'dbus-send'), mock.Mock(returncode=0)]) as run:
                    reveal.reveal(path)
                    self.assertEqual(run.call_args_list[-1].args[0], ['xdg-open', str(path.parent)])

    def test_linux_registration_is_opt_in(self):
        with tempfile.TemporaryDirectory() as tmp:
            with mock.patch.object(reveal.Path, 'home', return_value=Path(tmp)), \
                 mock.patch.object(reveal.subprocess, 'run') as run:
                reveal.install_linux()
                desktop = Path(tmp) / '.local/share/applications/musescore-vtest-reveal.desktop'
                self.assertIn('x-scheme-handler/vtest-reveal', desktop.read_text())
                self.assertIn('reveal-score.py', desktop.read_text())
                self.assertEqual(run.call_args.args[0][0:2], ['xdg-mime', 'default'])


if __name__ == '__main__':
    main()
