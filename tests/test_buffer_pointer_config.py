from test_inputs import ROOT, engine_path, elf_path, output
"""Build/run configuration guards under host ASan/UBSan using the exact APK."""
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess
from elftools.elf.elffile import ELFFile
root=Path(__file__).resolve().parents[1]
with TemporaryDirectory(prefix='buffer-pointer-test-') as tmp:
 p=Path(tmp);b=bytearray(0x3100000)
 with (engine_path()).open('rb') as f:
  for s in ELFFile(f).iter_segments():
   if s['p_type']=='PT_LOAD':b[s['p_vaddr']:s['p_vaddr']+s['p_filesz']]=s.data()
 (p/'image.bin').write_bytes(b)
 subprocess.run(['clang','-fsanitize=address,undefined','-I',str(root/'src'),str(root/'tests/test_buffer_pointer_config.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test'),str(p/'image.bin')],check=True)
