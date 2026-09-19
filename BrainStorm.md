# VoiceKey 产品方案

当前唯一平台是 **reSpeaker XVF3800 USB 4-Mic Array + 标准 XIAO ESP32S3**。Voice PE 方案已被替代，旧计划保留在历史 OpenSpec，不能再用于接线或烧录。

## 目标与使用方式

设备负责远场拾音、本地唤醒、标准 USB 麦克风/扬声器及 Shift+Option+Command+S 输入事件。AI、转写和语音合成由主机 ChatGPT 负责。用户通过 XIAO USB-C 一根数据线连接 Mac，默认将有源音箱插入 reSpeaker 3.5 mm 接口。主机音频进入 XVF3800，同时提供实际播放和 AEC 参考；麦克风静音不影响回答继续播放。

首次准备：Mac 唤醒且已解锁、ChatGPT 已登录并后台运行，权限、输入/输出设备和 Shift+Option+Command+S 快捷键已配置。日常用户说唤醒词，看到蓝灯后短暂停顿，再说请求。停顿长度需实测；蓝灯和 HID 完成均不代表应用已开始收音。无需驱动或常驻辅助程序，也不承诺睡眠、锁屏或应用退出时唤醒。

目标唤醒词为 **你好小智**，使用 ESP-SR 2.2.0 内置 `wn9_nihaoxiaozhi_tts`。模型已选入构建；设备烧录及识别质量验收仍待完成。触发快捷键为 **Shift+Option+Command+S（⇧⌥⌘S）**。

## 硬件与音频

- 标准 XIAO ESP32S3：8 MB Flash、8 MB Octal PSRAM。
- XVF3800 为 I2S master；XIAO 为 slave。连续 48 kHz、32-bit stereo 双向传输。
- BCLK GPIO8、WS GPIO7、RX GPIO43、TX GPIO44；I2C SDA5、SCL6、地址 0x2C。BOOT GPIO0 只作开发触发。
- 固定官方 `application_xvf3800_i2s_master_v1.0.8_48k.bin`；版本、commit、SHA256 见 `firmware/xmos/manifest.json`。XMOS USB 仅供单独维护/恢复，不是日常主机音频连接，不自动刷写。
- 左输出明确设置并读回 `(6,3)` processed auto-select beam，右路不混入拾音。左 48 kHz 经过 181-tap FIR 三倍降采样，再转 s16；要求 0–6.5 kHz 波纹≤0.2 dB、8 kHz 起衰减≥60 dB。
- 同一份 16 kHz mono 样本进入独立 USB/WakeNet 有界队列。采集、播放和推理互不等待消费者。
- USB IN 16 kHz/s16/mono；USB OUT 48 kHz/s16/stereo，经主音量/播放静音转成 32-bit I2S。显式反馈依据 DMA 实际消费量和有界队列水位。

## 静音、故障与交互

独立 I2C 任务每 20 ms 轮询 GPO_READ_VALUES 和 I2S 状态。X0D30 high 表示物理麦克风静音，固件从不发解除静音命令。读失败、过短响应、未知状态、超时或缓存超过 100 ms 关闭采集和新唤醒；busy64 最多重试 3 次。静音、输入失败及恢复清除 FIR、USB/WakeNet 缓存并更新推理 epoch，拒绝旧结果。播放不受麦克风静音控制。

XMOS 管理灯效：红色静音，蓝色检测，紫色故障，待机熄灭。无直连 LED、reset、参考固定电平或旧板电源 GPIO。Shift+Option+Command+S 默认按下 30 ms、冷却 2 秒、排队 200 ms 超时；断连/暂停后不重放触发。

停止播放、欠载、溢出和重连丢弃旧音频，未供给的输出为零。已经送达外部 DAC/USB 主机的在途样本不能撤回。播放是语音用途，不作高保真承诺。

## 验收边界

需独立验证 0.5/1/2/3 米与多角度的拾音、唤醒、误触发、首句完整性；覆盖安静、空调、键盘、音乐和 ChatGPT 播放/打断。对比耳机和 reSpeaker 有源音箱，测量回声、双讲、削波及长运行。接到 Mac 或蓝牙设备的其他音箱不会自动获得这条 XMOS 播放参考。

实物已连接并完成首次写入校验；应用 USB 尚未枚举，I2S 电气格式、PSRAM 运行、AEC、声学、主机快捷键与功耗均待验证。软件测试和交叉构建只证明其覆盖的代码行为。不开发云唤醒、Wi-Fi/蓝牙音频、ChatGPT API、常驻 Mac 程序、OTA 或自动 XMOS 刷写。

## 保留的产品验收与后续方向

距离目标仍为1米稳定、2米可用、3米远场目标；0.5米作近场基线。方向覆盖0°/45°/90°/180°，说话人覆盖成年男性、成年女性及儿童（由监护人安排），记录唤醒成功率WSR、误接受率FAR、误拒绝率FRR及原始次数/观察时长。数值通过门槛需在实测前明确，不能用“可用”代替量化数据。

检测LED通常保持约500ms；这只是灯效时长，不是用户停顿长度或ChatGPT开始收音时间。停顿、首句完整性与主机行为独立实测。

完成原型验证后再评估自研量产硬件，包括MCU/独立DSP、麦克风阵列、USB-C、物理静音、LED、声学结构、成本、VID/PID和模型许可；当前不承诺量产实现。原始完整产品稿保存在[已替代历史版本](docs/history/brainstorm-voice-pe-superseded.md)，其中旧板/单向USB内容不再指导当前产品。

## XVF3800 官方资料复核补充（2026-09-18）

保持单USB、3.5 mm有源音箱及处理后自动选束(6,3)默认方案。明确启用48 kHz总线所需的处理输出上采样，并检查运行固件模式和格式。设备启动或时钟恢复后先用至少250 ms窗口核验实际速率，再接收新语音与唤醒；这不是ChatGPT已经准备好录音的提示，唤醒后短暂停顿的交互约定仍保留。

安装说明必须标出进音孔朝向与外壳开孔要求；测试记录保留音箱位置和音量。诊断增加构建信息、格式/速率异常及AEC收敛、旁路、参考增益，供按需排查，不新增后台服务。AEC参数、板载功放和conference/ASR通道选择保持默认，待真实音箱与双讲对比后决定。

审查修复：播放与采集共同要求实测48 kHz资格；麦克风静音仍独立。维护诊断镜像和模拟播放/AEC验收分别见 `docs/maintenance.md`、`docs/playback-aec-contract.md`。
