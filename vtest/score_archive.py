"""Read and prepare MuseScore MSCX/MSCZ scores without losing archive resources.

MSCs compressed archives can contain Pictures/ and other assets. Repacking the
score XML inside the ZIP retains those members and their uncompressed contents;
extracting just an MSCX file would silently discard those assets.
"""
import os
import hashlib
import re
from pathlib import Path
import shutil
import tempfile
import xml.etree.ElementTree as ET
import zipfile

SUPPORTED = {'.mscx', '.mscz'}


def external_name(path):
    source = Path(path).expanduser().resolve()
    if source.suffix.lower() not in SUPPORTED:
        raise ValueError('external scores must be .mscx or .mscz')
    stem = re.sub(r'[^A-Za-z0-9_-]+', '-', source.stem).strip('-')[:48] or 'score'
    return f'external-{stem}-{hashlib.sha256(os.fsencode(source)).hexdigest()[:12]}'

MAX_SCORE_XML = 100 * 1024 * 1024


def score_member(archive):
    members = archive.namelist()
    if len(members) != len(set(members)):
        raise ValueError('MSCZ archive contains duplicate member names')
    if 'META-INF/container.xml' in members:
        container = ET.fromstring(archive.read('META-INF/container.xml'))
        roots = [node.attrib.get('full-path') for node in container.iter()
                 if node.tag.rsplit('}', 1)[-1] == 'rootfile']
        roots = [path for path in roots if path and path.lower().endswith('.mscx')]
        if len(roots) == 1 and roots[0] in members:
            return roots[0]
        if roots:
            raise ValueError('MSCZ container has no unambiguous MSCX rootfile')
    candidates = [member for member in members if member.lower().endswith('.mscx')]
    if len(candidates) == 1:
        return candidates[0]
    raise ValueError('MSCZ archive needs exactly one MSCX score or a valid rootfile manifest')


def load_score_xml(path):
    path = Path(path)
    suffix = path.suffix.lower()
    if suffix == '.mscx':
        return path.read_bytes(), None
    if suffix != '.mscz':
        raise ValueError(f'unsupported MuseScore score extension: {path.suffix}')
    try:
        with zipfile.ZipFile(path) as archive:
            member = score_member(archive)
            info = archive.getinfo(member)
            if info.file_size > MAX_SCORE_XML:
                raise ValueError(f'compressed score XML exceeds {MAX_SCORE_XML} bytes')
            data = archive.read(info)
    except (zipfile.BadZipFile, RuntimeError) as error:
        raise ValueError(f'invalid MuseScore MSCZ archive: {path}: {error}') from error
    ET.fromstring(data)
    return data, member


def write_prepared(path, destination, xml_bytes, member):
    """Write an MSCX or an MSCZ with only its XML score member changed."""
    path, destination = Path(path), Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix='.' + destination.stem + '-', suffix=destination.suffix,
                                     dir=str(destination.parent))
    os.close(fd)
    try:
        if member is None:
            Path(temporary).write_bytes(xml_bytes)
        else:
            with zipfile.ZipFile(path, 'r') as original, zipfile.ZipFile(temporary, 'w') as output:
                output.comment = original.comment
                for info in original.infolist():
                    if info.filename == member:
                        output.writestr(info, xml_bytes)
                    elif info.is_dir():
                        output.writestr(info, b'')
                    else:
                        with original.open(info) as source, output.open(info, 'w') as target:
                            shutil.copyfileobj(source, target, length=1024 * 1024)
        os.replace(temporary, destination)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
