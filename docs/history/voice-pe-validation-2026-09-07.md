> SUPERSEDED: historical Voice PE evidence only. Does not validate the XVF3800 migration.

# 本轮验证记录

日期：2026-09-07。OpenSpec 变更：`voicekey-usb-voice-poc`。

**状态：基础固件实现与本机检查完成；目标词模型开发依赖和实机验收未完成。** 用户确认暂时没有设备；本轮没有烧录、录音或操作主机应用。

## 已执行的检查

| 检查 | 结果 | 范围 |
| --- | --- | --- |
| `./scripts/test.sh` | PASS，10 组 | 实际核心 C 模块，UBSan；50,000 次随机 FIFO 对照；60 秒模拟 ±10 samples/s 时钟偏差；静音、停读、失效推理、断连、HID 脉冲与冷却 |
| `./scripts/test-descriptors.sh` | PASS | 实际 166-byte UAC2 + HID 描述符，拓扑、接口、端点、字符串、控制声明及 PCM 格式 |
| `./scripts/test-usb.sh` | PASS | 实际 USB 回调与核心状态机；采样率/静音控制、物理静音、流开关、暂停恢复、HID 传输失败及事件过期；仅替代调度和传输 |
| `./scripts/build.sh` | PASS，exit 0 | ESP32-S3 源文件编译、最终链接、应用分区容量检查、完整烧录参数与真实模型镜像 |

构建过程中已修复 USB PHY 所属组件依赖、TinyUSB 描述符配置和静态库回调链接问题。最终应用镜像为 0x6a300 字节，4 MiB 应用分区剩余约 90%；这不是运行时堆内存余量。

工具链：ESP-IDF v5.5.2（`30aaf64524299d3bde422ca9a2848090d1bc5d0f`），Xtensa GCC 14.2.0 / esp-14.2.0_20251107，Python 3.11.16，CMake 3.30.9，Ninja 1.13.2。组件版本与哈希见 `firmware/dependencies.lock`。配置为 ESP32-S3、16 MB Flash、Octal PSRAM、240 MHz CPU。

最终构建日志保存在本机 `firmware/build/build-evidence.log`。应用配置启用 QIO；ESP-IDF 生成的烧录参数使用 DIO 引导模式，应遵循生成参数，不手工改写。

本轮补充修复：状态任务提前启动，避免 XMOS/模型初始化阻塞静音灯更新；修正 TinyUSB `tud_audio_set_itf_close_EP_cb` 的大小写契约。最终 ELF 已确认该回调为应用强符号（`T`），USB 组件开启缺少函数原型即编译失败的检查。新增 USB 测试使用真实 TinyUSB 头文件并直接调用该回调。

## 构建产物

路径相对于 `firmware/build/`。本次镜像 SHA-256 用于识别产物，重建后应重新记录。

| 文件 | Flash 偏移 | 字节数 | SHA-256 |
| --- | --- | ---: | --- |
| `bootloader/bootloader.bin` | `0x0` | 22,496 | `3f4725b6b65ac01c435892c3ef13a436742467126dae29a557dc6aac16b0d1b5` |
| `voicekey.bin` | `0x10000` | 434,944 | `e3ea09a45dca9b62dde37d2a3c38e48dbafe57cd29fd97579af2f96c272e0011` |
| `partition_table/partition-table.bin` | `0x8000` | 3,072 | `e6ba778248a208cdfe13393b383815ab9be5e0f38d36cf5677915cabb5d170f3` |
| `srmodels/srmodels.bin` | `0x410000` | 291,142 | `2a162e78cb30938d9c55172df2a1b0f9bee307f8ee27ddf90e3ffbcd8ece736a` |

`srmodels.bin` 实际包含 `wn9_hiesp`，模型信息为 `wakeNet9_v1h24_Hi,ESP_3_0.63_0.635`。这是 **Hi ESP**，不是 Hey Chat / Hello Chat。完整刷写必须包含全部四个镜像，不能只写 `voicekey.bin`。

## 未完成及限制

- 目标词模型取得、授权和识别质量验收（获取途径及接入准备见 `docs/wake-word-model.md`，不是仅待硬件）；当前未获得用户将 Hi ESP 作为正式产品词的同意。
- 实物板版本/USB 通路、XMOS PCM 格式、PSRAM、UAC/HID 枚举、硬件静音及恢复。
- 长录音与真实时钟漂移、任务调度和模型吞吐；1–3 米音质、唤醒率、误触发和扬声器回声。
- 目标 Mac/ChatGPT 的 F18 绑定、后台触发、重复触发语义和短暂停顿后的首句完整性。
- 系统 AddressSanitizer 连空 main 程序也在启动时崩溃；本机仅提供 UBSan 通过证据。此限制不能代替内存安全验收。

按 `docs/hardware-validation.md` 执行后填写 `docs/device-test-record.md`。以上检查不构成产品可靠性、USB 功耗合规或实机验收通过。

## 开发停止点核查

已复核当前源文件、测试覆盖、最终构建日志和四个镜像的实际 SHA-256；产物与上述记录一致，应用镜像晚于当前应用及组件源文件。没有将旧构建结果用于新代码。

| 要求 | 当前直接证据 | 尚需证据 |
| --- | --- | --- |
| 同一连接常驻 UAC2 + HID、16 kHz/mono/s16 | 实际描述符及控制回调测试、最终 ELF | Voice PE 上的 macOS 枚举与录音 |
| 同源、独立消费者、有界缓存、恢复不重放 | 实际核心模块测试、I²S 分发与 DMA 重启实现 | 实际 PCM 格式、调度、长录音 |
| 输入失败输出静音 | 音频状态门控测试、板级错误传播和保留 USB 的启动顺序 | 断开/异常 XMOS 的设备行为 |
| 物理静音优先、释放按键、清理上下文 | 核心与 USB 回调测试、发送前 GPIO 检查、模型 epoch 清理 | 物理开关、在途样本和指示灯响应 |
| 本地真实模型、报告实际词 | `wake_engine.c`、真实 `wn9_hiesp` 分区 | 上板推理；目标词模型尚未取得 |
| F18 脉冲、冷却、断连/暂停丢弃旧事件 | 核心与 USB 发送失败测试、最终正确回调强符号 | 总线时序及目标应用行为 |
| 检测灯不宣称应用就绪 | 独立状态任务与 LED 实现、使用手册 | 实际灯效、用户停顿与首句 |
| 固定依赖、构建入口、恢复及验收交付 | 锁文件、三套测试入口、成功构建、验收模板 | 烧录恢复、声学和完整产品验收 |

OpenSpec 保持 11/15，未归档。要继续任务 5.2–5.4，需要 Voice PE 与可传数据的 USB 连接；任务 5.1 还需要兼容目标词模型，或用户明确接受其他正式词。现有 Hi ESP 可以先开展基础设备调试，但不能据此判定目标词开发完成。基础固件开发的可验证结果已交付，整体目标因上述外部依赖保留未完成状态。
