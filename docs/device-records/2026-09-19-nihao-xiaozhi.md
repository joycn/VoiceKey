# 你好小智模型切换

固定ESP-SR2.2.0内置wn9_nihaoxiaozhi_tts已选入生产/bringup配置。运行时精确选择并拒绝缺失/错配模型。生成的model分区只有此模型，各内部文件长度/SHA256均与固定组件源文件相同，源文件清单见docs/models/nihao-xiaozhi-manifest.json。

完整test.sh、交叉编译、8MB镜像校验通过，⇧⌥⌘S快捷键不变。

voicekey.bin: 412336 bytes SHA256 `c9cc91d627d91988f9fccfb7c525e4fc756d81f59cec24d71fcb4d3aed5913b6`

srmodels/srmodels.bin: 291042 bytes SHA256 `56990e6b97faeced7aeea48c89f15b0d0c579c012a8106b91e96141f490051a6`

已完成下述烧录，设备不再运行Hi, Joy。BUG-001控制错误/静音及低音量问题继续Deferred；目标词更换不代表修复该故障，也不代表实际语音识别或Mac快捷键验收通过。

## 烧录完成

核对ESP32S3 MAC68:ee:8f:46:c6:2c后完整写入bootloader、应用、分区表与model，四项Hash verified，命令退出0，watchdog reset。HID枚举恢复，运行ELF前缀fdc955a1875daef7匹配当前构建，wake_init_error=0、wake_ready=true，精确模型选择成功。快捷键保持⇧⌥⌘S；实际口头唤醒/主机事件尚未验收。初始I2C错误17、USB缺样285，原BUG-001仍存在。本轮临时共享HID读取未改动仓库诊断脚本。
