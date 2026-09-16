#!/usr/bin/env python3
"""Exercise operational validators with and without Python optimization."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
source = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary); (root/'scripts').mkdir()
    for script in ('validate-images.py','verify-xmos.py'):
        shutil.copyfile(source/'scripts'/script,root/'scripts'/script)
    fw = root/'firmware'; build = fw/'build'
    for name in ('bootloader','partition_table','srmodels'):
        (build/name).mkdir(parents=True)
    (fw/'xmos').mkdir()
    config = 'CONFIG_IDF_TARGET="esp32s3"\nCONFIG_ESPTOOLPY_FLASHSIZE_8MB=y\nCONFIG_SPIRAM_MODE_OCT=y\nCONFIG_ESP_CONSOLE_NONE=y\n'
    (fw/'dependencies.lock').write_text('dependencies: {}\n')
    entries = [(1,2,0x9000,0x6000,'nvs',0), (1,1,0xf000,0x1000,'phy_init',0),
               (0,0,0x10000,0x400000,'factory',0), (1,0x82,0x410000,0x300000,'model',0)]
    def generated_table(records):
        binary = b''.join(struct.pack('<HBBII16sI',0x50aa,t,st,offset,size,name.encode(),flags) for t,st,offset,size,name,flags in records)
        binary += b'\xeb\xeb'+b'\xff'*14+hashlib.md5(binary).digest()
        return binary + b'\xff'*(0xc00-len(binary))
    table = generated_table(entries)
    header = bytes([0xe9,2,0,0x30])+b'\0'*124
    (build/'bootloader/bootloader.bin').write_bytes(header)
    (build/'srmodels/srmodels.bin').write_bytes(b'model')
    files = {'0x0':'bootloader/bootloader.bin','0x8000':'partition_table/partition-table.bin','0x10000':'voicekey.bin','0x410000':'srmodels/srmodels.bin'}
    (build/'flasher_args.json').write_text(json.dumps({'flash_files':files}))
    image = b'pinned-image-fixture'
    (fw/'xmos/manifest.json').write_text(json.dumps({'version':'fixture','bytes':len(image),'sha256':hashlib.sha256(image).hexdigest()}))
    for optimize in ('0','1'):
        env = dict(os.environ,PYTHONOPTIMIZE=optimize)
        def run(ok, script='validate-images.py', *args):
            r = subprocess.run([sys.executable,str(root/'scripts'/script),*map(str,args)],capture_output=True,text=True,env=env)
            if (r.returncode==0)!=ok:
                raise RuntimeError(f'Unexpected optimize={optimize} result: '+r.stdout+r.stderr)
        (fw/'sdkconfig').write_text(config); (build/'voicekey.bin').write_bytes(header)
        (build/'partition_table/partition-table.bin').write_bytes(table)
        run(True);run(True,'validate-images.py','--config-only')
        for invalid in (config.replace('FLASHSIZE_8MB','FLASHSIZE_16MB'),config+'CONFIG_ESP_CONSOLE_UART_DEFAULT=y\n'):
            (fw/'sdkconfig').write_text(invalid);run(False,'validate-images.py','--config-only')
        (fw/'sdkconfig').write_text(config)
        (build/'voicekey.bin').write_bytes(header+b'\0'*(4*1024*1024));run(False)
        (build/'voicekey.bin').write_bytes(header[:3]+bytes([0x40])+header[4:]);run(False)
        (build/'voicekey.bin').write_bytes(header)
        corruptions = [table[:-1], table[:144]+bytes([table[144]^1])+table[145:], table[:-1]+b'\0',
                       generated_table(entries+[entries[0]])]
        for index,value in ((0,0),(1,0),(2,0x400000),(3,0x500000),(5,1)):
            records = list(entries); changed = list(records[3]);changed[index]=value;records[3]=tuple(changed)
            corruptions.append(generated_table(records))
        for corrupted in corruptions:
            (build/'partition_table/partition-table.bin').write_bytes(corrupted);run(False)
        (build/'partition_table/partition-table.bin').write_bytes(table)
        (root/'image.bin').write_bytes(image);run(True,'verify-xmos.py',root/'image.bin')
        (root/'image.bin').write_bytes(b'X'+image[1:]);run(False,'verify-xmos.py',root/'image.bin')
        (root/'image.bin').write_bytes(image[:-1]);run(False,'verify-xmos.py',root/'image.bin')
print('PASS: real validators under PYTHONOPTIMIZE=0/1: cache/header/capacity, partition MD5/types/subtypes/flags/duplicates/integrity and XMOS wrong hash/size')
