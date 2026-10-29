"""Build an optional, byte-identical startup texture subset; inputs read only."""
import argparse, hashlib, json, struct
from pathlib import Path

def fnv(b):
 h=14695981039346656037
 for c in b: h=((h^c)*1099511628211)&0xffffffffffffffff
 return h

def build(index, pack, output):
 b=index.read_bytes(); magic,version,count,record,identity=struct.unpack_from('<IIIIQ',b)
 assert (magic,version,record)==(0x31435053,1,304)
 assert len(b)==24+count*record and fnv(b[24:])==identity
 selected=[];payload=bytearray()
 with pack.open('rb') as f:
  for i in range(count):
   r=b[24+i*304:24+(i+1)*304];name=r[:256].split(b'\0')[0].decode()
   if not (name.startswith('images/relics/') or name.startswith('images/characters/')) or struct.unpack_from('<I',r,284)[0]==0x9276:continue
   offset,size=struct.unpack_from('<II',r,276);f.seek(offset);data=f.read(size)
   assert len(data)==size and fnv(data)==struct.unpack_from('<Q',r,264)[0]
   payload+=struct.pack('<II',i,size)+data;selected.append({'index':i,'name':name,'bytes':size})
 out=struct.pack('<IIQII',0x314c5053,1,identity,len(selected),24+len(payload))+payload
 assert len(out)<=20*1024*1024
 output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(out)
 report={'entries':len(selected),'bytes':len(out),'sha256':hashlib.sha256(out).hexdigest(),'identity':hex(identity),'selected':selected,'limits':'Subset coverage is not an observed startup trace. No Vita timing claim.'}
 output.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='selected'}))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('index',type=Path);p.add_argument('pack',type=Path);p.add_argument('output',type=Path);a=p.parse_args();build(a.index,a.pack,a.output)
