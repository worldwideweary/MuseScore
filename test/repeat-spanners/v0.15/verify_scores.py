#!/usr/bin/env python3
"""Check pair content; optionally verify Studio roundtrips and exported MIDI.

python3 verify_scores.py
python3 verify_scores.py --studio-output /path/to/native/exports

The optional directory contains CASE-475.mscx and CASE-475.mid from Studio.
Only the Python standard library is required.
"""
import argparse
from collections import Counter
from fractions import Fraction as F
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as E

from generate_scores import ROOT, at, cases

def scan(path):
    r=E.parse(path).getroot();score=r.find('Score')
    assert score is not None, f'{path.name}: no Score'
    notes=[];rhythm=[];connectors=Counter()
    for s,staff in enumerate(score.findall('Staff')):
        for m,measure in enumerate(staff.findall('Measure'),1):
            for v,voice in enumerate(measure.findall('voice')):
                b=0
                for el in voice:
                    address=at(m,b,s,v)
                    if el.tag=='Spanner':read_connector(connectors,el,address,'voice')
                    if el.tag not in ('Chord','Rest'):continue
                    assert el.findtext('durationType')=='quarter', f'{path.name}: non-quarter event'
                    rhythm.append((m,b,s,v,el.tag))
                    for sp in el.findall('Spanner'):read_connector(connectors,sp,address,'chord')
                    for n,note in enumerate(el.findall('Note')):
                        a=at(m,b,s,v,n)
                        notes.append((*a,int(note.findtext('pitch'))))
                        for sp in note.findall('Spanner'):read_connector(connectors,sp,a,'note')
                    b+=1
                assert b==4, f'{path.name}: m{m}, staff {s+1}, voice {v+1} is not 4/4'
    for (kind,anchor,a,b,d),count in connectors.items():
        other='prev' if d=='next' else 'next'
        assert connectors[(kind,anchor,b,a,other)]==count, f'{path.name}: unmatched {kind} {a}->{b}'
    return r,notes,rhythm

def read_connector(out,sp,a,anchor):
    for d in ('next','prev'):
        loc=sp.find(d+'/location')
        if loc is None:continue
        m,b,s,v,n=a;dm=int(loc.findtext('measures','0'));df=F(loc.findtext('fractions','0'));dv=int(loc.findtext('voices','0'))
        tick=F(b,4)+df
        # Fractions are a signed offset within the referenced measure.
        assert tick.denominator in (1,2,4), 'Unexpected fractional anchor'
        absolute=(m+dm,int(tick*4),s+(v+dv)//4,(v+dv)%4,n+int(loc.findtext('notes','0')))
        out[(sp.get('type'),anchor,a,absolute,d)]+=1

def midi_attacks(path):
    data=path.read_bytes();assert data[:4]==b'MThd'
    size=struct.unpack('>I',data[4:8])[0];_,tracks,division=struct.unpack('>HHH',data[8:14]);assert not division&0x8000
    pos=8+size;result=[]
    def var(buf,p):
        value=0
        while True:
            x=buf[p];p+=1;value=(value<<7)|(x&127)
            if x<128:return value,p
    for _ in range(tracks):
        assert data[pos:pos+4]==b'MTrk';length=struct.unpack('>I',data[pos+4:pos+8])[0]
        buf=data[pos+8:pos+8+length];pos+=8+length;p=0;tick=0;running=0
        while p<len(buf):
            delta,p=var(buf,p);tick+=delta;status=buf[p]
            if status>=128:p+=1
            else:status=running
            if status==255:
                p+=1;n,p=var(buf,p);p+=n;continue
            if status in (240,247):n,p=var(buf,p);p+=n;continue
            assert 128<=status<240
            running=status;kind=status&240;n=1 if kind in (192,208) else 2
            args=buf[p:p+n];p+=n
            if kind==144 and args[1]>0:result.append((F(tick,division),args[0]))
    return Counter(result)

def expected_attacks(c):
    incoming={a for a,d in c.partials if d=='incoming'}
    outgoing={a for a,d in c.partials if d=='outgoing'}
    full=dict(c.ties);result=Counter();previous={};q=0
    for m in c.route:
        for b in range(4):
            next_previous={}
            for s in range(c.staves):
                for v in range(c.voices):
                    for n,pitch in enumerate(c.pitch(at(m,b,s,v))):
                        a=at(m,b,s,v,n);key=(s,v,n);prior=previous.get(key)
                        tied=False
                        if prior:
                            source,old_pitch=prior
                            tied=full.get(source)==a
                            if source[1]==3 and b==0 and source[0]+1!=m:
                                tied=old_pitch==pitch and a in incoming and (source in full or source in outgoing)
                        if not tied:result[F(q),pitch]+=1
                        next_previous[key]=(a,pitch)
            previous=next_previous;q+=1
    return result

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--studio-output',type=Path);parser.add_argument('--report',type=Path)
    args=parser.parse_args();rows=[]
    for c in cases():
        left,ln,lr=scan(ROOT/f'{c.key}-37E.mscx');right,rn,rr=scan(ROOT/f'{c.key}-475.mscx')
        assert ln==rn and lr==rr, f'{c.key}: pair music differs'
        assert len(left.findall('./Score/Staff'))==c.staves
        for r in (left,right):
            assert all(len(st.findall('Measure'))==c.bars for st in r.findall('./Score/Staff'))
            assert len(r.findall('.//PartialTie'))==len(c.partials)
            assert len(r.findall('.//Slur'))==len(c.slurs)
            assert len(r.findall('.//Tie'))==len(c.ties)
        expected_lines=sum(l['direction'] in ('incoming','both') for l in c.lyrics)
        assert len(right.findall('.//PartialLyricsLine'))==expected_lines
        assert sum(l.find('partialSpannerDirection') is not None for l in left.findall('.//Lyrics'))==sum(bool(l['direction']) for l in c.lyrics)
        assert not any(l.find('text') is None for l in right.findall('.//Lyrics'))
        row=dict(id=c.key,notes_per_written_score=len(ln),bars=c.bars,partial_ties=len(c.partials),slurs=len(c.slurs),native_partial_lyric_lines=expected_lines,pair_check='PASS')
        if args.studio_output:
            saved,notes,rhythm=scan(args.studio_output/f'{c.key}-475.mscx')
            assert notes==rn and rhythm==rr, f'{c.key}: native save/reopen changed music'
            for tag in ('PartialTie','Slur','Tie','PartialLyricsLine'):
                assert len(saved.findall('.//'+tag))==len(right.findall('.//'+tag)), f'{c.key}: native save lost {tag}'
            actual=midi_attacks(args.studio_output/f'{c.key}-475.mid');wanted=expected_attacks(c)
            row.update(native_roundtrip='PASS',midi_attacks=sum(actual.values()),midi_route_and_ties='MATCH' if actual==wanted else 'DIFF')
            row['midi_extra']=[dict(quarter=str(t),pitch=pitch,count=count) for (t,pitch),count in sorted((actual-wanted).items())]
            row['midi_missing']=[dict(quarter=str(t),pitch=pitch,count=count) for (t,pitch),count in sorted((wanted-actual).items())]
        rows.append(row)
    report={'version':'0.15','cases':rows,'paired_files':len(rows)*2,'structural_status':'PASS'}
    if args.studio_output:report['native_midi_comparison']='MATCH' if all(r['midi_route_and_ties']=='MATCH' for r in rows) else 'BASELINE_DIFFERENCES_RECORDED'
    if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
