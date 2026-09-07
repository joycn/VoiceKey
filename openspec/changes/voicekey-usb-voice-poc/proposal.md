## Why

VoiceKey 要验证在不自研 PCB 的前提下，使用 Home Assistant Voice Preview Edition 为 Mac 提供远场麦克风和本地语音触发入口。依据 BrainStorm.md，用户允许唤醒后短暂停顿，且 Mac 已解锁、保持唤醒、ChatGPT 已登录并在后台运行。

## What Changes

- 建立可构建的 ESP-IDF ESP32-S3 固件，保留现有 XMOS 固件，接入板级初始化、PCM、按钮、灯环和硬件静音。
- 单根 USB 提供持续枚举的 16 kHz / 16-bit / mono UAC 麦克风及 HID 键盘，唤醒默认发送 F18。
- 同源音频分发到 USB 与本地唤醒引擎，包含有界缓存、溢出恢复、重复触发抑制及静音清理。
- 提供可复现构建、逻辑自动测试与分阶段实机验收手册，明确代码验证与硬件验证边界。
- 先通过引擎自带模型验证真实本地检测；目标词 Hey Chat / Hello Chat 的模型可用性独立记录，不将其他词的测试冒充目标词通过。

## Capabilities

### New Capabilities

- `voicekey-audio`: Voice PE 音频获取、同源分发与常驻 USB 麦克风。
- `voicekey-trigger`: 本地唤醒、按钮触发、HID 事件与反馈状态。
- `voicekey-mute`: 物理静音优先与缓冲、事件清理。
- `voicekey-validation`: 可复现构建、主机配置和分阶段验收证据。

### Modified Capabilities

无；当前项目没有已有产品规格或固件。

## Impact

新增 firmware、tests、scripts 和 docs；引入 ESP-IDF、TinyUSB、ESP-SR 和灯环驱动依赖。主机只使用标准 USB 音频与键盘接口；不开发驱动、后台守护程序、云端唤醒或 ChatGPT API 客户端。USB Audio OUT、设备端完整 AEC 和量产硬件不在本次实现范围。

用户当前没有 Voice PE，烧录、USB 枚举、真实录音、远场、回声、目标词及 ChatGPT 闭环验收必须保持待验证，不能因编译或单元测试通过而标记完成。
