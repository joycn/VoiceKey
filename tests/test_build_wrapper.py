#!/usr/bin/env python3
"""Mock only IDF and validator processes; never access hardware."""
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
source = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as temporary:
    root=Path(temporary);(root/'scripts').mkdir();(root/'firmware').mkdir();(root/'idf/bin').mkdir(parents=True)
    shutil.copyfile(source/'scripts/build.sh',root/'scripts/build.sh')
    (root/'firmware/sdkconfig').write_text('fixture')
    (root/'idf/bin/python').symlink_to(sys.executable)
    log=root/'calls.jsonl'
    code='''import json,os,sys
with open(os.environ['VK_WRAPPER_LOG'],'a') as f: f.write(json.dumps([ROLE]+sys.argv[1:])+'\\n')
if ROLE=='validate' and '--config-only' not in sys.argv and os.environ.get('VK_FAIL_VALIDATION'): sys.exit(1)
'''
    (root/'scripts/validate-images.py').write_text(code.replace('ROLE',repr('validate')))
    idf=root/'idf/bin/idf.py';idf.write_text('#!'+sys.executable+'\n'+code.replace('ROLE',repr('idf')));idf.chmod(0o755)
    (root/'idf/export.sh').write_text('export PATH='+shlex.quote(str(root/'idf/bin'))+':"$PATH"\n')
    env=dict(os.environ,IDF_PATH=str(root/'idf'),VOICEKEY_PYTHON=sys.executable,VK_WRAPPER_LOG=str(log))
    for name in ('SDKCONFIG','SDKCONFIG_DEFAULTS','IDF_BUILD_DIR'):
        env.pop(name,None)
    def run(args, ok=True, extra=None):
        log.unlink(missing_ok=True)
        r=subprocess.run(['bash',str(root/'scripts/build.sh'),*args],env=dict(env,**(extra or {})),capture_output=True,text=True)
        if (r.returncode==0)!=ok:raise RuntimeError(r.stdout+r.stderr)
        return [json.loads(line) for line in log.read_text().splitlines()] if log.exists() else []
    assert run([])==[['validate','--config-only'],['idf','build'],['validate']]
    assert run(['menuconfig'])==[['idf','menuconfig']]
    assert run(['clean'])==[['idf','clean']]
    assert run(['-p','/dev/never-opened','flash'])==[['validate','--config-only'],['idf','build'],['validate'],['idf','-p','/dev/never-opened','flash'],['validate']]
    for option in ('-B','-Belsewhere','--build-dir','--build-dir=elsewhere','-C','-Celsewhere','--project-dir=elsewhere','-D','-DSDKCONFIG=elsewhere','--define-cache-entry=SDKCONFIG=elsewhere'):
        assert run([option,'elsewhere','flash'],False)==[]
    for variable in ('SDKCONFIG','SDKCONFIG_DEFAULTS','IDF_BUILD_DIR'):
        assert run(['flash'],False,{variable:'elsewhere'})==[]
    assert run(['flash'],False,{'VK_FAIL_VALIDATION':'1'})==[['validate','--config-only'],['idf','build'],['validate']]
print('PASS: build wrapper rejects path/config overrides before IDF and stops requested flash on validation failure; all IDF calls mocked')
