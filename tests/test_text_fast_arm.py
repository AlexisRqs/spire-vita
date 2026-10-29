from test_inputs import ROOT, engine_path, elf_path, output
"""Compare actual APK FontHelper with compiled fast path using modelled font/GPU methods."""
from pathlib import Path
import struct,math,re,json,random
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM,UC_HOOK_CODE
from unicorn.arm_const import *
from arm_adapters import load
u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9)
u.reg_write(UC_ARM_REG_C1_C0_2,0x00f00000);u.reg_write(UC_ARM_REG_FPEXC,0x40000000)
BASE=0x10000000;HEAP=0x40000000;STACK=0x30000000;STUB=0x32000000;STOP=STUB+0xff00
for a,n in ((BASE,0x3100000),(HEAP,0x100000),(STACK,0x10000),(STUB,0x10000)):u.mem_map(a,n)
word=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def pf(a,v):u.mem_write(a,struct.pack('<f',v))
def gf(a):return struct.unpack('<f',u.mem_read(a,4))[0]
with engine_path().open('rb') as f:
 e=ELFFile(f);tables={};syms={s.name:s for s in e.get_section_by_name('.dynsym').iter_symbols()}
 for seg in e.iter_segments():
  if seg['p_type']=='PT_LOAD':u.mem_write(BASE+seg['p_vaddr'],seg.data())
 for sec in e.iter_sections():
  if not isinstance(sec,RelocationSection):continue
  tab=tables.setdefault(sec['sh_link'],list(e.get_section(sec['sh_link']).iter_symbols()))
  for r in sec.iter_relocations():
   a=BASE+r['r_offset'];t=r['r_info_type'];s=tab[r['r_info_sym']];v=s['st_value'];old=word(a)
   if t==23:out=BASE+old
   elif t==17:out=1
   elif t==18:out=v+old
   elif t in (2,21,22):
    resolved=0 if s['st_shndx']=='SHN_UNDEF' else (v if s['st_shndx']=='SHN_ABS' else BASE+v)
    out=resolved if t==22 else (((resolved&~1)+old)|(resolved&1) if s['st_info']['type']=='STT_FUNC' else resolved+old)
   elif t==0:continue
   else:raise AssertionError(t)
   put(a,out)
app=load(u,elf_path())
src=(ROOT/'src/text_fast.c').read_text();names=re.findall(r'"([^"]+)"',src.split('const char *names[]={')[1].split('};')[0]);addresses=[BASE+syms[n]['st_value'] for n in names]
assert bytes(u.mem_read(addresses[0]&~1,8))==bytes.fromhex('2de9f0472ded0a8b')
for name,addr in zip(('layout_slot','restore_slot','matrix_slot','vector_slot','text_initialized','batch_setter','round_float','round_double'),addresses[1:]):put(app[name],addr)
for name,addr in zip(('font_draw_expected','cache_add_expected','add_prepared'),addresses[9:]):put(app[name],addr)
put(app['text_original_resume'],(addresses[0]&~1)+8|1)
u.mem_write(addresses[5],b'\1')
for n,s in syms.items():
 if 'ClassCtor' in n and 'MathUtils' in n and 'initalized' in n:u.mem_write(BASE+s['st_value'],b'\1')
heap=HEAP;callbacks={};serial=0
def alloc(n=1024):
 global heap
 p=heap;heap+=(n+15)&~15;return p
def stub(fn,address=None):
 global serial
 a=(address&~1) if address else STUB+serial*4;serial+=1;callbacks[a]=fn
 if not address:u.mem_write(a,b'\x70\x47')
 return a|1
def object():
 p,t,vt=alloc(),alloc(),alloc();put(p,t);put(t+4,vt);return p,vt
def method(vt,offset,fn,address=None):put(vt+offset,stub(fn,address));u.mem_write(vt+offset+4,b'\0')
R=[UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3]
def reg(i):return u.reg_read(R[i])
def arg(i):return reg(i) if i<4 else word(u.reg_read(UC_ARM_REG_SP)+4*(i-4))
def fl(i):return struct.unpack('<f',struct.pack('<I',arg(i)))[0]
def result(v=0):u.reg_write(R[0],v)
identity=[1.0 if i%5==0 else 0.0 for i in range(16)]
def matrix():
 p,vt=object();a=alloc();data=alloc();shape=alloc();put(p+8,a);put(a+8,data);put(a+20,shape);put(shape,16)
 u.mem_write(data,struct.pack('<16f',*identity));return p,vt,data
batch,bvt=object();font,fvt=object();layout,lvt=object();vec,vvt=object();scratch,mvt,sv=matrix();restore,_,rv=matrix();current,_,cv=matrix();data=alloc();color=alloc();text=alloc()
put(batch+36,current);put(addresses[1],layout);put(addresses[2],restore);put(addresses[3],scratch);put(addresses[4],vec)
cache,cvt=object();put(font+16,cache)
counts={};drawn=[];width,height=33.5,14.75

def hit(n):counts[n]=counts.get(n,0)+1

def end():hit('end');u.mem_write(batch+32,b'\0');result()
def begin():hit('begin');u.mem_write(batch+32,b'\1');result()
def transform():hit('transform');u.mem_write(cv,bytes(u.mem_read(word(word(arg(1)+8)+8),64)));result()
def rotation():
 hit('rotation');a=math.radians(fl(4));v=identity[:];v[0]=v[5]=f32(math.cos(a));v[1]=f32(math.sin(a));v[4]=-v[1];u.mem_write(sv,struct.pack('<16f',*v));result(scratch)
def rotate():
 hit('rotate');a=math.radians(fl(1));x,y=gf(vec+8),gf(vec+12);pf(vec+8,f32(x*math.cos(a)-y*math.sin(a)));pf(vec+12,f32(x*math.sin(a)+y*math.cos(a)));result(vec)
def translate():
 hit('translate');pf(sv+48,f32(gf(sv+48)+fl(1)));pf(sv+52,f32(gf(sv+52)+fl(2)));result(scratch)
def settext():hit('layout');assert arg(1)==font and arg(2)==text;pf(layout+12,width);pf(layout+16,height);result()
def setcolor():hit('color');assert arg(1)==color;result()
def draw():
 hit('draw');hit('second_layout');assert arg(1)==batch and arg(2)==text
 x,y=fl(3),fl(4)
 if bytes(u.mem_read(font+21,1))==b"\1":x,y=f32(math.floor(x+0.5)),f32(math.floor(y+0.5))
 v=struct.unpack('<16f',u.mem_read(cv,64))
 drawn.append((f32(f32(x*v[0]+y*v[4])+v[12]),f32(f32(x*v[1]+y*v[5])+v[13])));result(layout)
def rfloat():result(math.floor(fl(0)+0.5)&0xffffffff)
def rdouble():
 v=struct.unpack('<d',struct.pack('<II',reg(0),reg(1)))[0];v=float(math.floor(v+0.5) if v>=0 else math.ceil(v-0.5));lo,hi=struct.unpack('<II',struct.pack('<d',v));result(lo);u.reg_write(R[1],hi)
method(bvt,40,end);method(bvt,32,begin);method(bvt,264,transform,addresses[6]);method(fvt,248,lambda:result(data));method(fvt,88,setcolor);method(fvt,40,draw,addresses[9]);method(lvt,32,settext);method(mvt,296,rotation);method(mvt,120,translate);method(vvt,472,rotate)
prepared_xy=(0,0)
def cache_clear():hit('cache_clear');result()
def prepared():
 global prepared_xy
 hit('prepared');assert arg(0)==cache and arg(1)==layout
 prepared_xy=(fl(2),fl(3));result()
def cache_draw():
 hit('draw');assert arg(0)==cache and arg(1)==batch
 x,y=prepared_xy
 if bytes(u.mem_read(font+21,1))==b"\1":x,y=f32(math.floor(x+0.5)),f32(math.floor(y+0.5))
 v=struct.unpack('<16f',u.mem_read(cv,64));drawn.append((f32(f32(x*v[0]+y*v[4])+v[12]),f32(f32(x*v[1]+y*v[5])+v[13])));result()
method(cvt,200,lambda:result(),addresses[10]);method(cvt,152,cache_clear);method(cvt,128,cache_draw);stub(prepared,addresses[11])
stub(rfloat,addresses[7]);stub(rdouble,addresses[8])
def hook(uc,pc,size,unused):
 if pc==STOP:uc.emu_stop();return
 if pc in callbacks:callbacks[pc]();uc.reg_write(UC_ARM_REG_PC,uc.reg_read(UC_ARM_REG_LR))
u.hook_add(UC_HOOK_CODE,hook)
def invoke(entry,vals,scale=0.8,integer=False,transformed=False,active=True,custom=False):
 counts.clear();drawn.clear();u.mem_write(cv,struct.pack('<16f',*identity));u.mem_write(rv,struct.pack('<16f',*identity));u.mem_write(sv,bytes(64));u.mem_write(font+21,bytes([integer]));u.mem_write(batch+32,bytes([active]));pf(data+60,scale)
 put(batch+64,123 if custom else 0)
 if transformed:pf(cv,2)
 raw=[batch,font,text]+[struct.unpack('<I',struct.pack('<f',v))[0] for v in vals[:5]]+[vals[5],color]
 sp=STACK+0x8000;u.reg_write(UC_ARM_REG_SP,sp);u.reg_write(UC_ARM_REG_LR,STOP|1)
 for i,v in enumerate(raw):
  if i<4:u.reg_write(R[i],v)
  else:put(sp+4*(i-4),v)
 u.emu_start(entry,0,count=100000);assert u.reg_read(UC_ARM_REG_PC)==STOP,hex(u.reg_read(UC_ARM_REG_PC))
 return drawn[:],counts.copy(),bytes(u.mem_read(cv,64)),bytes(u.mem_read(sv,64)),bytes(u.mem_read(vec+8,8))
def profile_clock():
 result(0);u.reg_write(R[1],0)
stub(profile_clock,app['scene_clock'])
put(app['text_profile_enabled'],1)
rng=random.Random(41);max_error=0;cases=0
for scale in (1.0,0.8,1.3):
 for rounded in (0,1):
  for _ in range(30):
   vals=[f32(rng.uniform(-1000,1000)),f32(rng.uniform(-500,500)),f32(rng.uniform(-100,100)),f32(rng.uniform(-100,100)),0,rounded]
   old=invoke(addresses[0],vals,scale);new=invoke(app['text_fast'],vals,scale)
   error=max(abs(a-b) for a,b in zip(old[0][0],new[0][0]));max_error=max(max_error,error);assert error<0.00025,(vals,old,new)
   assert old[1]['begin']==old[1]['end']==old[1]['transform']==2
   assert not any(k in new[1] for k in ('begin','end','transform','second_layout'));assert new[1]['prepared']==1 and old[1]['second_layout']==1;assert old[2:]==new[2:];cases+=1
for options in ({'integer':True},{'transformed':True},{'active':False},{'custom':True}):
 vals=[123.5,75.25,3.5,-2.5,0,1];old=invoke(addresses[0],vals,**options);new=invoke(app['text_fast'],vals,**options);assert old==new;cases+=1
vals=[123.5,75.25,3.5,-2.5,15,1];assert invoke(addresses[0],vals)==invoke(app['text_fast'],vals);cases+=1
reasons=[word(app['fallback_reason']+4*i) for i in range(11)]
assert reasons==[0,1,0,0,0,1,1,1,1,0,0],reasons
assert sum(reasons)==word(app['fallback_count'])==5
# Opt-in rotated path retains all original batch operations and scratch state.
put(app['rotation_enabled'],1)
rotation_cases=0
for angle in (-180,-45,-15,-0.01,0.01,12,45,180,360):
 for scale in (1.0,0.8,1.3):
  for rounded in (0,1):
   for _ in range(4):
    vals=[f32(rng.uniform(-1000,1000)),f32(rng.uniform(-500,500)),f32(rng.uniform(-100,100)),f32(rng.uniform(-100,100)),angle,rounded]
    old=invoke(addresses[0],vals,scale);new=invoke(app['text_fast'],vals,scale)
    assert old[0]==new[0],(vals,old,new)
    assert old[2:]==new[2:],(vals,old,new)
    assert all(old[1][k]==new[1][k]==2 for k in ('begin','end','transform'))
    assert new[1]['prepared']==1 and 'second_layout' not in new[1]
    rotation_cases+=1
for options in ({'integer':True},{'transformed':True},{'active':False},{'custom':True}):
 vals=[123.5,75.25,3.5,-2.5,15,1]
 assert invoke(addresses[0],vals,**options)==invoke(app['text_fast'],vals,**options)
 rotation_cases+=1
print('PASS rotated cases',rotation_cases)
# Exercise successful flattened submission integration separately from fallback.
def flat_submit():
 hit('draw');hit('flat');x,y=prepared_xy
 if bytes(u.mem_read(font+21,1))==b"\1":x,y=f32(math.floor(x+0.5)),f32(math.floor(y+0.5))
 m=struct.unpack('<16f',u.mem_read(sv,64))
 drawn.append((f32(f32(x*m[0]+y*m[4])+m[12]),f32(f32(x*m[1]+y*m[5])+m[13])));result(1)
stub(flat_submit,app['text_submit_rotated_page'])
for angle in (-45,-15,0.01,12,45):
 vals=[123.5,75.25,3.5,-2.5,angle,1]
 old=invoke(addresses[0],vals);new=invoke(app['text_fast'],vals)
 assert old[0]==new[0] and old[2:]==new[2:]
 assert new[1]['flat']==1 and not any(k in new[1] for k in ('begin','end','transform'))
put(app['integer_enabled'],1)
integer_cases=0
for angle in (0,-15,12):
 for scale in (0.8,1.0):
  for x in (-10.5,0.25,123.5):
   vals=[x,75.25,3.5,-2.5,angle,1]
   old=invoke(addresses[0],vals,scale,integer=True);new=invoke(app['text_fast'],vals,scale,integer=True)
   assert old[0]==new[0] and old[2:]==new[2:],(vals,old,new)
   assert new[1]['flat']==1
   integer_cases+=1
print('PASS integer local-coordinate integration',integer_cases)
output('text-fast-arm-validation.json').write_text(json.dumps({'passed':True,'cases':cases,'rotated_cases':rotation_cases,'flattened_integration_cases':5,'integer_cases':integer_cases,'max_origin_delta':max_error,'limits':['Font/GPU methods modelled: validates call arguments and batch/scratch state, not complete glyph rasterization','Real APK method and compiled native wrapper executed on ARM emulator','Hardware visual and FPS validation required']},indent=2)+'\n')
print('PASS',cases,'ARM comparisons; max origin error',max_error,'unrotated path avoids batch operations; rotated path preserves them')
