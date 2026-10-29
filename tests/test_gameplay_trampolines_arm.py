from test_inputs import ROOT, engine_path, elf_path, output
"""Compare compiled profiling trampolines to exact APK entry instructions."""
from pathlib import Path
import re,struct,random
from elftools.elf.elffile import ELFFile
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM
from unicorn.arm_const import *
from arm_adapters import load
u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9)
u.reg_write(UC_ARM_REG_C1_C0_2,0xf00000);u.reg_write(UC_ARM_REG_FPEXC,0x40000000)
u.mem_map(0x10000000,0x3100000);u.mem_map(0x30000000,0x10000)
with engine_path().open('rb') as f:
 e=ELFFile(f)
 for seg in e.iter_segments():
  if seg['p_type']=='PT_LOAD':u.mem_write(0x10000000+seg['p_vaddr'],seg.data())
app=load(u,elf_path())
src=(ROOT/'src/gameplay_profile.c').read_text();targets=re.findall(r'\{"(_ZN[^"]+)","[^"]+",(0x[0-9a-f]+),\d+,(\d+),\{([^}]+)\},\(void\*\)hook_(\d+),[01]\}',src)
assert len(targets)==33
regs=[UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3,UC_ARM_REG_R4,UC_ARM_REG_R5,UC_ARM_REG_R6,UC_ARM_REG_R7,UC_ARM_REG_R8,UC_ARM_REG_R9,UC_ARM_REG_R10,UC_ARM_REG_R11,UC_ARM_REG_SP,UC_ARM_REG_LR]+[UC_ARM_REG_D0+i for i in range(16)]
rng=random.Random(42)
for name,value,length,raw,index in targets:
 at=0x10000000+(int(value,16)&~1);length=int(length);i=int(index)
 assert bytes(u.mem_read(at,length))==bytes(int(x,16) for x in raw.split(','))
 resume=at+length;u.mem_write(app['gameplay_resumes']+4*i,struct.pack('<I',resume|1))
 for case in range(16):
  initial=[rng.getrandbits(32) for r in regs];initial[12]=0x30008000
  states=[]
  for entry in (at|1,app[f'gameplay_original_{i}']):
   u.mem_write(0x30007000,b'\0'*0x1000)
   for r,v in zip(regs,initial):u.reg_write(r,v)
   u.emu_start(entry,resume,count=30)
   assert u.reg_read(UC_ARM_REG_PC)==resume
   states.append(([u.reg_read(r) for r in regs],bytes(u.mem_read(0x30007000,0x1000))))
  assert states[0]==states[1],(i,case)
print('PASS 528 exact APK / compiled trampoline register and stack comparisons (ip is ABI scratch)')
