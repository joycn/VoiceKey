# XMOS Safe Mode detection — 2026-09-18

User reported blinking red LED after holding Mute during power-on. Host IORegistry then identified reSpeaker XVF3800 4-Mic Array, VID:PID2886:001a, serial114993702262500311.

Installed dfu-util0.11 and libusb1.0.30 using Homebrew. Read-only `dfu-util -l` succeeded and reported the same serial on path20-4.1, interface0:

- alt0: reSpeaker DFU Factory
- alt1: reSpeaker DFU Upgrade
- alt2: reSpeaker DFU DataPartition

This confirms an accessible XMOS DFU recovery interface and a working data connection on the current XMOS port. No firmware was written, no partition was erased, and the installed normal XMOS application version is still unknown. DFU descriptor ver0003 is not evidence of the target application version1.0.8.

No XIAO USB serial/download port was observed. This does not establish XIAO startup or VoiceKey audio/HID enumeration. Prior no-device observations are superseded for the XMOS connection only.
