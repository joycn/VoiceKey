# XMOS runtime verified; production USB still unresolved — 2026-09-18

Read repeated maintenance responses from XIAO /dev/cu.usbmodem144101 after reconnect:
- VERSION1.0.8
- BLD_MSG inthost-lr48-sqr-i2c
- USB_BIT_DEPTH(0,0), I2S_INACTIVE0
- OP_L(7,0), OP_PACKED(0,0), OP_UPSAMPLE(1,1) before production route configuration
- AEC convergence0, bypass0, reference gain8.0

This confirms the new XMOS application is running and the control interface works. No physical clock measurement or audio/AEC acceptance follows from these queries alone.

Ran `./scripts/build.sh -p /dev/cu.usbmodem144101 flash`. Build/image validations and all four write hashes passed, exit0. Restored production bootloader, partition table, app and model. App SHA2563c21c147f0a275ad658ce3d6d9bd62310208f804967016db03c1f0da10ea2864; all final image hashes appear in the saved log.

After reset no Espressif/VoiceKey USB device or usbmodem serial was observed. One-shot HID diagnostic returned VoiceKey HID device not found. XMOS mismatch is therefore not sufficient to explain the full application's USB failure. Maintenance basic startup/serial/I2C works; production PSRAM/TinyUSB/audio/model initialization still needs isolation. No claim of microphone/speaker/HID operational success.

Current installed XIAO firmware: production. Requested BOOT/RESET again to regain ROM download before further staged diagnosis.

Evidence: [XMOS runtime](logs/2026-09-18-xmos-runtime-1.0.8.log), [production flash](logs/2026-09-18-production-after-xmos.log).
