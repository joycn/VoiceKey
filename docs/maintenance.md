# XIAO startup maintenance

The production image uses TinyUSB Audio/HID. If it fails to enumerate, HID diagnostics cannot explain startup. The separate `firmware/diagnostic` project uses the ESP32-S3 USB Serial/JTAG console and internal RAM. It repeatedly prints reset reason, flash size, heap and read-only XMOS queries. It does not start I2S, TinyUSB, PSRAM or WakeNet, write codec/routing settings, or update XMOS. It is not an audio product image and does not prove PSRAM/AEC works.

Build production first with `./scripts/build.sh`, then `./scripts/build-diagnostic.sh`. The latter accepts no flashing arguments and runs `scripts/validate-diagnostic.py`, checking console/target/size, identical production partition bytes and exact image offsets. Build directories and sdkconfig files are separate. A diagnostic build must never be passed to the production build wrapper as an override.

Before writing:

1. Connect the XIAO USB-C. Enter ROM download mode with BOOT held during RESET or reconnect. XMOS Mute/Safe Mode is a different recovery path.
2. Identify the actual `/dev/cu.usbmodem…` port and verify ESP32-S3,8MB Flash and device MAC using esptool. Previously backed-up device MAC: `68:ee:8f:46:c6:2c`. Do not select an unrelated serial port.
3. Preserve the full original backup and first-flash log listed in `device-records/2026-09-18-first-flash.md`.
4. In an activated IDF5.5.2 environment, run the production image validator, then the diagnostic validator. Only then, in `firmware/diagnostic`, run `idf.py -p ACTUAL_PORT flash monitor`. This writes the diagnostic bootloader, identical partition table and diagnostic app. It does not erase NVS or the model partition. Replace ACTUAL_PORT with the identified port; do not copy an old port blindly.
5. Verify write hashes, then observe repeated maintenance logs after reset. Save exact log/error responses. Missing XMOS clocks do not block the console. This image intentionally leaves PSRAM untested; do not report8MB PSRAM validation from this stage.
6. Once baseline startup/control is understood, restore the production bootloader/app using `./scripts/build.sh -p ACTUAL_PORT flash`, returning to ROM download mode if needed. Verify VoiceKey enumeration, then run the one-shot HID diagnostics. If production still fails, keep the saved maintenance evidence and isolate the next subsystem before claiming completion.

Current flash authorization does not substitute for an identifiable device. When the ROM port is absent, build and validation can finish but flashing must remain pending.

For playback and echo cancellation, use [the commissioning contract](playback-aec-contract.md). USB writes, USB enumeration and audio/AEC are separate acceptance stages.

## Full-subsystem staged bring-up

`./scripts/build-bringup.sh` builds `firmware/bringup` separately, reusing production core, board, I2S, WakeNet and TinyUSB sources. It uses the same8MB/Octal80MHz PSRAM/model/partition configuration, with USB Serial/JTAG startup console. It starts control/audio/model before taking over the shared USB PHY for Audio/HID, logging each step and waiting10seconds before takeover. The console is expected to disappear at takeover; use the normal HID diagnostic if enumeration succeeds. This changed startup order is diagnostic evidence, not proof the production startup order is fixed.

Run `python3 scripts/validate-images.py` and `python3 scripts/validate-bringup.py` before writing. Identify the same ESP32-S3 MAC and use only the generated bringup build's flash_args. This variant also writes the development model at0x410000; NVS remains untouched. Restore the full production image after diagnosis. A ROM recovery or watchdog reset may be required after writing, as the captured maintenance record documents.

### 历史回退基线（2026-09-18，已被后续变更替代）

按用户要求恢复首次报告Mute问题时的代码行为。后续共享HID打开、持续采样脚本及增益诊断扩展已撤回；schema3的174–191字节重新保留为零。macOS诊断仍可能遇到当时已知的HID独占打开失败。后续实验和证据见 [BUG-001](bugs/BUG-001-mute-recovery-low-input.md)。本次仅恢复代码，不烧录设备。

当前主机诊断脚本已恢复共享打开和有界多样本采集；固件版本须以HID报告中的ELF标识核验，不能依据历史回退段落判断。诊断布局见[构建说明](build.md)。
