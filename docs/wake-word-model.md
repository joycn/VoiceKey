# 你好小智 唤醒模型

2026-09-18用户将目标词从Hi, Joy改为 **你好小智**。使用固定ESP-SR2.2.0已包含的 `wn9_nihaoxiaozhi_tts`，不是重命名Hi ESP模型。官方[模型列表](https://github.com/espressif/esp-sr/blob/master/wakeword_list.md)列出你好小智。模型元数据：`wakenet9l_tts1h8_你好小智_3_0.631_0.635`。

生产及bringup默认配置开启 `CONFIG_SR_WN_WN9_NIHAOXIAOZHI_TTS` 并关闭Hi ESP与Hi, Joy。运行时明确选择 `wn9_nihaoxiaozhi_tts`，缺少或错配时启动失败，不自动退回其他词。源模型来自固定组件，哈希记录在 `docs/models/nihao-xiaozhi-manifest.json`。组件随附许可仍适用；未单独购买或提交定制申请。

输入维持16kHz、s16、单声道，经XMOS48k左声道FIR降采样后进入WakeNet。检测成功发送Shift+Option+Command+S（⇧⌥⌘S），保留30ms按下、全键释放、2秒冷却和200ms触发过期。用户说“你好小智”，停顿后再说请求；

模型加入构建不等于设备验收。烧录需包含新的model分区；单独app-flash不会更新唤醒词模型。设备验证需确认启动实际模型/词名、正样本与近音负样本、1–3米拾音、播放干扰、静音与故障恢复，以及Mac收到完整组合键。BUG-001暂缓修复仍影响声学质量，不能宣称远场或唤醒准确率已达标。

运行时短按B是绕过语音识别的开发触发。启动时不会自动发送快捷键；启动时一直按住B也不会触发。必须先松开并稳定30ms，再重新按下稳定30ms才允许触发；B配合复位的ROM下载入口不变。

## 灯环交互（2026-09-19）

正常就绪时白色呼吸（官方effect1），唤醒并发送快捷键时使用彩虹灯（effect2），持续5秒后恢复白色呼吸。短按B开发触发使用相同反馈。静音红色常亮和故障紫色优先；灯效不是Mac对话状态。启动B键防误触修复随本版保留。

XMOS自行执行动画；独立控制任务仅在状态变化时配置LED_COLOR/LED_EFFECT，并在初始化或故障后重设LED_BRIGHTNESS=30、白色和彩虹LED_SPEED=1（用户指定，不再使用15或速度回退），所有写入均读回校验。取消逐帧灯环写入和蓝色自定义动画。实际视觉效果与通信稳定性待烧录验收。

协议依据：https://github.com/respeaker/reSpeaker_XVF3800_USB_4MIC_ARRAY/blob/a652fe79da3a292b25decc0e1e7f267d29bb0284/python_control/xvf_host.py 。

用户实测反馈：在当前XMOS1.0.8上，白色呼吸速度参数从1提高到3时频率变快。按用户要求恢复1；这是实测方向，不表示已确认线性倍率或完整有效范围。
