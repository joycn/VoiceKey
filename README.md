# VoiceKey

基于 Home Assistant Voice Preview Edition 的 ESP32-S3 固件原型：本地唤醒 + 标准 USB 麦克风 + HID 键盘。

产品设计见 [BrainStorm.md](BrainStorm.md)，实施计划见 [OpenSpec 变更](openspec/changes/voicekey-usb-voice-poc/proposal.md)。

本工程默认唤醒词为 **Hi ESP**（ESP-SR 内置模型），不是 Hey Chat。目标词模型与实机验收分别记录，不能将当前原型视为产品验收通过。

## 开发入口

```bash
./scripts/test.sh                  # 纯 C 核心逻辑 + UBSan
./scripts/build.sh                 # ESP32-S3 固件构建，不烧录
./scripts/test-descriptors.sh      # 依赖下载后检查实际 USB 描述符
./scripts/test-usb.sh              # 实际 USB 回调与核心状态，模拟传输边界
```

先按 [构建说明](docs/build.md) 安装 ESP-IDF v5.5.2 及依赖。硬件准备好后按 [实机验收手册](docs/hardware-validation.md) 核对板版本、恢复方法，再显式烧录。

## 实现边界

- USB UAC2：VoiceKey Microphone，16 kHz / 16-bit / mono；HID interface：VoiceKey，默认 F18。
- XMOS 输出同一声道分发给 USB 和 WakeNet，消费者独立；USB 停读时不累积历史音频。
- 物理静音优先；红灯表示静音，蓝灯表示有效触发，黄灯表示初始化/音频/模型故障。蓝灯不表示 ChatGPT 已开始收音。
- 默认按键保持 30 ms、触发冷却 2 秒、触发排队过期 200 ms。没有主机会话状态回传，冷却不是“已进入对话”的检测。
- 需要 Mac 唤醒且已解锁，ChatGPT 已登录并后台运行，完成权限、输入设备和快捷键配置；允许说完唤醒词后短暂停顿。
- 不修改 XMOS 固件，不写 eFuse，不安装 Mac 驱动，不使用 Wi-Fi、蓝牙、ChatGPT API 或云端唤醒。

当前验证状态见 [验证记录](docs/validation-status.md)。
