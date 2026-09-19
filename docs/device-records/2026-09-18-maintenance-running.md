# XIAO maintenance running — 2026-09-18

After the user's BOOT/RESET operation, USB303a:1001 and /dev/cu.usbmodem144101 appeared. esptool confirmed ESP32-S3 rev0.2, MAC68:ee:8f:46:c6:2c,8MB flash(c8:4017), matching the original backup device. Production and maintenance image validators passed before writing.

Wrote maintenance bootloader at0, identical partition table at0x8000 and maintenance app at0x10000. All three hash checks passed. No NVS/model/XMOS writes. App SHA2567ba2a3e99ac27cdb888a7a843f205fa17bd1a27c05eceffa36f4a94c40852d7d.

Initial serial log after ordinary reset showed DOWNLOAD(USB/UART0). Used esptool's supported --after watchdog_reset read_mac, then reopened serial with DTR/RTS disabled before open. Captured SPI_FAST_FLASH_BOOT, diagnostic app entry and repeated live maintenance snapshots. XIAO basic app startup/USB Serial-JTAG/I2C confirmed; PSRAM intentionally disabled and not validated.

Actual XMOS read-only responses:
- VERSION:1.0.4
- BLD_MSG:intdev-lr16-sqr-i2c
- USB_BIT_DEPTH:(0,0), INT profile
- GPO_READ_VALUES:(0,0,0,1,0), mute output low at sample time
- I2S_INACTIVE:1 (diagnostic image does not initialize I2S)
- OP_L:(8,0), OP_PACKED:(0,0), OP_UPSAMPLE:(0,0)
- AEC_CONVERGED:0, SHF_BYPASS:0, REF_GAIN:float32 8.0

The normal XMOS firmware does not match pinned1.0.8_48k. Build string alone is not physical clock measurement. This mismatch prevents production audio admission but does not establish why the earlier full app failed USB enumeration.

Current installed XIAO application is MAINTENANCE, not VoiceKey Audio/HID. Next: user moves cable to XMOS port and enters Mute Safe Mode; identify serial114993702262500311, verify pinned image, update Upgrade only, preserve Factory/DataPartition. Then move back to XIAO for readback and production restoration/validation.

Evidence: [flash log](logs/2026-09-18-maintenance-flash.log), [console log](logs/2026-09-18-maintenance-console.log).
