"""Private derived DXT cache; originals untouched. Build on host, never Vita."""
import argparse,ctypes as C,struct,subprocess,json,hashlib
from pathlib import Path
from PIL import Image
import texture2ddecoder as decoder
p=argparse.ArgumentParser();p.add_argument('assets',type=Path);p.add_argument('output',type=Path);p.add_argument('--library',type=Path,required=True);a=p.parse_args()
if a.output.exists() and any(a.output.iterdir()):raise SystemExit('Output directory must be new or empty')
a.output.mkdir(parents=True,exist_ok=True)
lib=C.CDLL(str(a.library.resolve()));lib.texture_cache_hash.argtypes=[C.c_void_p,C.c_size_t];lib.texture_cache_hash.restype=C.c_uint64;lib.stb_compress_dxt_block.argtypes=[C.c_void_p,C.c_void_p,C.c_int,C.c_int]
# C helper performs block iteration instead of millions of Python calls.
lib.cache_compress.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_int,C.c_void_p]
records=[]
for path in sorted(a.assets.rglob('*')):
 if not path.is_file():continue
 b=path.read_bytes()
 if b[:12]!=b'\xabKTX 11\xbb\r\n\x1a\n':continue
 h=struct.unpack_from('<13I',b,12);fmt=h[4];w,ht=h[6:8]
 if fmt not in (0x9274,0x9276,0x9278):continue
 assert h[11]==1 and not h[8] and not h[9] and h[10]==1,path
 off=64+h[12];n=struct.unpack_from('<I',b,off)[0];data=b[off+4:off+4+n];assert n==((w+3)//4)*((ht+3)//4)*(16 if fmt==0x9278 else 8)
 key=lib.texture_cache_hash(data,n);name=f'{fmt:x}-{w}x{ht}-{key:016x}.bin';dest=a.output/name
 dw,dh=((w+1)//2,(ht+1)//2) if max(w,ht)>512 else (w,ht)
 if not dest.exists():
  decode={0x9274:decoder.decode_etc2,0x9276:decoder.decode_etc2a1,0x9278:decoder.decode_etc2a8}[fmt]
  im=Image.frombytes('RGBA',(w,ht),decode(data,w,ht),'raw','BGRA')
  if (dw,dh)!=(w,ht):im=im.resize((dw,dh),Image.Resampling.LANCZOS)
  size=((dw+3)//4)*((dh+3)//4)*(8 if fmt==0x9274 else 16);out=C.create_string_buffer(size);lib.cache_compress(im.tobytes(),dw,dh,fmt!=0x9274,out)
  header=struct.pack('<8IQ',0x33334354,fmt,w,ht,dw,dh,size,n,lib.texture_cache_hash(out,size));dest.write_bytes(header+out.raw)
 records.append({'source':str(path.relative_to(a.assets)),'file':name,'original':[w,ht],'physical':[dw,dh],'bytes':dest.stat().st_size,'sha256':hashlib.sha256(dest.read_bytes()).hexdigest()})
(a.output/'manifest.json').write_text(json.dumps(records,indent=2)+'\n');print('Converted',len(records),'textures; unique bytes',sum(p.stat().st_size for p in a.output.glob('*.bin')))

entries=[];offset=0
with (a.output/'data.bin').open('wb') as out:
 for p in sorted(a.output.glob('*.bin')):
  if p.name in ('data.bin','index.bin'):continue
  fmt,wh,key=p.stem.split('-');w,h=map(int,wh.split('x'));data=p.read_bytes()
  entries.append((int(key,16),int(fmt,16),w,h,offset,len(data),0));out.write(data);offset+=len(data)
entries.sort();(a.output/'index.bin').write_bytes(struct.pack('<4I',0x33334954,len(entries),offset,0)+b''.join(struct.pack('<Q6I',*e) for e in entries))
print('Packed',len(entries),'unique textures',offset,'bytes')
