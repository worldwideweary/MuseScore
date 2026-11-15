#!/usr/bin/env python3
"""Self-contained reference-cache identity, validity and corruption checks."""
import json
from pathlib import Path
import struct
import tempfile
import unittest
import zlib

from reference_cache import ReferenceCache, png_dimensions, read_pages, sha256


def make_png(path, width=4, height=3):
    def chunk(kind, data):
        return (struct.pack('>I', len(data)) + kind + data
                + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff))
    rows = b''.join(b'\x00' + b'\x00\x00\x00' * width for _ in range(height))
    payload = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
    payload += chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b'')
    Path(path).write_bytes(payload)


class ReferenceCacheTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.executable = self.root / 'renderer'
        self.executable.write_bytes(b'renderer-v1')
        self.work = self.root / 'work'
        self.work.mkdir()
        make_png(self.work / 'tie-1-1.png')
        make_png(self.work / 'tie-1-2.png', 6, 4)
        self.source_hash = 'a' * 64

    def cache(self, dpi=130, salt=''):
        return ReferenceCache(self.root / 'cache', self.executable, dpi, salt)

    def test_reuse_and_multiple_pages(self):
        cache = self.cache()
        pages = read_pages(self.work, 'tie-1')
        self.assertEqual(len(pages), 2)
        cache.store('tie-1', self.source_hash, self.work, pages)
        for path in self.work.iterdir():
            path.unlink()
        self.assertTrue(cache.restore('tie-1', self.source_hash, self.work))
        self.assertEqual(read_pages(self.work, 'tie-1'), pages)
        self.assertFalse(cache.restore('tie-1', self.source_hash, self.work, refresh=True))

    def test_invalidated_by_input_executable_dpi_and_salt(self):
        cache = self.cache()
        cache.store('tie-1', self.source_hash, self.work, read_pages(self.work, 'tie-1'))
        self.assertFalse(cache.restore('tie-1', 'b' * 64, self.work))
        self.assertFalse(self.cache(dpi=132).restore('tie-1', self.source_hash, self.work))
        self.assertFalse(self.cache(salt='new-fonts').restore('tie-1', self.source_hash, self.work))
        self.executable.write_bytes(b'renderer-v2')
        self.assertFalse(self.cache().restore('tie-1', self.source_hash, self.work))

    def test_corruption_detected(self):
        cache = self.cache()
        cache.store('tie-1', self.source_hash, self.work, read_pages(self.work, 'tie-1'))
        (self.root / 'cache/tie-1/tie-1-2.png').write_bytes(b'invalid')
        self.assertFalse(cache.restore('tie-1', self.source_hash, self.work))
        self.assertEqual(png_dimensions(self.work / 'tie-1-1.png'), [4, 3])

    def test_noncontiguous_page_rejected(self):
        (self.work / 'tie-1-2.png').unlink()
        make_png(self.work / 'tie-1-3.png')
        with self.assertRaises(ValueError):
            read_pages(self.work, 'tie-1')


if __name__ == '__main__':
    unittest.main()
