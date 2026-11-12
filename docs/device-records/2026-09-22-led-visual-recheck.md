# LED full recheck — 2026-09-22

User explicitly requested all checks and visible LED demonstration.

- Host suite (9 CLI tests plus core/adapter/protocol/LED tests), actual USB callbacks and descriptors: PASS.
- Running ELF matches current build; firmware1.6/schema5, XMOS1.0.8.
- 38 visible steps recorded: red/green/blue/white/off, blue breathe, rainbow, DOA colors,12-color ring,brightness20→100,speed1→5,gamma enabled,indices0..11,seatsA..D,receiving and3 repeats each of granted/denied double flashes.
- Every step returned applied; all recorded board snapshots valid; I2C errors remained0.
- Simultaneous CoreAudio capture and silent playback during seat demos: 498688 input frames,1488896 output frames; no callback status flags, no new transport faults or I2C errors. No audio samples saved; not acoustic/AEC acceptance.
- User confirmed: “全部正常，静音和恢复也正常”, in response to blue seat → red mute → blue restored, colors, breathing, per-LED progression and grant/deny double flashes.
- Returned to auto after confirmation. Example layout remains in RAM.

Actual acoustic multi-seat ownership remains outside this demonstration: spatial mode is disabled. Visual seat previews do not alter audio ownership. Physical seat-to-angle calibration must still match actual seating; user accepted displayed effects, not an independently measured angular calibration.

Evidence: led-visual-recheck.jsonl, led-visual-recheck-audio.json, led-visual-recheck-mute.json (same directory/date prefix). Physical visual/mute acceptance includes user observation; telemetry may not span every button press.
