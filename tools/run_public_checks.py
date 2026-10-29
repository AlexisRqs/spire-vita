"""Small asset-independent check set. Never contacts a Vita or a network."""
import argparse, pathlib, subprocess, sys, tempfile
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cc',default='cc');a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1]
subprocess.run([sys.executable,root/'tests/test_public_resources.py'],check=True)
subprocess.run([sys.executable,root/'tests/test_build_inputs.py'],check=True)
with tempfile.TemporaryDirectory(prefix='spire-public-checks-') as temp:
    for name,sources in {'utf8':['android_utf8.c'],'frame_input':['frame_input.c','android_input.c'],'files':['android_files.c','android_log.c'], 'text_submit':[], 'text_memo_config':[], 'gameplay_profile':[], 'grid_batch_contract':['grid_batch_contract.c'], 'grid_commands':['grid_commands.c'], 'audio_mix':['audio_mix.c'], 'audio_wave':['audio_wave.c','audio_ima.c'], 'audio_ima':['audio_ima.c']}.items():
        binary=pathlib.Path(temp)/name
        command=[a.cc,'-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-g','-pthread','-I'+str(root/'src')]
        command += [str(root/'src'/s) for s in sources]+[str(root/'tests'/('test_'+name+'.c')),'-lm','-o',str(binary)]
        subprocess.run(command,check=True);subprocess.run([binary],check=True)
        print('PASS',name,flush=True)
print('Public checks passed; this does not validate the whole port or GPU rendering.')
