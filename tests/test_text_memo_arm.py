from test_inputs import ROOT, engine_path, elf_path, output
"""Execute compiled memo in ARM; model managed allocation, glyph lookup and layout."""
from pathlib import Path
import os
exec(compile((ROOT/'tests/test_text_fast_arm.py').read_text().split('rng=random.Random(41)')[0], 'tests/test_text_fast_arm.py','exec'))
def clib_copy():
 dest,source,size=arg(0),arg(1),arg(2);u.mem_write(dest,bytes(u.mem_read(source,size)));result(dest)
stub(clib_copy,app['sceClibMemcpy'])
large_capacity=os.environ.get('SPIRE_TEST_LARGE_CACHE')=='1'
active_slots=512 if large_capacity else 128
def set_capacity(enabled):
 u.reg_write(R[0],int(enabled));u.reg_write(UC_ARM_REG_SP,STACK+0x8000);u.reg_write(UC_ARM_REG_LR,STOP|1)
 u.emu_start(app['text_cache_capacity_enable'],STOP,count=10000);assert u.reg_read(UC_ARM_REG_PC)==STOP;return reg(0)
assert set_capacity(large_capacity)==0
assert word(app['memo_active_slots'])==active_slots
assert word(app['vertex_active_slots'])==(1024 if large_capacity else 256)
src=(ROOT/'src/text_memo.c').read_text();names=re.findall(r'"([^"]+)"',src.split('const char *names[]={')[1].split('};')[0]);ma=[BASE+syms[n]['st_value'] for n in names]
for name,a in zip(('memo_create','memo_set','memo_glyph','memo_kerning','memo_roots','memo_pack','memo_base_runs','memo_ft_runs','memo_ft_glyph','memo_gc_stop','memo_data_init','memo_clear','memo_draw'),ma):put(app[name],a)
u.mem_write(ma[10],b'\1');put(ma[9],0)
dtype,dvt=object();put(data,word(dtype));put(dvt+72,ma[2]);put(dvt+80,ma[6]);put(cvt+152,ma[11]);put(cvt+128,ma[12]);stub(cache_clear,ma[11]);stub(cache_draw,ma[12])
letters={};kern=0;missing=False;created=[];roots=[];preparations=0;bad_ref=None
for ch in ' abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789é':
 g,vt=object();put(vt+32,ma[3]);put(g+20,6);put(g+44,1);put(g+52,7);letters[ord(ch)]=g
# Fixture default heap is small; extend it for up to 128 managed layouts.
u.mem_map(HEAP+0x100000,0x1000000)
def new_layout():
 q,vt=object();created.append(q);result(q)
def make_array(values):
 container=alloc(32);items=alloc(32);shape=alloc(16);storage=alloc(max(16,len(values)*4))
 put(container+8,items);put(container+12,len(values));put(items+8,storage);put(items+20,shape);put(shape,len(values))
 for i,v in enumerate(values):put(storage+i*4,v)
 return container,storage
def set_owned():
 global preparations
 preparations+=1;assert arg(1)==font
 q=arg(0);n=word(arg(2)+8);chars=struct.unpack('<'+str(n)+'H',u.mem_read(arg(2)+12,n*2));width=sum(word(letters[c]+52) for c in chars)*gf(data+60)+kern*max(n-1,0)
 pf(q+12,width);pf(q+16,14.75)
 run=alloc(32);rc=alloc(32);put(run+28,rc);u.mem_write(rc+8,bytes(u.mem_read(color+8,16)));pf(run+16,0);pf(run+20,0);pf(run+24,width)
 glyph_list,gp=make_array([letters[c] for c in chars]);advances,ap=make_array([0]+[struct.unpack('<I',struct.pack('<f',7*gf(data+60)))[0]]*n)
 put(run+8,glyph_list);put(run+12,advances);runs,_=make_array([run]);put(q+8,runs)
 if bad_ref=='x':pf(run+16,1)
 if bad_ref=='color':pf(rc+8,99)
 if bad_ref=='advance':pf(ap,99)
 if bad_ref=='glyph':put(gp,0)
 if bad_ref=='width':pf(q+12,99)
 result()
def glyph_lookup():result(0 if missing else letters.get(arg(1),0))
def root():roots.append((arg(0),arg(1)));result()
stub(new_layout,ma[0]);stub(set_owned,ma[1]);stub(glyph_lookup,ma[2]);stub(lambda:result(kern),ma[3]);stub(root,ma[4]);stub(lambda:hit('pack'),ma[5])
def set_string(s,p=text):
 raw=s.encode('utf-16le');put(p+8,len(raw)//2);u.mem_write(p+12,raw);return p
pf(data+60,0.8)
def get(s=None,p=text):
 if s is not None:set_string(s,p)
 for i,v in enumerate((font,data,p,color)):u.reg_write(R[i],v)
 u.reg_write(UC_ARM_REG_SP,STACK+0x8000);u.reg_write(UC_ARM_REG_LR,STOP|1)
 u.emu_start(app['text_memo_get'],0,count=300000);assert u.reg_read(UC_ARM_REG_PC)==STOP,hex(u.reg_read(UC_ARM_REG_PC));return reg(0)
def fresh_change(mutator,label):
 before=preparations;mutator();q=get();assert q and preparations==before+1,label
 assert get()==q and preparations==before+1,label
q=get('Damage 6');assert set_capacity(not large_capacity)==0xffffffff
assert word(app['memo_active_slots'])==active_slots
assert q and q!=layout and preparations==1
for _ in range(100):assert get()==q
assert preparations==1 and len(roots)==1 and roots[0][0]==app['memo_entries']
other=alloc();set_string('Damage 6',other);assert get(p=other)==q and preparations==1
fresh_change(lambda:set_string('Damage 9'),'content at same address')
fresh_change(lambda:pf(color+8,0.5),'color')
fresh_change(lambda:pf(data+60,1.1),'scale')
fresh_change(lambda:pf(data+44,2),'font ascent')
fresh_change(lambda:put(letters[ord('D')]+20,9),'glyph metrics')
kern=2;fresh_change(lambda:None,'kerning in place')
g,vt=object();put(vt+32,ma[3]);put(g+20,6);put(g+52,7);letters[ord('D')]=g
fresh_change(lambda:None,'glyph replacement')
# Shared FontHelper layout is neither retained nor written by memo.
pf(layout+12,-999);assert get()!=layout and gf(layout+12)==-999
for s in ('[red]Damage','A\nB','A'*49,''):
 before=preparations;assert not get(s) and preparations==before
get('Damage 9');missing=True;assert not get();missing=False;assert get()
put(ma[9],1);assert not get();put(ma[9],0)
u.mem_write(ma[10],b'\0');assert not get();u.mem_write(ma[10],b'\1')
# FreeType dirty atlas bypass and packToTexture side effect.
put(dvt+72,ma[8]);put(dvt+80,ma[7]);packer,pvt=object();put(pvt+216,ma[5]);put(data+120,packer)
u.mem_write(data+128,b'\1');assert not get();u.mem_write(data+128,b'\0');assert get();counts.clear();assert get();assert counts.get('pack')==1
put(pvt+216,0);assert not get();put(pvt+216,ma[5])
# More keys than capacity, managed layout count stays bounded; evicted text rebuilds.
for i in range(active_slots*3):assert get('Card '+str(i))
assert len(created)<=active_slots and word(app['memo_evict'])>0
before=preparations;assert get('Damage 6');assert preparations==before+1
print('PASS ARM memo: content/color/scale/font/glyph/kerning invalidation, GC roots, dirty atlas, bounded eviction;',len(created),'layouts')
# Integration: same text is laid out once, but each draw uses current coordinates.
def prepared_owned():
 global prepared_xy
 assert arg(0)==cache and arg(1) in created
 prepared_xy=(fl(2),fl(3));hit('prepared');result()
stub(prepared_owned,addresses[11])
set_string('Damage 6');before=preparations
first=invoke(app['text_fast'],[100,200,0,0,0,0]);second=invoke(app['text_fast'],[100,200,0,0,0,0]);assert first[0]==second[0] and preparations==before+1
moved=invoke(app['text_fast'],[150,210,0,0,0,0]);assert preparations==before+1
assert abs(moved[0][0][0]-second[0][0][0]-50)<0.001 and abs(moved[0][0][1]-second[0][0][1]-10)<0.001
set_string('Damage 9');changed=invoke(app['text_fast'],[150,210,0,0,0,0]);assert preparations==before+2
print('PASS text_fast integration: repeat skips preparation, movement and changed content handled')

# Fresh original layout verification compares positions, color, glyphs and advances.
put(app['memo_hits'],255);assert get();assert word(app['memo_checks'])==1 and not word(app['memo_disabled'])
for fault in ('x','color','advance','glyph','width'):
 bad_ref=fault;put(app['memo_disabled'],0);put(app['memo_hits'],255)
 assert not get() and word(app['memo_disabled'])==1,fault
before=preparations;assert not get() and preparations==before
assert word(app['memo_mismatch'])==5 and len(created)<=active_slots+1
print('PASS reference-layout comparator and automatic fallback on five mismatch classes')
output('text-memo-large-arm-validation.json' if large_capacity else 'text-memo-arm-validation.json').write_text(json.dumps({'passed':True,'reuse_checks':100,'active_slots':active_slots,'churn_keys':active_slots*3,'managed_layouts':len(created),'preparations':preparations,'root_registrations':len(roots),'integration_passed':True,'reference_mismatch_classes':5,'limits':['Managed allocation, glyph lookup and layout shaping modelled','Compiled production ARM memo executed','Actual pixel equivalence and Vita memory/FPS not established by this test']},indent=2)+'\n')
