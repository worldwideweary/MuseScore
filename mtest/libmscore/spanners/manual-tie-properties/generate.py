#!/usr/bin/env python3
"""Author paired native-4.7/37E XML fixtures; no MuseScore executable required."""
from pathlib import Path
import xml.etree.ElementTree as E

ROOT = Path(__file__).resolve().parent
VERSION = 'v0.15'

def tag(parent, element_name, text=None, **attrs):
    node = E.SubElement(parent, element_name, attrs)
    if text is not None:
        node.text = str(text)
    return node

def score(kind, studio, away=False):
    root = E.Element('museScore', version='4.70' if studio else '3.02')
    s = tag(root, 'Score'); tag(s, 'Division', 480)
    style = tag(s, 'Style')
    values = dict(pageWidth=8.26772, pageHeight=11.69291, pagePrintableWidth=7.36772,
                  pageOddLeftMargin=.45, pageEvenLeftMargin=.45, pageOddTopMargin=.45,
                  pageEvenTopMargin=.45, pageOddBottomMargin=.45, pageEvenBottomMargin=.45,
                  musicalSymbolFont='Bravura', musicalTextFont='Bravura Text',
                  titleFontFace='FreeSerif', titleFontSize=17, subTitleFontFace='FreeSerif',
                  subTitleFontSize=10, staffTextFontFace='FreeSerif', staffTextFontSize=9,
                  minSystemDistance=11, maxSystemDistance=14, spatium=1.764,
                  createMultiMeasureRests=0, hideEmptyStaves=0,
                  tiePlacementSingleNote='outside', tiePlacementChord='outside',
                  tieDotsPlacement='auto', minHangingTieLength=1.5, minLaissezVibLength=3,
                  laissezVibUseSmuflSym=0, angleHangingSlursAwayFromStaff=int(away))
    for key, value in values.items(): tag(style, key, value)
    tag(s, 'metaTag', VERSION+' '+kind, name='workTitle')
    part = tag(s, 'Part', id='1'); st = tag(part, 'Staff', id='1')
    typ = tag(st, 'StaffType', group='pitched'); tag(typ, 'name', 'stdNormal')
    tag(st, 'defaultClef', 'G'); tag(part, 'trackName', 'Piano')
    instrument = tag(part, 'Instrument', id='keyboard.piano' if studio else 'piano')
    tag(instrument, 'longName', 'Piano'); tag(instrument, 'shortName', 'Pno.')
    tag(instrument, 'trackName', 'Piano')
    if studio: tag(instrument, 'instrumentId', 'keyboard.piano')
    channel = tag(instrument, 'Channel')
    tag(channel, 'program', value='0')
    staff = tag(s, 'Staff', id='1'); box = tag(staff, 'VBox'); tag(box, 'height', 16)
    title = tag(box, 'Text'); tag(title, 'style', 'title' if studio else 'Title'); tag(title, 'text', VERSION+' '+kind)
    sub = tag(box, 'Text'); tag(sub, 'style', 'subtitle' if studio else 'Subtitle')
    tag(sub, 'text', ('475 native XML' if studio else '37E paired XML')
        + (' | Angle away' if away else ' | Follow contour') + ' | See README')
    return root, staff

def measure(staff, label=None, first=False):
    m = tag(staff, 'Measure'); v = tag(m, 'voice')
    if first:
        # Staff.defaultClef already supplies the opening G clef. An explicit,
        # non-header Clef here creates a second clef after the time signature.
        ts = tag(v, 'TimeSig'); tag(ts, 'sigN', 4); tag(ts, 'sigD', 4)
    if label:
        text = tag(v, 'StaffText'); tag(text, 'text', label)
    return m, v

def chord(v, pitches=(60,64,67), duration='half', stem='up', dotted=False):
    c = tag(v, 'Chord')
    if dotted: tag(c, 'dots', 1)
    tag(c, 'durationType', duration); tag(c, 'StemDirection', stem)
    notes=[]
    tpcs = {57:17,60:14,64:18,67:15,69:17,71:19,72:14}
    for pitch in pitches:
        n=tag(c, 'Note'); tag(n, 'pitch', pitch); tag(n, 'tpc', tpcs[pitch]); notes.append(n)
    return c, notes

def rest(v, duration='half'):
    r=tag(v,'Rest'); tag(r,'durationType',duration)
    if duration=='measure': tag(r,'duration','1/1')

def line(m):
    b=tag(m,'LayoutBreak'); tag(b,'subtype','line')

def connection(parent, kind, direction, measures=0, fraction='1/2', properties=None):
    sp=tag(parent,'Spanner',type=kind)
    if direction=='next':
        content=tag(sp,kind)
        for key,value in (properties or {}).items():tag(content,key,value)
    loc=tag(tag(sp,direction),'location')
    if measures:tag(loc,'measures',measures)
    if fraction is not None:tag(loc,'fractions',fraction)
    return sp

def slur(first,last,partial=None,measures=0,fraction='1/2'):
    props={'up':'down'}
    if partial:props['partialSpannerDirection']=partial
    connection(first,'Slur','next',measures,fraction,props)
    # The reverse location includes both the negative measure and fractional offsets.
    reverse = fraction if fraction is None or fraction.startswith('-') else '-'+fraction
    if fraction in ('0/1','0'):reverse='0/1'
    connection(last,'Slur','prev',-measures,reverse)

def volta(v1,v2,ending):
    start=connection(v1,'Volta','next',1,None,{'endHookType':1,'beginText':str(ending)+'.','endings':ending})
    end=connection(v2,'Volta','prev',-1,None)
    v1.remove(start);v1.insert(0,start)
    v2.remove(end);v2.insert(0,end)

def hanging(studio):
    root,staff=score('C - Hanging ties',studio)
    # Each outgoing tie must be on the final chord before a repeat jump,
    # and each matching incoming tie must be at that repeat's destination.
    # Preserve case IDs, so the chronological order is C2,C1,C4,C3,C6,C5.
    for pair,(out_placement,in_placement,out_stem,in_stem) in enumerate([
            ('auto','auto','up','down'),('inside','outside','down','up'),
            ('auto','auto','up','up')]):
        for incoming in (True,False):
            case=2*pair+(2 if incoming else 1)
            direction='incoming' if incoming else 'outgoing'
            placement=in_placement if incoming else out_placement
            m,v=measure(staff,f'C{case} {direction} / {placement}',first=pair==0 and incoming)
            if incoming:tag(m,'startRepeat')
            else:
                tag(m,'endRepeat',2)
                rest(v)
            c,notes=chord(v,stem=in_stem if incoming else out_stem)
            for n in notes:
                p=tag(n,'PartialTie');tag(p,'partialSpannerDirection',direction)
                if placement!='auto':tag(p,'tiePlacement',placement)
            if incoming:rest(v)
            if not incoming or pair==2:line(m)
    return root

def slurs(studio,away=False):
    root,staff=score('D - Partial slurs',studio,away)
    descriptions=[('D1 Normal',60,72,None),('D2 Outgoing / repeat end',60,57,'outgoing'),
                  ('D3 Incoming / repeat start',60,72,'incoming'),('D4 Both / one-bar repeat',60,57,'both'),
                  ('D5 Rising / outgoing / system break',60,64,'outgoing'),
                  ('D6 Rising / incoming / repeat start',67,71,'incoming'),
                  ('D7 First ending / outgoing',60,67,'outgoing'),
                  ('D8 Second ending / incoming',60,72,'incoming')]
    voices=[];bars=[]
    for i,(label,a,b,direction) in enumerate(descriptions,1):
        if away and direction=='both':
            direction=None
            label='D4 Normal control / both-hanging case excluded'
        m,v=measure(staff,label,first=i==1);bars.append(m);voices.append(v)
        ca,_=chord(v,(a,));cb,_=chord(v,(b,));slur(ca,cb,direction)
        if i in (2,3,4,5,7):tag(m,'endRepeat',2)
        if i in (3,4,5,6):tag(m,'startRepeat')
        if i in (2,4,5,6,8):line(m)
    m9,v9=measure(staff,'D9 End of second ending');rest(v9,'measure');line(m9)
    volta(voices[6],voices[7],1);volta(voices[7],v9,2)
    m10,v10=measure(staff,'D10 Normal slur / forced break');a,_=chord(v10,(60,));chord(v10,(64,));line(m10)
    m11,v11=measure(staff,'D11 Normal slur continuation');chord(v11,(67,));b,_=chord(v11,(71,));line(m11)
    slur(a,b,None,1,'1/2')
    m12,v12=measure(staff,'D12 Both-hanging case excluded' if away else 'D12 One-note / both / one-bar repeat')
    tag(m12,'startRepeat');tag(m12,'endRepeat',2)
    only,_=chord(v12,(60,),duration='whole')
    if not away:slur(only,only,'both',0,'0/1')
    return root

def lv(studio):
    root,staff=score('E - Laissez vibrer',studio)
    cases=[('E1 Single / 2.0 sp / Auto',(60,),'auto','auto','up',2,False),
           ('E2 Chord / 3.0 sp / Up / Inside',(60,64,67),'inside','up','up',3,False),
           ('E3 Chord / 3.0 sp / Down / Outside',(60,64,67),'outside','down','down',3,False),
           ('E4 Dotted chord / Auto',(60,64,67),'auto','auto','up',3,True),
           ('E5 Up stem / Up / Outside',(60,64,67),'outside','up','up',3,False),
           ('E6 Up stem / Down / Outside',(60,64,67),'outside','down','up',3,False),
           ('E7 Down stem / Up / Outside',(60,64,67),'outside','up','down',3,False),
           ('E8 Down stem / Down / Outside',(60,64,67),'outside','down','down',3,False)]
    for i,(label,pitches,placement,direction,stem,length,dotted) in enumerate(cases,1):
        m,v=measure(staff,label,first=i==1);c,notes=chord(v,pitches,stem=stem,dotted=dotted)
        for n in notes:
            mark=tag(n,'LaissezVib');tag(mark,'minLength',length)
            if placement!='auto':tag(mark,'tiePlacement',placement)
            if direction!='auto':tag(mark,'up',direction)
        rest(v,'quarter' if dotted else 'half')
        if i%2==0:line(m)
    return root

def write(root,name):
    E.indent(root,space='  ')
    E.ElementTree(root).write(ROOT/name,encoding='UTF-8',xml_declaration=True)
    with (ROOT/name).open('ab') as f:f.write(b'\n')

if __name__=='__main__':
    for studio in (True,False):
        suffix='475' if studio else '37E'
        for kind,build in [('C-hanging-ties',hanging),('D-partial-slurs',slurs),('E-laissez-vibrer',lv)]:
            write(build(studio),f'{VERSION}-{kind}-{suffix}.mscx')
        write(slurs(studio,True),f'{VERSION}-D-partial-slurs-angle-away-{suffix}.mscx')
