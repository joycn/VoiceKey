#!/usr/bin/env python3
"""Control VoiceKey over XIAO USB HID. Requires hidapi; basic colors >=1.4, full LED parameters >=1.5."""
import argparse
import json
import re
import sys
import struct
import time
import secrets
import zlib
from pathlib import Path
from diagnose import decode, open_device

UNCHANGED = object()

COLORS = {'red': 'ff0000', 'green': '00ff00', 'blue': '0000ff',
          'white': 'ffffff', 'yellow': 'ffff00', 'cyan': '00ffff',
          'magenta': 'ff00ff', 'purple': '800080', 'orange': 'ff8000',
          'off': '000000', 'black': '000000'}


def color(value):
    value = value.lower()
    if value == 'auto':
        return None
    value = COLORS.get(value, value[1:] if value.startswith('#') else value)
    if not re.fullmatch(r'[0-9a-f]{6}', value):
        raise argparse.ArgumentTypeError('Use #RRGGBB, RRGGBB, a named color, off or auto')
    return bytes.fromhex(value)


def rgb_color(value):
    rgb = color(value)
    if rgb is None:
        raise argparse.ArgumentTypeError('auto is a mode, not an RGB color')
    return rgb


def led_report(rgb):
    payload = bytearray(224)
    payload[:6] = b'VKLC\x01' + bytes([rgb is not None])
    if rgb is not None:
        payload[6:9] = rgb
    # hidapi requires report ID 0 before an unnumbered report.
    return bytes([0]) + payload


EFFECTS = {'off': 0, 'breathe': 1, 'breath': 1, 'rainbow': 2, 'solid': 3, 'doa': 4, 'ring': 5}


def byte_value(value):
    try:
        n = int(value)
    except ValueError:
        raise argparse.ArgumentTypeError('Expected integer 0..255')
    if not 0 <= n <= 255:
        raise argparse.ArgumentTypeError('Expected integer 0..255')
    return n


def effect_value(value):
    if value in EFFECTS:
        return EFFECTS[value]
    n = byte_value(value)
    if n > 5:
        raise argparse.ArgumentTypeError('Effect must be off/breathe/rainbow/solid/doa/ring or 0..5')
    return n


def config_report(args):
    payload = bytearray(224)
    payload[:6] = b'VKLC\x02\x01'
    mask = 0
    fields = [('effect', 0, 0), ('brightness', 1, 1), ('speed', 2, 2), ('gamma', 3, 3)]
    for name, bit, offset in fields:
        value = getattr(args, name, None)
        if value is not None:
            mask |= 1 << bit
            payload[7 + offset] = value
    rgb = getattr(args, 'color_option', None)
    positional = getattr(args, 'color', UNCHANGED)
    if rgb is not None and positional is not UNCHANGED:
        raise ValueError('Specify color once, either positional or --color')
    if positional is None:
        raise ValueError('led auto cannot be combined with parameter options')
    if rgb is None and positional is not UNCHANGED:
        rgb = positional
    if rgb is not None:
        mask |= 16
        payload[11:14] = rgb
    for name, bit, start in [('doa_colors', 5, 14), ('ring_colors', 6, 20)]:
        values = getattr(args, name, None)
        if values is not None:
            if any(v is None for v in values):
                raise ValueError('auto is not a color for DOA/ring')
            raw = b''.join(values)
            mask |= 1 << bit
            payload[start:start + len(raw)] = raw
    if not mask:
        raise ValueError('Specify a color or at least one LED parameter')
    payload[6] = mask
    return bytes([0]) + payload


def uses_config(args):
    return any(getattr(args, name, None) is not None for name in
               ('effect', 'brightness', 'speed', 'gamma', 'color_option', 'doa_colors', 'ring_colors'))


def load_layout(path):
    data = json.loads(Path(path).read_text())
    if not isinstance(data, dict) or set(data) != {'zero_degrees', 'direction', 'seats'}:
        raise ValueError('Layout requires exactly zero_degrees, direction, seats')
    def integer(v, low, high):
        return type(v) is int and low <= v <= high
    if not integer(data['zero_degrees'], 0, 359) or data['direction'] not in ('clockwise','counterclockwise'):
        raise ValueError('Invalid zero_degrees or direction')
    seats = data['seats']
    if not isinstance(seats, list) or not 1 <= len(seats) <= 8:
        raise ValueError('Expected 1..8 seats')
    names, regions, used = set(), set(), 0
    for seat in seats:
        if not isinstance(seat, dict) or set(seat) != {'name','region','center_degrees','width_degrees'}:
            raise ValueError('Each seat requires name, region, center_degrees, width_degrees')
        name, region, center, width = (seat[k] for k in ('name','region','center_degrees','width_degrees'))
        if not isinstance(name,str) or not name.strip() or len(name)>32 or name in names:
            raise ValueError('Seat names must be unique nonempty strings <=32 characters')
        if not integer(region,0,7) or region in regions or not integer(center,0,359) or not integer(width,1,360):
            raise ValueError('Invalid/duplicate region, center or width')
        names.add(name); regions.add(region)
        angles = [(data['zero_degrees'] + (1 if data['direction']=='clockwise' else -1)*j*30)%360 for j in range(12)]
        distances = [min((a-center)%360,(center-a)%360) for a in angles]
        mask = sum(1<<j for j,d in enumerate(distances) if 2*d<=width)
        if not mask: mask = 1<<distances.index(min(distances))
        if mask & used: raise ValueError('Seat LED arcs overlap; change centers or widths')
        used |= mask
    return data


def layout_payload(data):
    body = bytearray(48)
    struct.pack_into('<BHB',body,0,len(data['seats']),data['zero_degrees'],data['direction']=='counterclockwise')
    for i,seat in enumerate(data['seats']):
        struct.pack_into('<BHH',body,4+i*5,seat['region'],seat['center_degrees'],seat['width_degrees'])
    token = zlib.crc32(json.dumps(data,sort_keys=True,separators=(',',':'),ensure_ascii=False).encode())
    struct.pack_into('<I',body,44,token)
    return bytes(body)


def layout_cache(args):
    return Path.home()/'.local/share/voicekey'/f'led-layout-{args.vid:04x}-{args.pid:04x}.json'


def semantic_report(op, body, request):
    payload = bytearray(384)
    payload[:6] = b'VKLC\x03' + bytes([op])
    struct.pack_into('<I', payload, 6, request)
    payload[10:10+len(body)] = body
    return b'\0' + payload


def read_status(device):
    return decode(device.get_feature_report(0,385))


def execute(device, args):
    status = read_status(device)
    action = getattr(args, 'action', None)
    if args.command == 'status': return status
    if action == 'status':
        if 'led_control' not in status: raise RuntimeError('LED status requires firmware 1.6')
        return status['led_control']
    release = tuple(map(int, status['firmware_release'].split('.')))
    extended = uses_config(args)
    if action and release < (1,6): raise RuntimeError('Seat LED commands require firmware release 1.6 or newer')
    if extended and release < (1,5): raise RuntimeError('Full LED parameters require firmware release 1.5 or newer')
    if release < (1,4): raise RuntimeError('LED control requires firmware release 1.4 or newer; flash the new ESP firmware first')
    layout_data = None
    if action == 'layout':
        layout_data = load_layout(args.file)
        op,body = 3,layout_payload(layout_data)
    elif action == 'identify': op,body = 4,struct.pack('<BI',args.index,args.duration_ms)
    elif action == 'preview':
        layout_data = load_layout(args.layout or layout_cache(args))
        if struct.unpack_from('<I',layout_payload(layout_data),44)[0] != status['led_control']['layout_token']:
            raise RuntimeError('Layout differs from device (or device rebooted); apply layout first')
        seats = [s for s in layout_data['seats'] if s['name']==args.seat]
        if not seats: raise ValueError('Unknown seat name')
        op,body = 5,struct.pack('<BBI',seats[0]['region'],['owner','receiving','granted','denied'].index(args.state),args.duration_ms)
    else:
        if not extended and args.color is UNCHANGED: raise ValueError('Specify a color or LED parameter')
        legacy = config_report(args) if extended else led_report(args.color)
        if extended: op,body = 2,legacy[7:57]
        elif args.color is None: op,body = 1,b''
        else:
            config = bytearray(49); config[:7]=bytes([3 if any(args.color) else 0,30,1,0])+args.color
            op,body=2,bytes([31])+config
    request = secrets.randbits(32) or 1
    report = semantic_report(op,body,request) if release >= (1,6) else legacy
    written = device.send_feature_report(report)
    if written != len(report): raise RuntimeError(f'Incomplete HID write: {written}/{len(report)} bytes')
    if release < (1,6):
        return {'status':'submitted','note':'USB transfer only; no device execution acknowledgement'}
    deadline = time.monotonic()+3
    while time.monotonic()<deadline:
        time.sleep(.05)
        latest=read_status(device)['led_control']
        if latest['request_id'] != request: continue
        if latest['result']=='failed': raise RuntimeError('LED request failed: '+latest['reason'])
        if latest['result'] in ('applied','covered','expired'):
            if action=='layout':
                path=layout_cache(args);path.parent.mkdir(parents=True,exist_ok=True)
                temporary=path.with_suffix(f'.{request}.tmp')
                temporary.write_text(json.dumps(layout_data,ensure_ascii=False,indent=2)+'\n')
                temporary.replace(path)
            return latest
    raise RuntimeError('No matching execution result within 3s (pending or superseded); run led status')


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    semantic = None
    if 'led' in argv:
        index = argv.index('led')
        if len(argv)>index+1 and argv[index+1] in ('status','layout','identify','preview'):
            semantic = argv[index+1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--vid', type=lambda x: int(x, 0), default=0xcafe)
    parser.add_argument('--pid', type=lambda x: int(x, 0), default=0x4014)
    commands = parser.add_subparsers(dest='command', required=True)
    commands.add_parser('status', help='Read device diagnostics')
    led = commands.add_parser('led', help='LED colors/parameters; status, layout apply, identify, preview, auto', epilog='Seat commands: led status | led layout apply FILE | led identify --index N | led preview --seat NAME --state owner')
    if semantic:
        led.add_argument('action', choices=[semantic])
        if semantic=='layout':
            led.add_argument('operation', choices=['apply'])
            led.add_argument('file')
        elif semantic=='identify':
            led.add_argument('--index', type=int, choices=range(12), required=True)
            led.add_argument('--duration-ms', type=int, default=3000)
        elif semantic=='preview':
            led.add_argument('--seat', required=True)
            led.add_argument('--state', choices=['owner','receiving','granted','denied'], required=True)
            led.add_argument('--duration-ms', type=int, default=3000)
            led.add_argument('--layout', help='Use this layout instead of the last successfully applied local file')
    else:
        led.add_argument('color', type=color, nargs='?', default=UNCHANGED)
        led.add_argument('--color', dest='color_option', type=rgb_color)
        led.add_argument('--effect', type=effect_value)
        led.add_argument('--brightness', type=byte_value, help='0..255')
        led.add_argument('--speed', type=byte_value, help='Raw XVF value 0..255; visual meaning depends on effect')
        led.add_argument('--gamma', type=int, choices=[0, 1])
        led.add_argument('--doa-colors', type=rgb_color, nargs=2, metavar=('BASE', 'DIRECTION'))
        led.add_argument('--ring-colors', type=rgb_color, nargs=12, metavar='RGB')
    args = parser.parse_args(argv)
    if hasattr(args,'duration_ms') and not 100 <= args.duration_ms <= 60000:
        parser.error('duration-ms must be 100..60000')
    if args.command == 'led' and not semantic:
        # Validate the complete command before opening the device.
        try:
            if uses_config(args):
                config_report(args)
            elif args.color is UNCHANGED:
                raise ValueError('Specify a color or LED parameters')
        except ValueError as error:
            parser.error(str(error))
    try:
        import hid
        device = open_device(hid, args.vid, args.pid)
        try:
            result = execute(device, args)
        finally:
            device.close()
        print(json.dumps(result, ensure_ascii=False, indent=2))
        return 0
    except (ImportError, OSError, RuntimeError, ValueError, AttributeError) as error:
        print(f'voicekey: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
