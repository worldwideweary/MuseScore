#!/usr/bin/env python3
"""Regression checks for vtest's automatic Studio-parity beam input preparation.

Run from any directory: python3 vtest/test-modern-beam-inputs.py
No MuseScore executable is needed.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent
SCRIPT = ROOT / 'prepare-inputs.py'
CONFIG = ROOT / 'engraving-profiles.json'


def convert(html, *names, extra=()):
    proc = subprocess.run([sys.executable, str(SCRIPT), '--scores-dir', str(ROOT),
                           '--html-dir', str(html), '--config', str(CONFIG),
                           *extra, *names], capture_output=True, text=True)
    if proc.returncode:
        raise AssertionError(proc.stderr)
    return json.loads((html / 'input-manifest.json').read_text())['scores']


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class ModernBeamInputs(unittest.TestCase):
    def test_ordinary_catalogue_includes_modern_beam_for_all_profiles(self):
        with tempfile.TemporaryDirectory() as temp:
            records = convert(Path(temp), 'tie-1', 'beams-1', 'noteheadposition-1')
            for record in records.values():
                self.assertIn('<beamLayout4>1</beamLayout4>', Path(record['effective']).read_text())
                self.assertNotIn('<beamLayout4>', Path(record['effective_studio']).read_text())
                self.assertEqual(record['37e_only_overrides']['beamLayout4'], '1')
                self.assertEqual(record['source_sha256'], sha256(record['effective']))
                self.assertEqual(record['studio_source_sha256'], sha256(record['effective_studio']))
                self.assertNotIn('beam_legacy_effective', record)

    def test_legacy_variant_only_when_explicitly_requested(self):
        with tempfile.TemporaryDirectory() as temp:
            record = convert(Path(temp), 'tie-1', extra=['--beam-ab'])['tie-1']
            self.assertIn('<beamLayout4>0</beamLayout4>', Path(record['beam_legacy_effective']).read_text())
            self.assertIn('<beamLayout4>1</beamLayout4>', Path(record['effective']).read_text())
            self.assertEqual(record['beam_legacy_source_sha256'], sha256(record['beam_legacy_effective']))

    def test_raw_mode_keeps_both_renderer_inputs_untouched(self):
        with tempfile.TemporaryDirectory() as temp:
            record = convert(Path(temp), 'tie-1', extra=['--raw', '--beam-ab'])['tie-1']
            self.assertEqual(record['source_sha256'], record['origin_sha256'])
            self.assertEqual(record['studio_source_sha256'], record['origin_sha256'])
            self.assertEqual(record['style_overrides'], {})
            self.assertNotIn('beam_legacy_effective', record)

    def test_external_file_uses_modern_beam_without_changing_source(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = Path(temp)
            score = temp / 'My Test Score.mscx'
            score.write_bytes((ROOT / 'tie-1.mscx').read_bytes())
            # External IDs encode paths; use the exact helper's CLI.
            external = subprocess.check_output([sys.executable, str(SCRIPT),
                '--external-name', str(score)], text=True).strip()
            record = convert(temp / 'html', external, extra=['--external', str(score)])[external]
            self.assertEqual(record['origin_sha256'], sha256(score))
            self.assertIn('<beamLayout4>1</beamLayout4>', Path(record['effective']).read_text())
            self.assertNotIn('<beamLayout4>', Path(record['effective_studio']).read_text())
            self.assertNotIn('<beamLayout4>', score.read_text())

    def test_original_only_beam_property_stripped_for_studio(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = Path(temp)
            score = temp / 'Custom Beam.mscx'
            score.write_bytes((ROOT / 'beams-1.mscx').read_bytes())
            xml = score.read_text()
            xml = xml.replace('<Style>', '<Style>\n      <beamLayout4>0</beamLayout4>', 1)
            score.write_text(xml)
            external = subprocess.check_output([sys.executable, str(SCRIPT),
                '--external-name', str(score)], text=True).strip()
            record = convert(temp / 'html', external, extra=['--external', str(score)])[external]
            self.assertIn('<beamLayout4>0</beamLayout4>', score.read_text())
            self.assertIn('<beamLayout4>1</beamLayout4>', Path(record['effective']).read_text())
            self.assertNotIn('<beamLayout4>', Path(record['effective_studio']).read_text())
            raw = convert(temp / 'raw', external, extra=['--raw', '--external', str(score)])[external]
            self.assertEqual(raw['origin_sha256'], raw['source_sha256'])
            self.assertEqual(raw['origin_sha256'], raw['studio_source_sha256'])


if __name__ == '__main__':
    unittest.main()
