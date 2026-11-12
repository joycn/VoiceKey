import argparse
import contextlib
import io
import sys
import unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import voicekey

class Device:
    def __init__(self, minor=4, short=False):
        self.raw = bytearray(224)
        self.raw[0] = 4
        self.raw[5:7] = bytes([1, minor])
        self.writes = []
        self.short = short
        self.closed = False
    def get_feature_report(self, report_id, length):
        assert report_id == 0 and length == 385
        return self.raw
    def send_feature_report(self, report):
        self.writes.append(report)
        return len(report) - int(self.short)
    def close(self):
        self.closed = True

class CLI(unittest.TestCase):
    def test_colors(self):
        self.assertEqual(voicekey.color('#12AbEF'), bytes.fromhex('12abef'))
        self.assertEqual(voicekey.color('red'), b'\xff\0\0')
        self.assertEqual(voicekey.color('off'), b'\0\0\0')
        self.assertIsNone(voicekey.color('auto'))
        for bad in ['#123', 'ffffff00', 'zzzzzz', '-1', '']:
            with self.assertRaises(argparse.ArgumentTypeError): voicekey.color(bad)
    def test_protocol(self):
        d = Device()
        result = voicekey.execute(d, argparse.Namespace(command='led', color=bytes.fromhex('123456')))
        self.assertEqual(result['status'], 'submitted')
        self.assertEqual(d.writes, [b'\0VKLC\x01\x01\x12\x34\x56' + bytes(215)])
        self.assertEqual(voicekey.led_report(None), b'\0VKLC\x01\0' + bytes(218))
    def test_compatibility_and_short_write(self):
        d = Device(minor=3)
        args = argparse.Namespace(command='led', color=None)
        with self.assertRaisesRegex(RuntimeError, 'requires firmware'): voicekey.execute(d, args)
        self.assertEqual(d.writes, [])
        with self.assertRaisesRegex(RuntimeError, 'Incomplete'): voicekey.execute(Device(short=True), args)
    def test_status_readonly(self):
        d = Device(minor=3)
        self.assertEqual(voicekey.execute(d, argparse.Namespace(command='status'))['firmware_release'], '1.3')
        self.assertEqual(d.writes, [])
    def test_main_closes_on_error(self):
        for minor, expected in [(3, 1), (4, 0)]:
            d = Device(minor=minor)
            with patch.dict(sys.modules, {'hid': object()}), patch.object(voicekey, 'open_device', return_value=d), contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(voicekey.main(['led', 'blue']), expected)
            self.assertTrue(d.closed)

class SemanticCLI(unittest.TestCase):
    def test_full_parameters(self):
        args=argparse.Namespace(color=voicekey.UNCHANGED,effect=5,brightness=255,speed=0,gamma=1,
            color_option=bytes.fromhex('123456'),doa_colors=[b'\0\0\0',b'\xff\0\0'],ring_colors=[b'\0\0\xff']*12)
        report=voicekey.config_report(args)
        self.assertEqual(report[7],127)
        self.assertEqual(report[8:15],bytes.fromhex('05ff0001123456'))
        self.assertEqual(report[21:57],b'\0\0\xff'*12)
        args.gamma=0;args.color=None
        with self.assertRaises(ValueError): voicekey.config_report(args)
        for value in ['-1','256','1.5']:
            with self.assertRaises(argparse.ArgumentTypeError): voicekey.byte_value(value)
    def test_semantic_ack(self):
        import struct
        d=Device(minor=6);d.raw=bytearray(384);d.raw[0]=5;d.raw[5:7]=bytes([1,6]);d.raw[4]=5
        def write(report):
            d.writes.append(report);d.raw[224:228]=report[7:11];d.raw[228]=2
            return len(report)
        d.send_feature_report=write
        with patch.object(voicekey.time,'sleep'):
            r=voicekey.execute(d,argparse.Namespace(command='led',color=b'\xff\0\0'))
        self.assertEqual(r['result'],'applied');self.assertEqual(len(d.writes[0]),385)
        self.assertEqual(d.writes[0][1:7],b'VKLC\x03\x02')
        self.assertNotEqual(struct.unpack_from('<I',d.writes[0],7)[0],0)
    def test_layout(self):
        import tempfile,json
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'seats.json'
            d={'zero_degrees':0,'direction':'clockwise','seats':[
                {'name':'A','region':0,'center_degrees':359,'width_degrees':1},
                {'name':'B','region':1,'center_degrees':90,'width_degrees':40}]}
            path.write_text(json.dumps(d));self.assertEqual(voicekey.load_layout(path),d)
            self.assertEqual(len(voicekey.layout_payload(d)),48)
            d['seats'][1]['center_degrees']=0;path.write_text(json.dumps(d))
            with self.assertRaisesRegex(ValueError,'overlap'):voicekey.load_layout(path)
    def test_invalid_preview_duration_before_open(self):
        with patch.object(voicekey,'open_device') as op, contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):voicekey.main(['led','identify','--index','0','--duration-ms','0'])
            op.assert_not_called()

if __name__ == '__main__': unittest.main()
