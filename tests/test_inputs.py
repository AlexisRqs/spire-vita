"""Explicit external private-test inputs, shared by the ARM harnesses."""
import os,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def external(name):
    p=Path(os.environ[name]).resolve()
    if any((q/'.git').exists() for q in (p,*p.parents)) or p.is_relative_to(ROOT):
        raise ValueError('Keep test inputs/output outside Git')
    return p
def engine_path():
    p=external('SPIRE_TEST_ENGINE')
    if hashlib.sha256(p.read_bytes()).hexdigest()!='619e71cd19c9209dff17d943f94a6e14f9f6e899add6f3b33c14f909594f9d18':
        raise ValueError('Unsupported private engine')
    return p
def elf_path():return external('SPIRE_TEST_ELF')
def output(name):
    if Path(name).name!=name:raise ValueError('Expected a report filename')
    p=external('SPIRE_TEST_OUTPUT');p.mkdir(parents=True,exist_ok=True)
    return p/name
