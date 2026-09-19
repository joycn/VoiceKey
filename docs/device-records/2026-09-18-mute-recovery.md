# 静音恢复失败：待定位

用户实测：音箱播放输出正常；按 Mute 后静音生效，再次按 Mute 后红灯持续亮，录音仍无声音。静音解除验收失败，不能标记完成。

代码检查：XIAO只读取 GPO_READ_VALUES 第二项 X0D30，没有写入静音 GPO；运行时静音门控还包含控制状态有效性和 I2S 时钟资格。runtime UI 将这些保护性关闭也显示为红色，因此红灯不能单独证明 X0D30 仍为高电平。尚未确认用户看到的是独立静音指示灯还是 RGB 灯环。

本轮重复 HID 诊断仍在 open_path 失败；尝试非独占打开也失败。未取得现场静音位、缓存有效性、时钟资格或故障计数。未改写固件、未自动解除静音、未烧录。

下一步：完全断电并正常重启，确认按 Mute 前录音能否恢复，再复现一次静音/解除静音。若重现，需取得 X1D09 按钮输入、X0D30 输出和采集门控状态，区分 XMOS 按钮状态与 XIAO 采集恢复问题。

官方依据：[GPIO控制](https://wiki.seeedstudio.com/respeaker_xvf3800_xiao_gpio/)、[板卡介绍](https://wiki.seeedstudio.com/respeaker_xvf3800_introduction/)。

## 后续实测与诊断占用

用户确认：断电重启后录音恢复但音量很小、只能微弱听到声音；再次切换 Mute 又卡住；亮的是独立红色静音指示灯，而非 RGB 灯环。官方 GPIO 文档将独立指示灯和麦克风静音电路同接 X0D30；现象更支持硬件静音输出保持，但尚无寄存器读回证据，不能确定按钮、XMOS固件或控制交互中的具体根因。

通过 hidapi 原生 hid_error 取得明确错误：`0xE00002C5 exclusive access and device already open`。非独占打开仍失败。IORegistry 在 VoiceKey 的 HID 子树显示 Karabiner-Core-S 的 IOHIDLibUserClient，ClientOptions=1、ClientOpened=Yes；已请求用户暂时仅关闭 VoiceKey 的 Modify events，以验证占用来源并重试诊断。未终止进程或修改 Karabiner 配置。

PCM软件配置当前为32位有符号样本除65536转s16，CONFIG_VOICEKEY_PCM_GAIN=1。只有样本有效位、峰值/RMS及缺失样本计数确认后才能判断应修复格式还是调整增益；暂不凭主观音量提高增益。麦克风USB端未提供音量控制，仅提供静音。

## HID读取恢复及现场采样

用户关闭 VoiceKey 的 Karabiner Modify events 后，独占模式仍打开失败，但非独占模式已成功读取193字节 HID Feature Report。已修复 scripts/diagnose.py：在 hid.device() 初始化之后设置 hid_darwin_set_open_exclusive(0)，避免默认独占访问。不会修改系统权限或终止其他程序；对应顺序/失败清理回归通过。

首个成功报告：XMOS1.0.8/inthost-lr48-sqr-i2c；board/audio/wake init均0；8MB PSRAM；wake ready；I2S约47999Hz；board valid、未静音；AEC报告converged、未bypass（不等于声学验收）。

随后约15秒31个快照：I2C errors767→793、busy28401→29571、USB missing26017→27155；2个快照board无效并保护性静音。传输故障与capture fault generation均0；时钟47998–48000Hz。可确认控制通信反复失败，不能将此直接等同独立静音指示灯卡住的根因，也不能据此确定低音量为增益问题。快照见logs/2026-09-18-control-snapshot.json。

用户再次按两次Mute后报告独立红灯已熄灭。约30秒采样捕获有效静音状态(True,True)随后转为有效未静音(True,False)，之后多次出现I2C失败引起的保护性静音并自动恢复，最终有效未静音。此次按钮解除成功，但先前无法解除的根因尚未复现；不能归因于Karabiner或标记已修复。证据logs/2026-09-18-mute-transition.json。当前安装固件未改变，本轮仅修复主机诊断脚本。
