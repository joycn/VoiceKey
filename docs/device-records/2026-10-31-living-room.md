# 客厅收音基线构建记录

日期：2026-10-31。配置提交：a41a831b0450f27557b9daafbe5d7f4e5b504353。
构建包含工作区已有的未提交灯效/空间功能；本次提交只收录本次增量，保留原有修改。

完整主机回归、USB、描述符测试通过（UBSan）。读回错误/短响应及恢复覆盖七项参数。
独立审查已补齐有效 sdkconfig 检查说明和读取失败恢复用例；更细的参数失败诊断延期。

ESP-IDF 5.5.2 ARM 工具链下 scripts/build.sh 和 validate-images.py 通过。
构建命令：

```bash
IDF_TOOLS_PATH=/Users/joy/.espressif-arm64 PATH=/tmp/voicekey-build-tools/bin:$PATH ./scripts/build.sh
```

/tmp/voicekey-build-tools 为此次本机 CMake 3.30.5 / Ninja 1.11.1.3 环境，临时目录清理后需重建或改用已安装的工具。
Python 依赖因 pip 代理兼容问题改用 uv 安装；旧 Intel 工具链未删除。

应用：firmware/build/voicekey.bin，422608 字节。
SHA256：987d7e375c5fe86c6277ec51284d17e550f4ab28e72786978dd98191965899ab。
模型 SHA256：56990e6b97faeced7aeea48c89f15b0d0c579c012a8106b91e96141f490051a6。
实际配置：SPATIAL 关闭、PCM_GAIN=1、NIHAOXIAOZHI_TTS 开启。

未烧录，未实机读回参数，未验证两米覆盖、回声消除及静音后音量恢复。
现场验证见 ../living-room-audio.md。同目录 living-room-{tests,usb,descriptors,build}.log 保存证据。


## 烧录执行

用户明确授权后，通过 /dev/cu.usbmodem83301 确认 ESP32-S3、8 MB Flash / 8 MB PSRAM、MAC 68:ee:8f:46:c6:2c，与原设备记录一致。
执行 build.sh -p /dev/cu.usbmodem83301 flash 成功，四个镜像均 Hash of data verified，应用和模型哈希与上文相同。
自动 RTS 重置后仍枚举 USB JTAG/serial debug unit，尚无 VoiceKey HID；已请用户松开 BOOT 并按 RESET/重新插拔。
烧录完成，运行身份/参数和声学验收仍待确认；没有刷新 XMOS 固件。

## 启动恢复与初次运行核对

用户重启后串口日志仍为 DOWNLOAD waiting for download。对已确认设备执行 esptool --before no_reset --after watchdog_reset read_mac 后，VoiceKey HID 正常枚举。
运行 ELF 前缀 b84e0ef194d2e01b 与本次 ELF SHA256 b84e0ef194d2e01b94b3a50190951fc02b1c12305d1054f22d763b21dcc0fbb3 匹配。
初始化完成，board/audio/wake error 均为 0，麦克风未静音，采集门控开放，spatial_enabled=false，wake_ready=true。
XVF3800 1.0.8 INT，实测 47999 Hz；reference_gain=8.0，动态 AGC≈2.00165。
新固件中 board_valid=true 表示初始化官方基线写入/读回通过，不表示所有参数均有独立实时遥测。AEC 状态读回不代替声学验收。

## 30秒运行观察

{"samples": 31, "seconds": 30.163, "deltas": {"i2c_errors": 0, "transport_faults": 0, "wake_epoch": 2, "usb_dropped": 0, "usb_missing": 105, "playback_missing": 0}, "inferences": 942, "all_valid": true}

启动期 playback_missing=1680，此窗口无增长；不将它当成声学播放验收。两米唤醒、音质、双讲和静音后恢复仍待现场实测。

观察末尾30.163秒出现 usb_missing +105、wake_epoch +2。每个采样快照均有效不代表采样间无瞬态；原因未确认，不宣称连续音频验收通过。控制错误、transport_faults、capture_fault_generation 未增加。

后续10秒复查增量：{"usb_missing": 0, "wake_epoch": 0, "i2c_errors": 0, "transport_faults": 0}。
