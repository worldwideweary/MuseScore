"""Verified, disposable PNG cache for unchanging external MuseScore renderers.

A cache is keyed by exact score bytes, renderer executable bytes, export DPI,
optional resource salt, and explicit schema. Rendered PNGs are verified against
stored SHA-256 and IHDR dimensions before reuse. The cache is NOT a 37E
before-change baseline and can be deleted without losing source fixtures.
"""
import hashlib
import json
from pathlib import Path
import shutil
import struct
import tempfile

SCHEMA = 1


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def png_dimensions(path):
    with Path(path).open('rb') as stream:
        header = stream.read(24)
    if len(header) != 24 or header[:8] != b'\x89PNG\r\n\x1a\n' or header[12:16] != b'IHDR':
        raise ValueError(f'not a PNG: {path}')
    w, h = struct.unpack('>II', header[16:24])
    if w <= 0 or h <= 0:
        raise ValueError(f'invalid PNG dimensions: {path}')
    return [w, h]


def read_pages(directory, name):
    """Validate a renderer's sequential NAME-N.png output."""
    directory = Path(directory)
    pages = []
    n = 1
    while (directory / f'{name}-{n}.png').is_file():
        path = directory / f'{name}-{n}.png'
        pages.append({'file': path.name, 'sha256': sha256(path),
                      'dimensions': png_dimensions(path)})
        n += 1
    if not pages:
        raise ValueError(f'no PNG page rendered for {name}')
    # Do not accept a noncontiguous page sequence as a complete export.
    extras = [p.name for p in directory.glob(f'{name}-[0-9]*.png')
              if p.name not in {entry['file'] for entry in pages}]
    if extras:
        raise ValueError(f'non-contiguous PNG pages for {name}: {extras[:5]}')
    return pages


class ReferenceCache:
    def __init__(self, root, renderer_path, dpi, salt=''):
        self.root = Path(root)
        self.root.mkdir(parents=True, exist_ok=True)
        self.identity = {'schema': SCHEMA,
                         'executable': str(Path(renderer_path).resolve()),
                         'executable_sha256': sha256(renderer_path),
                         'dpi': dpi, 'resource_salt': salt}

    def _record(self, name, source_sha256):
        return {**self.identity, 'test': name, 'source_sha256': source_sha256}

    def restore(self, name, source_sha256, work, refresh=False):
        """Return true only after byte-hash-valid PNG pages are copied to work."""
        if refresh:
            return False
        cache_dir = self.root / name
        try:
            meta = json.loads((cache_dir / 'cache.json').read_text(encoding='utf-8'))
            pages = meta['pages']
            if not isinstance(pages, list) or not pages:
                return False
            for key, value in self._record(name, source_sha256).items():
                if meta.get(key) != value:
                    return False
            for number, page in enumerate(pages, 1):
                filename = f'{name}-{number}.png'
                if page.get('file') != filename:
                    return False
                file = cache_dir / filename
                if (not file.is_file() or sha256(file) != page.get('sha256')
                        or png_dimensions(file) != page.get('dimensions')):
                    return False
            for page in pages:
                shutil.copy2(cache_dir / page['file'], Path(work) / page['file'])
            return True
        except (OSError, ValueError, TypeError, KeyError, AttributeError, json.JSONDecodeError):
            return False

    def store(self, name, source_sha256, work, pages):
        """Store a fully validated set; never replace other tests' cache entries."""
        cache_dir = self.root / name
        with tempfile.TemporaryDirectory(prefix='.ref-cache-', dir=self.root) as scratch:
            temp = Path(scratch)
            for page in pages:
                shutil.copy2(Path(work) / page['file'], temp / page['file'])
            meta = {**self._record(name, source_sha256), 'pages': pages}
            (temp / 'cache.json').write_text(
                json.dumps(meta, sort_keys=True, indent=2) + '\n', encoding='utf-8')
            if cache_dir.exists():
                shutil.rmtree(cache_dir)
            shutil.move(str(temp), str(cache_dir))
