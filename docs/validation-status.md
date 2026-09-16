# XVF3800 迁移验证记录

日期：2026-09-16。变更：`migrate-respeaker-xvf3800`。唯一目标：reSpeaker XVF3800 USB 4-Mic Array + 标准 XIAO ESP32S3。

**迁移实现、审查修复、完整软件测试、8 MB交叉构建及镜像校验全部通过。** 最终产物晚于所有固件源文件，以下哈希来自本次构建。无硬件，本轮未烧录；设备/AEC/声学/目标词仍未完成。

## 实际执行的检查

| 命令 | 结果 | 直接覆盖范围 |
| --- | --- | --- |
| `./scripts/test.sh` | PASS，exit 0，UBSan | 完整核心/DSP/协议/适配器及Python测试：10组核心测试、50,000次随机FIFO；实际FIR分块、复位、饱和和5 Hz频响扫描；控制协议、漂移及下列新增回归 |
| `./scripts/build.sh` | PASS，exit 0 | ESP-IDF 5.5.2重新编译链接；自动检查真实8 MB配置、镜像头、分区MD5/type/subtype/flags和容量，无OTA/led_strip/UART0；日志在`firmware/build/build-evidence.log` |
| `openspec validate migrate-respeaker-xvf3800 --strict` | PASS，exit 0 | 新迁移proposal/design/specs/tasks严格校验 |
| `./scripts/test-usb.sh` | PASS，UBSan | 实际SET_INTERFACE回调不提前提交feedback；随后运行捕获的USB任务必须逐次发送反馈，队列/时间/实际消费变化导致feedback变化；该任务发出的feedback驱动±1000ppm/48-frame量化漂移仿真，实际OUT回调入队且队列有界无drop/missing；原控制/mute/alternate/HID边界；schema2完整96byte Feature Report/class buffer |
| `./scripts/test-adapters.sh` | PASS，UBSan | 实际runtime.c、audio_input.c、board.c，仅替代板/OS/驱动传输。registered RX overflow ISR后、任务恢复前模拟另一核推理返回：立即拒绝、队列清除。真实capture task覆盖短7byte、合法部分块、带部分数据的timeout、epoch驱动RX重启/FIR重置；transport fault发生在read之间和期间均能恢复；时钟故障时host继续发送被丢弃，TX先于暂停RX恢复也只播放新样本。RX-only故障/物理mic mute保持正常TX。实际board task覆盖持续LED失败无短暂健康cache，以及control重配后强制重新应用LED。诊断覆盖初始化错误/ready和内存值 |
| `./scripts/test-descriptors.sh` | PASS，UBSan | 实际283-byte配置；4接口及原端点；HID Feature count96/class buffer96，键盘中断端点仍8byte；UAC拓扑、s16mono/stereo和显式feedback3byte |
| `python3 tests/test_image_validation.py` | PASS | 实际validate-images.py和verify-xmos.py在PYTHONOPTIMIZE=0/1下都拒绝错误配置/header/超大镜像；使用真实生成表结构（3072byte、MD5、FF padding），验证type/subtype/flags、重复分区、损坏checksum/表尾、越界；XMOS同大小错误hash和错误size均失败 |
| `python3 tests/test_build_wrapper.py` | PASS | 实际build.sh，IDF/validator进程全部为recording mock，无硬件写入；拒绝-B/-C/-D和长选项及配置环境覆盖；默认build调用验证，clean/menuconfig除外；preflash验证失败时不执行flash |
| `python3 tests/test_diagnostics.py` | PASS | schema2的96/97byte报告，init error/pending/ready，PSRAM/current/minimum heap、身份和fault计数；拒绝短报告/旧schema；ESP_FAIL=-1不误解成pending |
| `git diff --check` | PASS | 修改的空白格式检查 |

审查修复后已重新执行完整`test.sh`、`test-usb.sh`、`test-descriptors.sh`、默认构建及OpenSpec严格校验；退出码均为0。实际FIR系数的通带波动为 **0.000744 dB**，8 kHz起阻带抑制 **86.472 dB**，满足0.2 dB/60 dB要求。最终ELF确认USB OUT post-read、feedback params、HID GET_REPORT及capture fault ISR为应用强符号。所有native适配器使用确定性注入，不模拟真实双核抢占、FreeRTOS调度、USB控制器或声学。

## 当前实现与诊断契约

RX overflow ISR通过ISR-safe runtime锁同步关闭采集/新推理准入；fault generation使已经开始的read及旧推理无法越过恢复。capture会检测跨循环generation变化，避免运输故障在两次read之间发生后永久关闭采集。

独立TX进展监控在>5ms无DMA完成时清空播放队列/已登记DMA，并阻止host继续积累旧数据。首个恢复TX完成回调清空尚未发送的缓冲，第二个连续新完成才恢复播放准入；RX另行恢复。物理mic mute与RX-only故障不影响仍有TX时钟的播放。时钟故障检测不是瞬时：例如5 ms轮询尚未过期、6 ms恢复时，首个完成回调前硬件可能已发送预填DMA样本。软件不承诺所有短时钟中断均无尾音；测试明确覆盖这一检测前边界以及检测后的清空。硬件验收须测量轮询/ISR时延、3×48帧DMA在途数据和外部DAC尾音。USB停止/断连的显式清空仍独立执行。

board task在所有所需readback（包含LED）完成后只发布一次cycle状态；故障/重新配置使LED缓存失效，持续LED失败不会短暂放开采集。

HID Feature Report现为**schema2、96byte、固件协议release1.1**，仍只用EP0；键盘中断端点8byte不变。前64byte保留原字段排列（byte7增加wake ready/init complete/transport ready位）。64/68/72为board/audio/wake int32初始化状态，0成功、0x80000000未尝试，-1为ESP_FAIL；76/80/84为PSRAM/current free heap/minimum free heap字节数；88/92为capture fault generation/transport faults。详见`docs/build.md`与`scripts/diagnose.py`。

## 最终构建产物

配置为ESP32-S3、8 MB Flash、Octal PSRAM、240 MHz、无运行UART控制台；IDF5.5.2、TinyUSB0.18.0~6、ESP-SR2.2.0。工具链为Xtensa GCC14.2.0、Python3.11.16。以下文件均位于`firmware/build/`。

| 文件 | Flash偏移 | 字节数 | SHA256 |
| --- | --- | ---: | --- |
| bootloader/bootloader.bin | 0x0 | 22464 | 85ab3f61e5d257cb9d1be152ae3a0e2111460b407df352991f8e60456765c71d |
| voicekey.bin | 0x10000 | 410800 | 6b7bb60c131a88d977441cb76212ca90476be9ccceb87c8120924f3fa8369761 |
| partition_table/partition-table.bin | 0x8000 | 3072 | 2b20bd22ecb09964c233a82504470c6818863aea98e4250ea4321d9e88fe8a10 |
| srmodels/srmodels.bin | 0x410000 | 291142 | 2a162e78cb30938d9c55172df2a1b0f9bee307f8ee27ddf90e3ffbcd8ece736a |

应用及模型分别小于4 MiB和3 MiB分区；完整模型仍为Hi ESP。模型分区末端0x710000，处于8 MiB内。Flash余量不等于运行时堆余量。

前一轮产物记录保存在`docs/history/xvf3800-before-review-2026-09-16.md`并标记superseded，更早Voice PE结果另存`docs/history/voice-pe-validation-2026-09-07.md`；均不替代上述新构建结果。

XMOS固定artifact未改变：commit `a652fe79da3a292b25decc0e1e7f267d29bb0284`，`xmos_firmwares/i2s/application_xvf3800_i2s_master_v1.0.8_48k.bin`，888832字节，SHA256 `d60d0bc2c7f5a67ffa9c9206e066b2d8f1ccb49ebba673f3a7bcff1cf197dfb2`。此前下载/校验成功，本次增加优化模式下的负例验证，不重下载或刷写。

## 仍待完成

- [ ] 实物8MB Flash/8MB PSRAM、引脚/时钟/左右格式、连续双工、恢复方式。
- [ ] 实际macOS USB/feedback/HID96byte诊断、F18、音量/静音、长运行与功耗。
- [ ] 实际中断/跨核时序、I2C故障/LED恢复、RX overflow、clock stop/restart、快速重连和在途尾音。
- [ ] reSpeaker3.5mm有源音箱播放、AEC残余回声/双讲/打断，以及1–3米声学、误唤醒、短暂停顿首句完整性。
- [ ] Hey Chat/Hello Chat授权兼容模型与验收；Hi ESP只是开发词。

本机仅提供UBSan成功；已知ASan连空程序启动也崩溃。按`docs/hardware-validation.md`执行并填写`docs/device-test-record.md`，不能将软件测试勾成设备/AEC/声学通过。
