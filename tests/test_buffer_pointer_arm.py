from test_inputs import ROOT, engine_path, elf_path, output
"""Execute compiled candidate and exact APK getPointer with modelled GC handles."""
from pathlib import Path
import struct
from elftools.elf.elffile import ELFFile
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM,UC_HOOK_CODE
from unicorn.arm_const import *
from arm_adapters import load
u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9)
u.mem_map(0x10000000,0x3100000);u.mem_map(0x30000000,0x10000)
with engine_path().open('rb') as f:
 e=ELFFile(f)
 for seg in e.iter_segments():
  if seg['p_type']=='PT_LOAD':u.mem_write(0x10000000+seg['p_vaddr'],seg.data())
app=load(u,elf_path())
def put(a,v):u.mem_write(a,struct.pack('<I',v))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def ret(v):u.reg_write(UC_ARM_REG_R0,v);u.reg_write(UC_ARM_REG_PC,u.reg_read(UC_ARM_REG_LR))
B,A,D,T,S=0x30000100,0x30000200,0x30000300,0x30000400,0x30000500
handle={};events=[]
def intercept(uc,a,size,data):
 if a==0x12e04fd0:events.append('allocated');ret(int(get(uc.reg_read(UC_ARM_REG_R0))!=0))
 elif a==0x12e04ff0:
  assert uc.reg_read(UC_ARM_REG_R1)==3
  handle[7]=uc.reg_read(UC_ARM_REG_R0);events.append('pin');ret(7)
 elif a==0x12b66a18:
  h=get(uc.reg_read(UC_ARM_REG_R0));events.append('lookup');ret(get(handle[h]+8))
u.hook_add(UC_HOOK_CODE,intercept)
put(app['buffer_pointer_resume'],0x122ac6e9)
for i in range(5):put(app['bp_types']+4*i,T+i*4);put(T+i*4,0x30001000+i*0x100)
put(app['bp_array_type'],T+20);put(T+20,0x30002000)
put(app['bp_gc_stop'],S)
def init(kind=0,pin=7,active=1,stop=0):
 u.mem_write(B,b'\0'*24);u.mem_write(A,b'\0'*24)
 put(B,get(T+kind*4)+0x38);put(B+16,A);put(B+20,pin);put(A,get(T+20)+0x38);put(A+8,D)
 put(S,stop)
 for n in ['bp_fast','bp_bypass','bp_checks','bp_mismatch']:put(app[n],0)
 put(app['bp_active'],active);handle.clear();handle[7]=A;events.clear()
def run(entry):
 u.reg_write(UC_ARM_REG_R0,B);u.reg_write(UC_ARM_REG_SP,0x30008000);u.reg_write(UC_ARM_REG_LR,0x30009000)
 initial=[0xabc000+i for i in range(8)]
 for i,v in enumerate(initial):u.reg_write(UC_ARM_REG_R4+i,v)
 u.emu_start(entry,0x30009000,count=1000)
 assert u.reg_read(UC_ARM_REG_PC)==0x30009000
 assert [u.reg_read(UC_ARM_REG_R4+i) for i in range(8)]==initial
 assert u.reg_read(UC_ARM_REG_SP)==0x30008000
 return u.reg_read(UC_ARM_REG_R0)
for kind in range(5):
 for pin in (0,7):
  init(kind,pin);expected=run(0x122ac6e1);original_state=get(B+20)
  init(kind,pin);assert run(app['buffer_pointer_hook'])==expected;assert get(B+20)==original_state
  events.clear();assert run(app['buffer_pointer_hook'])==D
  if pin:assert not events # repeated pinned call avoids mutex-backed lookup
  # dispose clears the pin, so the next call must pin again.
  put(B+20,0);events.clear();assert run(app['buffer_pointer_hook'])==D;assert 'pin' in events
  # position/limit have no bearing on returned base address.
  put(B+8,73);put(B+12,99);assert run(app['buffer_pointer_hook'])==D
for case in ['off','gc','unknown','array_unknown','missing_data']:
 init()
 if case=='off':put(app['bp_active'],0)
 if case=='gc':put(S,1)
 if case=='unknown':put(B,0x30003000)
 if case=='array_unknown':put(A,0x30003000)
 if case=='missing_data':put(A+8,0)
 expected=run(0x122ac6e1);events.clear();assert run(app['buffer_pointer_hook'])==expected;assert 'lookup' in events
# Validation detects an ownership mismatch, returns original and disables fast path.
init();put(A+8,D);put(0x30000608,D+64);handle[7]=0x30000600
assert run(app['buffer_pointer_hook'])==D+64
assert get(app['bp_active'])==0 and get(app['bp_mismatch'])==1
print('PASS exact APK/candidate pointers, pin/dispose, guards, validation, ARM registers/stack')
