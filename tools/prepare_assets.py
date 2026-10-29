"""Extract the user's APK assets locally. Never downloads or uploads game data."""
import argparse,collections,hashlib,json,pathlib,stat,zipfile
p=argparse.ArgumentParser();p.add_argument('apk',type=pathlib.Path);p.add_argument('destination',type=pathlib.Path);p.add_argument('--report',type=pathlib.Path,required=True);args=p.parse_args()
root=args.destination.resolve()
if root.exists() and any(root.iterdir()):raise SystemExit('Destination must be new or empty')
files=[];seen=set();textures=collections.Counter()
with zipfile.ZipFile(args.apk) as archive:
    entries=[]
    for info in archive.infolist():
        if not info.filename.startswith('assets/') or info.is_dir():continue
        name=info.filename[7:];relative=pathlib.PurePosixPath(name)
        if relative.is_absolute() or any(x in ('','..','.') for x in name.split('/')) or ':' in name or '\\' in name or stat.S_ISLNK(info.external_attr>>16):raise ValueError('Invalid asset path: '+name)
        if name.casefold() in seen:raise ValueError('Duplicate asset path: '+name)
        seen.add(name.casefold());entries.append((info,relative))
    root.mkdir(parents=True,exist_ok=True)
    for info,relative in entries:
        target=root.joinpath(*relative.parts);target.parent.mkdir(parents=True,exist_ok=True)
        h=hashlib.sha256();header=b''
        with archive.open(info) as source,target.open('xb') as out:
            while chunk:=source.read(1048576):
                if not header:header=chunk[:64]
                h.update(chunk);out.write(chunk)
        if header.startswith(b'\xabKTX 11\xbb\r\n\x1a\n') and len(header)>=64:
            fmt=int.from_bytes(header[28:32],'little');textures[hex(fmt)]+=1
        files.append({'path':str(relative),'bytes':info.file_size,'sha256':h.hexdigest()})
report={'apk_sha256':hashlib.file_digest(args.apk.open('rb'),'sha256').hexdigest(),'files':files,'asset_count':len(files),'asset_bytes':sum(x['bytes'] for x in files),'ktx_internal_formats':dict(textures),'destination':str(root),'transferred_to_vita':False}
args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='files'},indent=2))
