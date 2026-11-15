#!/usr/bin/env python3
"""Resolve vtest fixtures or external MSCX/MSCZ scores and apply engraving profiles.

All generated files stay below vtest/html/. Source scores remain untouched.
The manifest records both renderer inputs and their hashes; Studio omits 37E-only properties.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET

from score_archive import external_name, load_score_xml, write_prepared

VALID_NAME = re.compile(r'[A-Za-z0-9][A-Za-z0-9_.-]*\Z')
E37_ONLY = {'beamLayout4', 'spacingLayout4'}

ALLOWED_STYLE = {
    'musicalSymbolFont', 'musicalTextFont',
    'enableIndentationOnFirstSystem', 'firstSystemIndentationValue',
    'beamLayout4', 'spacingLayout4',
}


def sha256_bytes(data):
    return hashlib.sha256(data).hexdigest()


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def load_profiles(config_path):
    config = json.loads(config_path.read_text(encoding='utf-8'))
    if config.get('schema') != 1:
        raise ValueError('unsupported engraving-profiles.json schema')
    presets = config.get('profiles')
    assignments = config.get('assignments')
    if not isinstance(presets, dict) or not isinstance(assignments, dict):
        raise ValueError('profiles and assignments must be objects')
    default = config.get('default', 'original')
    if default not in presets:
        raise ValueError('default profile is not defined')
    for name, values in presets.items():
        if not isinstance(values, dict):
            raise ValueError(f'profile {name} must contain a style-property object')
        unknown = values.keys() - ALLOWED_STYLE
        if unknown:
            raise ValueError(f'profile {name}: unsupported properties {sorted(unknown)}')
        if any(not isinstance(val, (str, int, float, bool)) for val in values.values()):
            raise ValueError(f'profile {name}: values must be simple scalars')
    modern = config.get('studio_comparison_37e_overrides', {})
    if not isinstance(modern, dict) or modern.keys() - E37_ONLY:
        raise ValueError('studio_comparison_37e_overrides must contain only 37E-specific style keys')
    if any(not isinstance(v, (str, int, float, bool)) for v in modern.values()):
        raise ValueError('studio_comparison_37e_overrides values must be simple scalars')
    for name, profile in assignments.items():
        if not VALID_NAME.fullmatch(name) or profile not in presets:
            raise ValueError(f'invalid assignment: {name}: {profile}')
    return config


def apply_style(data, properties):
    if not properties:
        return data
    source = data.decode('utf-8-sig')
    # Parse and validate first. Preserve the original XML and musical content
    # byte-for-byte outside the first Score/Style rather than reserializing it.
    tree = ET.fromstring(source)
    score = tree.find('Score')
    if tree.tag != 'museScore' or score is None:
        raise ValueError('expected a MuseScore MSCX document with a Score element')
    for key, value in properties.items():
        if not re.fullmatch(r'[A-Za-z][A-Za-z0-9]*', key):
            raise ValueError(f'invalid style tag: {key}')
        if isinstance(value, bool):
            value = int(value)
        if any(char in str(value) for char in '<>&'):
            raise ValueError(f'unsafe style value for {key}')
        properties[key] = str(value)
    style_match = re.search(r'(?P<open><Style(?:\s[^>]*)?>)(?P<body>.*?)(?P<close></Style>)',
                            source, flags=re.DOTALL)
    if style_match is None:
        # Historical fixtures sometimes omit Style. Add it under Score, with
        # no change to the original score's version or musical elements.
        markup = '\n'.join(f'      <{k}>{v}</{k}>' for k, v in properties.items())
        source, count = re.subn(r'(<Score(?:\s[^>]*)?>)',
                                 lambda match: match.group(1) + '\n    <Style>\n' + markup + '\n    </Style>',
                                 source, count=1)
        if count != 1:
            raise ValueError('cannot insert Style into this MSCX score')
    else:
        body = style_match.group('body')
        for key, value in properties.items():
            pattern = re.compile(r'<'+re.escape(key)+r'>[^<]*</'+re.escape(key)+r'>')
            new = f'<{key}>{value}</{key}>'
            if pattern.search(body):
                body = pattern.sub(new, body, count=1)
            else:
                body += f'\n      {new}'
        source = source[:style_match.start('body')] + body + source[style_match.end('body'):]
    updated = source.encode('utf-8')
    ET.fromstring(updated)  # Ensure modified markup is well-formed.
    return updated


def strip_37e_only_style(data):
    """Remove unsupported backport tags from Studio's *first* score Style.

    Preserve all other XML as written, including old-score version metadata.
    The original input is never modified.
    """
    xml = data.decode('utf-8-sig')
    first_style = re.search(r'(?P<open><Style(?:\s[^>]*)?>)(?P<body>.*?)(?P<close></Style>)',
                            xml, flags=re.DOTALL)
    if first_style is None:
        return data
    body = first_style.group('body')
    for key in sorted(E37_ONLY):
        body = re.sub(r'\s*<' + key + r'>[^<]*</' + key + r'>', '', body)
    if body == first_style.group('body'):
        return data
    result = (xml[:first_style.start('body')] + body + xml[first_style.end('body'):]).encode('utf-8')
    ET.fromstring(result)
    return result


def write_atomic(path, content):
    target = path.with_name(path.name + '.tmp')
    target.write_bytes(content)
    target.replace(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--external-name', metavar='MSCX_OR_MSCZ',
                        help='print stable test identifier for an external file and exit')
    parser.add_argument('--external', type=Path, help='one external .mscx or .mscz score')
    parser.add_argument('--external-map', type=Path, help='selected external directory score IDs/paths')
    parser.add_argument('--raw', action='store_true', help='render originals without any style overrides')
    parser.add_argument('--beam-ab', action='store_true', help='render extra 37E beamLayout4=0 reference for these tests')
    parser.add_argument('--scores-dir', type=Path)
    parser.add_argument('--html-dir', type=Path)
    parser.add_argument('--config', type=Path)
    parser.add_argument('tests', nargs='*')
    args = parser.parse_args()
    try:
        if args.external_name:
            print(external_name(args.external_name))
            return
        if not args.scores_dir or not args.html_dir or not args.config:
            parser.error('--scores-dir, --html-dir, and --config are required')
        if not args.tests or len(set(args.tests)) != len(args.tests):
            raise ValueError('specify unique test names')
        if any(not VALID_NAME.fullmatch(name) or name in ('.', '..') for name in args.tests):
            raise ValueError('test names must contain only letters, numbers, dot, _ or -')
        profiles = load_profiles(args.config)
        source_dir = args.scores_dir.resolve()
        html_dir = args.html_dir.resolve()
        html_dir.mkdir(parents=True, exist_ok=True)
        prepared_dir = html_dir / 'prepared-inputs'
        prepared_dir.mkdir(exist_ok=True)
        if args.external and args.external_map:
            raise ValueError('--external and --external-map cannot be combined')
        if args.external and (len(args.tests) != 1 or args.tests[0] != external_name(args.external)):
            raise ValueError('external score name mismatch')
        if args.external and not args.external.expanduser().is_file():
            raise ValueError(f'external score does not exist: {args.external}')
        external_map = {}
        if args.external_map:
            selection = json.loads(args.external_map.read_text(encoding='utf-8'))
            if selection.get('schema') != 1 or not isinstance(selection.get('scores'), dict):
                raise ValueError('invalid external score selection manifest')
            external_map = selection['scores']
            if set(args.tests) != set(external_map):
                raise ValueError('external directory score IDs do not match selected tests')
            for key, path in external_map.items():
                if not isinstance(path, str) or external_name(path) != key:
                    raise ValueError(f'invalid external directory score mapping: {key}')
        metadata = {'schema': 1, 'scores': {}}
        jobs = []
        for name in args.tests:
            original = (args.external.expanduser().resolve() if args.external else
                        Path(external_map[name]).resolve() if args.external_map else
                        source_dir / f'{name}.mscx')
            if not original.is_file():
                raise ValueError(f'missing test score: {original}')
            original_bytes = original.read_bytes()
            original_xml, score_member = load_score_xml(original)
            profile_name = ('original' if (args.external or args.external_map or args.raw)
                            else profiles['assignments'].get(name, profiles['default']))
            style_properties = dict(profiles['profiles'][profile_name])
            # Studio parity defaults apply to *all* tests (including external
            # scores), without changing musical fonts or other inherited styles.
            # They are disabled in explicit raw/historical-reference mode.
            modern = {} if args.raw else dict(profiles.get('studio_comparison_37e_overrides', {}))
            style_properties.update(modern)
            common_properties = {k: v for k, v in style_properties.items() if k not in E37_ONLY}
            current_only = {k: v for k, v in style_properties.items() if k in E37_ONLY}
            studio_bytes = (original_xml if args.raw else
                            strip_37e_only_style(apply_style(original_xml, common_properties)))
            if studio_bytes != original_xml:
                studio_input = prepared_dir / f'{name}-studio{original.suffix.lower()}'
                write_prepared(original, studio_input, studio_bytes, score_member)
            else:
                studio_input = original
            if current_only:
                current_bytes = apply_style(studio_bytes, current_only)
                effective = prepared_dir / f'{name}-37e{original.suffix.lower()}'
                write_prepared(original, effective, current_bytes, score_member)
            else:
                effective = studio_input
            record = {'origin': str(original), 'origin_sha256': sha256_bytes(original_bytes),
                      'effective': str(effective), 'source_sha256': sha256_file(effective),
                      'effective_studio': str(studio_input),
                      'studio_source_sha256': sha256_file(studio_input),
                      'profile': profile_name, 'style_overrides': style_properties,
                      'studio_comparison_37e_overrides': modern,
                      'studio_style_overrides': common_properties,
                      '37e_only_overrides': current_only}
            if args.beam_ab and not args.raw:
                # Requested explicitly: compare only beamLayout4=0 with the
                # already-rendered modern 37E input (beamLayout4=1).
                if str(current_only.get('beamLayout4')) != '1':
                    raise ValueError('beam A/B requires beamLayout4=1 for the modern 37E input')
                # Studio continues to receive its single modern-reference input.
                legacy_file = prepared_dir / f'{name}-37e-beam-legacy{original.suffix.lower()}'
                legacy_xml, _ = load_score_xml(effective)
                legacy_bytes = apply_style(legacy_xml, {'beamLayout4': 0})
                write_prepared(effective, legacy_file, legacy_bytes, score_member)
                record['beam_legacy_effective'] = str(legacy_file)
                record['beam_legacy_source_sha256'] = sha256_file(legacy_file)
            metadata['scores'][name] = record
            jobs.append({'in': str(effective), 'out': f'{name}.png'})
            print(f'Input {name}: {profile_name}' +
                  (' (modern 37E overrides: ' + ', '.join(f'{k}={v}' for k,v in modern.items()) + ')'
                   if modern else ' (unchanged)' if not style_properties else ' (prepared inputs)'), flush=True)
        write_atomic(html_dir / 'input-manifest.json',
                     (json.dumps(metadata, indent=2, sort_keys=True) + '\n').encode('utf-8'))
        write_atomic(html_dir / 'vtestjob.json',
                     (json.dumps(jobs, indent=2) + '\n').encode('utf-8'))
    except (OSError, ValueError, ET.ParseError, UnicodeError, json.JSONDecodeError) as error:
        parser.error(str(error))


if __name__ == '__main__':
    main()
