# First XIAO flash: 2026-09-18

Status: flash write verified; application USB enumeration pending troubleshooting. No recording/playback/AEC success claimed.

- Detected before write: Espressif USB Serial/JTAG 303a:1001, /dev/cu.usbmodem144101.
- esptool 4.12.0: ESP32-S3 QFN56 rev0.2, embedded 8 MB PSRAM AP_3v3, 40 MHz crystal, 8 MB quad Flash (c8:4017), MAC 68:ee:8f:46:c6:2c.
- Full original 8,388,608-byte Flash read succeeded before write.
- Backup: `/Users/joy/.local/share/voicekey/backups/68ee8f46c62c/before-voicekey-20260918.bin`.
- Backup SHA256: `afe7b55ef757e56434c6f20add6db9604ffe951c1e24742cb6def5248b6c6362`.
- Command: `./scripts/build.sh -p /dev/cu.usbmodem144101 flash`, exit 0. Pre/post image validation passed. esptool verified all four writes and issued hardware reset.
- Application SHA256: `1a1693d3b8bfb305d266999b51c81f06062f100910783f96a4ba47fe13bfa2c7`; other artifacts match docs/validation-status.md.
- Flash log: `/Users/joy/.local/share/voicekey/backups/68ee8f46c62c/first-flash-20260918.log`.
- After reset: original serial interface disappeared; no VoiceKey USB/audio/HID observed. One-shot diagnostic reports device not found. Cold reconnect requested; cause not established.
- XMOS firmware was NOT written or identified in this operation.
- Dedicated host diagnostic environment: `/Users/joy/.local/share/voicekey/diagnostic-venv` with hidapi 0.15.0; no resident service.

Recovery: hold XIAO BOOT while plugging its USB cable, then release; alternatively hold BOOT and press RESET. Identify the re-enumerated port before writing. Original backup is available; no eFuses were written.

Reference: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/#bootloader-mode
