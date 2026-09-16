#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import struct
spec=importlib.util.spec_from_file_location('diagnose',Path(__file__).resolve().parents[1]/'scripts/diagnose.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
b=bytearray(96);b[:8]=bytes([2,1,0,8,5,1,1,0x60]);b[56:64]=bytes.fromhex('1234567890abcdef')
struct.pack_into('<8I',b,64,0,0,0x105,8388608,123456,100000,7,2)
for raw in (b,bytes([0])+b):
    report=module.decode(raw)
    assert report['schema']==2 and report['wake_init_error']==0x105 and not report['wake_ready']
    assert report['initialization_complete'] and report['transport_ready']
    assert report['psram_bytes']==8388608 and report['free_heap_bytes']==123456 and report['minimum_free_heap_bytes']==100000
    assert report['application_elf_sha256_prefix']=='1234567890abcdef'
    assert report['capture_fault_generation']==7 and report['transport_faults']==2
struct.pack_into('<I',b,72,0x80000000);assert module.decode(b)['wake_init_error'] is None
struct.pack_into('<I',b,72,0xffffffff);assert module.decode(b)['wake_init_error'] == -1
for bad in (b[:16],b[:64],bytes([1])+b[1:]):
    try:module.decode(bad)
    except ValueError:continue
    raise AssertionError('accepted short or obsolete diagnostic schema')
print('PASS: schema2 diagnostics96/97bytes, init error/pending/readiness, PSRAM/heap, identity and malformed report rejection')
