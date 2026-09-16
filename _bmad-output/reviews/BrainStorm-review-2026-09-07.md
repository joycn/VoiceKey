> 历史评审（已被替代）：针对旧Voice PE设计；当前唯一目标及有效验证见`BrainStorm.md`和`docs/validation-status.md`。保留原始评审内容，不作为新板验证证据。

# BrainStorm.md 讨论审阅记录

日期：2026-09-07。对象：[BrainStorm.md](../../BrainStorm.md)。按构想成熟度审阅，发现表示需要讨论或验证，不代表已证实的实现缺陷。未修改原文，未进行硬件或主机实测。

运行镜头：adversarial、edge-case-hunter、structure。未运行 verification-gap（非代码）或 prose（未请求逐句编辑）。不同镜头在启动时序、重复唤醒、缓冲、静音和 AEC 上重叠，分别保留。

## 官方来源核实

- 当前 ChatGPT 桌面语音文档提供 Settings > Voice > Voice chat hotkey。它支持快捷键方向，但没有证明 F18 绑定、应用退出时启动、锁屏行为或收音时延；这些必须在目标版本和账号实测。[ChatGPT Voice](https://learn.chatgpt.com/docs/features/voice)
- Voice PE 官方 dev 配置使用 16 kHz、32-bit、secondary 模式、stereo 的 I²S 麦克风输入，唤醒配置选择 channels: 1。应确认所用固件版本的通道含义与单声道转换，不据此推定左右通道可以直接平均。[官方固件配置](https://raw.githubusercontent.com/esphome/home-assistant-voice-pe/dev/home-assistant-voice.yaml)
- ESP32-S3 的 USB OTG 与 USB Serial/JTAG 共享内部 PHY，需要安排固件调试和恢复路径；芯片能力不等于已确认 Voice PE 板级连接。[Espressif USB 文档](https://docs.espressif.com/projects/esp-usb/en/latest/esp32s3/usb_host.html)
- Voice PE 原理图的网页读取失败，本轮未完成板级 USB 走线核实。因此没有作出“只刷固件必然可行”或“必须改板”的结论。

## 讨论切入点

将主机快捷键启动和真实收音时序提前验证。分别保留“无需自定义驱动”“是否允许辅助程序”“是否要求无停顿续说”的决策。先用现有麦克风隔离主机侧变量；再验证板级 USB、PCM 和唤醒并发；目标词和远场指标分别验收。

## 假设质疑（adversarial）

**§11、13、24**

- 问题／触发：F18 到 Voice 的配置与启动路径尚未验证，直到 P5 才验证完整闭环。
- 建议：固件开发前用现有键盘或 HID 验证，记录配置、权限和应用状态。
- 后果：硬件完成后才发现主机入口不能满足目标。

**§7、11**

- 问题／触发：无需后台守护程序与使用第三方监听工具的边界不明确。
- 建议：区分麦克风免驱与应用唤醒是否允许常驻辅助软件。
- 后果：验收约束互相冲突，低估配置成本。

**§8、14、28**

- 问题／触发：设备在线和 HID 成功不证明应用已开始采集。
- 建议：分别测量检测、HID、Voice 启动、收音时间和首句完整率，再决定反馈与停顿要求。
- 后果：灯亮后讲话仍丢失句首。

**§4、5、22**

- 问题／触发：不改硬件与 XMOS 固件的前提缺少板级接口验证。
- 建议：核对目标板 USB 数据通路、XMOS 初始化、I²S 时钟和通道格式并实测 PCM。
- 后果：实现建立在错误接口假设上，需返工或改变范围。

**§9、24 P4**

- 问题／触发：未确认 Hey Chat / Hello Chat 对应的可运行模型。
- 建议：分别验证现成模型链路和目标词模型，明确来源及训练工作量。
- 后果：链路可用却无法验证目标唤醒词体验。

**§10、22**

- 问题／触发：双缓冲未定义速率不匹配、满空及主机停止取音策略。
- 建议：确定容量、满空处理和丢帧计数，观察长录音及暂停恢复时水位。
- 后果：长期运行丢样、噪声或延迟累积。

**§11、13、19**

- 问题／触发：未规定会话中重复呼叫或播放唤醒词时的行为。
- 建议：确定去重冷却，实测会话中 F18 行为并加入播放唤醒词测试。
- 后果：重复按键改变会话状态或干扰讲话。

**§18、19、27**

- 问题／触发：主机回声能力未经验证，USB OUT 的参考信号与实际播放路由关系未定义。
- 建议：分别测试 Mac、外接、蓝牙音箱；明确参考信号获取和同步关系。
- 后果：补救方案不覆盖实际输出，需改变音频路由或硬件。

**§6.1、16、25、28**

- 问题／触发：3 米在目标与硬性验收之间摇摆，稳定可靠没有定义。
- 建议：统一距离门槛，为成功率、误触发频率、首句完整率、延迟设暂定阈值与样本量。
- 后果：无法据测试决定继续或调整方向。

**§10、15**

- 问题／触发：静音切换未处理音频缓存和排队唤醒。
- 建议：切换时清理尚未发出的音频与唤醒，恢复后只处理新采样并测试。
- 后果：静音后仍残留音频或延迟触发。

**§13、26**

- 问题／触发：Always-Available 未限定登录、解锁、应用运行与 Voice 可用等条件。
- 建议：定义并测试前台、后台、退出、睡眠、锁屏下的支持范围。
- 后果：准备好的演示环境成功，日常状态无法复现。

**§17、27**

- 问题／触发：主要通过应用对话评价质量，双麦不足时直接考虑四麦，缺少故障归因。
- 建议：保留 USB 本地录音，比较近场及现有麦克风，分离采集、固件、启动和应用问题。
- 后果：增加麦克风成本却未修复真实限制。


## 边界场景（edge-case-hunter）

**§13、14、28**

- 问题／触发：HID 已发送但应用尚未开始采集。
- 建议：定义启动预算并测试紧接唤醒词的句首；必要时调整交互或增加可验证就绪反馈。
- 后果：用户看见灯亮仍被吞句首。

**§9、11、13**

- 问题／触发：同一句连续命中或会话开启后再次命中。
- 建议：规定去重冷却及会话中再次触发的行为。
- 后果：重启、切换或关闭会话。

**§11、13、26**

- 问题／触发：主机休眠锁屏、应用退出、权限缺失或快捷键失效。
- 建议：列出支持的前置状态及失败表现，逐项验收。
- 后果：设备提示成功而无法对话。

**§10、15**

- 问题／触发：静音时缓冲仍有语音或存在待发事件。
- 建议：清空设备端缓存及待发唤醒；静音输出零样本，解除后只取新音频。
- 后果：静音后输出残留语音或唤醒。

**§8、10、22**

- 问题／触发：主机不采集、暂停读取或消费者落后于采样。
- 建议：明确满空和恢复策略：丢旧帧、欠载补静音、保持实时位置并记录丢帧。
- 后果：恢复时输出旧音频或堵塞另一条链路。

**§11、12**

- 问题／触发：按下 F18 后释放前 USB 暂停或断连。
- 建议：恢复连接清除按键状态，不重放离线唤醒，并测试按下与释放之间断连。
- 后果：可能残留按键状态或恢复后误触发。

**§19、20、27**

- 问题／触发：增加 USB OUT 后实际声音仍从 Mac 或蓝牙音箱播放。
- 建议：定义如何取得实际播放参考；若需改用设备扬声器，将路由变更写入方案。
- 后果：新增 USB 输出仍无有效回声参考。

## 文档结构（structure）

| Pass | Original Text | Revised Text | Changes |
|---|---|---|---|
| structure | §28 核心成功条件位于文末 | MOVE：移到 §2 产品目标之后 | 先用唤得醒、听得清、足够自然组织后续讨论。 |
| structure | §23 第一阶段不做晚于固件设计 | MOVE：移到定位之后、硬件之前 | 先声明范围再展开实现。 |
| structure | §6 只有 6.1，其他需求同级散布 | MERGE：核心功能统领 §6.1–15；§10 放入架构 | 统一标题层级，分离需求与实现，保留约束。 |
| structure | §10 音频共享与 §22 Pipeline 分散 | MERGE：保留 §22 图并接入 §10 的同源、解耦约束 | 集中解释同一处分流，预计减少约 25 个脚本计数词。 |
| structure | §18 AEC 策略和 §19 验证分章 | MERGE：当前选择 → 测试条件 → 结果对应动作 | 保留全部内容，明确策略的验证依据。 |
| structure | §26 再次展开 §2、13 的体验流程 | CONDENSE：用户无须触碰设备或电脑，即可从房间内自然发起 AI 语音对话。 | 保留简短回顾，预计减少约 80 个脚本计数词。 |
| structure | §5 架构图与 §13 用户流程 | PRESERVE：保留两个视角 | 分别服务系统理解和用户操作理解，非无效重复。 |

结构镜头脚本口径全文为 3,483 words，并非中文字数；全部接受预计减少约 105（3.0%），未设长度目标。

## 完整 JSON

```json
[
  {
    "lens": "adversarial",
    "location": "§11、13、24",
    "trigger_condition": "F18 到 Voice 的配置与启动路径尚未验证，直到 P5 才验证完整闭环。",
    "guard_snippet": "固件开发前用现有键盘或 HID 验证，记录配置、权限和应用状态。",
    "potential_consequence": "硬件完成后才发现主机入口不能满足目标。"
  },
  {
    "lens": "adversarial",
    "location": "§7、11",
    "trigger_condition": "无需后台守护程序与使用第三方监听工具的边界不明确。",
    "guard_snippet": "区分麦克风免驱与应用唤醒是否允许常驻辅助软件。",
    "potential_consequence": "验收约束互相冲突，低估配置成本。"
  },
  {
    "lens": "adversarial",
    "location": "§8、14、28",
    "trigger_condition": "设备在线和 HID 成功不证明应用已开始采集。",
    "guard_snippet": "分别测量检测、HID、Voice 启动、收音时间和首句完整率，再决定反馈与停顿要求。",
    "potential_consequence": "灯亮后讲话仍丢失句首。"
  },
  {
    "lens": "adversarial",
    "location": "§4、5、22",
    "trigger_condition": "不改硬件与 XMOS 固件的前提缺少板级接口验证。",
    "guard_snippet": "核对目标板 USB 数据通路、XMOS 初始化、I²S 时钟和通道格式并实测 PCM。",
    "potential_consequence": "实现建立在错误接口假设上，需返工或改变范围。"
  },
  {
    "lens": "adversarial",
    "location": "§9、24 P4",
    "trigger_condition": "未确认 Hey Chat / Hello Chat 对应的可运行模型。",
    "guard_snippet": "分别验证现成模型链路和目标词模型，明确来源及训练工作量。",
    "potential_consequence": "链路可用却无法验证目标唤醒词体验。"
  },
  {
    "lens": "adversarial",
    "location": "§10、22",
    "trigger_condition": "双缓冲未定义速率不匹配、满空及主机停止取音策略。",
    "guard_snippet": "确定容量、满空处理和丢帧计数，观察长录音及暂停恢复时水位。",
    "potential_consequence": "长期运行丢样、噪声或延迟累积。"
  },
  {
    "lens": "adversarial",
    "location": "§11、13、19",
    "trigger_condition": "未规定会话中重复呼叫或播放唤醒词时的行为。",
    "guard_snippet": "确定去重冷却，实测会话中 F18 行为并加入播放唤醒词测试。",
    "potential_consequence": "重复按键改变会话状态或干扰讲话。"
  },
  {
    "lens": "adversarial",
    "location": "§18、19、27",
    "trigger_condition": "主机回声能力未经验证，USB OUT 的参考信号与实际播放路由关系未定义。",
    "guard_snippet": "分别测试 Mac、外接、蓝牙音箱；明确参考信号获取和同步关系。",
    "potential_consequence": "补救方案不覆盖实际输出，需改变音频路由或硬件。"
  },
  {
    "lens": "adversarial",
    "location": "§6.1、16、25、28",
    "trigger_condition": "3 米在目标与硬性验收之间摇摆，稳定可靠没有定义。",
    "guard_snippet": "统一距离门槛，为成功率、误触发频率、首句完整率、延迟设暂定阈值与样本量。",
    "potential_consequence": "无法据测试决定继续或调整方向。"
  },
  {
    "lens": "adversarial",
    "location": "§10、15",
    "trigger_condition": "静音切换未处理音频缓存和排队唤醒。",
    "guard_snippet": "切换时清理尚未发出的音频与唤醒，恢复后只处理新采样并测试。",
    "potential_consequence": "静音后仍残留音频或延迟触发。"
  },
  {
    "lens": "adversarial",
    "location": "§13、26",
    "trigger_condition": "Always-Available 未限定登录、解锁、应用运行与 Voice 可用等条件。",
    "guard_snippet": "定义并测试前台、后台、退出、睡眠、锁屏下的支持范围。",
    "potential_consequence": "准备好的演示环境成功，日常状态无法复现。"
  },
  {
    "lens": "adversarial",
    "location": "§17、27",
    "trigger_condition": "主要通过应用对话评价质量，双麦不足时直接考虑四麦，缺少故障归因。",
    "guard_snippet": "保留 USB 本地录音，比较近场及现有麦克风，分离采集、固件、启动和应用问题。",
    "potential_consequence": "增加麦克风成本却未修复真实限制。"
  },
  {
    "lens": "edge-case-hunter",
    "location": "§13、14、28",
    "trigger_condition": "HID 已发送但应用尚未开始采集。",
    "guard_snippet": "定义启动预算并测试紧接唤醒词的句首；必要时调整交互或增加可验证就绪反馈。",
    "potential_consequence": "用户看见灯亮仍被吞句首。"
  },
  {
    "lens": "edge-case-hunter",
    "location": "§9、11、13",
    "trigger_condition": "同一句连续命中或会话开启后再次命中。",
    "guard_snippet": "规定去重冷却及会话中再次触发的行为。",
    "potential_consequence": "重启、切换或关闭会话。"
  },
  {
    "lens": "edge-case-hunter",
    "location": "§11、13、26",
    "trigger_condition": "主机休眠锁屏、应用退出、权限缺失或快捷键失效。",
    "guard_snippet": "列出支持的前置状态及失败表现，逐项验收。",
    "potential_consequence": "设备提示成功而无法对话。"
  },
  {
    "lens": "edge-case-hunter",
    "location": "§10、15",
    "trigger_condition": "静音时缓冲仍有语音或存在待发事件。",
    "guard_snippet": "清空设备端缓存及待发唤醒；静音输出零样本，解除后只取新音频。",
    "potential_consequence": "静音后输出残留语音或唤醒。"
  },
  {
    "lens": "edge-case-hunter",
    "location": "§8、10、22",
    "trigger_condition": "主机不采集、暂停读取或消费者落后于采样。",
    "guard_snippet": "明确满空和恢复策略：丢旧帧、欠载补静音、保持实时位置并记录丢帧。",
    "potential_consequence": "恢复时输出旧音频或堵塞另一条链路。"
  },
  {
    "lens": "edge-case-hunter",
    "location": "§11、12",
    "trigger_condition": "按下 F18 后释放前 USB 暂停或断连。",
    "guard_snippet": "恢复连接清除按键状态，不重放离线唤醒，并测试按下与释放之间断连。",
    "potential_consequence": "可能残留按键状态或恢复后误触发。"
  },
  {
    "lens": "edge-case-hunter",
    "location": "§19、20、27",
    "trigger_condition": "增加 USB OUT 后实际声音仍从 Mac 或蓝牙音箱播放。",
    "guard_snippet": "定义如何取得实际播放参考；若需改用设备扬声器，将路由变更写入方案。",
    "potential_consequence": "新增 USB 输出仍无有效回声参考。"
  },
  {
    "lens": "structure",
    "Pass": "structure",
    "Original_Text": "§28 核心成功条件位于文末",
    "Revised_Text": "MOVE：移到 §2 产品目标之后",
    "Changes": "先用唤得醒、听得清、足够自然组织后续讨论。"
  },
  {
    "lens": "structure",
    "Pass": "structure",
    "Original_Text": "§23 第一阶段不做晚于固件设计",
    "Revised_Text": "MOVE：移到定位之后、硬件之前",
    "Changes": "先声明范围再展开实现。"
  },
  {
    "lens": "structure",
    "Pass": "structure",
    "Original_Text": "§6 只有 6.1，其他需求同级散布",
    "Revised_Text": "MERGE：核心功能统领 §6.1–15；§10 放入架构",
    "Changes": "统一标题层级，分离需求与实现，保留约束。"
  },
  {
    "lens": "structure",
    "Pass": "structure",
    "Original_Text": "§10 音频共享与 §22 Pipeline 分散",
    "Revised_Text": "MERGE：保留 §22 图并接入 §10 的同源、解耦约束",
    "Changes": "集中解释同一处分流，预计减少约 25 个脚本计数词。"
  },
  {
    "lens": "structure",
    "Pass": "structure",
    "Original_Text": "§18 AEC 策略和 §19 验证分章",
    "Revised_Text": "MERGE：当前选择 → 测试条件 → 结果对应动作",
    "Changes": "保留全部内容，明确策略的验证依据。"
  },
  {
    "lens": "structure",
    "Pass": "structure",
    "Original_Text": "§26 再次展开 §2、13 的体验流程",
    "Revised_Text": "CONDENSE：用户无须触碰设备或电脑，即可从房间内自然发起 AI 语音对话。",
    "Changes": "保留简短回顾，预计减少约 80 个脚本计数词。"
  },
  {
    "lens": "structure",
    "Pass": "structure",
    "Original_Text": "§5 架构图与 §13 用户流程",
    "Revised_Text": "PRESERVE：保留两个视角",
    "Changes": "分别服务系统理解和用户操作理解，非无效重复。"
  }
]
```
