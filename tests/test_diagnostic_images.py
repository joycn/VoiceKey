"""Check maintenance image validator against real artifacts and mutations."""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as directory:
    fixture=Path(directory)
    for name in ('scripts/validate-diagnostic.py', 'firmware/diagnostic/sdkconfig',
                 'firmware/diagnostic/build/flasher_args.json',
                 'firmware/diagnostic/build/voicekey_diagnostic.bin',
                 'firmware/diagnostic/build/bootloader/bootloader.bin',
                 'firmware/diagnostic/build/partition_table/partition-table.bin',
                 'firmware/build/partition_table/partition-table.bin'):
        target=fixture/name; target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(root/name,target)
    script=fixture/'scripts/validate-diagnostic.py'
    def run(expected):
        result=subprocess.run([sys.executable,'-O',str(script)],stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        assert (result.returncode==0)==expected,result.stdout.decode()
    run(True)
    config=fixture/'firmware/diagnostic/sdkconfig'; original=config.read_text()
    config.write_text(original.replace('CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y','CONFIG_ESP_CONSOLE_UART_DEFAULT=y'))
    run(False); config.write_text(original)
    table=fixture/'firmware/diagnostic/build/partition_table/partition-table.bin'; original=table.read_bytes()
    table.write_bytes(b'bad'+original[3:]); run(False); table.write_bytes(original)
    app=fixture/'firmware/diagnostic/build/voicekey_diagnostic.bin'; original=app.read_bytes()
    data=bytearray(original);data[3]=0;app.write_bytes(data);run(False);app.write_bytes(original)
    args=fixture/'firmware/diagnostic/build/flasher_args.json';original=args.read_text()
    data=json.loads(original);data['flash_files']['0x410000']='srmodels/srmodels.bin'
    args.write_text(json.dumps(data));run(False);args.write_text(original)
    run(True)
print('PASS: real maintenance images; reject UART0, changed partitions, wrong image header and extra model writes under optimized Python')
