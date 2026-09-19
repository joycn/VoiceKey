# XVF3800 迁移验证记录

> **当前设备：LED修正版已实机确认正常。** 白色呼吸/彩虹速度1、亮度30、Gamma0；增加低频参数纠正。应用529947da…、运行ELF88741b30…匹配，四段烧录哈希通过，30秒无新增控制或采集故障。用户在灯效验收提示后确认“现在功能正常了”。[记录](device-records/2026-09-19-led-gamma-reconcile.md)。

> **未解决：BUG-001静音切换尖刺及后续低音量。** 同步录音/AGC观测已复现，不能将LED验收扩大为音频验收。音箱无声经用户确认是音箱单独静音，停止该项排查。[Bug记录](bugs/BUG-001-mute-recovery-low-input.md)。

以下为按实验阶段保留的历史记录；其中“最新/当前”均指该记录产生时，现状以上述条目为准。

> **最新已恢复：白色和彩虹速度1、亮度30，应用20391d22…，ELF0853e153…匹配。** 用户实测数字增大使白色呼吸变快，按要求从3恢复1；烧录验证与实际白色参数读回通过，30秒无新增控制错误/epoch变化，推理937次。[记录](device-records/2026-09-19-breathe1-restored.md)。

> **最新运行：白色速度3、彩虹速度1、亮度30版af61365e…，ELFbe92bd4d…匹配。** 完整烧录验证通过，实际读回白色speed3/brightness30且供电开启；30秒控制错误/epoch无增长，推理938次。[记录](device-records/2026-09-19-breathe3.md)。

> **最新运行：白色/彩虹速度1、亮度30版20391d22…，ELF0853e153…匹配。** 完整烧录验证通过，LED实际参数和供电已读回；30秒控制错误/epoch无增长、推理940次、检测与USB按下各1次，视觉待确认。[记录](device-records/2026-09-19-led-1-30.md)。

> **最新运行：白色15请求＋5回退保护版f4ef3577…，运行ELF9ce10749…已核对。** 30秒无新增控制错误或epoch变化，938次推理、2次检测/USB接受按下。实际白色速度15或5尚无法由现有诊断区分，主机与视觉验收待用户确认；已替换此前速度20失败版。[记录](device-records/2026-09-19-breathe15-guard.md)。

> **最新异常：速度20固件已烧录，但控制错误持续增长，验收失败。** 30秒错误增加394、epoch增加787。速度5恢复版已构建且哈希与之前稳定版本一致，设备仍待ROM模式以完成回退。[失败记录](device-records/2026-09-19-breathe20-failed.md)。

> **最新运行：灯效减速候选141e26e6…，ELF569cc330…匹配，完整烧录校验通过。** 亮度10、白色速度参数5/彩虹3；实际速度方向和倍率待视觉确认。启动累计I2C错误1，后续30秒无增长、epoch无变化、推理937次。[候选记录](device-records/2026-09-19-led-speed-candidate.md)。

> **最新已烧录：内置白色呼吸/唤醒彩虹版a43fcadf…，运行ELF b1ae2238…已核对。** 含启动持B防误触。30秒控制错误和wake epoch无增长、推理937次，视觉与唤醒灯效切换待用户确认。[记录](device-records/2026-09-19-native-led-effects.md)。

> **2026-09-19 当前状态：用户已授权继续修复控制通信与连续唤醒。** 控制修复版27e5741d…已烧录，运行ELF318485fc…匹配，30秒观察I2C错误和wake epoch增量均0，完成推理937次。连续口述触发仍待验收；AGC/低音量问题继续保留。模型为“你好小智”，快捷键为⇧⌥⌘S。[本轮记录](device-records/2026-09-19-control-repair-wake.md)。

以下为此前按时间倒序保留的历史状态，其中“当前/最新”仅指记录当时，不代表上述现状。

> **当前决定：用户要求暂停修复并恢复首次反馈Mute问题时的代码行为基线。** 后续控制/增益/主机诊断实验已撤回，Bug保持Open / Deferred。以下过程记录均为历史证据，不代表当前实现；本轮不烧录设备。[BUG-001](bugs/BUG-001-mute-recovery-low-input.md)。

> 当前：AGC快速恢复候选实测失败，用户报告完全听不到人声；源码已撤回，上一版相同哈希固件已重新构建校验，设备回退等待下载模式及XMOS整板断电。不能宣布低音量问题解决。[失败及回退记录](device-records/2026-09-18-agc-recovery-candidate.md)。

> 当前：只读增益诊断版08beebe8…已烧录，运行身份核对通过，30秒控制/传输基线无错误增量。静音前后增益及低音量原因继续排查，尚未声学验收。[增益诊断](device-records/2026-09-18-gain-diagnostics.md)。

> 用户最新澄清：第三轮Mute可解除、独立红灯熄灭；语音备忘录录音仍偏小。静音无法解除的先前描述已更正，当前重点为输入幅度和录音质量；声学验收仍未通过。

> 最新：第三轮14d41812…已烧录，运行ELF身份核对通过。60秒内I²C错误、USB缺样/丢样无增长，61快照全部有效，时钟合格。当前固件的三轮静音、低音量、声学与长期验证仍待完成。[实机记录](device-records/2026-09-18-i2c-turnaround.md)。

> 当前设备为第二轮I²C退避固件95a3db19…：60秒错误增量为0，但3次缓存超过100ms，持续采集仍未通过。第三轮分散检查固件14d41812…已构建校验，尚未烧录。[详细证据](device-records/2026-09-18-i2c-turnaround.md)。

> 最新诊断：关闭Karabiner的VoiceKey占用并使用非独占HID后，诊断读取成功。本次Mute解除成功且读回未静音；仍实测到反复I²C错误引起的保护性采集关闭，低音量与此前静音卡住尚未解决。

> 用户实测更新：播放输出正常；Mute 静音后再次按键无法恢复，红灯持续且录音无声，静音恢复验收失败，待定位。见[故障记录](device-records/2026-09-18-mute-recovery.md)。

> 最新：定位到WakeNet clean()空指针重启并修复；分阶段镜像已被Mac枚举为16k单声道麦克风、48k双声道扬声器和HID。PSRAM启动自检通过；正式版也已烧录且四项写入校验通过，Mac已确认正式版音频及HID枚举；HID打开失败及音频稳定性仍待查。[启动修复记录](device-records/2026-09-18-wakenet-startup-fix.md)。

> 最新进展：XMOS已读回1.0.8、`inthost-lr48-sqr-i2c`；随后正式XIAO固件四项写入校验通过，但VoiceKey仍未枚举。当前安装的是正式固件，需重新进入ROM下载模式继续隔离启动故障。见[最新实机记录](device-records/2026-09-18-production-after-xmos.md)。

> 最新实机进展：XIAO 已进入下载模式，维护镜像写入校验通过且成功启动；USB串口和I²C可用。实际XMOS为1.0.4，与目标1.0.8_48k不符；正式音频仍待验证。见[维护运行记录](device-records/2026-09-18-maintenance-running.md)。

> 最新审查修复：正式应用 SHA256 `3c21c147f0a275ad658ce3d6d9bd62310208f804967016db03c1f0da10ea2864`，并新增独立维护镜像；两套构建与软件验证通过。本轮未烧录，用户暂时无法进入下载模式。以下旧产物表属于前一阶段；最新产物与检查见[修复记录](device-records/2026-09-18-review-repair.md)。

> 后续实机进展（2026-09-18）：已连接 XIAO，完成 8 MB Flash 备份及首次烧录，四个镜像写入校验通过；重启后尚未枚举 VoiceKey，正在排查。以下“无硬件、未烧录”描述仅属于此前软件验证阶段。详见 [首次烧录记录](device-records/2026-09-18-first-flash.md)。

日期：2026-09-18。变更：`migrate-respeaker-xvf3800`。唯一目标：reSpeaker XVF3800 USB 4-Mic Array + 标准 XIAO ESP32S3。

**迁移实现、审查修复、完整软件测试、8 MB交叉构建及镜像校验全部通过。** 最终产物晚于所有固件源文件，以下哈希来自本次构建。无硬件，本轮未烧录；设备/AEC/声学/目标词仍未完成。

## 实际执行的检查

| 命令 | 结果 | 直接覆盖范围 |
| --- | --- | --- |
| `./scripts/test.sh` | PASS，exit 0，UBSan | 完整核心/DSP/协议/适配器及Python测试：10组核心测试、50,000次随机FIFO；实际FIR分块、复位、饱和和5 Hz频响扫描；控制协议、漂移及下列新增回归 |
| `./scripts/build.sh` | PASS，exit 0 | ESP-IDF 5.5.2重新编译链接；自动检查真实8 MB配置、镜像头、分区MD5/type/subtype/flags和容量，无OTA/led_strip/UART0；日志在`firmware/build/build-evidence.log` |
| `openspec validate migrate-respeaker-xvf3800 --strict` | PASS，exit 0 | 新迁移proposal/design/specs/tasks严格校验 |
| `./scripts/test-usb.sh` | PASS，UBSan | 实际SET_INTERFACE回调不提前提交feedback；随后运行捕获的USB任务必须逐次发送反馈，队列/时间/实际消费变化导致feedback变化；该任务发出的feedback驱动±1000ppm/48-frame量化漂移仿真，实际OUT回调入队且队列有界无drop/missing；原控制/mute/alternate/HID边界；schema3完整192byte Feature Report/class buffer |
| `./scripts/test-adapters.sh` | PASS，UBSan | 实际runtime.c、audio_input.c、board.c，仅替代板/OS/驱动传输。registered RX overflow ISR后、任务恢复前模拟另一核推理返回：立即拒绝、队列清除。真实capture task覆盖短7byte、合法部分块、带部分数据的timeout、epoch驱动RX重启/FIR重置；transport fault发生在read之间和期间均能恢复；时钟故障时host继续发送被丢弃，TX先于暂停RX恢复也只播放新样本。RX-only故障/物理mic mute保持正常TX。实际board task覆盖持续LED失败无短暂健康cache，以及control重配后强制重新应用LED。诊断覆盖初始化错误/ready和内存值 |
| `./scripts/test-descriptors.sh` | PASS，UBSan | 实际283-byte配置；4接口及原端点；HID Feature count192/class buffer192，键盘中断端点仍8byte；UAC拓扑、s16mono/stereo和显式feedback3byte |
| `python3 tests/test_image_validation.py` | PASS | 实际validate-images.py和verify-xmos.py在PYTHONOPTIMIZE=0/1下都拒绝错误配置/header/超大镜像；使用真实生成表结构（3072byte、MD5、FF padding），验证type/subtype/flags、重复分区、损坏checksum/表尾、越界；XMOS同大小错误hash和错误size均失败 |
| `python3 tests/test_build_wrapper.py` | PASS | 实际build.sh，IDF/validator进程全部为recording mock，无硬件写入；拒绝-B/-C/-D和长选项及配置环境覆盖；默认build调用验证，clean/menuconfig除外；preflash验证失败时不执行flash |
| `python3 tests/test_diagnostics.py` | PASS | schema3的192/193byte报告，init error/pending/ready，PSRAM/current/minimum heap、身份和fault计数；拒绝短报告/旧schema；ESP_FAIL=-1不误解成pending |
| `git diff --check` | PASS | 修改的空白格式检查 |

审查修复后已重新执行完整`test.sh`、`test-usb.sh`、`test-descriptors.sh`、默认构建及OpenSpec严格校验；退出码均为0。实际FIR系数的通带波动为 **0.000744 dB**，8 kHz起阻带抑制 **86.472 dB**，满足0.2 dB/60 dB要求。最终ELF确认USB OUT post-read、feedback params、HID GET_REPORT及capture fault ISR为应用强符号。所有native适配器使用确定性注入，不模拟真实双核抢占、FreeRTOS调度、USB控制器或声学。

## 当前实现与诊断契约

RX overflow ISR通过ISR-safe runtime锁同步关闭采集/新推理准入；fault generation使已经开始的read及旧推理无法越过恢复。capture会检测跨循环generation变化，避免运输故障在两次read之间发生后永久关闭采集。

独立TX进展监控在>5ms无DMA完成时清空播放队列/已登记DMA，并阻止host继续积累旧数据。首个恢复TX完成回调清空尚未发送的缓冲，第二个连续新完成才恢复播放准入；RX另行恢复。物理mic mute与RX-only故障不影响仍有TX时钟的播放。时钟故障检测不是瞬时：例如5 ms轮询尚未过期、6 ms恢复时，首个完成回调前硬件可能已发送预填DMA样本。软件不承诺所有短时钟中断均无尾音；测试明确覆盖这一检测前边界以及检测后的清空。硬件验收须测量轮询/ISR时延、3×48帧DMA在途数据和外部DAC尾音。USB停止/断连的显式清空仍独立执行。

board task在所有所需readback（包含LED）完成后只发布一次cycle状态；故障/重新配置使LED缓存失效，持续LED失败不会短暂放开采集。

HID Feature Report现为**schema3、192byte、固件协议release1.2**，仍只用EP0；键盘中断端点8byte不变。前64byte保留原字段排列（byte7增加wake ready/init complete/transport ready位）。64/68/72为board/audio/wake int32初始化状态，0成功、0x80000000未尝试，-1为ESP_FAIL；76/80/84为PSRAM/current free heap/minimum free heap字节数；88/92为capture fault generation/transport faults。详见`docs/build.md`与`scripts/diagnose.py`。

## 最终构建产物

配置为ESP32-S3、8 MB Flash、Octal PSRAM、240 MHz、无运行UART控制台；IDF5.5.2、TinyUSB0.18.0~6、ESP-SR2.2.0。工具链为Xtensa GCC14.2.0、Python3.11.16。以下文件均位于`firmware/build/`。

| 文件 | Flash偏移 | 字节数 | SHA256 |
| --- | --- | ---: | --- |
| bootloader/bootloader.bin | 0x0 | 22464 | 85ab3f61e5d257cb9d1be152ae3a0e2111460b407df352991f8e60456765c71d |
| voicekey.bin | 0x10000 | 412128 | 1a1693d3b8bfb305d266999b51c81f06062f100910783f96a4ba47fe13bfa2c7 |
| partition_table/partition-table.bin | 0x8000 | 3072 | 2b20bd22ecb09964c233a82504470c6818863aea98e4250ea4321d9e88fe8a10 |
| srmodels/srmodels.bin | 0x410000 | 291142 | 2a162e78cb30938d9c55172df2a1b0f9bee307f8ee27ddf90e3ffbcd8ece736a |

应用及模型分别小于4 MiB和3 MiB分区；完整模型仍为Hi ESP。模型分区末端0x710000，处于8 MiB内。Flash余量不等于运行时堆余量。

前一轮产物记录保存在`docs/history/xvf3800-before-profile-checks-2026-09-18.md`并标记superseded，更早Voice PE结果另存`docs/history/voice-pe-validation-2026-09-07.md`；均不替代上述新构建结果。

XMOS固定artifact未改变：commit `a652fe79da3a292b25decc0e1e7f267d29bb0284`，`xmos_firmwares/i2s/application_xvf3800_i2s_master_v1.0.8_48k.bin`，888832字节，SHA256 `d60d0bc2c7f5a67ffa9c9206e066b2d8f1ccb49ebba673f3a7bcff1cf197dfb2`。此前下载/校验成功，本次增加优化模式下的负例验证，不重下载或刷写。

## 仍待完成

- [ ] 实物8MB Flash/8MB PSRAM、引脚/时钟/左右格式、连续双工、恢复方式。
- [ ] 实际macOS USB/feedback/HID192byte诊断、F18、音量/静音、长运行与功耗。
- [ ] 实际中断/跨核时序、I2C故障/LED恢复、RX overflow、clock stop/restart、快速重连和在途尾音。
- [ ] reSpeaker3.5mm有源音箱播放、AEC残余回声/双讲/打断，以及1–3米声学、误唤醒、短暂停顿首句完整性。
- [ ] Hey Chat/Hello Chat授权兼容模型与验收；Hi ESP只是开发词。

本机仅提供UBSan成功；已知ASan连空程序启动也崩溃。按`docs/hardware-validation.md`执行并填写`docs/device-test-record.md`，不能将软件测试勾成设备/AEC/声学通过。

## 本轮新增回归

协议测试覆盖50字节元数据、INT/UA拒绝、输出上采样和packing设置/读回、运行中格式变化及AEC读取失败后的失效关闭与恢复。实际runtime DMA回调以48帧/3 ms模拟16 kHz错误时钟，以48帧/1 ms模拟48 kHz；覆盖启动资格检查、恢复及过期AEC状态。实际audio adapter回归经过真实回调的250 ms资格窗口。schema3包含格式、构建字符串、实测速率及AEC有效性；Python拒绝旧schema、将无效AEC值输出null。

这些检查证明软件边界与故障处理，不证明运行XMOS镜像的密码学身份、物理I2S有效位或AEC声学效果。新实机项目还包括进音孔朝向、可选功放关闭与conference/ASR受控A/B。
