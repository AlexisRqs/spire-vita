"""Prepare local game resources; never downloads, uploads or modifies source assets."""
import argparse, hashlib, json, shutil, subprocess, sys
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    group=p.add_mutually_exclusive_group(required=True)
    group.add_argument('--apk',type=Path); group.add_argument('--assets',type=Path)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--cc',default='cc')
    a=p.parse_args();root=Path(__file__).resolve().parents[1];out=a.output.resolve()
    source=(a.apk or a.assets).resolve()
    if not source.exists():raise SystemExit('Input does not exist')
    if any((parent/'.git').exists() for parent in (source,*source.parents)):
        raise SystemExit('Private input must be outside every Git checkout')
    if out.exists():raise SystemExit('Use a new output directory')
    if any((parent/'.git').exists() for parent in (out,*out.parents)) or out.is_relative_to(root) or (a.assets and out.is_relative_to(source)):
        raise SystemExit('Keep private output outside source and asset directories')
    out.mkdir(parents=True)
    def run(args):subprocess.run(list(map(str,args)),check=True)
    if a.apk:
        run([sys.executable,root/'tools/extract_engine.py',source,out/'libSpire_ANDROID.so'])
        assets=out/'assets'
        run([sys.executable,root/'tools/prepare_assets.py',source,assets,'--report',out/'extraction.json'])
    else:assets=source
    lib=out/'libcache.so'
    run([a.cc,'-shared','-fPIC','-O2','-pthread','-I'+str(root/'src'),'-I'+str(root/'vendor/stb'),root/'src/texture_cache.c',root/'vendor/stb/stb_dxt.c',root/'tools/cache_compress.c','-o',lib])
    run([sys.executable,root/'tools/prepare_texture_cache.py',assets,out/'textures','--library',lib])
    run([sys.executable,root/'tools/prepare_path_cache.py',assets,out/'textures',out/'path-manifest.json','--hash-library',lib])
    run([sys.executable,root/'tools/build_direct_cache_index.py','--manifest',out/'path-manifest.json','--assets',assets,'--pack',out/'textures/data.bin','--output',out/'direct-index.bin','--header',out/'direct_cache_identity.h','--hash-library',lib])
    run([sys.executable,root/'tools/check_build_index.py',out/'direct-index.bin',out/'direct_cache_identity.h'])
    run([sys.executable,root/'tools/build_loading_pack.py',out/'direct-index.bin',out/'textures/data.bin',out/'loading-pack.bin'])
    run([sys.executable,root/'tools/build_launcher_splashes.py',assets,out/'launcher-splashes.bin'])
    run([sys.executable,root/'tools/build_livearea.py',assets,out/'sce_sys',out/'livearea-preview.png'])
    shutil.copy2(root/'assets/sce_sys/livearea/contents/template.xml',out/'sce_sys/livearea/contents/template.xml')
    names=['direct-index.bin','direct_cache_identity.h','textures/data.bin','textures/index.bin',
           'loading-pack.bin','launcher-splashes.bin','sce_sys/icon0.png',
           'sce_sys/livearea/contents/bg0.png','sce_sys/livearea/contents/startup.png',
           'sce_sys/livearea/contents/template.xml']
    files={name:hashlib.sha256((out/name).read_bytes()).hexdigest() for name in names}
    (out/'completed.json').write_text(json.dumps({'completed':True,'assets':str(assets),'files':files,
        'limits':'Private game-derived data; no console transfer; do not publish this workspace'},indent=2)+'\n')
    run([sys.executable,root/'tools/check_build_inputs.py',out])

if __name__=='__main__':main()
