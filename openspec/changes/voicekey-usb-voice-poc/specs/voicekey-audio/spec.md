## Purpose

为桌面应用提供常驻标准 USB 麦克风，并将同一音频源供给本地唤醒检测，保证两种使用方式并行时可独立运行与恢复，且不因触发事件而重新枚举设备。

## ADDED Requirements

### Requirement: Persistent composite microphone
设备 SHALL 在同一 USB 连接上暴露名为 VoiceKey Microphone 的 UAC 麦克风和 HID 键盘，音频格式为 16000 Hz、16-bit、单声道，不需要自定义主机驱动。

#### Scenario: Wake while recording
- **WHEN** 主机正在录音且设备检测到唤醒词
- **THEN** 麦克风继续传输音频，设备不重新枚举，HID 可独立发送

### Requirement: Independent live audio consumers
设备 SHALL 将同一麦克风 PCM 分发至 USB 与唤醒消费者；任一消费者停读不得无限阻塞采集或另一消费者。

#### Scenario: Host pauses recording
- **WHEN** 主机停止采集后再次打开麦克风
- **THEN** 设备恢复传输当前音频，不重放停读期间缓存的历史对话

#### Scenario: Buffer limit reached
- **WHEN** 某消费者落后导致缓存达到上限
- **THEN** 设备丢弃旧音频并记录丢样统计，保持有界存储与实时性

### Requirement: Safe input failure
设备 SHALL 在无 PCM、初始化失败或音频欠载时输出静音并禁止音频故障导致的唤醒，保留诊断信息。

#### Scenario: XMOS fails to respond
- **WHEN** 板级音频初始化失败
- **THEN** USB 麦克风仍可枚举但输出静音，设备显示故障，不把失败当作有效音频
