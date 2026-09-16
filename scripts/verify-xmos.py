#!/usr/bin/env python3
"""Verify a separately obtained pinned XMOS image. Never connects to hardware."""
import argparse
import hashlib
import json
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('image', type=Path)
a = p.parse_args()
m = json.loads((Path(__file__).resolve().parents[1] / 'firmware/xmos/manifest.json').read_text())
data = a.image.read_bytes()
if len(data) != m['bytes']:
    raise SystemExit('XMOS image size differs')
if hashlib.sha256(data).hexdigest() != m['sha256']:
    raise SystemExit('XMOS image digest differs')
print('PASS: pinned XMOS', m['version'], len(data), m['sha256'])
