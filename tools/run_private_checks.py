"""Optional exact-engine/compiled-ARM checks. No console or personal save access."""
import argparse,os,subprocess,sys
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for name in ['engine','elf','output','resources']:p.add_argument('--'+name,type=Path,required=True)
a=p.parse_args();root=Path(__file__).resolve().parents[1]
out=a.output.resolve()
if out.exists() or any((q/'.git').exists() for q in (out,*out.parents)):
    p.error('Use a new output directory outside every Git checkout')
out.mkdir(parents=True)
env=dict(os.environ,SPIRE_TEST_ENGINE=str(a.engine.resolve()),SPIRE_TEST_ELF=str(a.elf.resolve()),SPIRE_TEST_OUTPUT=str(out))
for name in ['test_buffer_pointer_config.py','test_buffer_pointer_arm.py','test_gameplay_trampolines_arm.py',
             'test_text_fast_arm.py','test_text_memo_arm.py','test_text_memo_variants_arm.py','test_text_submit_arm.py']:
    with (out/(name+'.log')).open('w') as f:
        subprocess.run([sys.executable,root/'tests'/name],env=env,cwd=out,stdout=f,stderr=subprocess.STDOUT,check=True)
    print('PASS',name,flush=True)
res=a.resources.resolve()
for name,args in [('direct_cache',[res/'direct-index.bin',res/'textures/data.bin',res/'assets',out/'receipt-cache']),
                  ('loading_pack',[res/'direct-index.bin',res/'textures/data.bin',res/'assets',out/'receipt-loading',res/'loading-pack.bin',out/'loading-scratch'])]:
    binary=out/name
    subprocess.run(['cc','-fsanitize=address,undefined','-g','-pthread','-I'+str(root/'src'),'-I'+str(res),root/'tests'/('test_'+name+'.c'),'-o',binary],check=True)
    subprocess.run([str(binary),*map(str,args)],check=True,cwd=out)
    print('PASS',name,flush=True)
(out/'PASSED.txt').write_text('Seven exact-engine ARM/configuration harnesses and two cache/data suites passed.\nGPU/OS services are modelled; hardware acceptance remains pending.\n')
