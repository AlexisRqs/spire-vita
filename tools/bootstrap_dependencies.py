"""Build pinned dependencies into an explicitly supplied, dedicated VitaSDK."""
import argparse, json, os, re, shutil, subprocess
from pathlib import Path

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--sdk', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--source-cache', type=Path, help='Optional local git repositories; commits are still verified')
    p.add_argument('--jobs', type=int, default=4)
    a = p.parse_args(); sdk = a.sdk.resolve(); work = a.work.resolve()
    if work.exists(): raise SystemExit('Use a new work directory')
    if not (sdk/'bin/arm-vita-eabi-gcc').is_file(): raise SystemExit('Invalid SDK')
    work.mkdir(parents=True)
    root = Path(__file__).resolve().parents[1]
    env = dict(os.environ, VITASDK=str(sdk), PATH=str(sdk/'bin')+os.pathsep+os.environ['PATH'])
    def run(args, cwd=work): subprocess.run(list(map(str,args)),cwd=cwd,env=env,check=True)
    dependencies=json.loads((root/'DEPENDENCIES.json').read_text())['dependencies']
    for dep in dependencies:
        if dep['name']=='kubridge': continue # import assembly already in source
        origin = a.source_cache/dep['name'] if a.source_cache else dep['upstream']
        dest=work/dep['name']
        run(['git','clone','--no-hardlinks','--no-checkout',origin,dest])
        run(['git','checkout','--detach',dep['commit']],dest)
        actual=subprocess.check_output(['git','rev-parse','HEAD'],cwd=dest,text=True).strip()
        if actual!=dep['commit']:raise SystemExit('Commit mismatch')
    # Generate import NIDs using the upstream export description, without building a kernel plugin.
    tai=work/'taiHEN'; exports=(tai/'exports.yml').read_text()
    symbols=re.findall(r'^\s+- ([A-Za-z_][A-Za-z0-9_]*)\s*$',exports,re.M)
    if not symbols:raise SystemExit('No exported symbols found')
    dummy=work/'taihen-exports.c'
    dummy.write_text('\n'.join('void '+s+'(void){}' for s in dict.fromkeys(symbols))+'\n')
    run(['arm-vita-eabi-gcc','-nostdlib','-Wl,-e,0',dummy,'-o','taihen-exports.elf'])
    run(['vita-elf-export','kernel','taihen-exports.elf',tai/'exports.yml','taihen-imports.yml'])
    run(['vita-libs-gen','taihen-imports.yml','taihen-stubs'])
    run(['make','-j'+str(a.jobs)],work/'taihen-stubs')
    shutil.copy2(work/'taihen-stubs/libtaihen_stub.a',sdk/'arm-vita-eabi/lib')
    shutil.copy2(tai/'taihen.h',sdk/'arm-vita-eabi/include')
    run(['cmake','-S',work/'SceShaccCgExt','-B',work/'shacc-build','-DCMAKE_INSTALL_PREFIX='+str(sdk/'arm-vita-eabi'),'-DCMAKE_C_FLAGS=-std=gnu11'])
    run(['cmake','--build',work/'shacc-build','-j',a.jobs])
    run(['cmake','--install',work/'shacc-build'])
    for name in ('math-neon','vitaShaRK'):
        run(['make','-j'+str(a.jobs)],work/name)
        run(['make','install'],work/name)
    run(['git','apply',root/'vendor/graphics-licenses/vitaGL-local.patch'],work/'vitaGL')
    run(['make','-j'+str(a.jobs),'NO_SPLASHSCREEN=1'],work/'vitaGL')
    run(['make','install','NO_SPLASHSCREEN=1'],work/'vitaGL')
    (work/'completed.json').write_text(json.dumps({'dependencies':dependencies,'flags':{'vitaGL':['NO_SPLASHSCREEN=1'],'ABI':'hard-float'}},indent=2)+'\n')

if __name__=='__main__': main()
