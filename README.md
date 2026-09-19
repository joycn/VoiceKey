# VoiceKey

reSpeaker XVF3800 USB 4-Mic Array + **标准 XIAO ESP32S3** 固件原型。唯一日常连接是 XIAO 的 USB-C：同时提供麦克风、ChatGPT 播放和 Shift+Option+Command+S 键盘。默认使用接在 reSpeaker 3.5 mm 接口上的有源音箱；无需常驻 Mac 软件。

目标唤醒词为 **你好小智**，已选择内置 `wn9_nihaoxiaozhi_tts` 模型，触发 **⇧⌥⌘S**。尚未烧录及验证识别效果；说完唤醒词后仍需短暂停顿。静音/低音量问题见 BUG-001，仍待修复。

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

Mac 需保持唤醒且已解锁，ChatGPT 已登录并后台运行；事先配置输入、输出、权限和 Shift+Option+Command+S。详见[实机清单](docs/hardware-validation.md)。

## 官方资料复核后的运行检查

启动时核对 XMOS 1.0.8、INT 模式和音频格式，并在至少250 ms的实测窗口确认48 kHz速率后才开放采集、新唤醒和播放。输出上采样显式开启；故障恢复丢弃旧音频。一次性诊断脚本现使用192字节schema3，包含构建字符串、格式、实测速率和缓存的AEC状态，须与新固件配套使用。

安装时让带Seeed标志、具有麦克风进音孔的一面朝向声源，外壳不得遮挡进音孔。默认仍使用3.5 mm有源音箱；功放关闭及conference/ASR通道对比须按[实机验收](docs/hardware-validation.md)测试后决定。软件通过情况和新产物哈希见[验证记录](docs/validation-status.md)。

USB 未枚举时使用独立的[启动维护诊断](docs/maintenance.md)；播放和 AEC 按[路径验收契约](docs/playback-aec-contract.md)逐项验证。

## 已知问题

[BUG-001：Mute恢复异常及录音幅度过低](docs/bugs/BUG-001-mute-recovery-low-input.md) 已记录并推迟修复。当前代码按首次报告问题时的行为基线恢复；后续调参与诊断实验不在当前实现中。硬件上的固件不会随代码回退而改变。
