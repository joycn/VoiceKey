## 1. Software implementation

- [x] 1.1 Replace board, control protocol/cache, epoch handling and filtered duplex I2S.
- [x] 1.2 Implement UAC2 microphone/speaker/feedback/master controls and EP0 HID diagnostics.
- [x] 1.3 Add production-path native protocol/FIR/buffer/drift/lifecycle and actual USB callback/descriptor tests.
- [x] 1.4 Update current documents, pin XMOS artifact and mark Voice PE evidence superseded.
- [x] 1.5 Complete final cross-build, actual8MB partition/image validation and record fresh hashes.
- [x] 1.6 Validate this OpenSpec change strictly.

## 2. Hardware and target word (pending)

- [ ] 2.1 Complete 你好小智 on-device recognition and acoustic acceptance; bundled wn9_nihaoxiaozhi_tts selected, hardware evidence pending.
- [ ] 2.2 Verify physical board/memory/I2S/USB controls/feedback/diagnostics and recovery.
- [ ] 2.3 Validate mute/fault/recovery, stop/reconnect, clock drift, concurrency and long-running behavior.
- [ ] 2.4 Validate3.5mm speaker playback/AEC/double-talk, far-field acoustics, Shift+Option+Command+S and short-pause ChatGPT workflow.

## 3. Official-source review follow-up

- [x] 3.1 Add bounded build-message/INT-profile queries, explicit output packing/upsampling readback and periodic format verification.
- [x] 3.2 Gate capture/new wake on measured 48 kHz consumption, with warmup, fault and fresh recovery tests; keep microphone mute independent from playback.
- [x] 3.3 Add cached low-rate AEC diagnostics and schema3 host decoding, without I2C in USB callbacks or additional endpoints.
- [x] 3.4 Document microphone orientation, amplifier and conference/ASR A/B device checks; validate software, cross-build and refresh artifact evidence.


## 4. Review repair and device bring-up

- [x] 4.1 Require measured48k qualification for playback; test wrong clock, rate changes, mute independence and fresh recovery.
- [x] 4.2 Build and validate a separate USB Serial/JTAG maintenance image and document recovery/restoration.
- [x] 4.3 Define playback/DAC/AEC commissioning evidence and update current hardware-validation scope.
- [x] 4.4 Run software validation and record fresh production/maintenance image hashes.
- [ ] 4.5 Identify ROM download device, flash validated maintenance image and verify startup/control logs; then restore and verify production enumeration.


## 5. WakeNet startup crash found on device

- [x] 5.1 Add isolated full-subsystem startup console and capture/decode device panic.
- [x] 5.2 Replace crashing WakeNet clean with fresh-instance epoch reset; test initial silence, history reset and allocation failure/retry.
- [x] 5.3 Cross-build and flash staged fix; verify PSRAM startup test and macOS microphone/speaker/HID enumeration.
- [x] 5.4 Flash fixed production image and confirm original startup-order microphone/speaker/HID enumeration.
- [ ] 5.5 Resolve HID diagnostic access and remaining audio/control stability evidence; enumeration alone is not acceptance.


## 6. Deferred mute/input defect

- [x] 6.1 Record BUG-001 and preserve failed experiments/evidence; withdraw post-complaint control/gain/host diagnostic changes at user request.
- [ ] 6.2 Fix BUG-001 in a future authorized iteration and validate actual speech capture, mute recovery and stable control.

## 7. 你好小智 and Mac shortcut

- [x] 7.1 Replace trigger with Shift+Option+Command+S; verify modifier/key press, retry, release and input-report readback, then cross-build.
- [x] 7.2 Select bundled wn9_nihaoxiaozhi_tts, reject missing/wrong model, package and validate build; acoustic verification is tracked in 2.1.
- [ ] 7.3 Flash and validate the chord with the configured Mac application shortcut; source/build success is not device acceptance.

## 8. Wake path diagnosis

- [x] 8.1 Add read-only inference/detection/epoch/HID/suppression and bounded-freshness PCM peak telemetry; native test and cross-build without restoring deferred tuning/control repairs.
- [x] 8.2 Flash diagnostic build and correlate controlled speech/BOOT tests with counters: one detection/one press, continued inference and repeated control-driven epoch invalidation; not a complete acoustic root-cause proof.

## 9. Authorized control repair

- [x] 9.1 Restore bounded command settling/backoff and staggered housekeeping only, retain wake telemetry, test and build.
- [ ] 9.2 Flash, measure control errors/epochs, and verify repeated spoken wake and HID delivery; device observed 6 detections/6 accepted presses in 90s with zero error/epoch growth, Mac application response awaiting user confirmation. Do not conflate this with deferred AGC/acoustic repair.

## 10. LED visibility and diagnostic follow-up

- [x] 10.1 Add shared, bounded host diagnostic sampling and read-only AGC freshness telemetry; verify tests, build and device identity without AGC tuning.
- [x] 10.2 Restore brightness after effect transitions, explicitly disable Gamma for brightness30, reconcile overridden effect/speed/brightness/Gamma without periodic animation restart; native tests, build and flash verified.
- [x] 10.3 Record user confirmation of normal LED behavior after white-breathe/rainbow acceptance prompt; retain BUG-001 and broader acoustic acceptance as open.
