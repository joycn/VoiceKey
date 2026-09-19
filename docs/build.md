# 构建、诊断与恢复

唯一目标：标准 XIAO ESP32S3 + reSpeaker XVF3800。ESP-IDF **5.5.2**、TinyUSB **0.18.0~6**、ESP-SR **2.2.0**；完整依赖固定在 `firmware/dependencies.lock`。需要 Python3.11、CMake、Ninja 及 ESP32-S3 工具链。默认 IDF 路径 `~/esp/esp-idf-v5.5.2`，可指定 `IDF_PATH`、`VOICEKEY_PYTHON`；脚本也可发现 uv Python3.11。

```bash
./scripts/build.sh
./scripts/test.sh
./scripts/test-usb.sh
./scripts/test-descriptors.sh
./scripts/validate-images.py
openspec validate migrate-respeaker-xvf3800 --strict
```

输出为 `firmware/build/` 的 bootloader、分区表、`voicekey.bin`、`srmodels/srmodels.bin`、ELF、`flasher_args.json`。NVS 0x9000/0x6000、PHY 0xf000/0x1000、应用 0x10000/4 MiB、模型 0x410000/3 MiB，末端 0x710000，小于8 MiB；没有 OTA。默认 Octal PSRAM，实物8 MiB容量仍需验证。不能省略模型分区。

旧 `firmware/sdkconfig` 会覆盖默认值。迁移时先备份再移除旧文件，重新构建生成8 MB/无UART控制台配置；默认`build.sh`成功构建后自动调用`validate-images.py`，明确请求flash时在写硬件前也会构建和验证；clean/menuconfig不运行镜像检查。为保证验证与实际构建/flash使用同一产物路径，拒绝-B/-C/-D及对应长选项、SDKCONFIG/SDKCONFIG_DEFAULTS/IDF_BUILD_DIR环境覆盖。它会检查实际配置、镜像header、二进制分区表的MD5/完整性/重复项/type/subtype/flags，以及烧录映射，拒绝16 MB缓存和led_strip依赖。验证使用显式失败检查，PYTHONOPTIMIZE=1也不能绕过。迁移前缓存已备份到本机 `/tmp/voicekey-sdkconfig-before-xvf3800`。菜单仅提供增益、冷却及开发 VID/PID；不再允许改成右声道。

## 可执行证据

`test.sh` 编译实际核心、FIR、播放和协议代码：50,000次随机FIFO操作，静音/epoch失效、分块等价、重置、饱和、5 Hz网格响应、busy/短回复/超时/状态失效，以及±1000 ppm反馈仿真（48-frame DMA量化）。实际runtime.c、audio_input.c、board.c适配测试覆盖缓存过期、registered RX overflow ISR即时关闭跨核推理准入、实际read的部分块/超时、epoch驱动RX重启、两次read之间/期间的TX时钟故障恢复、LED失败时一致缓存和恢复重新应用。TX无进展超过5ms时关闭播放准入并清空队列/DMA；首个恢复TX完成回调清空尚未发送的缓冲，至少250ms重新确认48kHz后才接收新host音频，RX另行用新generation恢复。物理麦克风静音及RX-only错误不会关闭正常运行的TX。USB任务实际发出的反馈驱动正负1000ppm仿真，SET_INTERFACE回调内部不得提前提交反馈。USB测试调用实际TinyUSB回调；描述符测试解析实际283-byte复合配置。它们替代调度和传输，不模拟USB控制器、FreeRTOS并发或模拟声学。USB组件保留WHOLE_ARCHIVE和missing-prototypes错误检查。

本机UBSan可用；ASan在空程序启动时也崩溃，不声称ASan通过。当前选择你好小智（wn9_nihaoxiaozhi_tts）模型，模型构建不代表声学验收；更新设备必须同时烧录model分区。

## 一次性诊断

`python3 -m pip install hidapi` 后，在设备已枚举时手动运行 `./scripts/diagnose.py`。脚本读取现有HID端点0的192字节schema3 Feature Report，不安装后台服务，不增加CDC/USB端点。默认 VID0xCAFE/PID0x4014仅用于开发，可传`--vid`、`--pid`。macOS共享访问已实机读取验证；先初始化hidapi，再关闭独占打开。若共享接口不可用，脚本报错，不抢占键盘。`--samples 151 --interval 0.2` 可收集约30秒JSONL，完成后退出；报告不包含原始音频。

报告为小端：byte0 schema3；1–3 XMOS版本；4 board bits(valid/muted/I2S active)；5–6 VoiceKey release1.2；7 runtime bits0–6(audio/connected/mic stream/speaker stream/wake ready/initialization complete/transport ready)；8–55为12个uint32：I2C错误、busy、缓存年龄ms、USB样本数、wake样本数、播放帧数、USB丢样、USB欠载、wake丢样、播放丢帧、播放欠载、实际DMA消费帧数；56–63为应用ELF SHA256前8字节，用于识别运行固件；64/68/72为board/audio/wake初始化错误（int32，0成功，0x80000000表示未尝试，ESP_FAIL=-1单独保留）；76为实测PSRAM字节数，80/84为当前/历史最小free heap字节数，88为capture fault generation，92为transport fault计数。每项后半部字段均4字节。HID class buffer和Feature长度均192，键盘中断端点仍8字节，EP0最大包仍64并支持多包控制传输。计数器为低32位并允许回绕。

## XMOS维护与ESP恢复

XMOS镜像pin见`firmware/xmos/manifest.json`。从该固定commit下载后执行 `./scripts/verify-xmos.py <镜像路径>` 验证888832字节及SHA256。镜像不自动下载到设备或自动刷写；日常只用XIAO USB，XMOS USB仅在用户安排的维护/恢复时连接。固件要求VERSION1.0.8并验证左beam读回，错误时静音采集；绝不以自动升级掩盖不匹配。

上板前记录板版本、原始备份、官方XIAO BOOT/RESET进入ROM下载模式的方法。需要刷ESP时，用户明确识别实际串口后使用 `./scripts/build.sh -p <实际串口> flash`，遵循生成的完整烧录参数。此轮未刷设备、未写eFuse。UART0 GPIO43/44用于I2S，无运行控制台；USB OTG与USB Serial/JTAG不能假设同时可用。出现问题通过已确认的ROM下载路径恢复备份。

### XMOS Factory Safe Mode与手动DFU恢复

以下是待硬件可用时执行的手动维护步骤；构建脚本和XIAO固件不会调用它们。先完全断开设备所有供电（包括XIAO线），按住reSpeaker的Mute键，再由靠近3.5mm插孔的XMOS USB口接回电源。红灯闪烁表示正在运行Factory Safe Mode；I2S应用自身不提供USB DFU，此时Factory模式才提供该恢复入口。[官方Safe Mode说明](https://wiki.seeedstudio.com/respeaker_xvf3800_introduction/#safe-mode)

1. Mac安装`dfu-util`后执行`dfu-util -l`，记录这块实物的serial/path，并确认枚举的`alt=1`名称为Upgrade、`alt=0`为Factory；不能照抄文档中的示例serial。
2. 使用manifest固定commit的I2S-master1.0.8_48k镜像，先运行`./scripts/verify-xmos.py <实际镜像>`。
3. 仅在实际设备身份已确认且安排维护时，使用`dfu-util -S <实际serial> -R -e -a 1 -D <已校验镜像路径>`。只写Upgrade alt1，保留Factory alt0恢复分区。该命令依据[固定commit的DFU指南](https://github.com/respeaker/reSpeaker_XVF3800_USB_4MIC_ARRAY/blob/a652fe79da3a292b25decc0e1e7f267d29bb0284/xmos_firmwares/dfu_guide.md)，此轮未执行。
4. 完成后断电重启，回到仅XIAO USB的日常连接，通过HID诊断核验XMOS1.0.8及采集有效状态。若失败，重新进入Factory Safe Mode检查身份/镜像，不能覆盖Factory来试错。

时钟故障存在检测延迟：若短中断在下一轮状态检查前恢复，DMA可能已在完成回调前送出预填样本，软件无法撤回。不能把回调中的补零当作已经发往DAC的样本为零；实机需测量3个48帧DMA块及外部DAC的尾音边界。测试区分检测前在途数据和检测后必须清空的队列。

## 配置核验与schema3扩展

启动查询VERSION1.0.8、完整50-byte BLD_MSG和USB_BIT_DEPTH=(0,0)（INT模式）。BLD_MSG仅作为原始构建元数据，不猜测其固定内容，不能证明正在运行的XMOS镜像SHA256。维护时仍必须核对manifest中的文件哈希。设置并读回左(6,3)、输入非打包、OP_PACKED=(0,0)、OP_UPSAMPLE=(1,1)；随后每500ms复查格式。格式不匹配先关闭采集，下一轮重新配置。

每个至少250ms的DMA消费测量窗口计算实际帧率，47520–48480Hz视为48k兼容（±1%用于故障识别，不是声卡精度指标）。启动和>5ms时钟中断后重新测量，未合格前USB录音输出零样本并拒绝新唤醒；播放仍由独立传输状态控制。该测量不能代替逻辑分析仪验证主从、位宽和PCM有效位。

schema3为192bytes，前96bytes布局保持，schema=3、release=1.2；扩展均为小端：

| 偏移 | 内容 |
| --- | --- |
| 96 | 错误uint8：0正常、1传输/非法响应、2版本不符、3非INT模式、4格式不符、5构建元数据非法 |
| 97 | bits0/1/2：INT模式兼容、采集时钟合格、AEC快照有效 |
| 98/100/102 | 各2bytes：L/R输出packed、upsample、USB位深 |
| 104 | uint32实测I²S帧率Hz；必须同时检查时钟合格位，值可为上次测量 |
| 108 | uint32 AEC快照年龄ms |
| 112/116/120 | uint32收敛状态、uint32旁路状态、IEEE754 float参考增益（线性） |
| 124–155 | extension3/4：有界32-byte BLD_MSG；旧扩展为50bytes |
| 156–164 | LED有效/供电/回退标志、效果/亮度/速度/gamma、uint32快照年龄 |
| 165 | extension4：bit0 AGC快照有效 |
| 166/170 | extension4：float当前AGC线性增益、uint32快照年龄ms |
| 174 | 扩展版本：当前源码4；已烧录历史版本可为3或2 |
| 175 | bit0 PCM峰值有效、bit1采集门关闭、bit2主机麦克风静音 |
| 176/180 | uint32推理代次、已完成推理数 |
| 184/186/188/190 | uint16唤醒检测数、HID按下数、有效PCM峰值、触发抑制数 |

AEC每秒最多采样一次，全部I²C在控制任务内执行；任一读取失败关闭采集并使快照无效。快照超过1500ms或板缓存无效时，脚本显示AEC值为null，避免把过期数据当成有效零值。AEC未收敛不等于故障，尤其在无播放参考时；此轮不自动调整AEC参数。新脚本明确拒绝旧schema，需与新固件配套使用。

## 独立启动维护

USB未枚举时，按[维护流程](maintenance.md)构建USB Serial/JTAG诊断镜像；正式构建与诊断构建完全分离。诊断镜像不初始化PSRAM、音频或模型，只读取控制状态。

AGC诊断每秒最多读一次17/13，不写任何增益参数；失败或板缓存无效、超过2秒时显示null。小于1的非负有限读数保留，不按文档名义范围截断。可选AGC读取失败只使该快照失效；安全控制缓存的100ms失效关闭规则仍适用。extension4尚需烧录和实机总线稳定性验证。
