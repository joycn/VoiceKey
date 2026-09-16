#!/usr/bin/env python3
"""Validate built binary partitions, image bounds and actual target settings."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

def require(condition, message):
    if not condition:
        raise SystemExit('Validation failed: ' + message)

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--config-only', action='store_true')
options = parser.parse_args()
root = Path(__file__).resolve().parents[1] / 'firmware'
build = root / 'build'
config = set((root / 'sdkconfig').read_text().splitlines())
for setting in ('CONFIG_IDF_TARGET="esp32s3"', 'CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y', 'CONFIG_SPIRAM_MODE_OCT=y', 'CONFIG_ESP_CONSOLE_NONE=y'):
    require(setting in config, setting)
require('CONFIG_ESP_CONSOLE_UART_DEFAULT=y' not in config, 'UART0 console is unsupported')
if options.config_only:
    print('PASS: cached configuration is XIAO8MB/Octal/noUART')
    raise SystemExit(0)
require('led_strip' not in (root/'dependencies.lock').read_text(), 'obsolete led_strip dependency')
data = (build/'partition_table/partition-table.bin').read_bytes()
require(len(data) == 0xc00, 'generated partition table must be 3072 bytes')
parts = {}
checksum_seen = False
for offset in range(0, len(data), 32):
    entry = data[offset:offset+32]
    if entry[:2] == b'\xeb\xeb':
        require(not checksum_seen and entry[:16] == b'\xeb\xeb' + b'\xff'*14, 'malformed partition MD5 record')
        require(entry[16:] == hashlib.md5(data[:offset]).digest(), 'partition table MD5 mismatch')
        require(data[offset+32:] == b'\xff'*(len(data)-offset-32), 'unexpected records after partition MD5')
        checksum_seen = True
        break
    magic, typ, subtype, start, size, label, flags = struct.unpack('<HBBII16sI', entry)
    require(magic == 0x50aa, 'invalid/missing partition record or checksum')
    require(b'\0' in label, 'unterminated partition label')
    name_bytes, padding = label.split(b'\0', 1)
    require(padding == b'\0'*len(padding), 'nonzero partition label padding')
    try:
        name = name_bytes.decode('ascii')
    except UnicodeDecodeError:
        raise SystemExit('Validation failed: non-ASCII partition label')
    require(name not in parts, 'duplicate partition ' + name)
    require(start + size <= 8*1024*1024 and start >= 0x9000 and size > 0, 'partition bounds: ' + name)
    parts[name] = (typ, subtype, start, size, flags)
require(checksum_seen, 'partition table checksum absent')
require(parts == {'nvs': (1,2,0x9000,0x6000,0), 'phy_init': (1,1,0xf000,0x1000,0),
                  'factory': (0,0,0x10000,4*1024*1024,0), 'model': (1,0x82,0x410000,3*1024*1024,0)},
        'unexpected partition names/types/subtypes/offsets/sizes/flags')
ranges = sorted((p[2],p[2]+p[3]) for p in parts.values())
require(all(a[1] <= b[0] for a,b in zip(ranges,ranges[1:])), 'overlapping partitions')
args = json.loads((build/'flasher_args.json').read_text())
files = args['flash_files']
expected = {0: ('bootloader/bootloader.bin',0x8000), 0x8000: ('partition_table/partition-table.bin',0x1000),
            0x10000: ('voicekey.bin',4*1024*1024), 0x410000: ('srmodels/srmodels.bin',3*1024*1024)}
require(len(files) == len(expected) and {int(k,16) for k in files} == set(expected), 'unexpected flash offsets')
for address, filename in files.items():
    start = int(address,16)
    require(filename == expected[start][0], 'unexpected image path')
    image = (build/filename).read_bytes()
    require(0 < len(image) <= expected[start][1], 'empty/oversized image: ' + filename)
    if start in (0, 0x10000):
        require(len(image) >= 24 and image[0] == 0xe9 and image[3] >> 4 == 3, 'image flash-size header must be 8MB')
    print(f'{filename} | 0x{start:x} | {len(image)} | {hashlib.sha256(image).hexdigest()}')
print('PASS: 8 MB headers/configuration, partition MD5/integrity/types/flags/bounds, NVS/PHY, 4 MB app, 3 MB model, no OTA or led_strip')
