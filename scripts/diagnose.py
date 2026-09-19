#!/usr/bin/env python3
"""One-shot EP0 HID Feature Report; pip install hidapi. No resident software."""
import argparse
import json
import struct
import sys
import time

def open_device(hid, vid, pid):
    candidates = [d for d in hid.enumerate(vid, pid)
                  if d.get('interface_number', -1) in (-1, 3)
                  and d.get('usage_page') == 1 and d.get('usage') == 6]
    if len(candidates) != 1:
        raise RuntimeError('Expected exactly one VoiceKey keyboard HID device')
    device = hid.device()
    try:
        if sys.platform == 'darwin':
            import ctypes
            # Construct first: hidapi initialization can reset its open policy.
            library = ctypes.CDLL(hid.__file__)
            shared = library.hid_darwin_set_open_exclusive
            shared.argtypes = [ctypes.c_int]
            shared.restype = None
            shared(0)
        device.open_path(candidates[0]['path'])
        return device
    except BaseException:
        device.close()
        raise
def decode(raw):
    b = bytes(raw)
    # hidapi includes the report-ID byte on some backends for unnumbered reports.
    if len(b) == 193 and b[0] == 0:
        b = b[1:]
    if len(b) != 192 or b[0] != 3:
        raise ValueError('Expected diagnostic schema 3, 192 bytes')
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
    errors = ['ok', 'i2c_or_invalid_response', 'version_mismatch', 'not_int_profile',
              'audio_format_mismatch', 'invalid_build_metadata']
    valid = bool(b[97] & 4)
    result.update(profile_error=errors[b[96]] if b[96] < len(errors) else f'unknown_{b[96]}',
                  int_profile_compatible=bool(b[97]&1), capture_clock_qualified=bool(b[97]&2),
                  output_packed=list(b[98:100]), output_upsample=list(b[100:102]),
                  usb_bit_depth=list(b[102:104]), measured_i2s_rate_hz=struct.unpack_from('<I',b,104)[0],
                  aec_snapshot_valid=valid, aec_age_ms=struct.unpack_from('<I',b,108)[0],
                  aec_converged=bool(struct.unpack_from('<I',b,112)[0]) if valid else None,
                  aec_bypass=bool(struct.unpack_from('<I',b,116)[0]) if valid else None,
                  reference_gain=struct.unpack_from('<f',b,120)[0] if valid else None,
                  xmos_build_message=b[124:156 if b[174] in (3,4) else 174].split(b'\0',1)[0].decode('ascii',errors='replace'))
    led_ext=b[174] in (3,4)
    led_valid=led_ext and bool(b[156]&1)
    result.update(led_snapshot_valid=led_valid,
                  led_power_enabled=bool(b[156]&2) if led_ext and result['board_valid'] else None,
                  led_speed_fallback=bool(b[156]&4) if led_ext else None,
                  led_effect=b[157] if led_valid else None,
                  led_brightness=b[158] if led_valid else None,
                  led_speed=b[159] if led_valid else None,
                  led_gamma=b[160] if led_valid else None,
                  led_age_ms=struct.unpack_from('<I',b,161)[0] if led_ext else None)
    agc_valid=b[174]==4 and bool(b[165]&1) and result['board_valid']
    result.update(agc_snapshot_valid=agc_valid,
                  agc_gain=struct.unpack_from('<f',b,166)[0] if agc_valid else None,
                  agc_age_ms=struct.unpack_from('<I',b,170)[0] if b[174]==4 else None)
    ext=b[174] in (2,3,4)
    result.update(wake_diagnostics_available=ext,
                  wake_epoch=struct.unpack_from('<I',b,176)[0] if ext else None,
                  wake_inferences=struct.unpack_from('<I',b,180)[0] if ext else None,
                  wake_detections_mod65536=struct.unpack_from('<H',b,184)[0] if ext else None,
                  hid_presses_mod65536=struct.unpack_from('<H',b,186)[0] if ext else None,
                  wake_pcm_peak=struct.unpack_from('<H',b,188)[0] if ext and b[175]&1 else None,
                  trigger_suppressed_mod65536=struct.unpack_from('<H',b,190)[0] if ext else None,
                  capture_gate_closed=bool(b[175]&2) if ext else None,
                  host_microphone_muted=bool(b[175]&4) if ext else None)
    return result

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--vid', type=lambda x:int(x,0), default=0xcafe)
    p.add_argument('--pid', type=lambda x:int(x,0), default=0x4014)
    p.add_argument('--samples', type=int, default=1, help='Bounded number of reports (multiple reports use JSONL)')
    p.add_argument('--interval', type=float, default=0.2, help='Seconds between reports, minimum 0.1')
    a = p.parse_args()
    if not 1 <= a.samples <= 3000 or not 0.1 <= a.interval <= 60:
        p.error('samples must be 1..3000 and interval 0.1..60 seconds')
    import hid
    device = open_device(hid, a.vid, a.pid)
    try:
        start = time.monotonic()
        for index in range(a.samples):
            if index:
                time.sleep(a.interval)
            report = decode(device.get_feature_report(0,193))
            if a.samples > 1:
                report['elapsed_seconds'] = round(time.monotonic() - start, 3)
            print(json.dumps(report, indent=2 if a.samples == 1 else None), flush=True)
    finally:
        device.close()
if __name__ == '__main__':
    main()
