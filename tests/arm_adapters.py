from test_inputs import ROOT, engine_path, elf_path, output
"""Build the production adapters as ARM code, leaving only OS imports modelled."""
import os, pathlib, subprocess
from elftools.elf.elffile import ELFFile
ROOT=pathlib.Path(__file__).resolve().parents[1]
IMPORTS=('malloc','calloc','free','memset','memcpy','strlen','strcmp','pthread_mutex_lock','pthread_mutex_unlock',
         '__errno','pthread_once','pthread_key_create','pthread_getspecific','pthread_setspecific','abort')
def build(directory):
    sdk=pathlib.Path(os.environ['VITASDK'])
    cc=sdk/'bin/arm-vita-eabi-gcc'
    directory=pathlib.Path(directory)
    asm=directory/'imports.S'
    asm.write_text('.syntax unified\n.arm\n.text\n'+''.join(f'.global {n}\n.type {n},%function\n{n}:\n bx lr\n.size {n},.-{n}\n' for n in IMPORTS)+'.section .note.GNU-stack,"",%progbits\n')
    out=directory/'adapters.elf'
    subprocess.run([str(cc),'-O2','-g','-Wall','-Wextra','-Werror','-marm','-mfpu=vfpv3-d16','-mfloat-abi=hard',
        '-fno-builtin','-nostdlib','-I',str(ROOT/'src'),
        *[str(ROOT/'src'/n) for n in ('android_jump.S','android_jump_mask.c','bionic_modules.c','gc_probe.c','bionic_cxa.c','android_dlfcn.c','android_utf8.c','android_input.c')],str(asm),
        '-Wl,-Ttext=0x20000000,-e,android_setjmp','-o',str(out)],check=True)
    return out

def load(uc,path):
    with open(path,'rb') as f:
        elf=ELFFile(f)
        segments=[s for s in elf.iter_segments() if s['p_type']=='PT_LOAD']
        lo=min(s['p_vaddr']&~4095 for s in segments)
        hi=max((s['p_vaddr']+s['p_memsz']+4095)&~4095 for s in segments)
        uc.mem_map(lo,hi-lo)
        for s in segments:uc.mem_write(s['p_vaddr'],s.data())
        return {s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name and s['st_shndx']!='SHN_UNDEF'}
