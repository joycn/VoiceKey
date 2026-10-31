# 客厅全方向收音基线

目标：以设备为中心约 2 米、正常讲话音量、各方向可唤醒并完成对话。
这是一项目标，不是距离限制或已通过的声学验收。软件修改不代表设备已烧录。

## 配置

默认关闭 `CONFIG_VOICEKEY_SPATIAL`，不按座位方向限制 USB 收音或唤醒。
保留 XVF3800 自动选束 `(6,3)`，PCM 数字增益为 1；全方向表示可以从各方向
讲话，不承诺同时分离多个说话人。主机通过已有接口显式开启方向门控后会覆盖默认行为；
客厅测试前重启设备，并确认诊断中的方向功能未启用。

已有 `firmware/sdkconfig` 会覆盖 `sdkconfig.defaults`。构建前确认实际配置包含
`# CONFIG_VOICEKEY_SPATIAL is not set` 和 `CONFIG_VOICEKEY_PCM_GAIN=1`；若不匹配，
通过 `./scripts/build.sh menuconfig` 在 VoiceKey 菜单关闭 experimental single-sector
模式并把 PCM gain 设为 1，然后重新构建。仅重启无法纠正镜像内的配置。

WakeNet 使用 `wn9_nihaoxiaozhi_tts`、`DET_MODE_90` 的模型默认阈值，初次创建和
静音/故障后的重建均相同。不把其他唤醒词的阈值移植过来，也不新增音量门槛。

XVF3800 在启动及控制故障后重新配置时应用以下 Seeed 基线，不在每轮轮询中重写：

| 参数 | 值 | 含义 |
| --- | ---: | --- |
| AUDIO_MGR_MIC_GAIN | 90.0 | 麦克风前级线性增益 |
| AUDIO_MGR_REF_GAIN | 8.0 | AEC 参考线性增益 |
| AUDIO_MGR_SYS_DELAY | 12 | 参考信号延迟，单位为采样点，不是毫秒 |
| PP_FMIN_SPEINDEX | 1300.0 | 双讲抑制频率分界 |
| PP_AGCMAXGAIN | 64.0 | 自动增益上限，线性倍数 |
| PP_AGCGAIN | 2.0 | 初始化当前增益，此后允许 AGC 自动变化 |
| AEC_ASROUTGAIN | 1.0 | ASR 输出增益 |

静态参数必须准确读回才开放采集。当前 AGC 增益是动态状态，写入后仅验证读回
有效性，不要求它一直等于 2.0。不改 AGC 时间常数、物理静音和现有灯效，不执行
XMOS SAVE_CONFIGURATION/CLEAR_CONFIGURATION。已有静音后低音量问题仍需独立验收。
回退旧 XIAO 固件后需整板断电，以清除本次易失配置；重新启动新固件会再次应用基线。

## 现场验证

1. 先确认新镜像已烧录、设备身份匹配，控制通信与采集状态正常，方向门控关闭。
2. 在 1 米、2 米处分别选 0°/90°/180°/270°，每个位置正常音量说唤醒词 5 次，
   记录成功次数；初步筛查目标为每个位置至少 4/5，属于项目筛查标准，非官方保证。
3. 唤醒后短暂停顿，说相同完整句子；记录首句是否缺失、录音是否清楚、有无削波。
4. 分别测试安静、空调/电视开启、设备音箱播放回答三种条件；记录音量和摆放。
   电视干扰与设备自身播放回声要分别分析，AEC 不会自动消除无参考的电视声音。
5. 最差位置保留 10–20 秒录音；同时观察误唤醒。先判断是拾音质量还是模型漏检，
   再一次只改一个参数。短期筛查通过后仍需长时间误唤醒与双讲测试。

| 距离/方向 | 环境 | 唤醒成功/次数 | 首句/音质 | 备注 |
| --- | --- | --- | --- | --- |
| 待实测 | 待实测 | 未测 | 未测 | 未测 |

## 官方依据（2026-10-31 核对）

- [Seeed 调参基线](https://github.com/respeaker/reSpeaker_XVF3800_USB_4MIC_ARRAY/blob/master/host_control/README.md#tunning)
- [Seeed 命令编号与类型](https://github.com/respeaker/reSpeaker_XVF3800_USB_4MIC_ARRAY/blob/master/python_control/xvf_host.py)
- [WakeNet 模型默认阈值说明](https://github.com/espressif/esp-skainet/blob/master/examples/wake_word_detection/afe/README.md#modify-detection-threshold)

上述基线不是官方“2 米预设”。当前固定的 XMOS 1.0.8 INT 固件仍须实机验证参数
接受情况、播放/AEC 和音质；若参数不被接受，保持采集关闭并报告错误。
