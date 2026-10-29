"""Validate original assets and existing cache; build a versioned path manifest.

Offline only: no engine patches, no FTP, no modification of source/cache files.
The result is not a runtime enable flag. Original SHA256 must be validated by
an eventual installer/runtime before bypassing source reads.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import struct

MAGIC=b'\xabKTX 11\xbb\r\n\x1a\n'

def sha(data): return hashlib.sha256(data).hexdigest()
def inside(root,name):
    if not isinstance(name,str):raise ValueError("invalid relative path")
    p=PurePosixPath(name)
    if not name or p.is_absolute() or '\\' in name or ':' in name or any(x in ('','.','..') for x in name.split('/')):
        raise ValueError('invalid relative path')
    candidate=(root/name).resolve()
    if not candidate.is_relative_to(root.resolve()):raise ValueError('path escapes root')
    return candidate

def fnv(data):
    value=14695981039346656037
    for b in data:value=((value^b)*1099511628211)&0xffffffffffffffff
    return value

def prepare(assets,cache,output,hash64=fnv):
    assets,cache,output=map(Path,(assets,cache,output))
    if any(output.resolve().is_relative_to(root.resolve()) for root in (assets,cache)):
        raise ValueError('output must be outside source and cache roots')
    records=json.loads((cache/'manifest.json').read_text())
    index=(cache/'index.bin').read_bytes()
    if len(index)<16:raise ValueError('short pack index')
    magic,count,pack_size,reserved=struct.unpack_from('<4I',index)
    if magic!=0x33334954 or reserved or not 0<count<=4096 or len(index)!=16+32*count:raise ValueError('invalid pack index')
    entries={};previous=None
    for i in range(count):
        key,fmt,w,h,offset,size,flags=struct.unpack_from('<Q6I',index,16+32*i)
        identity=(key,fmt,w,h)
        if flags or (previous is not None and identity<=previous) or size<40 or offset+size>pack_size:raise ValueError('invalid pack entry')
        previous=identity;entries[identity]=(offset,size)
    paths=set();result=[];verified={};source_bytes=0
    with (cache/'data.bin').open('rb') as pack:
        pack.seek(0,2)
        if pack.tell()!=pack_size:raise ValueError('pack size mismatch')
        for record in sorted(records,key=lambda r:r['source']):
            name=record['source']
            if name in paths:raise ValueError('duplicate source path')
            paths.add(name)
            source=inside(assets,name).read_bytes()
            if len(source)<68 or source[:12]!=MAGIC:raise ValueError('invalid KTX')
            hdr=struct.unpack_from('<13I',source,12)
            endian,typ,typsz,glfmt,fmt,base,w,h,depth,arrays,faces,mips,kv=hdr
            if endian!=0x04030201 or typ or typsz!=1 or glfmt or fmt not in (0x9274,0x9276,0x9278) or not 0<w<=4096 or not 0<h<=4096 or depth or arrays or faces!=1 or mips!=1 or kv>len(source)-68:raise ValueError('unsupported KTX layout')
            off=64+kv;n=struct.unpack_from('<I',source,off)[0];payload=source[off+4:]
            if n!=len(payload) or n!=((w+3)//4)*((h+3)//4)*(16 if fmt==0x9278 else 8):raise ValueError('source payload size')
            key=hash64(payload);identity=(key,fmt,w,h)
            expected=f'{fmt:x}-{w}x{h}-{key:016x}.bin'
            if record['file']!=expected or record['original']!=[w,h]:raise ValueError('source/cache identity mismatch')
            if identity not in entries:raise ValueError('cache missing from pack')
            offset,size=entries[identity]
            if identity not in verified:
                pack.seek(offset);cached=pack.read(size)
                loose=inside(cache,record['file']).read_bytes()
                if cached!=loose or len(cached)!=size:raise ValueError('pack differs from loose cache')
                hh=struct.unpack_from('<8IQ',cached)
                dw,dh=((w+1)//2,(h+1)//2) if max(w,h)>512 else (w,h)
                dn=((dw+3)//4)*((dh+3)//4)*(8 if fmt==0x9274 else 16)
                if hh[:8]!=(0x33334354,fmt,w,h,dw,dh,dn,n) or len(cached)!=40+dn or hash64(cached[40:])!=hh[8]:raise ValueError('invalid cached payload')
                verified[identity]=(sha(cached),sha(cached[40:]),dw,dh,dn)
            whole,payload_sha,dw,dh,dn=verified[identity]
            if whole!=record['sha256'] or record['bytes']!=size or record['physical']!=[dw,dh]:raise ValueError('manifest cache mismatch')
            result.append(dict(path=name,source_bytes=len(source),source_sha256=sha(source),source_payload_fnv64=f'{key:016x}',source_format=fmt,logical=[w,h],physical=[dw,dh],gpu_format=0x83f0 if fmt==0x9274 else 0x83f3,pack_offset=offset,entry_bytes=size,payload_bytes=dn,entry_sha256=whole,payload_sha256=payload_sha))
            source_bytes+=len(source)
    if not result:raise ValueError('empty manifest')
    # Stream full-pack digest: never allocate the 250+ MiB pack in RAM.
    with (cache/'data.bin').open('rb') as f:pack_digest=hashlib.file_digest(f,'sha256').hexdigest()
    manifest=dict(schema='spire-path-cache',version=1,engine_assets='android-2.6.1',runtime_enabled=False,pack=dict(bytes=pack_size,sha256=pack_digest,index_sha256=sha(index)),entries=result)
    raw=(json.dumps(manifest,ensure_ascii=False,sort_keys=True,indent=2)+'\n').encode()
    output.parent.mkdir(parents=True,exist_ok=True)
    temp=output.with_name(output.name+'.tmp');temp.write_bytes(raw);temp.replace(output)
    return dict(entries=len(result),unique_entries=len(verified),source_bytes=source_bytes,unique_payload_bytes=sum(v[4] for v in verified.values()),manifest_bytes=len(raw),manifest_sha256=sha(raw),runtime_enabled=False)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('assets',type=Path);p.add_argument('cache',type=Path);p.add_argument('output',type=Path);p.add_argument('--hash-library',type=Path)
    a=p.parse_args();hash64=fnv
    if a.hash_library:
        import ctypes as C
        lib=C.CDLL(str(a.hash_library.resolve()));lib.texture_cache_hash.argtypes=[C.c_void_p,C.c_size_t];lib.texture_cache_hash.restype=C.c_uint64
        hash64=lambda b:lib.texture_cache_hash(b,len(b))
    print(json.dumps(prepare(a.assets,a.cache,a.output,hash64),indent=2))
