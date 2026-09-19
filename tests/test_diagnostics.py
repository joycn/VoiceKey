#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import struct
spec=importlib.util.spec_from_file_location('diagnose',Path(__file__).resolve().parents[1]/'scripts/diagnose.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
b=bytearray(192);b[:8]=bytes([3,1,0,8,5,1,2,0x60]);b[56:64]=bytes.fromhex('1234567890abcdef')
struct.pack_into('<8I',b,64,0,0,0x105,8388608,123456,100000,7,2)
b[97]=7; b[100:102]=b'\1\1'; b[124:141]=b'fixture INT build'
struct.pack_into('<4If',b,104,48000,20,1,0,1.0)
for raw in (b,bytes([0])+b):
    report=module.decode(raw)
    assert report['schema']==3 and report['wake_init_error']==0x105 and not report['wake_ready']
    assert report['initialization_complete'] and report['transport_ready']
    assert report['psram_bytes']==8388608 and report['free_heap_bytes']==123456 and report['minimum_free_heap_bytes']==100000
    assert report['application_elf_sha256_prefix']=='1234567890abcdef'
    assert report['capture_fault_generation']==7 and report['transport_faults']==2
assert report['capture_clock_qualified'] and report['measured_i2s_rate_hz']==48000
assert report['aec_converged'] and report['reference_gain']==1.0 and not report['aec_bypass']
assert report['xmos_build_message']=='fixture INT build'
b[97]=0
assert module.decode(b)['aec_converged'] is None and module.decode(b)['reference_gain'] is None
struct.pack_into('<I',b,72,0x80000000);assert module.decode(b)['wake_init_error'] is None
struct.pack_into('<I',b,72,0xffffffff);assert module.decode(b)['wake_init_error'] == -1
for bad in (b[:16],b[:64],bytes([1])+b[1:]):
    try:module.decode(bad)
    except ValueError:continue
    raise AssertionError('accepted short or obsolete diagnostic schema')
print('PASS: schema3 diagnostics192/193bytes, init error/pending/readiness, PSRAM/heap, identity and malformed report rejection')

assert module.decode(b)['wake_inferences'] is None
b[174]=2; b[175]=7
struct.pack_into('<IIHHHH',b,176,123,999,7,5,32768,2)
r=module.decode(b)
assert r['wake_epoch']==123 and r['wake_inferences']==999
assert r['wake_detections_mod65536']==7 and r['hid_presses_mod65536']==5
assert r['wake_pcm_peak']==32768 and r['trigger_suppressed_mod65536']==2
assert r['capture_gate_closed'] and r['host_microphone_muted']
b[175]=0; assert module.decode(b)['wake_pcm_peak'] is None
b[174]=1; assert module.decode(b)['wake_inferences'] is None
print('PASS: wake extension distinguishes absent/stale data and reports inference/detection/HID counters')

b[174]=3; b[156]=7; b[157:161]=bytes([1,10,5,1]);struct.pack_into('<I',b,161,1000)
r=module.decode(b)
assert r['led_snapshot_valid'] and r['led_power_enabled'] and r['led_speed_fallback']
assert (r['led_effect'],r['led_brightness'],r['led_speed'],r['led_gamma'])==(1,10,5,1)
b[156]=0;assert module.decode(b)['led_speed'] is None
assert module.decode(b)['wake_inferences']==999

# Shared open must be configured after HID initialization, before opening.
from types import SimpleNamespace
from unittest.mock import patch
calls=[]
class Shared:
    def __call__(self, value): calls.append(('shared',value))
class Device:
    def __init__(self): calls.append('construct')
    def open_path(self,path): calls.append(('open',path))
    def close(self): calls.append('close')
candidate={'interface_number':3,'usage_page':1,'usage':6,'path':b'keyboard'}
hid=SimpleNamespace(enumerate=lambda *_:[candidate],device=Device,__file__='mock')
with patch.object(module.sys,'platform','darwin'), patch('ctypes.CDLL',return_value=SimpleNamespace(hid_darwin_set_open_exclusive=Shared())):
    device=module.open_device(hid,0xcafe,0x4014)
assert calls==['construct',('shared',0),('open',b'keyboard')]
device.close()
calls.clear()
with patch.object(module.sys,'platform','darwin'), patch('ctypes.CDLL',return_value=SimpleNamespace()):
    try: module.open_device(hid,0xcafe,0x4014)
    except AttributeError: pass
    else: raise AssertionError('unsafe exclusive fallback')
assert calls==['construct','close']
hid.enumerate=lambda *_:[candidate,candidate]
try: module.open_device(hid,0xcafe,0x4014)
except RuntimeError: pass
else: raise AssertionError('ambiguous device accepted')
print('PASS: shared open ordering, no exclusive fallback, cleanup and ambiguous device rejection')

b[174]=4;b[165]=1;struct.pack_into('<fI',b,166,0.03125,100)
r=module.decode(b)
assert r['agc_snapshot_valid'] and r['agc_gain']==0.03125 and r['agc_age_ms']==100
b[165]=0;assert module.decode(b)['agc_gain'] is None
b[174]=3;assert module.decode(b)['agc_age_ms'] is None
print('PASS: read-only gain extension and stale/legacy absence')
