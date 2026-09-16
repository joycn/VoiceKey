# VoiceKey

reSpeaker XVF3800 USB 4-Mic Array + **标准 XIAO ESP32S3** 固件原型。唯一日常连接是 XIAO 的 USB-C：同时提供麦克风、ChatGPT 播放和 F18 键盘。默认使用接在 reSpeaker 3.5 mm 接口上的有源音箱；无需常驻 Mac 软件。

开发唤醒词为 **Hi ESP**。**Hey Chat / Hello Chat 尚未完成**。用户说完唤醒词后仍需短暂停顿；蓝灯仅表示检测成功，不表示 ChatGPT 已开始收音。无硬件，编译和测试不能证明 AEC、声学或设备验收通过。

产品设计见 [BrainStorm.md](BrainStorm.md)，迁移计划见 [OpenSpec](openspec/changes/migrate-respeaker-xvf3800/proposal.md)，结果见 [验证记录](docs/validation-status.md)。

```bash
./scripts/test.sh
./scripts/build.sh                 # 只构建，默认不烧录
./scripts/test-descriptors.sh
./scripts/test-usb.sh
./scripts/validate-images.py
openspec validate migrate-respeaker-xvf3800 --strict
```

- UAC2 输入：16 kHz、16-bit、mono；输出：48 kHz、16-bit、stereo，异步显式反馈。
- XVF3800 主时钟，XIAO 从模式，48 kHz / 32-bit stereo 全双工。左声道自动波束 `(6,3)` 经状态化抗混叠 FIR 降采样，USB 和 WakeNet 共用结果，队列独立。
- I2C 独立任务每 20 ms 查询；失败或状态超过 100 ms 时关闭采集和新唤醒。硬件静音来自 X0D30，永不写入解除静音；麦克风静音不停止播放。
- 主机播放音量/静音在进入 XMOS 前应用，播放同时作为 XMOS AEC 参考。停止、欠载和断连补零并丢弃旧数据。不承诺高保真或已验证的回声消除效果。
- 8 MB Flash / 8 MB Octal PSRAM；4 MB 应用、3 MB 模型，无 OTA、CDC、UART0 控制台或旧板 LED GPIO。
- [一次性 HID 诊断](docs/build.md)使用现有端点 0；XMOS USB 仅用于单独的维护/恢复，不自动刷写。

Mac 需保持唤醒且已解锁，ChatGPT 已登录并后台运行；事先配置输入、输出、权限和 F18。详见[实机清单](docs/hardware-validation.md)。
