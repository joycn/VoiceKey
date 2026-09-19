# WakeNet startup failure isolated — 2026-09-18

Full-subsystem bringup reused production sources with USB Serial/JTAG console, Octal80MHz PSRAM and I2S/model before USB takeover. Device8MB PSRAM initialization and IDF SRAM startup memory test passed; board/control/I2S start returned ESP_OK.

Original staged image2530a14d028e763ffc0a3acead6f38a687fcb5a9c299c4d0973a94866bf5a937 reproduced repeated LoadProhibited (EXCVADDR0x10) immediately after WakeNet creation. addr2line against its ELF315437109 decoded:
-0x420173a8 dl_convq_queue_bzero, dl_lib_convq_queue.c:565
-0x42012cb1 model_clean, wakenet9_quantized.c:1039
-0x4200ac5f detect_task, firmware/main/wake_engine.c:20

Removed clean() path in favor of fresh model instances at used-history epoch boundaries. Empty queues do not recreate; failure drops input and retries. Production-path native regression checks fresh startup, empty epoch changes, used-model destroy/recreate, failed allocation/retry and new-epoch triggers. Full test.sh and production/staged cross-build validations passed.

Fixed staged image1a1b43385798cb88ac8accdecb1384fdeff58163b8c36bf055d03e57d82fe156 was written with four verified hashes; watchdog reset used. Log reached after-WakeNet ESP_OK and USB takeover. Mac enumerated VoiceKey CAFE:4014,16kHz mono microphone,48kHz stereo speaker, HID keyboard and vendor collection(interface3). Current Mac output remained built-in speakers. No recording/playback/acoustic test performed.

hidapi enumerated interface3 but open_path returned OSError open failed. Cause unconfirmed; diagnostics not yet readable. Staged console also shows repeated model recreations, suggesting recurring epoch invalidation that still requires investigation. One initial I2C busy timeout was observed in the pre-fix run. Enumeration is not proof of stable audio/control.

Fixed production app built:412304bytes, SHA256d720d1436e4b42bfdc01cf9557b5919922e0b7eefe9ec48af279881f54acb5b3. Not yet flashed at this record's creation. User requested to reenter ROM download for production restoration. Current application is staged bringup.

Evidence: [original panic](logs/2026-09-18-wakenet-panic.log), [fixed staged startup](logs/2026-09-18-bringup-fixed-console.log), [fixed staged flash](logs/2026-09-18-bringup-fixed-flash.log).

## Production restoration completed

Following the user's confirmation of ROM download mode, flashed the validated production image through `/dev/cu.usbmodem144101` with esptool at 460800 baud and `--after watchdog_reset`. All four writes reported `Hash of data verified`; command exited 0. App SHA256 remains `d720d1436e4b42bfdc01cf9557b5919922e0b7eefe9ec48af279881f54acb5b3` (412304 bytes). The installed image is now production, replacing staged bringup.

macOS enumerated CAFE:4014 VoiceKey with 16 kHz mono input and 48 kHz stereo output. HID interface 3 enumerated both keyboard (usage page 1, usage 6) and vendor (0xff00, usage 1) collections. A subsequent USB/audio snapshot still found the device. Mac default output remained built-in speakers; no system output setting was changed. This establishes production startup and enumeration, not continuous audio stability.

Production `scripts/diagnose.py` still fails at hidapi `open_path` with `OSError: open failed`. Read-only IOHIDCheckAccess(ListenEvent) returned raw 0; no permission request or setting change was made, and the cause of open failure remains unconfirmed. Board health, recurring epoch invalidation, live audio and AEC acceptance remain pending. No recording/playback was performed.

Evidence: [production flash](logs/2026-09-18-production-fixed-flash.log), [production audio enumeration](logs/2026-09-18-production-fixed-audio.txt).
