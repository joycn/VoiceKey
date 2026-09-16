> SUPERSEDED by `migrate-respeaker-xvf3800`. Retained as historical Voice PE planning/evidence; not current hardware instructions.

## Purpose

保留 Voice PE 的物理麦克风静音语义，在本地检测与 USB 音频同时工作时确保静音优先于软件操作，并清除尚未送出的音频和触发以避免解除静音后的历史重放。

## ADDED Requirements

### Requirement: Physical mute dominates
设备 SHALL 在硬件静音开启时发送零值 USB 音频、停止有效唤醒和新 HID 按下，并以红灯显示静音；软件设置不得解除硬件静音。

#### Scenario: Mute during queued audio
- **WHEN** 音频与唤醒事件排队时开启静音
- **THEN** 清除设备侧尚未发送的音频与事件，下一次传输输出零样本；已送达主机的数据不可撤回

#### Scenario: Mute during key down
- **WHEN** 已发送 F18 按下后开启静音
- **THEN** 仍完成按键释放，但不开始新的按下

### Requirement: Fresh unmute
设备 SHALL 在解除静音后只处理新的 PCM，不复用静音前的唤醒模型上下文或排队事件。

#### Scenario: Resume microphone
- **WHEN** 用户解除物理静音
- **THEN** USB 枚举保持不变，旧缓存不重放，检测从新音频重新开始
