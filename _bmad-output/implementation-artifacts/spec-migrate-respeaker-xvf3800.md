---
title: 'reSpeaker XVF3800 + XIAO ESP32S3 migration'
type: feature
created: '2026-09-16'
status: done
route: dispatch
review_loop_iteration: 0
baseline_commit: 3c0d429de3fdb6840a8ca17465a71baedd53b2cd
context: []
---

<frozen-after-approval reason="User explicitly approved and requested full implementation">

## Intent
Replace Voice PE with reSpeaker XVF3800 USB 4-Mic Array + standard XIAO ESP32S3 as the only firmware target. Implement firmware, tests, OpenSpec and all current product/technical documents. Same XIAO USB cable carries microphone, ChatGPT playback and Shift+Option+Command+S keyboard, without resident Mac software. Default 3.5 mm active speakers connect to reSpeaker. Mic mute never stops playback.

## Boundaries & Constraints
- XIAO: 8 MB Flash, 8 MB Octal PSRAM, IDF 5.5.2, TinyUSB 0.18.0~6, ESP-SR 2.2.0; remove led_strip. NVS/PHY retained; app 4 MB, model 3 MB; validate partition/image bounds; no OTA.
- Pin official XMOS application_xvf3800_i2s_master_v1.0.8_48k.bin, source commit and SHA256. XVF3800 master / XIAO slave, continuous duplex 48 kHz 32-bit stereo. BCLK8 WS7 RX43 TX44; I2C SDA5 SCL6 addr0x2C. No old reset/reference/LED/power GPIO bindings, no UART0 console. XMOS USB only separate maintenance/recovery, never auto-flash.
- Version resource48 command0. Explicitly set and read back left processed auto-select beam (6,3). Implement status/short response/timeout/busy64 finite retries. Dedicated I2C control task polls20ms; failed read or cached state older100ms closes capture and new wake. GPO_READ_VALUES X0D30 owns mute, never write unmute. BOOT0 development trigger only. XMOS LED red mute/blue detection/fault; no direct LED GPIO.
- Stateful anti-alias FIR decimate3 from left48k to16k then s16, one result feeds independent USB/WakeNet queues. Passband0–6.5k ripple≤0.2dB, stopband≥8k attenuation≥60dB. Mute/inputfailure/recovery reset FIR, queues and inference epochs.
- USB IN16k16bitmono; USB OUT48k16bitstereo converted to32bit I2S into XVF3800 for playback and AEC reference. Async OUT explicit feedback uses actual I2S consumption plus bounded queue water level, not fixed nominal rate. Bounded independent capture/playback/wake paths. Stop/underflow/disconnect output zeros and discard historical audio. Playback volume/master mute before XMOS. No hi-fi claim.
- HID Feature Report returns version, board state, buffers and error counters via existing endpoint0; provide on-demand diagnostic script. No CDC/extra endpoints.
- 你好小智 uses bundled wn9_nihaoxiaozhi_tts; on-device recognition remains pending. Short pause after wake remains required. First XIAO flash write verified but application USB enumeration unresolved; never conflate compile/tests with AEC/acoustic/device validation.

## I/O & Edge-Case Matrix
| Scenario | Expected behavior |
|---|---|
| I2C success/busy/short/timeout/stale | Validate replies, bounded retry, fail closed on failure or age>100ms |
| Mic mute/fault/recovery | Silence capture, invalidate stale inference, fresh samples on recovery; playback continues |
| Playback stop/underflow/overflow/reconnect | Bounded latency, silence shortages, no replay of old audio |
| USB duplex/alternate/suspend/resume/control | Correct descriptors/callbacks, feedback, volume and mute; HID independently available |
| Clock drift and host stalls | Bounded queues and measured feedback; wake not blocked |
| FIR chunk/reset/saturation/response | Continuous chunk equivalence, reset clears history, saturates safely, meets frequency limits |
</frozen-after-approval>

## Code Map
- firmware/main/{board,runtime,audio_input,app_main,wake_engine}.[ch]: old GPIO/I2S replaced; retain WakeNet model interface and generation checks.
- firmware/components/voicekey_core: portable bounded FIFOs and trigger FSM reusable. Extend or add portable DSP/playback/protocol components for actual production-path testing.
- firmware/components/voicekey_usb: actual TinyUSB descriptors/device callbacks, native tests with tests/usb_shim. Preserve WHOLE_ARCHIVE and missing-prototypes checks.
- scripts/{test,test-usb,test-descriptors,build}.sh and tests/: extend existing executable host tests; UBSan works, ASan runtime on this Mac crashes even empty programs.
- BrainStorm.md, README.md, docs/*, openspec/config.yaml: current documents must use new board. openspec/changes/voicekey-usb-voice-poc and old evidence: retain and mark superseded.

## Tasks & Acceptance
- [x] Implement complete new board, FIR/full duplex audio, safe cache/epoch/control handling.
- [x] Implement UAC microphone/speaker/feedback/control and HID diagnostics with executable actual-callback tests.
- [x] Add protocol, FIR response, buffers/drift/failure lifecycle tests; run all native tests and cross-build with actual 8 MB configuration.
- [x] Create openspec/changes/migrate-respeaker-xvf3800 proposal/design/specs/tasks and validate; update all current docs/defaults, pin XMOS artifact, record fresh firmware hashes and hardware-pending checklist.
- [x] Remove obsolete hardware dependencies; validate partitions and images. Hardware and target-word tasks stay unchecked.

## Implementation Notes
User plan is already approved. No unresolved intent questions, no hardware writes. Implementation choices may be resolved from authoritative sources; do not omit requirements for convenience. Complete all software work autonomously.

Official source: https://github.com/respeaker/reSpeaker_XVF3800_USB_4MIC_ARRAY commit a652fe79da3a292b25decc0e1e7f267d29bb0284; pinned image xmos_firmwares/i2s/application_xvf3800_i2s_master_v1.0.8_48k.bin, 888832 bytes, SHA256 d60d0bc2c7f5a67ffa9c9206e066b2d8f1ccb49ebba673f3a7bcff1cf197dfb2. Official python_control/xvf_host.py is command-map authority: VERSION(48,0,3 bytes), GPO_READ_VALUES(20,0,5 uint8 values X0D11/X0D30/X0D31/X0D33/X0D39), AUDIO_MGR_OP_L(35,15,2 uint8), OP_R(35,19,2), I2S_INPUT_PACKED(35,10,1), I2S_INACTIVE(35,24,1), LED_EFFECT(20,12,1), LED_COLOR(20,16,1 uint32 LE). Read packet [resource,command|0x80,payload_length+1]; status0 success,64 busy. X0D30 high means muted. Do not assume wiki's older bit-packed examples. Official clock explanation https://wiki.seeedstudio.com/respeaker_xvf3800_xiao_home_assistant/ ; other board pages xiao_i2s, xiao_gpio, xiao_rgb and respeaker_xvf_3800_i2c_list on wiki.seeedstudio.com. Verify write-response protocol in official source before implementing.

Build environment: scripts/build.sh; IDF /Users/joy/esp/esp-idf-v5.5.2; uv Python3.11 installed. Existing ignored firmware/sdkconfig caches old16MB/UART/led_strip: regenerate safely to ensure defaults take effect. Do not flash. Existing goal relates to old hardware and is irrelevant to software completion. Do not install skills or recreate git. No commit/push required.

## Spec Change Log

## Review Triage Log


Review pass 1 (three independent reviewers; implementation fixes retain the approved intent):

| Finding | Verdict | Evidence and disposition |
| --- | --- | --- |
| Blind1 build override preflight | high | build.sh preflight drops -B/-C/-D while flash forwards them; reject unsupported artifact/config overrides and regression-test wrapper. |
| Blind2 optimized Python checks | medium | Both operational validators rely on assert, removed by PYTHONOPTIMIZE; replace with explicit checks and negative optimized runs. |
| Blind3 partition metadata | medium | Validator unpacks but ignores type/subtype/flags and duplicate labels; validate production partition metadata/integrity, test corrupt fixtures. |
| Blind4 overflow admission delay | medium | rx_overflow only sets atomic flag; concurrent model completion can reach runtime before capture task sets audio_ok(false). Close gates in ISR-safe path, test interleaving. |
| Blind5 stopped playback clock | medium | RX timeout closes only capture while playback retains queue and DMA; add independent transport-fault clearing/recovery and tests. |
| Blind6 LED cache after reset | medium | last_led is not invalidated by controller configuration loss; reapply after reconfiguration and test. |
| Blind7 coherent board health | medium | Cache publishes healthy before failed LED retry; publish one coherent result after required cycle operations and test persistent failure. |
| Blind8 adapter coverage | medium | Runtime test calls fault setter directly; actual audio and board task adapters are absent from native builds. Add injected-driver tests. |
| Blind9 diagnostics observability | medium | UART disabled; readiness/init errors/actual PSRAM and heap only logged or unexposed. Add versioned EP0 diagnostics fields and decoder tests for hardware acceptance. |
| Blind10 IDF pin absent | false | firmware/main/idf_component.yml requires idf ==5.5.2 and dependency resolution enforces it; IDF_PATH selection does not bypass this constraint. |
| Edge1 first feedback format | high | TinyUSB audio_device.c calls set_itf callback at1947 before initializing format correction at1950; initial fb_set transmits default4-byte format. Defer until USB task. |
| Edge2 build override | high | Independently verified same preflight/flash mismatch as Blind1; covered by same fix, retained as separate finding. |
| Gap1 actual overflow callback | medium | Tests bypass actual callback registration/loop, so deleting registration leaves green tests; add actual adapter test. |
| Gap2 periodic feedback | medium | USB task is only run after playback stopped; direct helper drift tests cannot catch removed periodic feedback. Exercise task-emitted feedback under drift. |

All verified findings were corrected and covered by the final passing suites; no intent gap or change to the frozen requirements. IDF dependency pin finding was rejected with direct manifest evidence. Hardware timing/acoustics remain pending as agreed.

## Verification

Final root verification: full native suites, actual board/audio adapters, USB task feedback simulation, descriptor tests, optimized Python validator/wrapper/diagnostic tests, ESP-IDF build with automatic image validation and strict OpenSpec all passed. Final app410800 bytes, SHA2566b7bb60c131a88d977441cb76212ca90476be9ccceb87c8120924f3fa8369761. Hardware/AEC/acoustics/target-word remain pending in OpenSpec.

Focused follow-up review found no remaining capture-generation, independent mute/RX or first-feedback defects. One confirmed timing boundary is documented and modeled: before software detects a short clock interruption, DMA can transmit prefilled samples before its completion callback. Verdict: medium documentation/verification correction; no zero-tail claim for undetected clock gaps. Three48-frame DMA blocks and downstream DAC tail require hardware measurement; detected faults/USB stops clear queued and pending DMA data. This does not claim device acceptance.
Run scripts/test.sh, scripts/test-usb.sh, scripts/test-descriptors.sh and all added native tests; scripts/build.sh; partition/image capacity validator; openspec validate migrate-respeaker-xvf3800 --strict. Record exact commands/results and SHA256 in docs/validation-status.md. Keep separate executable/evidence assertions and hardware assumptions.

Current review follow-up: playback also waits for measured48k qualification, and clears stale data on loss/recovery. Separate internal-RAM USB Serial/JTAG maintenance firmware and staged acceptance are defined in docs/maintenance.md and docs/playback-aec-contract.md. Earlier build hashes below are historical; latest evidence is in docs/validation-status.md.
