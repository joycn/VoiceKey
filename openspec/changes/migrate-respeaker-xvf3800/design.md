## Context

Approved implementation specification: `_bmad-output/implementation-artifacts/spec-migrate-respeaker-xvf3800.md`. Hardware connected for the first flash; writes verified, application enumeration unresolved. Official command map pinned at reSpeaker commit a652fe79da3a292b25decc0e1e7f267d29bb0284. Existing WakeNet model interface, epoch checks, bounded FIFOs and Shift+Option+Command+S FSM are retained.

## Goals / Non-Goals

Goals: one XIAO USB for capture/playback/HID, independent bounded paths, fail-closed capture control, current documentation and executable software evidence. Non-goals: target-word model procurement, automatic XMOS flashing, high-fidelity claim, OTA, production CDC or resident host service. Hardware bring-up and acceptance are now in scope.

## Decisions

1. Standard XIAO ESP32S3 only:8MB Flash/8MB Octal PSRAM, IDF5.5.2/TinyUSB0.18.0~6/ESP-SR2.2.0. Retain NVS/PHY; factory4MB and model3MB, no OTA. Regenerate old sdkconfig and validate actual binary partition/image bounds, MD5/table integrity, type/subtype/flags and duplicates using explicit checks unaffected by PYTHONOPTIMIZE. The wrapper rejects build/project/config overrides so preflash validation cannot inspect a different output directory.
2. XVF3800 master and XIAO slave, BCLK8/WS7/RX43/TX44, continuous48k32-bit stereo. SDA5/SCL6/address0x2C. No old GPIO reset/reference/LED/power mappings. BOOT0 development trigger only.
3. Dedicated control task polls20ms, validates VERSION48/0=1.0.8, writes and reads back left(6,3) and unpacked I2S input. GPO20/0 returns5uint8, second=X0D30 mute. Never issue unmute command. Independent read transactions validate exact lengths/status, busy64 max3 retries, each driver operation10ms timeout. Invalid/cache>100ms disables capture/new wake. XMOS LED commands share task; red mute, blue detection, purple fault. LED cache is invalidated on configuration loss/reinitialization; each cycle publishes once after required LED readbacks, so repeated LED failure never opens capture transiently.
4. Stateful181-tap Kaiser FIR beta8.6 cutoff7250Hz operates before s16 conversion, decimates3. Both USB and WakeNet consume its same result through separate queues. Epoch changes reset FIR and RX DMA queue; registered RX overflow ISR synchronously closes runtime admission and invalidates queues/inference under the ISR lock. Capture tracks fault generations across iterations, rejects faults during reads and forces fresh recovery even when external transport faults occur between reads.
5. UAC2 has16kmono IN and48kstereo OUT with explicit3-byte FS feedback. Clock entities4/8; feature units2/6. OUT data is consumed in post-read callback, after TinyUSB fills FIFO. Manual feedback params are initialized; USB task refreshes16.16 API feedback derived from actual I2S DMA completions plus bounded water-level correction. No dependence on disabled SOF ISR. SET_INTERFACE submits no feedback: the task starts only after TinyUSB finishes initializing format correction, avoiding a premature4-byte transfer on the3-byte endpoint.
6. Playback queue960frames, target240; each TX DMA block48frames,3descriptors. Integer-only DMA callback refills just-consumed buffers after driver clear; stop/reset clears registered DMA buffers as well as queue. Shortage outputs zeros; overflow drops stale backlog. TX progress is independently monitored; >5ms without completion clears queues/DMA and blocks host accumulation. The first resumed TX completion callback clears not-yet-sent buffers, and a fresh >=250ms measured48k qualification reopens requested playback; old host packets sent while faulted are discarded, even if TX restarts before RX. Capture independently recovers its new generation. Physical/host mic mute and RX-only failure never mute a clock-healthy speaker. Master speaker -60..0dB/1dB and mute are applied before32-bit XMOS samples. External DAC/host in-flight samples already consumed cannot be recalled.
7. HID keyboard remains8-byte interrupt endpoint;192-byte schema3 Feature Report sharesEP0. Class buffer192 avoids TinyUSB GET_REPORT truncation. Report includes schema, firmware release and ELF-hash prefix, XMOS version, state, water levels and errors, plus wake readiness, board/audio/wake initialization errors, actual PSRAM/free/minimum heap and capture/transport fault counters. Not-attempted init uses0x80000000, distinct from ESP_FAIL=-1. One-shot hidapi script only.
8. Native tests execute portable production logic, actual board/audio/runtime adapters and descriptors/callbacks, using fixed TinyUSB headers. USB task-emitted feedback drives drift simulation; board LED retries, ISR-between-inference admission, partial/timeout reads and transport recovery ordering are injected. Validators are tested under optimized Python, and wrapper flash calls are replaced by recording mocks. Cross-build preserves WHOLE_ARCHIVE and missing-prototypes. Hardware USB scheduling, acoustics/AEC and Mac behavior remain separate evidence.

## Risks / Trade-offs

- Physical clocks, format and analog speaker/AEC path remain untested; no fidelity or echo-removal claim.
- Native shims do not reproduce ISR timing/USB host behavior. Clock-loss detection has latency: a short gap can resume before the next status poll, allowing prefilled DMA data to transmit before its completion callback. Callback clearing cannot retract it; no zero-tail guarantee is made for every clock interruption. Tests model the pre-callback boundary and post-detection clearing; device tests must measure the 3×48-frame DMA and downstream DAC tails.
- Control loss safely silences capture; it cannot force an LED update over a broken I2C bus. Cache freshness gates remain independent.
- Bundled 你好小智 model availability does not satisfy on-device recognition or acoustic acceptance.

## Migration Plan

Back up old ESP firmware/config, verify actual board, prepare official BOOT/RESET recovery, verify separately downloaded XMOS image against manifest. No automatic flash. Run native checks, build, validate partitions/images and OpenSpec, record hashes. Perform documented staged validation and update evidence without editing historical Voice PE results.

## Official-source review follow-up (2026-09-18)

Read the full bounded 50-byte BLD_MSG and query USB_BIT_DEPTH=(0,0) to verify INT rather than UA mode; do not invent an exact build-string allowlist without device evidence. These checks plus VERSION1.0.8 and measured48k consumption establish operational compatibility, not cryptographic identity of running XMOS firmware; the pinned image hash remains a separate maintenance check. Set/read back OP_PACKED=(0,0), OP_UPSAMPLE=(1,1), left(6,3) and I2S_INPUT_PACKED=0; periodically recheck format. Admit capture only after a >=250ms measured-rate window within +/-1% of48k; clock gaps reset qualification. Playback also requires measured48k qualification, independently of physical microphone mute. Publish full BLD_MSG, format/profile errors, measured rate and cached AEC converged/bypass/reference gain through schema3 (192 bytes). AEC snapshots at most once per second run only in the control task; failure closes capture, and invalid/stale values are labelled explicitly. Keep existing beam and amplifier defaults until hardware A/B and power tests. Add microphone inlet orientation and enclosure/speaker placement evidence.


## Review fixes and staged maintenance

Wrong-clock playback is rejected by the shared measured-rate qualification. Qualification loss clears FIFO and DMA history; recovery accepts only new host packets. Runtime detection is bounded by the 250ms measurement window, not instantaneous.

A separate maintenance project uses USB Serial/JTAG and internal RAM with no TinyUSB, I2S, PSRAM or WakeNet initialization. It repeatedly reports reset cause, flash/heap and read-only XMOS control values. This deliberately isolates basic startup and control; it does not validate PSRAM or full USB Audio. Production retains its original endpoint contract. See docs/maintenance.md for image validation, identity checks and restoration.

Playback/AEC commissioning SHALL follow docs/playback-aec-contract.md. Route, codec and reference acceptance must be supported by actual device evidence; no speculative codec writes or automatic XMOS flashing are introduced. Successful writes alone do not complete hardware tasks.


## On-device WakeNet reset repair

Staged startup reproduced ESP-SR2.2.0 wn9_hiesp clean() LoadProhibited in dl_convq_queue_bzero/model_clean, called on the worker's first epoch. New model instances are not cleaned. Once used, an instance is destroyed/recreated before inference in a new audio epoch. Reset is deferred until a full new frame exists; failed recreation drops the frame and retries with bounded delay, without stale inference. The worker alone owns this lifetime. Native tests cover this path; staged hardware verifies startup beyond model creation and macOS enumeration. Full production startup and acoustic stability remain separate checks.


## Deferred mute/input bug and baseline restoration

At user request, changes made after the first mute-recovery complaint are withdrawn from current code. Preserve earlier profile/clock checks, full-duplex audio, schema3 diagnostics and WakeNet startup repair. Restore immediate I2C response reads, bounded busy retries without added delay, batched500ms format/1s AEC checks and conservative pre-poll cache timestamps. Remove gain telemetry and shared-open host diagnostics added during this investigation. These known control/audio defects remain deferred, not fixed. Historical experiments are in docs/history/mute-investigation-design-2026-09-18.md and BUG-001. No device flashing is part of this code restoration.

## 你好小智 selection

Use ESP-SR2.2.0 bundled wn9_nihaoxiaozhi_tts and reject missing/wrong model names. Production and bringup configs disable Hi ESP and select 你好小智. Package model bytes from the fixed component, track file hashes in docs/models/nihao-xiaozhi-manifest.json, and retain Shift+Option+Command+S. No audio tuning is changed; BUG-001 remains deferred. Flashing and on-device speech/shortcut acceptance remain separate tasks.

## Wake-path observation only

For renewed investigation add extension2 in existing schema3 reserved bytes174–191. No I2C, gain, model threshold, wake reset or shortcut behavior is changed.174=2;175 flags: current inference PCM peak valid, capture gate closed, host microphone mute.176 wake epoch(u32),180 completed inference calls(u32),184 detections(u16),186 accepted HID presses(u16),188 latest inference-block absolute PCM peak(u16),190 suppressed trigger requests(u16).16-bit counters wrap modulo65536; accepted HID includes BOOT triggers and is not proof of host delivery. Peak validity requires fresh <=500ms observation, matching epoch, valid board and open healthy capture; old extension versions yield null in decoder. Counts include completed inference even if its epoch was invalidated during processing, allowing comparison with blocked triggers. No raw audio is retained.

## Control repair resumed by user

After the diagnostic trial found207 control errors and166 wake epoch changes in90seconds, user explicitly requested repair. Restore2ms task-side command settling,10ms wait between remaining busy retries(max3 attempts), one low-rate housekeeping read per steady-state poll, staged AEC snapshots and pre-safety-read clock timestamp. Retain100ms freshness closure. Keep extension2 telemetry, wn9_nihaoxiaozhi_tts and chord unchanged. No AGC writes, gain snapshot queries, or model threshold changes. Earlier baseline-restoration paragraphs are historical and superseded only for this control subset. Hardware error/epoch and repeated-wake evidence are required before claiming success.

## BOOT startup guard

The runtime developer trigger must observe a stable released BOOT input for 30ms after startup before accepting a fresh debounced press. A button held through startup or a release bounce cannot emit the shortcut. Normal runtime presses retain the existing chord and full release; ROM download behavior is unchanged.

## 灯环交互（2026-09-19）

正常就绪时白色呼吸（官方effect1），唤醒并发送快捷键时使用彩虹灯（effect2），持续5秒后恢复白色呼吸。短按B开发触发使用相同反馈。静音红色常亮和故障紫色优先；灯效不是Mac对话状态。启动B键防误触修复随本版保留。

XMOS自行执行动画；独立控制任务仅在状态变化时配置LED_COLOR/LED_EFFECT，并在初始化或故障后重设LED_BRIGHTNESS=30、白色和彩虹LED_SPEED=1（用户指定，不再使用15或速度回退），所有写入均读回校验。取消逐帧灯环写入和蓝色自定义动画。实际视觉效果与通信稳定性待烧录验收。

协议依据：https://github.com/respeaker/reSpeaker_XVF3800_USB_4MIC_ARRAY/blob/a652fe79da3a292b25decc0e1e7f267d29bb0284/python_control/xvf_host.py 。

灯环完全不亮排查增加HID extension3：保留192字节与唤醒计数，build文本限32字节，复用腾出的18字节携带LED模式/亮度/速度/Gamma、X0D33供电与回退标志。控制任务每250ms最多读一个参数，完整快照2秒过期，配置变化时作废。诊断查询不直接更改音频状态。

用户实测反馈：在当前XMOS1.0.8上，白色呼吸速度参数从1提高到3时频率变快。按用户要求恢复1；这是实测方向，不表示已确认线性倍率或完整有效范围。
