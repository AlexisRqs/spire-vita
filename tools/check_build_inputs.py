"""Check generated private inputs before configuring a build; no network/writes."""
import hashlib,json,struct,sys
from pathlib import Path
from check_build_index import validate

def check(root):
    root=Path(root).resolve()
    if any((p/'.git').exists() for p in (root,*root.parents)):
        raise ValueError('Private inputs must be outside every Git checkout')
    marker=json.loads((root/'completed.json').read_text())
    if marker.get('completed') is not True:raise ValueError('Preparation incomplete')
    required={'direct-index.bin','direct_cache_identity.h','textures/data.bin','textures/index.bin',
              'loading-pack.bin','launcher-splashes.bin','sce_sys/icon0.png',
              'sce_sys/livearea/contents/bg0.png','sce_sys/livearea/contents/startup.png',
              'sce_sys/livearea/contents/template.xml'}
    if set(marker.get('files',{}))!=required:raise ValueError('Unexpected resource manifest')
    for name in sorted(required):
        p=root/name
        if not p.resolve().is_relative_to(root):raise ValueError('Private resource escapes workspace')
        if hashlib.sha256(p.read_bytes()).hexdigest()!=marker['files'][name]:raise ValueError('Resource mismatch: '+name)
    count=validate(root/'direct-index.bin',root/'direct_cache_identity.h')
    identity=struct.unpack_from('<Q',(root/'direct-index.bin').read_bytes(),16)[0]
    pack=(root/'loading-pack.bin').read_bytes()
    if len(pack)<24:raise ValueError('Truncated loading pack')
    magic,version,index_id,entries,size=struct.unpack_from('<IIQII',pack)
    if (magic,version,index_id,size)!=(0x314c5053,1,identity,len(pack)) or entries>count:
        raise ValueError('Loading pack/index mismatch')
    splash=root/'launcher-splashes.bin'
    with splash.open('rb') as f:
        if f.read(8)!=b'SPLASH01' or splash.stat().st_size!=8+2*640*360*4:raise ValueError('Invalid splash pack')
    for name,size in [('icon0.png',(128,128)),('livearea/contents/bg0.png',(840,500)),('livearea/contents/startup.png',(280,158))]:
        b=(root/'sce_sys'/name).read_bytes()
        if b[:8]!=b'\x89PNG\r\n\x1a\n' or struct.unpack_from('>II',b,16)!=size or b[25]!=3:
            raise ValueError('Invalid indexed shell image: '+name)
    return count
if __name__=='__main__':
    try:print('PASS: complete private inputs,',check(sys.argv[1]),'textures')
    except (ValueError,OSError,KeyError,struct.error) as e:raise SystemExit(str(e))
