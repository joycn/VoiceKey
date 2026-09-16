> SUPERSEDED: pre-review software evidence and hashes. Later ISR/transport/diagnostic fixes changed firmware; these images do not validate current source.

# XVF3800 迁移验证记录

日期：2026-09-16。当前变更：`migrate-respeaker-xvf3800`。唯一目标：reSpeaker XVF3800 USB 4-Mic Array + 标准 XIAO ESP32S3。

**软件实现、native检查、8 MB交叉构建和镜像容量检查通过；无硬件，未烧录，设备/AEC/声学/目标词验收未完成。** 旧Voice PE结果原样保留并标记superseded于`docs/history/voice-pe-validation-2026-09-07.md`，不能作为当前设备证据。

## 本轮实际执行

| 命令 | 结果 | 直接覆盖范围 |
| --- | --- | --- |
| `./scripts/test.sh` | PASS，exit0，UBSan | 核心10组/50,000次随机FIFO；真实FIR分块/重置/饱和/声道；协议短回复/状态/超时/busy3次；实际控制poll的VERSION48/0、GPO5-byte/index1、版本不匹配、I2S inactive、故障恢复、写命令allowlist无unmute；播放生命周期与±1000ppm/DMA48-frame量化反馈模拟；实际runtime.c的>100ms关闭、新wake拒绝、旧epoch丢弃、DMA静音独立、停止/快速重启/断连清理、诊断身份；真实镜像validator隔离fixture拒绝16MB/UART缓存、超大应用、重叠/越界分区和错误image header |
| `./scripts/test-usb.sh` | PASS，exit0，UBSan | 实际USB回调：16k/48k clock、mic mute、speaker音量/静音和无效控制、OUT post-read、短/坏包、alternate、suspend/resume、断连、F18发送失败/过期、完整64byte Feature Report及class buffer容量、手动feedback参数初始化 |
| `./scripts/test-descriptors.sh` | PASS，exit0，UBSan | 实际283-byte UAC2+HID配置；4接口、mic IN/speaker OUT/显式feedback/HID端点、两路feature unit、s16mono/stereo格式和字符串 |
| `./scripts/build.sh` | PASS，exit0 | ESP-IDF5.5.2完整编译/链接；默认build入口自动执行image validator，flash入口在任何写硬件前先构建/验证；真实8MB配置、无UART0/led_strip、模型镜像生成、WHOLE_ARCHIVE及missing-prototypes检查 |
| `./scripts/validate-images.py` | PASS，exit0 | 实际sdkconfig/8MB镜像header、二进制分区无重叠、NVS/PHY、4MB app、3MB model及完整flasher映射容量，无OTA |
| `./scripts/verify-xmos.py /tmp/application_xvf3800_i2s_master_v1.0.8_48k.bin` | PASS，exit0 | 从固定官方commit下载到本机临时文件，实际888832字节及SHA256一致；没有写硬件 |
| `openspec validate migrate-respeaker-xvf3800 --strict` | PASS，exit0 | 当前proposal/design/specs/tasks结构校验 |
| Python导入`scripts/diagnose.py`，构造64/65byte及短回复 | PASS | schema解析、XMOS版本、ELF hash前缀、拒绝短报告；未连接HID设备 |
| `git diff --check` | PASS | 空白格式检查 |

FIR测试使用生产代码实际float系数，0–24kHz以5Hz网格计算：0–6.5kHz通带波纹 **0.000744 dB**，8kHz起最小阻带衰减 **86.472 dB**。阈值分别为≤0.2dB和≥60dB。这是数字频率响应检查，不是麦克风/扬声器实测。

最终ELF通过`xtensa-esp32s3-elf-nm firmware/build/voicekey.elf`确认以下为应用强符号`T`：`tud_audio_set_itf_close_EP_cb`、`tud_audio_rx_done_post_read_cb`、`tud_audio_feedback_params_cb`、`tud_hid_get_report_cb`。OUT数据在TinyUSB复制入FIFO后的post-read取走；HID class buffer64避免真实GET_REPORT截断；manual feedback参数初始化且由USB任务持续更新，不依赖未启用的SOF ISR。

## 构建产物

实际配置：ESP32-S3，8 MB Flash、Octal PSRAM、240 MHz、无运行UART控制台。工具链：IDF5.5.2，Xtensa GCC14.2.0/esp-14.2.0_20251107，Python3.11.16；TinyUSB0.18.0~6、ESP-SR2.2.0，完整锁定见`firmware/dependencies.lock`。本机日志`firmware/build/build-evidence.log`。迁移前旧sdkconfig已备份后再生成，未复用16MB缓存。

| 文件（相对firmware/build） | Flash偏移 | 字节 | SHA256 |
| --- | --- | ---: | --- |
| bootloader/bootloader.bin | 0x0 | 22464 | 85ab3f61e5d257cb9d1be152ae3a0e2111460b407df352991f8e60456765c71d |
| voicekey.bin | 0x10000 | 409872 | b3111fd2c64c789ad2f46f25956820841628d2eedfc5d743a139b0f91330ef00 |
| partition_table/partition-table.bin | 0x8000 | 3072 | 2b20bd22ecb09964c233a82504470c6818863aea98e4250ea4321d9e88fe8a10 |
| srmodels/srmodels.bin | 0x410000 | 291142 | 2a162e78cb30938d9c55172df2a1b0f9bee307f8ee27ddf90e3ffbcd8ece736a |

应用0x64110字节，小于4MiB，剩余0x39bef0；这是Flash余量，不是运行堆余量。模型实际仍为`wn9_hiesp`。NVS/PHY保留，模型分区末端0x710000，小于8MiB。

XMOS官方image pin：commit `a652fe79da3a292b25decc0e1e7f267d29bb0284`，`xmos_firmwares/i2s/application_xvf3800_i2s_master_v1.0.8_48k.bin`，888832字节，SHA256 `d60d0bc2c7f5a67ffa9c9206e066b2d8f1ccb49ebba673f3a7bcff1cf197dfb2`。manifest受版本控制，维护只走单独XMOS USB口；本轮只下载校验，没有自动刷写。

## 仍待完成

- [ ] 标准XIAO/reSpeaker实物版、8MB PSRAM、引脚/主从时钟/左右格式、连续I2S、恢复方式。
- [ ] 实际macOS USB双向枚举、feedback、音量/静音、HID64byte诊断与F18；native shim不模拟USB控制器/FreeRTOS/ISR时序。
- [ ] 硬件静音、I2C故障和>100ms缓存过期、RX溢出、真实主机停顿、快速重连、DMA尾音及长运行。已送入外部DAC或主机的样本不可撤回；软件清理只控制尚未发送的数据。
- [ ] reSpeaker3.5mm有源音箱真实播放、AEC参考/残余回声/双讲打断；接到其他输出设备的音箱不自动享有该参考。不作高保真或AEC已成功声明。
- [ ] 1–3米声学/唤醒率/误触发、短暂停顿与ChatGPT首句完整性；蓝灯只表示检测，不表示应用就绪。
- [ ] Hey Chat/Hello Chat兼容授权模型与验收；Hi ESP只是开发词，这项还需要模型开发依赖，不能只归因于没有硬件。
- [ ] USB电源/挂起功耗及量产VID/PID/模型许可。

本机ASan在空程序启动时崩溃，因此只记录UBSan成功。依据`docs/hardware-validation.md`执行后填写`docs/device-test-record.md`，保留原始WAV、诊断与环境；不能把软件检查勾成硬件通过。
