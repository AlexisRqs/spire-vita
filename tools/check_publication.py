#!/usr/bin/env python3
"""Audit indexed source paths/content and optionally all reachable Git history.

No ROM is read. Heuristics are not a proof of copyright ownership or absence
of every possible secret. Submodules must be audited separately at their pin.
"""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

FORBIDDEN={'.gba','.gb','.gbc','.nds','.sav','.pmb','.vpk','.elf','.velf','.self',
           '.o','.a','.so','.bin','.zip','.7z','.rar','.pickle','.ppm','.wav','.pcm',
           '.png','.jpg','.jpeg','.apk','.obb','.pyc','.log','.map','.dmp'}
GENERATED={'data_const_stubs.c','port_rom_tables.c','data_stubs_autogen.c','generated_sounds_embed.cpp'}
PATTERNS=[rb'gh[pousr]_[A-Za-z0-9]{20,}',rb'github_pat_[A-Za-z0-9_]{20,}',
          rb'-----BEGIN (?:RSA |OPENSSH |EC )?PRIVATE KEY-----',rb'AKIA[A-Z0-9]{16}',
          rb'/' + rb'Users/' + rb'[^/\s]+/']

def inspect(path,data,mode):
    reasons=[];p=Path(path)
    if mode=='120000':reasons.append('symlink')
    if p.suffix.lower() in FORBIDDEN or p.name in GENERATED or re.search(r'\.sav(?:\.|$)',path):reasons.append('private/binary file type')
    if any(part in {'private-assets','local-roms','local-saves','outputs','vitasdk','graphify-out','__pycache__','assets-source'} or part.startswith('.env') for part in p.parts):reasons.append('private directory')
    if b'\0' in data:reasons.append('binary content')
    if len(data)>2_000_000:reasons.append('oversized source')
    if any(re.search(pattern,data) for pattern in PATTERNS):reasons.append('secret or personal path pattern')
    return reasons

def audit(root,history=False):
    def git(*args):return subprocess.check_output(['git',*args],cwd=root)
    entries=[];issues=[];submodules=[]
    for row in git('ls-files','--stage','-z').decode().split('\0'):
        if not row:continue
        metadata,path=row.split('\t',1);mode,oid,stage=metadata.split()
        if stage!='0':issues.append({'path':path,'reason':['unmerged index']});continue
        if mode=='160000':submodules.append({'path':path,'commit':oid});continue
        entries.append((path,oid,mode,'index'))
    if history:
        seen=set()
        for commit in git('rev-list','--all').decode().splitlines():
            for row in git('ls-tree','-r','-z',commit).decode().split('\0'):
                if not row:continue
                meta,path=row.split('\t',1);mode,kind,oid=meta.split()
                if kind!='blob' or (path,oid,mode) in seen:continue
                seen.add((path,oid,mode));entries.append((path,oid,mode,'history'))
    oids=list(dict.fromkeys(x[1] for x in entries));data={}
    raw=subprocess.run(['git','cat-file','--batch'],cwd=root,input=('\n'.join(oids)+'\n').encode(),stdout=subprocess.PIPE,check=True).stdout
    offset=0
    for oid in oids:
        end=raw.index(b'\n',offset);header=raw[offset:end].split();size=int(header[2]);offset=end+1
        data[oid]=raw[offset:offset+size];offset+=size+1
    inventory=[]
    for path,oid,mode,scope in entries:
        content=data[oid];reasons=inspect(path,content,mode)
        if reasons:issues.append({'path':path,'scope':scope,'reason':reasons})
        if scope=='index':inventory.append({'path':path,'sha256':hashlib.sha256(content).hexdigest(),'bytes':len(content)})
    return {'ok':not issues,'history_checked':history,'files':len(inventory),'blobs':len(oids),'issues':issues,'submodules':submodules,'inventory':inventory}

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);parser.add_argument('--history',action='store_true');parser.add_argument('--json',type=Path)
    a=parser.parse_args();result=audit(a.root.resolve(),a.history)
    if a.json:a.json.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='inventory'},indent=2))
    raise SystemExit(0 if result['ok'] else 1)
if __name__=='__main__':main()
