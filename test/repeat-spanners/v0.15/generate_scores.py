#!/usr/bin/env python3
"""Generate paired 37E/4.7 repeat-spanner diagnostics. Python standard library only."""
from copy import deepcopy
from dataclasses import dataclass, field
from fractions import Fraction as F
import json
from pathlib import Path
import xml.etree.ElementTree as E

ROOT = Path(__file__).resolve().parent
VERSION = '0.15'
TPC = {0:14, 1:21, 2:16, 3:11, 4:18, 5:13, 6:20, 7:15, 8:10, 9:17, 10:12, 11:19}

def sub(p, tag, value=None, **attrs):
    el = E.SubElement(p, tag, {k:str(v) for k,v in attrs.items()})
    if value is not None: el.text = str(value)
    return el

def at(m, b=0, s=0, v=0, n=0): return (m,b,s,v,n)
def shifted(a, beats):
    m,b,s,v,n=a
    q=b+beats
    return (m+q//4,q%4,s,v,n)

@dataclass
class Case:
    key: str
    title: str
    bars: int = 6
    staves: int = 1
    voices: int = 1
    starts: list = field(default_factory=lambda:[2])
    ends: list = field(default_factory=lambda:[4])
    voltas: list = field(default_factory=list)
    breaks: dict = field(default_factory=lambda:{3:'line'})
    route: list = field(default_factory=lambda:[1,2,3,4,2,3,4,5,6])
    edges: list = field(default_factory=lambda:[[4,2]])
    pitches: dict = field(default_factory=dict)
    ties: list = field(default_factory=list)
    partials: list = field(default_factory=list)
    slurs: list = field(default_factory=list)
    lyrics: list = field(default_factory=list)
    labels: dict = field(default_factory=dict)
    markers: dict = field(default_factory=dict)
    jumps: dict = field(default_factory=dict)
    checks: list = field(default_factory=list)
    notes: str = ''

    def pitch(self,a):
        m,b,s,v,_=a
        return self.pitches.get((m,b,s,v), [60 if s==0 else 48] if v==0 else [55 if s==0 else 43])

    def label(self,a,text): self.labels.setdefault(a[:4],[]).append(text)
    def tie(self,a,b): self.ties.append((a,b))
    def partial(self,a,direction):
        if (a,direction) not in self.partials:self.partials.append((a,direction))
    def slur(self,a,b,direction='normal',up='up',line=0): self.slurs.append((a,b,direction,up,line))
    def lyric(self,a,text='',direction=None,kind=True,last=None,verse=0,placement='below',syllabic='single'):
        self.lyrics.append(dict(a=a,text=text,direction=direction,melisma=kind,last=last or a,verse=verse,placement=placement,syllabic=syllabic))

def volta(c):
    c.starts=[2];c.ends=[4];c.voltas=[(4,5,'1.',[1]),(5,6,'2.',[2])]
    c.route=[1,2,3,4,2,3,5,6];c.edges=[[4,2],[3,5]]

def cases():
    out=[]
    c=Case('R01','Ties: ordinary repeat and linear continuation')
    c.tie(at(1),at(1,1));c.tie(at(4,3),at(5));c.partial(at(2),'incoming')
    c.label(at(4,3),'A: source');c.label(at(2),'B: repeat landing');c.label(at(5),'C: linear landing')
    c.checks=['m1 beats 1-2 are an ordinary tie control.', 'A ties to B on the repeat and to C on the last pass.', 'B attacks on its first visit; it is sustained only when reached from A.']
    out.append(c)

    c=Case('R02','Ties: first and second endings');volta(c)
    c.tie(at(3,3),at(4));c.partial(at(5),'incoming');c.partial(at(4,3),'outgoing');c.partial(at(2),'incoming')
    for a,t in [(at(3,3),'A: before volta'),(at(4),'D: ending 1'),(at(5),'E: ending 2'),(at(4,3),'F: repeat only'),(at(2),'B: repeat landing')]:c.label(a,t)
    c.checks=['A has a normal tie to D and an incoming branch at E.', 'F has an outgoing partial to B; there is no full F-to-E tie.', 'Changing a repeat destination in the Inspector must survive undo/redo and save/reopen.']
    out.append(c)

    c=Case('R03','Ties: D.C. al Fine',starts=[],ends=[],route=[1,2,3,4,5,6,1,2,3],edges=[[6,1]])
    c.markers={3:('fine','Fine')};c.jumps={6:('start','fine','','D.C. al Fine')}
    c.partial(at(6,3),'outgoing');c.partial(at(1),'incoming')
    c.label(at(6,3),'A: D.C. source');c.label(at(1),'B: incoming after D.C.')
    c.checks=['No initial tie-in attack is suppressed on the first visit to m1.', 'The jump from m6 into m1 is sustained.', 'Playback ends at Fine on the return, not at the first Fine visit.']
    out.append(c)

    c=Case('R04','Ties: D.S. al Coda',bars=8,starts=[],ends=[],breaks={2:'line',4:'line',6:'line'},route=[1,2,3,4,5,6,2,3,4,7,8],edges=[[6,2],[4,7]])
    c.markers={2:('segno','Segno'),4:('coda','To Coda'),7:('codab','Coda')};c.jumps={6:('segno','coda','codab','D.S. al Coda')}
    c.partial(at(6,3),'outgoing');c.partial(at(2),'incoming')
    c.tie(at(4,3),at(5));c.partial(at(7),'incoming')
    for a,t in [(at(6,3),'A: D.S.'),(at(2),'B: Segno'),(at(4,3),'C: To Coda'),(at(7),'D: Coda')]:c.label(a,t)
    c.checks=['A sustains into B after D.S.', 'C ties linearly into m5 on pass 1 and into D on the Coda pass.', 'The first visits to B and D must follow the actual route; do not mute an unrelated attack.']
    out.append(c)

    c=Case('R05','Ties: chords, voices, and two staves',staves=2,voices=2,breaks={2:'line',4:'line'})
    for m in range(1,7):
        for b in range(4):
            c.pitches[m,b,0,0]=[60,64,67];c.pitches[m,b,0,1]=[55];c.pitches[m,b,1,0]=[48,52];c.pitches[m,b,1,1]=[43]
    for s,v,count in [(0,0,3),(0,1,1),(1,0,2),(1,1,1)]:
        for n in range(count):
            c.tie(at(4,3,s,v,n),at(5,0,s,v,n));c.partial(at(2,0,s,v,n),'incoming')
    c.label(at(4,3),'A: all voices');c.label(at(2),'B: chord landing')
    c.checks=['Each chord tone has its own incoming repeat tie.', 'Voice 2 and the lower staff retain separate ties and attacks.', 'Compare system-header clearance, stem direction, and stacked tie separation.']
    out.append(c)

    c=Case('R06','Ties: disabled, rest, and pitch controls',bars=12,starts=[2,6,10],ends=[3,7,11],breaks={4:'line',8:'line'},route=[1,2,3,2,3,4,5,6,7,6,7,8,9,10,11,10,11,12],edges=[[3,2],[7,6],[11,10]])
    for src,dst in [(3,4),(7,8),(11,12)]:c.tie(at(src,3),at(dst))
    c.pitches[6,0,0,0]=[];c.pitches[10,0,0,0]=[62]
    for a,t in [(at(2),'B: disabled'),(at(3,3),'A: branch OFF'),(at(6),'D: rest on landing'),(at(7,3),'C: rest control'),(at(10),'F: D vs source C'),(at(11,3),'E: pitch control')]:c.label(a,t)
    c.checks=['A has a valid repeat destination B, deliberately unchecked: B must reattack.', 'C has no repeat tie into a rest, even though a C note follows later in m6.', 'E has no repeat tie into the differently pitched F; all three linear ties remain.']
    out.append(c)

    c=Case('S01','Slurs: incoming and outgoing at a repeat')
    c.slur(at(2),at(2,2),'incoming');c.slur(at(4,1),at(4,3),'outgoing');c.slur(at(5),at(6,1))
    c.label(at(2),'B: incoming');c.label(at(4,1),'A: outgoing');c.label(at(5),'C: normal control')
    c.checks=['B hangs from the beginning of m2 and ends on beat 3.', 'A begins on beat 2 and hangs to the repeat barline.', 'C is an ordinary slur; Inspector direction and line style edits must remain independent.']
    out.append(c)

    c=Case('S02','Slurs: alternate endings');volta(c)
    # Keep D's stale incoming flag as a load-time normalization control.
    # No discontinuous jump arrives at the first ending; Studio resets D.
    c.slur(at(3,2),at(3,3),'outgoing');c.slur(at(4),at(4,1),'incoming');c.slur(at(5),at(5,1),'incoming')
    c.label(at(3,2),'A: outgoing');c.label(at(4),'D: first ending');c.label(at(5),'E: second ending')
    c.checks=['A is outgoing, D becomes ordinary on load (no jump arrives at the first ending), and E stays incoming.', 'Changing E must not reshape D.', 'Partial slurs are entered explicitly; automatic tie branching is not an automatic-slur promise.']
    out.append(c)

    c=Case('S03','Slurs: one-note anchors and both ends',starts=[2,4],ends=[2,4],breaks={2:'line',4:'line'},route=[1,2,2,3,4,4,5,6],edges=[[2,2],[4,4]])
    c.slur(at(2),at(2,3),'both');c.slur(at(4),at(4),'incoming','up');c.slur(at(4,3),at(4,3),'outgoing','down',2)
    c.label(at(2),'A: both ends');c.label(at(4),'B: one-note in');c.label(at(4,3),'C: one-note out')
    c.checks=['A hangs at both barlines, spanning the complete repeat bar.', 'B and C have one-note anchors and positive visual length.', 'C is below and dashed: compare direction/style persistence after save/reopen.']
    out.append(c)

    c=Case('S04','Slurs: system and page breaks',breaks={1:'line',2:'page',3:'line',4:'line'},staves=1)
    c.slur(at(2),at(4),'incoming');c.slur(at(4,1),at(4,3),'outgoing');c.slur(at(5),at(6,2))
    c.label(at(2),'A: incoming over breaks');c.label(at(4,1),'B: short outgoing');c.label(at(5),'C: normal control')
    c.checks=['A is an incoming partial spanning three systems and a forced page break.', 'B is a separate outgoing fragment on the final system; C is a normal slur.', 'A draft outgoing partial across several systems crashed stock 4.7.5; do not extend B across a system break during this baseline comparison.']
    out.append(c)

    c=Case('L01','Lyrics: melisma across a repeat')
    c.lyric(at(4,2),'sing','outgoing',True,at(4,3));c.lyric(at(2),'','incoming',True,at(2,1));c.lyric(at(2,2),'on');c.lyric(at(5),'on')
    c.label(at(4,2),'A: outgoing extension');c.label(at(2),'B: incoming extension')
    c.checks=['A continues to the repeat barline; B begins at the repeat landing.', 'B extends through beat 2, ending before the lyric on beat 3.', 'There is no printed empty syllable at B; selecting/deleting the line is undoable.']
    out.append(c)

    c=Case('L02','Lyrics: dashed continuation across a repeat')
    c.lyric(at(4,3),'re','outgoing',False);c.lyric(at(2),'','incoming',False,at(2,1));c.lyric(at(2,2),'turn',syllabic='end');c.lyric(at(5),'turn',syllabic='end')
    c.label(at(4,3),'A: re-');c.label(at(2),'B: dashed landing')
    c.checks=['A and B show dashes rather than solid melisma lines.', 'The incoming dashed span leads to turn on beat 3.', 'Toggle Extension line in the 37E Inspector, undo, save, and reopen.']
    out.append(c)

    c=Case('L03','Lyrics: two verses and voltas');volta(c)
    for v,kind,place,word,endword in [(0,True,'below','hold','on'),(1,False,'below','re','peat')]:
        c.lyric(at(3,3),word,'outgoing',kind,verse=v,placement=place)
        for m in (4,5):
            c.lyric(at(m),'','incoming',kind,at(m,1),v,place)
            c.lyric(at(m,2),endword,verse=v,placement=place,syllabic='single' if kind else 'end')
    c.label(at(3,3),'A: two verses');c.label(at(5),'E: second ending')
    c.checks=['Verse 1 is a melisma below; verse 2 is dashed on the next verse baseline below.', 'Both endings carry incoming lines for both verses.', 'Edit verse independently, then undo and save/reopen without moving another verse. L04 supplies the above-staff style control.']
    out.append(c)

    c=Case('L04','Lyrics: above staff, page break, and both ends',bars=8,starts=[2,7],ends=[5,7],breaks={1:'line',2:'page',4:'line',6:'line'},route=[1,2,3,4,5,2,3,4,5,6,7,7,8],edges=[[5,2],[7,7]])
    c.lyric(at(5,3),'Ah','outgoing',True);c.lyric(at(2),'','incoming',True,at(3,3));c.lyric(at(4),'rest');c.lyric(at(6),'end')
    c.lyric(at(7),'','both',True,at(7,3))
    for l in c.lyrics:l['placement']='above'
    c.label(at(2),'A: continuation over page');c.label(at(7),'B: both ends (geometry)')
    c.checks=['The score-wide lyric placement is Above. A starts at m2 and spans the page break through the last note of m3.', 'B is a manually prepared empty continuation hanging at both boundaries of a one-bar repeat.', 'B is a geometric stress case; lyrics do not synthesize sung words in playback.']
    out.append(c)

    c=Case('X01','Combined: grand staff, endings, ties, slurs, lyrics',bars=8,staves=2,breaks={2:'line',4:'page',6:'line'},starts=[2],ends=[5],voltas=[(5,6,'1.',[1]),(6,7,'2.',[2])],route=[1,2,3,4,5,2,3,4,6,7,8],edges=[[5,2],[4,6]])
    for m in range(1,9):
        for b in range(4): c.pitches[m,b,0,0]=[60,64,67]
    for n in range(3):
        c.tie(at(4,3,n=n),at(5,n=n));c.partial(at(6,n=n),'incoming');c.partial(at(5,3,n=n),'outgoing');c.partial(at(2,n=n),'incoming')
    c.slur(at(2),at(2,2),'incoming');c.slur(at(5,2),at(5,3),'outgoing');c.slur(at(6),at(6,2),'incoming')
    c.lyric(at(4,3),'hold','outgoing',True)
    for m in (5,6): c.lyric(at(m),'','incoming',True,at(m,1));c.lyric(at(m,2),'on')
    c.label(at(4,3),'A: combined source');c.label(at(6),'E: second ending')
    c.checks=['All three feature families coexist over a forced page break.', 'Chords branch to the correct ending, while the lower staff remains a rhythmic reference.', 'Save modified copies in each application, reopen, then repeat the route and Inspector checks.']
    out.append(c)

    c=Case('E01','Entry lab: create ties, slurs, and lyric continuations');volta(c)
    for a,t in [(at(3,3),'A: before volta'),(at(4),'D: ending 1'),(at(5),'E: ending 2'),(at(4,3),'F: repeat only'),(at(2),'B: repeat landing')]:c.label(a,t)
    c.checks=['Start from a fresh copy for each exercise; this score intentionally contains no ties, slurs, or lyrics.', 'Select A and press T outside note entry: D and E should receive ties.', 'Repeat in note entry and verify that the input cursor moves forward exactly once.', 'Select F and press T: create outgoing/incoming repeat ties without changing E.', 'Create explicit note+barline partial slurs and partial lyric lines; use the README step sequence.']
    out.append(c)
    return out

def location(parent,a,b):
    loc=sub(parent,'location')
    dm=b[0]-a[0];df=F(b[1]-a[1],4);dt=(b[2]-a[2])*4+b[3]-a[3]
    if dm:sub(loc,'measures',dm)
    if df:sub(loc,'fractions',str(df) if df.denominator!=1 else f'{df.numerator}/1')
    if dt:sub(loc,'voices',dt)
    if b[4]!=a[4]:sub(loc,'notes',b[4]-a[4])

def write_case(c,mode):
    native=mode=='475'
    root=E.Element('museScore',version='4.70' if native else '3.02')
    sub(root,'programVersion','4.7.5' if native else '3.7.0')
    score=sub(root,'Score');sub(score,'Division',480)
    style=sub(score,'Style')
    styles={'pageWidth':'8.26772','pageHeight':'11.69291','pagePrintableWidth':'7.36772','pageOddLeftMargin':'0.45','pageEvenLeftMargin':'0.45','pageOddTopMargin':'0.45','pageEvenTopMargin':'0.45','pageOddBottomMargin':'0.45','pageEvenBottomMargin':'0.45','musicalSymbolFont':'Bravura','musicalTextFont':'Bravura Text','titleFontFace':'FreeSerif','subTitleFontFace':'FreeSerif','lyricsOddFontFace':'FreeSerif','lyricsEvenFontFace':'FreeSerif','staffTextFontFace':'FreeSerif','staffTextFontSize':'9','titleFontSize':'17','subTitleFontSize':'10','lyricsOddFontSize':'10','lyricsEvenFontSize':'10','minSystemDistance':'9','maxSystemDistance':'12','showMeasureNumber':'1','measureNumberInterval':'1','showMeasureNumberOne':'1','hideEmptyStaves':'0'}
    if c.key=='L04':styles['lyricsPlacement']='0'
    for k,v in styles.items():sub(style,k,v)
    sub(style,'spatium' if native else 'Spatium','1.764')
    sub(score,'layoutMode','page');sub(score,'showInvisible',1)
    sub(score,'metaTag',f'{c.key}: {c.title}',name='workTitle')
    sub(score,'metaTag','Repeat-spanner comparison v0.15',name='subtitle')
    sub(score,'metaTag','Paired 37E / Studio 4.7 diagnostics',name='composer')
    part=sub(score,'Part',id=1)
    for s in range(c.staves):
        st=sub(part,'Staff',**({} if native else {'id':s+1}))
        sub(sub(st,'StaffType',group='pitched'),'name','stdNormal')
        sub(st,'defaultClef','G' if s==0 else 'F')
        if c.staves>1 and s==0:sub(st,'bracket',type=0,span=2,col=0);sub(st,'barLineSpan',2)
    sub(part,'trackName','Piano')
    ins=sub(part,'Instrument',id='keyboard.piano');sub(ins,'longName','Piano');sub(ins,'shortName','Pno.');sub(ins,'trackName','Piano');sub(ins,'instrumentId','keyboard.piano')
    if c.staves>1:sub(ins,'clef','F',staff=2)
    ch=sub(ins,'Channel');E.SubElement(ch,'program',{'value':'0'});E.SubElement(ch,'controller',{'ctrl':'7','value':'90'});E.SubElement(ch,'controller',{'ctrl':'10','value':'64'})
    voice_events={};chord_events={};note_events={}
    def add(mapping,a,item):mapping.setdefault(a,[]).append(item)
    def spanner(typ,a,b,props=None,anchor='chord'):
        start=E.Element('Spanner',type=typ);body=sub(start,typ)
        for k,v in (props or {}).items():sub(body,k,v)
        location(sub(start,'next'),a,b)
        end=E.Element('Spanner',type=typ);location(sub(end,'prev'),b,a)
        mapping={'voice':voice_events,'chord':chord_events,'note':note_events}[anchor]
        add(mapping,a if anchor=='note' else a[:4],start);add(mapping,b if anchor=='note' else b[:4],end)
    for a,b in c.ties:spanner('Tie',a,b,anchor='note')
    for a,d in c.partials:
        p=E.Element('PartialTie');sub(p,'partialSpannerDirection',d);add(note_events,a,p)
    for a,b,d,up,line in c.slurs:
        props={'up':up,'lineType':line}
        if d!='normal':props['partialSpannerDirection']=d
        spanner('Slur',a,b,props)
    for m,end,text,endings in c.voltas:
        spanner('Volta',at(m),at(end),{'endHookType':1,'beginText':text,'endings':','.join(map(str,endings))},'voice')
    lyric_nodes={}
    for l in c.lyrics:
        a,last=l['a'],l['last'];direction=l['direction'];text=l['text'];ticks=F((last[0]-a[0])*4+last[1]-a[1],4)
        if native and direction in ('incoming','both'):
            end=shifted(last,1)
            props={'isEndMelisma':int(l['melisma']),'no':l['verse']}
            spanner('PartialLyricsLine',a,end,props,'voice')
        if native and not text:continue
        lyr=E.Element('Lyrics')
        sub(lyr,'no',l['verse']);sub(lyr,'placement',l['placement'])
        syll=l['syllabic']
        if not native and direction:
            sub(lyr,'partialSpannerDirection',direction);sub(lyr,'partialLyricMelisma',int(l['melisma']));sub(lyr,'ticks_f',f'{ticks.numerator}/{ticks.denominator}')
        elif native and direction=='outgoing':
            if l['melisma']:
                t=max(ticks,F(1,1920));sub(lyr,'ticks_f',f'{t.numerator}/{t.denominator}')
            else:syll='begin'
        if syll!='single':sub(lyr,'syllabic',syll)
        if text:sub(lyr,'text',text)
        lyric_nodes.setdefault(a[:4],[]).append(lyr)
    for s in range(c.staves):
        staff=sub(score,'Staff',id=s+1)
        if s==0:
            box=sub(staff,'VBox');sub(box,'height',15)
            for kind,text in [('Title',f'{c.key}: {c.title}'),('Subtitle','Repeat-spanner comparison v0.15 | quarter = 96')]:
                t=sub(box,'Text');sub(t,'style',kind);sub(t,'text',text)
        for m in range(1,c.bars+1):
            measure=sub(staff,'Measure')
            if s==0:
                if m in c.starts:sub(measure,'startRepeat')
                if m in c.ends:sub(measure,'endRepeat',2)
                if m in c.markers:
                    label,text=c.markers[m];mark=sub(measure,'Marker');sub(mark,'style','Repeat Text Left' if label in ('segno','codab') else 'Repeat Text Right');sub(mark,'text',text);sub(mark,'label',label)
                if m in c.jumps:
                    target,until,cont,text=c.jumps[m];jump=sub(measure,'Jump');sub(jump,'style','Repeat Text Right');sub(jump,'text',text);sub(jump,'jumpTo',target);sub(jump,'playUntil',until);sub(jump,'continueAt',cont)
                if m in c.breaks:br=sub(measure,'LayoutBreak');sub(br,'subtype',c.breaks[m])
            for v in range(c.voices):
                voice=sub(measure,'voice')
                if m==1 and v==0:
                    sub(sub(voice,'KeySig'),'concertKey',0)
                    ts=sub(voice,'TimeSig');sub(ts,'sigN',4);sub(ts,'sigD',4)
                    if s==0:tempo=sub(voice,'Tempo');sub(tempo,'tempo','1.6');sub(tempo,'followText',0);sub(tempo,'text','Quarter = 96')
                for b in range(4):
                    a=at(m,b,s,v);key=a[:4]
                    for item in voice_events.get(key,[]):voice.append(deepcopy(item))
                    for text in c.labels.get(key,[]):
                        st=sub(voice,'StaffText');sub(st,'text',text)
                    ps=c.pitch(a)
                    node=sub(voice,'Chord' if ps else 'Rest');sub(node,'durationType','quarter')
                    if c.voices>1 and ps:sub(node,'stemDirection','up' if v==0 else 'down')
                    if ps:
                        for item in chord_events.get(key,[]):node.append(deepcopy(item))
                        for lyr in lyric_nodes.get(key,[]):node.append(deepcopy(lyr))
                        for n,pitch in enumerate(ps):
                            note=sub(node,'Note');sub(note,'pitch',pitch);sub(note,'tpc',TPC[pitch%12])
                            for item in note_events.get(at(m,b,s,v,n),[]):note.append(deepcopy(item))
    E.indent(root,space='  ')
    filename=f'{c.key}-{mode}.mscx'
    E.ElementTree(root).write(ROOT/filename,encoding='utf-8',xml_declaration=True)
    return filename

def main():
    manifest=[]
    for c in cases():
        files={mode:write_case(c,mode) for mode in ('37E','475')}
        manifest.append(dict(id=c.key,title=c.title,files=files,bars=c.bars,staves=c.staves,voices=c.voices,route=c.route,edges=c.edges,full_ties=len(c.ties),partial_ties=len(c.partials),slurs=len(c.slurs),partial_lyrics=sum(bool(l['direction']) for l in c.lyrics),checks=c.checks))
    (ROOT/'manifest.json').write_text(json.dumps({'version':VERSION,'cases':manifest},indent=2)+'\n')
    print(f'Generated {len(manifest)*2} scores in {ROOT}')

if __name__=='__main__':main()
