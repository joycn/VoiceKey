# Verification — 2026-09-22

- scripts/test.sh: PASS (includes 5 CLI tests and board arbitration tests).
- scripts/test-usb.sh: PASS (valid RGB/off/auto, malformed framing/version/opcode/reserved fields, wrong report type/ID/instance).
- scripts/test-descriptors.sh: PASS; no new endpoints.
- scripts/build.sh: PASS including image validation; no flash.
- openspec validate usb-led-cli --strict and git diff --check: PASS.
- App binary: 418272 bytes; SHA256 d9961ccd6d3a5a568f0da80404bf125e5f746591c920ef4d104798b0d1884e6a.

Logs: /tmp/voicekey-led-tests.log and /tmp/voicekey-led-build.log.
Physical HID SET_REPORT, visible colors, mute override and simultaneous audio remain pending hardware deployment. Mock tests do not establish hardware acceptance.

## Hardware deployment

2026-09-22: identified ESP32-S3 MAC 68:ee:8f:46:c6:2c, 8 MB flash on /dev/cu.usbmodem144101. Production wrapper flash succeeded with all write hashes verified. RTS reset left serial enumeration; esptool read_mac with --after watchdog_reset restored application enumeration. No XMOS flash.

HID reports firmware1.4, ELF prefix a751bbf03be0be59 matching built ELF, XMOS1.0.8, all initialization errors0. Actual CLI SET_REPORT tested red/green/blue/custom/white/off/auto: effect readback3 for static,0 for off,1 for auto; valid board/LED snapshots and I2C errors0. Restored auto. Evidence: docs/device-records/2026-09-22-led-cli.json and -flash.log.

Visible RGB accuracy, physical mute-button interaction and simultaneous host recording/playback remain unverified; hardware task remains open.
