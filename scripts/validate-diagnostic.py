#!/usr/bin/env python3
"""Validate maintenance-only build; never flashes a device."""
import hashlib
import json
from pathlib import Path
root = Path(__file__).resolve().parents[1]
project = root / 'firmware/diagnostic'
build = project / 'build'
def require(ok, message):
    if not ok:
        raise SystemExit('Diagnostic validation failed: ' + message)
config = set((project/'sdkconfig').read_text().splitlines())
for setting in ('CONFIG_IDF_TARGET="esp32s3"', 'CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y',
                'CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y', '# CONFIG_SPIRAM is not set'):
    require(setting in config, setting)
require('CONFIG_ESP_CONSOLE_UART_DEFAULT=y' not in config, 'UART0 prohibited')
# Use exactly the production partition table; preserve NVS/model locations.
require((build/'partition_table/partition-table.bin').read_bytes() ==
        (root/'firmware/build/partition_table/partition-table.bin').read_bytes(),
        'partition table differs from validated production build (build production first)')
files = json.loads((build/'flasher_args.json').read_text())['flash_files']
expected = {0: ('bootloader/bootloader.bin',0x8000),
            0x8000: ('partition_table/partition-table.bin',0x1000),
            0x10000: ('voicekey_diagnostic.bin',4*1024*1024)}
require(len(files)==3 and {int(x,16) for x in files}==set(expected), 'unexpected offsets')
for offset, name in files.items():
    address=int(offset,16)
    require(name==expected[address][0], 'unexpected filename')
    data=(build/name).read_bytes()
    require(0<len(data)<=expected[address][1], 'image capacity')
    if address in (0,0x10000):
        require(len(data)>=24 and data[0]==0xe9 and data[3]>>4==3, '8MB image header')
    print(f'{name} | 0x{address:x} | {len(data)} | {hashlib.sha256(data).hexdigest()}')
print('PASS: maintenance USB Serial/JTAG, internal RAM, unchanged partition layout; no model/NVS writes')
