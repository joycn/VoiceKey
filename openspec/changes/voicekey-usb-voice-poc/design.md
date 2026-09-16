> SUPERSEDED by `migrate-respeaker-xvf3800`. Retained as historical Voice PE planning/evidence; not current hardware instructions.

## Context

见 proposal.md。提案创建前仓库没有固件，用户暂无硬件。采用 ESP-IDF v5.5.2，ESP32-S3，16 MB Flash / 8 MB Octal PSRAM；板级参数必须与实物版本核对。官方 ESPHome voice_kit 和 XMOS 源码作为接口依据，不移植整个 ESPHome 运行时。

## Goals / Non-Goals

**Goals:** 同源音频、可构建固件、真实本地模型、可测试的缓存和触发状态、明确故障与静音语义。

**Non-Goals:** 不写 XMOS Flash，不触碰 eFuse，不自动烧录设备；不实现主机后台代理、云推理、历史音频补送、USB 扬声器或新 AEC。没有主机回执时不显示“应用已就绪”。

## Decisions

1. **板级初始化**：GPIO4 高脉冲复位 XMOS，等待启动，通过 GPIO5/6 的 I²C 地址 0x42 查询版本并设置通道处理阶段。I²S RX 使用 GPIO13 BCLK / GPIO14 WS / GPIO15 DIN、从模式、16 kHz、32-bit stereo。GPIO3 静音高有效，GPIO0 按钮低有效，GPIO21 驱动 12 颗 GRB LED，GPIO45 控制灯电源。GPIO10 在没有本机播放时保持输出低电平，为 XMOS 自主时钟下的参考输入提供确定的零 PCM，避免悬空；这不是 Mac 音箱的 AEC 参考。固件不写入 XMOS 镜像。相比省略控制直接读 I²S，这能明确记录输入来源和初始化错误。
2. **同源选择**：配置 XMOS channel 0 为 AGC，选 channel 0 转成 int16 PCM，再分别供给 USB 与 WakeNet。channel 1 保留 NS 配置但不混音。Q31 高 16 位是待实机录音核验的格式解释；通过配置允许选择声道与增益。这样遵守同源约束，避免两通道不同处理阶段叠加。
3. **USB**：直接使用固定版本 TinyUSB 0.18.0~6（音频回调在 USB 任务上下文），配置 UAC2 单麦与 HID keyboard 三个接口。设备 product 名 VoiceKey Microphone，HID interface 名 VoiceKey。16 kHz / mono / s16，每 USB 帧标称 16 个样本；异步 IN 端点允许 15–17 个样本，根据有界缓存水位缓慢校正独立时钟漂移。不增加 CDC，UART0 用于调试；保留 ROM 手动下载恢复方法。对比依赖整套 UAC 封装，直接配置更便于控制复合描述符、静音与采集开关。
4. **实时分发**：采集任务产生 PCM；USB 与唤醒各有有界样本 FIFO，短临界区保护共享状态，推理在锁外执行。USB 未采集时不积累其音频，恢复清空 FIFO；溢出丢旧样本并计数，欠载补零，日志周期性报告。USB task 同时处理协议事件和 HID 状态，按键正常保持 30ms，默认冷却 2000ms。
5. **静音与失效**：硬件状态轮询与发送前检查结合。静音、音频故障或 USB 连接状态变化使 generation 递增，清空待发数据和事件；推理结果携带 generation，过期结果丢弃，模型状态由推理任务重置。硬件静音不会阻止已经按下的键完成释放。USB 暂停/断连清除待发唤醒，恢复先发全键释放。已提交 USB 控制器/主机的数据无法撤销，验收区分设备侧待发缓存。
6. **本地模型**：ESP-SR WakeNet9 直接消费 XMOS 已处理的单声道 16 kHz 音频，避免重复运行 AFE。默认选择官方内置 Hi ESP 用作真实链路验证，启动日志报告模型与词；Hey Chat / Hello Chat 不作为已有能力宣称，目标词模型取得与验证保留独立任务。相比先移植 microWakeWord 的完整运行环境，这一路径具备 ESP-IDF 原生接口。
7. **测试分层**：纯 C 逻辑模块承载样本 FIFO、PCM 转换、去重、HID 按下/释放、断连及静音失效规则；主机测试实际编译这些模块，USB 描述符另做结构解析测试。固件交叉编译验证接口和链接。实机阶段单独验证电气接口、枚举、录音、按键、远场、回声和应用闭环。

## Risks / Trade-offs

- [无硬件] → 不自动烧录，实机任务保持未完成，并提供记录模板。
- [USB-C 板级通路未实测] → 上板前核对原理图与板版本；芯片支持 USB OTG 不等于板上已验证。
- [XMOS 固件版本差异] → 查询版本并读回配置，失败输出静音并显示故障；不静默升级。
- [异步时钟漂移与主机兼容性] → 控制包长和 FIFO 水位，长录音实测丢样与连续性；不能凭 USB 带宽计算判定通过。
- [播放时的回声和误唤醒] → 测试 Mac/外接扬声器与耳机对照；不把新增 USB OUT 当作已经解决 AEC。
- [F18 在不同应用状态的语义] → 手册要求先用普通键盘配置并测试，记录前后台、重复按键与开始收音时刻。
- [目标词模型未提供] → 内置词只用于工程验证，目标词验收保持待完成，不用检测阈值伪造。
- [3 米指标尚无数值门槛] → 验收记录成功次数/总次数、每小时误触发、首句完整率及延迟分布；不虚构已批准阈值或测试成绩。

## Migration Plan

新工程没有软件数据迁移。准备设备后先保存原 ESP 固件或确认官方恢复入口，记录 XMOS 版本；使用明确串口执行 ESP32-S3 烧录，不烧 eFuse，不更新 XMOS。回滚通过 ROM 下载模式恢复原 ESP 镜像或官方 Voice PE 固件。USB OTG 运行时不假设 USB Serial/JTAG 同时可用。
