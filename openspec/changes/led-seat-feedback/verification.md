# Verification — 2026-09-22

- Host suite, LED geometry/priority/expiry tests,9 CLI tests, schema3/4/5 decoding: PASS.
- Actual board task tests: partial updates, DOA byte order,48-byte ring payload/readback,10Hz coalescing: PASS.
- USB callback and descriptor tests: PASS; existing audio/HID interfaces and endpoints retained.
- Production build, image validation, OpenSpec validation, diff whitespace: PASS.
- App size 422352, SHA256 a67f6f4b89f73a91e1af40ca339beecda66b5be44cc23ce3dce954f5d6563829.
- ELF SHA256 4feeefc30aa467031fad80cd3d311423548e6190e4a714b6ed2cd1efb495be8a matched device diagnostic prefix.

## Deployment

Identified ESP32-S3 MAC68:ee:8f:46:c6:2c,8MB flash on /dev/cu.usbmodem144101. User entered bootloader. Production wrapper flashed all images with verified hashes; watchdog reset restored runtime. Firmware1.6/schema5; XMOS1.0.8 untouched.

Actual CLI checks passed: raw parameters,partial brightness preserving effect/color/speed/gamma,DOA,12-color ring,layout masks,identify timeout,owner/receiving/granted/denied preview and auto restoration. No added I2C errors; floor owner,generation,route generation and spatial enable unchanged. Evidence: docs/device-records/2026-09-22-led-seat-feedback.json.

Simultaneous CoreAudio capture16k mono and silent playback48k stereo during12 LED updates:82432 input frames,239616 output frames,no callback status flags,no added transport faults or I2C errors. Samples discarded. This proves stream coexistence only,not acoustic/AEC quality. Evidence: docs/device-records/2026-09-22-led-seat-audio.json.

Physical visual calibration and mute-button observation remain pending. Default spatial mode stays disabled; actual multi-seat acoustic acquisition/real-floor display on hardware is not claimed.

A45s trace during the visual-check window observed no physical mute transition; preview expired and restored idle automatically. User visual confirmation remains pending. Evidence: docs/device-records/2026-09-22-led-seat-mute.json.

## Full visual recheck and user confirmation

All host checks rerun successfully. User watched full color/effect/index/seat sequence and explicitly confirmed all effects plus physical mute/red/restore behavior normal. Simultaneous capture/playback repeated with no callback flags or new transport/I2C errors. See docs/device-records/2026-09-22-led-visual-recheck.md and its JSON evidence. Prior pending visual/mute checks are now satisfied by this observation; real multi-seat acoustic acquisition remains unclaimed. Restored auto.
