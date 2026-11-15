"""Common reader for exact MSCX inputs selected by vtest/gen."""
import hashlib
import json
from pathlib import Path


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def load_inputs(path, names):
    document = json.loads(Path(path).read_text(encoding='utf-8'))
    if document.get('schema') != 1 or not isinstance(document.get('scores'), dict):
        raise ValueError(f'{path}: unsupported input manifest')
    records = {}
    for name in names:
        record = document['scores'].get(name)
        if not isinstance(record, dict):
            raise ValueError(f'{path}: missing input for {name}')
        expected = record.get('source_sha256')
        effective = record.get('effective')
        origin = record.get('origin')
        if not isinstance(effective, str) or not isinstance(origin, str):
            raise ValueError(f'{name}: missing source path')
        if sha256(effective) != expected:
            raise ValueError(f'{name}: 37E input changed since input selection')
        studio = record.get('effective_studio', effective)
        studio_hash = record.get('studio_source_sha256', expected)
        if sha256(studio) != studio_hash:
            raise ValueError(f'{name}: Studio input changed since input selection')
        if 'beam_legacy_effective' in record:
            if sha256(record['beam_legacy_effective']) != record.get('beam_legacy_source_sha256'):
                raise ValueError(f'{name}: prepared legacy beam input changed since selection')
        if sha256(origin) != record.get('origin_sha256'):
            raise ValueError(f'{name}: original score has changed since input selection')
        records[name] = record
    return records
