from test_inputs import ROOT, engine_path, elf_path, output
"""Actual APK FontCache.draw versus native one-page submission; GPU call observed."""
from pathlib import Path
exec(compile((ROOT/'tests/test_text_fast_arm.py').read_text().split('rng=random.Random(41)')[0],'font-fixture','exec'))
import random
names=[
'_ZN3com8badlogic3gdx8graphics3g2d26BitmapFontCache$_draw_VoidEPNS3_15BitmapFontCacheEPNS3_5BatchE',
'_ZN3com8badlogic3gdx8graphics3g2d46BitmapFont$_get_Regions_Array$1_$TextureRegionEPNS3_10BitmapFontE',
'_ZN3com8badlogic3gdx5utils14Array$1$_get_TIPNS1_8graphics3g2d13TextureRegionEEET_PNS2_7Array$1IS8_EEi',
'_ZN3com8badlogic3gdx8graphics3g2d34TextureRegion$_get_Texture_TextureEPNS3_13TextureRegionE',
'_ZN3com8badlogic3gdx8graphics3g2d22SpriteBatch$_draw_VoidEPNS3_11SpriteBatchEPNS2_7TextureEPN6System6ArrayTIfLi1EEEii']
a=[BASE+syms[n]['st_value'] for n in names]
for i,v in enumerate(a):put(app['submit_expected']+i*4,v)
for v in a[:4]:callbacks.pop(v&~1,None)
stop=alloc(4);put(stop,0);put(app['submit_gc_stop'],stop)
# Populate the object header with the actual APK allocator, not a synthetic
# assumption equating the object type header and the cached RuntimeType.
runtime_type=alloc(0xd0);put(runtime_type+0x3c,word(word(batch)+4))
sp=STACK+0x8000;u.reg_write(UC_ARM_REG_SP,sp);u.reg_write(UC_ARM_REG_LR,STOP|1)
for register_id,value in zip(R,[runtime_type,1024,batch]):u.reg_write(register_id,value)
u.emu_start(BASE+syms['_ZN6System8Internal12ClassHelpers5AllocEPNS0_11RuntimeTypeEjPv']['st_value'],0,count=1000)
assert u.reg_read(UC_ARM_REG_PC)==STOP and word(batch)==runtime_type+0x38
assert word(batch)!=runtime_type
type_slot=alloc(4);put(type_slot,runtime_type);put(app['submit_batch_type'],type_slot)
def array(n):
 o=alloc(32);shape=alloc(4);raw=alloc(max(4,n*4));put(o+8,raw);put(o+20,shape);put(shape,n);return o,raw,shape
pages,pa,ps=array(1);indices,ix,ish=array(1);vertices,vs,vsh=array(960);regions,rvt=object();region,tvt=object();items,ip,ips=array(1);texture=alloc()
put(cache+8,font);put(font+12,regions);put(regions+8,items);put(regions+12,1);put(ip,region);put(region+8,texture)
put(cache+44,pages);put(cache+48,indices);put(pa,vertices)
for vt,slot,addr in [(cvt,128,a[0]),(fvt,128,a[1]),(rvt,72,a[2]),(tvt,56,a[3])]:put(vt+slot,addr);u.mem_write(vt+slot+4,b'\0')
drawcalls=[]
def sprite():
 args=[arg(i) for i in range(5)];drawcalls.append((args,bytes(u.mem_read(vs,args[4]*4))));result()
stub(sprite,a[4]);dispatch=alloc(8);put(dispatch,a[4]);put(dispatch+4,0)
stub(lambda:result(dispatch),BASE+0x2b35bc0)
stub(lambda:result(0),BASE+0x2b344f8)
instructions=0
def count(*args):
 global instructions
 instructions+=1
u.hook_add(UC_HOOK_CODE,count)
def run(entry,matrix_arg=0):
 global instructions
 instructions=0;drawcalls.clear();sp=STACK+0x8000;u.reg_write(UC_ARM_REG_SP,sp);u.reg_write(UC_ARM_REG_LR,STOP|1);u.reg_write(R[0],cache);u.reg_write(R[1],batch);u.reg_write(R[2],matrix_arg)
 u.emu_start(entry,0,count=500000);assert u.reg_read(UC_ARM_REG_PC)==STOP,hex(u.reg_read(UC_ARM_REG_PC))
 return drawcalls[:],bytes(u.mem_read(cache,64)),bytes(u.mem_read(font,32)),bytes(u.mem_read(vs,3840)),u.reg_read(R[0]),instructions
rng=random.Random(87);cases=0;cost=[]
for n in [0,20,40,80,160,320,640,960]:
 for _ in range(10):
  put(ix,n);u.mem_write(vs,rng.randbytes(3840));old=run(a[0]);new=run(app['text_submit_single_page']);assert old[:4]==new[:4] and new[4]==1
  if n:assert old[0][0][0]==[batch,texture,vertices,0,n];cost.append((old[5],new[5]))
  cases+=1
# Unsupported inputs leave all objects untouched and decline before drawing.
put(ix,20)
def reject(addr,value):
 old=word(addr);put(addr,value);before=bytes(u.mem_read(HEAP,heap-HEAP));out=run(app['text_submit_single_page']);assert out[4]==0 and not out[0] and bytes(u.mem_read(HEAP,heap-HEAP))==before;put(addr,old)
for addr,value in [(stop,1),(type_slot,0),(type_slot,word(font)),(cvt+128,0),(fvt+128,0),(rvt+72,0),(tvt+56,0),(ps,2),(ish,2),(ix,21),(ix,0xffffffff),(vsh,1),(region+8,0),(regions+12,2),(pages+8,0)]:reject(addr,value)
print('PASS',cases,'bit-identical draw arguments/vertices and unchanged objects; 15 fallbacks')
print('ARM instruction count per positive draw:',cost[0])
output('text-submit-arm-validation.json').write_text(json.dumps({'passed':True,'comparisons':cases,'fallbacks':15,'instruction_count_original':cost[0][0],'instruction_count_candidate':cost[0][1],'runtime_type_header':'Created by actual APK ClassHelpers::Alloc; header = RuntimeType + 0x38','scope':'Original APK FontCache.draw and compiled native helper; interface resolver and final SpriteBatch call modelled, GPU rasterization not executed'},indent=2)+'\n')

# Rotated submission transforms XY only and restores all managed source data.
u.mem_write(batch+32,b'\1');put(batch+64,0)
rotated_cases=0
for angle in (-45,-12,0.01,17,90):
 rad=math.radians(angle);m=identity[:];m[0]=m[5]=f32(math.cos(rad));m[1]=f32(math.sin(rad));m[4]=-m[1];m[12]=123.5;m[13]=-42.25
 u.mem_write(sv,struct.pack('<16f',*m))
 for n in (20,80,960):
  put(ix,n);raw=bytearray(rng.randbytes(3840))
  for i in range(0,n,5):struct.pack_into('<ff',raw,i*4,f32(rng.uniform(-500,500)),f32(rng.uniform(-500,500)))
  u.mem_write(vs,bytes(raw));out=run(app['text_submit_rotated_page'],scratch)
  assert out[4]==1 and out[3]==bytes(raw)
  expected=bytearray(raw[:n*4])
  for i in range(0,n,5):
   x,y=struct.unpack_from('<ff',raw,i*4)
   xx=f32(f32(f32(x*m[0])+f32(y*m[4]))+m[12]);yy=f32(f32(f32(x*m[1])+f32(y*m[5]))+m[13])
   struct.pack_into('<ff',expected,i*4,xx,yy)
  assert out[0][0][1]==bytes(expected),(angle,n)
  rotated_cases+=1
# Invalid matrix declines with no draw and no mutation.
pf(sv+12,1);before=bytes(u.mem_read(vs,3840));out=run(app['text_submit_rotated_page'],scratch);assert out[4]==0 and not out[0] and out[3]==before
print('PASS rotated submission',rotated_cases,'source restored and exact transformed payloads')
# Exact upper bound and rejection immediately above it, plus non-finite XY.
big,vs,vsh=array(5160);put(pa,big);put(ix,5120);u.mem_write(sv,struct.pack('<16f',*identity))
payload=struct.pack('<5160f',*[float(i%17) for i in range(5160)]);u.mem_write(vs,payload)
out=run(app['text_submit_rotated_page'],scratch);assert out[4]==1 and bytes(u.mem_read(vs,len(payload)))==payload
put(ix,5140);out=run(app['text_submit_rotated_page'],scratch);assert out[4]==0 and not out[0] and bytes(u.mem_read(vs,len(payload)))==payload
put(ix,20);pf(vs,float('nan'));before=bytes(u.mem_read(vs,len(payload)));out=run(app['text_submit_rotated_page'],scratch);assert out[4]==0 and not out[0] and bytes(u.mem_read(vs,len(payload)))==before
print('PASS rotated upper bound, oversized input and NaN rejection')

report=output('text-submit-arm-validation.json')
summary=json.loads(report.read_text());summary.update(rotated_payload_comparisons=rotated_cases,source_xy_restored=True,upper_bound_floats=5120,oversize_and_nonfinite_rejected=True)
report.write_text(json.dumps(summary,indent=2)+'\n')
