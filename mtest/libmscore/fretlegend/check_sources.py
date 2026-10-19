#!/usr/bin/env python3
"""Check patch/data integrity; this does not compile or run the Qt tests."""
from collections import Counter
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[3]
header = (root / 'libmscore/property.h').read_text()
body = re.search(r'enum class Pid\s*:\s*short\s*\{(.*?)\};', header, re.S).group(1)
body = re.sub(r'//[^\n]*|/\*.*?\*/', '', body, flags=re.S)
enumerators = [part.strip().split('=')[0].strip() for part in body.split(',')]
enumerators = [name for name in enumerators if name]
table = (root / 'libmscore/property.cpp').read_text().split('propertyList[] = {', 1)[1].split('};', 1)[0]
metadata = re.findall(r'\{\s*Pid::(\w+)', table)
assert metadata == enumerators, 'Property table must have exactly the enum order (including guitar PIDs).'
print(f'PASS: {len(metadata)} property IDs match their metadata table indices')

style_header = (root / 'libmscore/style.h').read_text()
style_body = re.search(r'enum class Sid[^\{]*\{(.*?)\};', style_header, re.S).group(1)
style_body = re.sub(r'//[^\n]*|/\*.*?\*/', '', style_body, flags=re.S)
style_ids = [part.strip().split('=')[0].strip() for part in style_body.split(',')]
style_ids = [name for name in style_ids if name and name not in ('NOSTYLE', 'STYLES')]
style_table = (root / 'libmscore/style.cpp').read_text().split('static const StyleType styleTypes[] {', 1)[1].split('\n      };', 1)[0]
style_table = re.sub(r'//[^\n]*|/\*.*?\*/', '', style_table, flags=re.S)
assert re.findall(r'\{\s*Sid::(\w+)', style_table) == style_ids, 'Style IDs must match metadata indices'
print(f'PASS: {len(style_ids)} style IDs match their metadata table indices')

database = ET.parse(root / 'share/harmony_to_diagram.xml').getroot()
names = [entry.findtext('Harmony/name') for entry in database]
assert all(names)
assert not [key for key, count in Counter(names).items() if count > 1]
for entry in database:
    diagram = entry.find('FretDiagram')
    assert diagram is not None and diagram.find('fretDiagram') is not None
    fields = list(diagram)
    assert fields[-1].tag == 'fretDiagram', '3.x reads properties before fret geometry'
    for string in diagram.findall('fretDiagram/string'):
        assert 0 <= int(string.attrib['no']) < 6
        assert all(item.tag in ('dot', 'marker') for item in string)
print(f'PASS: {len(names)} unique diagram definitions have supported XML geometry')
for resource in ('mscore/musescore.qrc', 'mtest/mtest.qrc'):
    tree = ET.parse(root / resource)
    entries = [file for file in tree.findall('.//file') if file.get('alias') == 'data/harmony_to_diagram.xml']
    assert len(entries) == 1
    assert ((root / resource).parent / entries[0].text).resolve() == root / 'share/harmony_to_diagram.xml'
print('PASS: application and tests embed the same diagram database')
fixture = ET.parse(root / 'mtest/libmscore/fretlegend/legend460.mscx').getroot()
assert fixture.attrib['version'] == '4.60'
frame = fixture.find('.//Staff/FBox')
assert frame is not None and len(frame.findall('FretDiagram')) == 2
assert len(fixture.findall('.//Measure/voice/Harmony')) == 2
print('PASS: Studio-format import fixture has two legend entries and two source chords')
print('NOT RUN: C++ compilation, Qt tests, GUI rendering and interaction')
