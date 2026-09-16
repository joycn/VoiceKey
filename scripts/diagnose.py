#!/usr/bin/env python3
"""One-shot EP0 HID Feature Report; pip install hidapi. No resident software."""
import argparse
import json
import struct

def decode(raw):
    b = bytes(raw)
    # hidapi includes the report-ID byte on some backends for unnumbered reports.
    if len(b) == 97 and b[0] == 0:
        b = b[1:]
    if len(b) != 96 or b[0] != 2:
        raise ValueError('Expected diagnostic schema 2, 96 bytes')
    names = ['i2c_errors', 'i2c_busy', 'cache_age_ms', 'usb_samples', 'wake_samples', 'playback_frames',
             'usb_dropped', 'usb_missing', 'wake_dropped', 'playback_dropped', 'playback_missing', 'i2s_consumed_frames']
    result = dict(zip(names, struct.unpack_from('<12I', b, 8)))
    result.update(schema=b[0], xmos_version='.'.join(map(str,b[1:4])), board_valid=bool(b[4]&1),
                  microphone_muted=bool(b[4]&2), i2s_active=bool(b[4]&4), firmware_release=f'{b[5]}.{b[6]}',
                  runtime_flags=b[7], application_elf_sha256_prefix=b[56:64].hex())
    names = ['board_init_error', 'audio_init_error', 'wake_init_error', 'psram_bytes',
             'free_heap_bytes', 'minimum_free_heap_bytes', 'capture_fault_generation', 'transport_faults']
    result.update(zip(names, struct.unpack_from('<8I', b, 64)))
    result.update(wake_ready=bool(b[7]&16), initialization_complete=bool(b[7]&32), transport_ready=bool(b[7]&64))
    for name in ('board_init_error','audio_init_error','wake_init_error'):
        if result[name] == 0x80000000:
            result[name] = None  # initialization stage not attempted yet
        elif result[name] & 0x80000000:
            result[name] -= 1 << 32  # preserve signed ESP_FAIL (-1)
    return result

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--vid', type=lambda x:int(x,0), default=0xcafe)
    p.add_argument('--pid', type=lambda x:int(x,0), default=0x4014)
    a = p.parse_args()
    import hid
    candidates = [d for d in hid.enumerate(a.vid,a.pid) if d.get('interface_number',-1) in (-1,3)]
    if not candidates:
        raise SystemExit('VoiceKey HID device not found')
    device = hid.device(); device.open_path(candidates[0]['path'])
    try:
        print(json.dumps(decode(device.get_feature_report(0,97)), indent=2))
    finally:
        device.close()
if __name__ == '__main__':
    main()
