> SUPERSEDED: original Voice PE product design retained for history. Current target and approved requirements are in ../../BrainStorm.md; this document is not current hardware guidance.

# 1. 项目名称

VoiceKey Far-field Voice Trigger

基于 Home Assistant Voice Preview Edition 的远场语音唤醒与 USB 麦克风原型。

---

# 2. 产品目标

开发一款面向 Mac 等桌面设备的语音交互硬件。

设备本身不负责语音识别、AI 推理或对话逻辑，仅承担：

1. 远场拾音
2. 本地唤醒词检测
3. 唤醒后向主机发送指定输入事件
4. 持续作为主机的标准麦克风输入设备

AI 对话、语音识别、TTS 等能力由主机上的 ChatGPT 等 AI 应用负责。

第一阶段使用前提：Mac 保持唤醒且已解锁，ChatGPT 已登录并在后台运行，麦克风权限已授予，Voice 功能可用。用户已完成麦克风选择与唤醒快捷键配置。

第一阶段交互允许用户说完唤醒词后短暂停顿，再开始讲话；不要求将唤醒词与请求一口气连续说完。所需停顿时长由 POC 实测确定。

核心体验：

```text
用户在房间内说：

“Hey Chat”

        ↓

设备本地识别唤醒词

        ↓

通过 USB HID 向 Mac 发送指定按键
例如 F18

        ↓

Mac 启动 ChatGPT Voice

        ↓

ChatGPT 使用同一设备
作为麦克风输入

        ↓

用户在说完唤醒词、短暂停顿后开始自然讲话
```

目标是实现类似智能音箱的：

> “说出唤醒词即可开始与电脑上的 AI 对话”

但 AI 能力不绑定在设备本身。

---

# 3. 产品定位

本产品不是：

- Alexa
- 小爱音箱
- HomePod
- 独立 AI Assistant

而是：

> 一个面向电脑 AI 应用的远场 Voice Interface。

可以理解为：

> Voice Button + Far-field Microphone

或：

> Voice version of Stream Deck / Flic Button。

---

# 4. 原型硬件平台

第一阶段使用：

**Home Assistant Voice Preview Edition**

作为原型硬件。

主要利用其现有硬件能力：

- ESP32-S3 主控
- 双 MEMS 麦克风
- XMOS XU316 音频处理
- I²S 数字音频链路
- USB-C
- 内置 Speaker
- LED Ring
- 物理麦克风静音开关
- 旋钮 / 按钮
- 8 MB PSRAM
- 16 MB Flash

第一阶段原则：

> 尽量不修改 Voice PE 硬件，不修改 XMOS 固件，仅替换 ESP32-S3 主固件。

---

# 5. 系统总体架构

```text
                 Voice PE

          Dual MEMS Microphone
                   │
                   ▼
              XMOS XU316
          Audio Front-End / DSP
                   │
                   │ I2S PCM
                   ▼
               ESP32-S3
          ┌────────┼──────────┐
          │        │          │
          ▼        ▼          ▼
    Wake Word    USB UAC    USB HID
      Engine       │          │
          │        │          │
       Trigger     │        Key Event
          │        │          │
          └────────┼──────────┘
                   │
                  USB-C
                   │
                   ▼
                  Mac

         macOS Input Device:
         VoiceKey Microphone

         macOS HID Device:
         VoiceKey
```

设备和 Mac 之间第一阶段不需要无线通信。

---

# 6. 核心功能需求

## 6.1 远场拾音

设备必须支持正常室内环境下的远场语音拾取。

第一阶段目标使用距离：

- 0.5 米：必须稳定
- 1 米：必须稳定
- 2 米：必须稳定
- 3 米：目标稳定
- 3 米以上：作为性能测试，不作为首版强制指标

目标场景包括：

- 安静房间
- 空调运行
- 普通家庭背景噪音
- 键盘敲击
- 低音量背景音乐
- 普通音量电脑扬声器

拾音必须适合：

- ChatGPT Voice
- 语音转文字
- 实时 Voice Conversation

而不仅仅用于唤醒词。

---

# 7. USB 麦克风需求

设备插入 Mac 后必须被识别为标准：

**USB Audio Class Microphone**

建议设备名称：

```text
VoiceKey Microphone
```

第一版最低规格：

```text
Sample Rate:
16 kHz

Bit Depth:
16 bit

Channel:
Mono
```

后续可评估：

```text
24 kHz
48 kHz
```

是否能够提升通用性。

设备不依赖：

- 专用 Mac Driver
- Kernel Extension
- 后台守护程序

macOS 应能够直接在：

```text
System Settings
→ Sound
→ Input
```

选择：

```text
VoiceKey Microphone
```

ChatGPT 可以直接使用该设备。

---

# 8. 麦克风持续在线原则

USB 麦克风必须在设备连接 Mac 后持续存在。

禁止采用：

```text
Wake Word
↓
启动 USB Microphone
↓
重新枚举 USB
```

正确方案：

```text
USB Microphone Always Connected

+

Wake Word
↓
只发送 HID Event
```

原因：

- 避免重新枚举延迟
- 避免 ChatGPT 已经开始监听但麦克风尚未就绪
- 提升唤醒后的即时响应体验

持续枚举仅保证 USB 麦克风可用，不代表 ChatGPT 已开始采集音频。第一阶段允许唤醒后短暂停顿，需实测应用开始收音的时间，以及停顿后首句是否完整。

---

# 9. 本地 Wake Word

设备必须在本地进行唤醒词检测。

第一阶段唤醒词示例：

```text
Hey Chat
```

或：

```text
Hello Chat
```

Wake Word 检测不得依赖：

- Internet
- Cloud API
- Mac 软件
- ChatGPT
- Home Assistant Server

基本流程：

```text
Microphone
↓
XMOS
↓
PCM
↓
Wake Word Model
↓
Detected
```

第一阶段可使用：

- ESP-SR / WakeNet

或：

- microWakeWord

优先目标是稳定性，而不是支持任意用户动态生成唤醒词。

---

# 10. Wake Word 与 USB Mic 音频共享

Wake Word 和 USB Audio 必须使用同一麦克风 PCM 数据源。

不得设计成两个独立麦克风链路。

推荐架构：

```text
XMOS
 ↓
I2S
 ↓
Audio Router
 ├────────────→ Wake Word
 │
 └────────────→ USB Audio
```

两个消费者之间必须解耦。

建议采用：

```text
Wake Word Ring Buffer

USB Audio Ring Buffer
```

避免：

- Wake Word inference 阻塞 USB Audio
- USB Host 抖动影响 Wake Word
- 任意一条 pipeline 导致音频丢帧

---

# 11. USB HID 功能

设备同时需要暴露：

**USB HID Keyboard**

建议设备名称：

```text
VoiceKey
```

第一阶段使用：

```text
F18
```

作为默认唤醒事件。

流程：

```text
Wake Word Detected
        ↓
HID Key Down F18
        ↓
20~50ms
        ↓
HID Key Up F18
```

选择 F18 的原因：

- 日常键盘使用中极少出现
- 不容易和正常快捷键冲突
- macOS 可以识别
- Karabiner / Shortcuts / 第三方应用容易监听

后续应支持：

- F13–F24
- Consumer Key
- 自定义组合键
- Vendor HID Event

---

# 12. USB Composite Device

设备必须通过同一根 USB-C 同时提供：

```text
USB Composite Device

├── USB Audio Class
│   └── Microphone
│
└── USB HID
    └── Keyboard
```

Mac 用户只需要连接一根 USB-C。

不得要求：

- USB Hub
- 两根数据线
- USB + Bluetooth HID 混合连接

第一阶段不需要 Bluetooth。

---

# 13. 用户主流程

首次使用前，用户完成以下准备：

- Mac 保持唤醒且已解锁
- ChatGPT 已登录并在后台运行，Voice 功能可用
- 已授予麦克风权限

用户将 VoiceKey 通过 USB-C 连接 Mac。

Mac 自动识别：

```text
Input:
VoiceKey Microphone

HID:
VoiceKey
```

用户将 ChatGPT 的输入设备设置为：

```text
VoiceKey Microphone
```

将 F18 设置为：

```text
启动 ChatGPT Voice
```

上述麦克风选择与快捷键配置属于首次准备。第一阶段日常唤醒流程以这些配置已完成、主机满足上述条件为前提，不要求从 Mac 睡眠、锁屏或 ChatGPT 已退出的状态启动语音。

正常状态：

```text
VoiceKey
↓
持续监听 Wake Word
↓
持续作为 Mac Microphone
```

用户：

```text
“Hey Chat”
```

设备：

```text
Wake Word Detected
↓
LED Feedback
↓
HID F18
```

Mac：

```text
F18
↓
ChatGPT Voice
```

用户说完唤醒词后短暂停顿，待 ChatGPT 开始收音后再讲话。所需停顿时长由实测确定：

```text
“帮我看看明天的天气。”
```

VoiceKey：

```text
Mic
↓
USB Audio
↓
Mac
↓
ChatGPT
```

完成首次准备后，在上述主机条件下，日常唤醒与对话过程中用户不需要触碰电脑。

---

# 14. 唤醒反馈

Wake Word 成功后必须提供立即可感知的反馈。

设备端 LED 表示“已检测到唤醒词”，不表示“ChatGPT 已开始收音”。当前 HID 发送完成也不构成应用收音就绪的确认。

第一阶段允许用户短暂停顿后再讲话。若后续增加“可以讲话”的反馈，必须以可验证的应用收音就绪状态为依据；不能仅凭 LED 点亮或固定计时宣称就绪。

第一阶段：

```text
LED Ring
```

推荐：

待机：

```text
LED Off
```

唤醒：

```text
LED Ring 点亮 / 动画
```

HID 发送完成：

```text
保持约 500ms
```

这里的约 500ms 仅为灯效持续时间，不是用户停顿时长或应用启动延迟的验收指标。

随后恢复待机。

后续可以增加短提示音：

```text
“ding”
```

但必须评估提示音是否会影响随后用户的语音输入。

---

# 15. 物理 Mute

必须保留 Voice PE 的物理麦克风静音功能。

Mute 状态：

```text
Mic Hardware Mute ON
↓
USB Mic silence
+
Wake Word disabled
```

设备必须明确显示：

```text
Mic Muted
```

建议使用红色 LED。

原则：

> Hardware Mute 优先级高于所有软件设置。

---

# 16. 远场 Wake Word 性能目标

第一阶段建议测试：

### 距离

```text
0.5m
1m
2m
3m
```

### 方向

```text
0°
45°
90°
180°
```

### 音量

正常自然讲话，不要求用户大声喊。

### 人群

至少覆盖：

- 成年男性
- 成年女性
- 儿童

### 环境

至少覆盖：

- 安静房间
- 空调
- 键盘
- 普通说话背景
- 音乐
- ChatGPT 播放声音

重点指标：

```text
Wake Success Rate

False Acceptance Rate

False Rejection Rate
```

POC 阶段目标：

在 3 米安静环境中使用正常说话音量，Wake Word 成功率应达到可实际使用水平。

量产阶段再建立严格测试标准。

---

# 17. 远场语音质量要求

Wake Word 成功并不代表产品成功。

必须单独验证：

```text
远场 Voice Input Quality
```

测试内容：

用户距离设备：

```text
1m
2m
3m
```

分别进行 ChatGPT Voice 对话。

关注：

- 是否频繁漏字
- 是否明显吞句首
- 是否需要提高音量
- 连续讲话稳定性
- 环境噪音影响
- ChatGPT 转写准确度
- 长句表现

目标：

> 用户无需明显靠近设备或提高音量，即可自然与 ChatGPT Voice 对话。

---

# 18. AEC 策略

第一阶段不强制设备自行实现完整 AEC。

理由：

ChatGPT Voice 和 macOS 本身已有：

- Voice Processing
- VAD
- Turn Detection
- Interruption Handling
- Noise Processing

因此第一版采取：

```text
Voice PE
↓
Clean Far-field Mic
↓
Mac
↓
ChatGPT Voice
```

先测试实际体验。

---

# 19. AEC 验证条件

必须测试：

```text
ChatGPT 正在通过 Mac / 外接音箱播放声音
```

与此同时：

```text
用户在 2~3 米外讲话
```

需要判断：

1. ChatGPT 是否将自己的输出重新识别为用户输入
2. 用户是否可以正常打断 ChatGPT
3. ChatGPT 播放声音是否显著降低用户语音识别率
4. Wake Word 是否仍能工作

如果系统表现正常：

```text
不增加设备端 AEC
```

如果明显出现问题：

第二阶段增加：

```text
USB Audio OUT
↓
AEC Reference
↓
XMOS
```

---

# 20. 第一阶段 Speaker 定义

Voice PE 内置扬声器不是第一阶段核心功能。

ChatGPT Audio Output 可继续：

```text
Mac
↓
Mac Speaker
```

或：

```text
Mac
↓
Bluetooth Speaker
```

第一版设备重点：

```text
Far-field Mic
+
Wake Word
+
USB HID
```

而不是音响能力。

---

# 21. 固件技术要求

建议使用：

**ESP-IDF**

而不是以 ESPHome 为主。

主要模块：

```text
main/
├── audio/
│   ├── i2s_input
│   ├── audio_router
│   └── ring_buffer
│
├── usb/
│   ├── uac_microphone
│   ├── hid_keyboard
│   └── composite_device
│
├── wakeword/
│   └── wake_engine
│
├── hardware/
│   ├── led
│   ├── button
│   └── mute
│
└── app_main
```

---

# 22. ESP32 Audio Pipeline

推荐 pipeline：

```text
XMOS XU316
    ↓
I2S RX
    ↓
32-bit PCM / 16kHz
    ↓
Audio Processing / Convert
    ↓
16-bit PCM
    │
    ├────────→ USB Ring Buffer
    │             ↓
    │          USB UAC
    │             ↓
    │            Mac
    │
    └────────→ Wake Ring Buffer
                  ↓
              Wake Engine
                  ↓
                Trigger
                  ↓
               USB HID
```

---

# 23. 第一阶段明确不做

为控制原型范围，以下功能暂不纳入：

- Bluetooth Microphone
- Bluetooth HID
- Wi-Fi Audio
- Cloud Wake Word
- AI 推理
- Speech-to-Text
- Text-to-Speech
- ChatGPT API
- MCP
- Home Assistant Server
- 自定义 Mac Driver
- 自定义虚拟声卡
- 内置 LLM
- Battery
- Wireless Mode

第一阶段严格聚焦：

```text
Far-field Mic
+
Wake Word
+
USB Audio
+
USB HID
```

---

# 24. POC 开发阶段

## P0：获取 Voice PE PCM

目标：

```text
XMOS
↓
ESP32
```

稳定取得音频。

验证：

- PCM 不丢帧
- 声音正常
- Mic mute 正常

---

## P1：USB Microphone

实现：

```text
I2S
↓
USB UAC
↓
Mac
```

验收：

macOS 出现：

```text
VoiceKey Microphone
```

QuickTime / Audacity 能正常录音。

---

## P2：USB Composite

增加：

```text
USB HID
```

Mac 同时识别：

```text
VoiceKey Microphone
VoiceKey Keyboard
```

---

## P3：Button → HID

使用 Voice PE 实体按钮触发：

```text
F18
```

验证：

Mac 可以监听并执行指定动作。

---

## P4：Wake Word → HID

将：

```text
Button → F18
```

替换成：

```text
Hey Chat → F18
```

同时 USB Mic 持续工作。

---

## P5：ChatGPT 完整闭环

前提：Mac 唤醒且已解锁，ChatGPT 已登录并在后台运行，Voice 可用，麦克风权限、输入设备与快捷键均已配置。

```text
Hey Chat
↓
F18
↓
ChatGPT Voice
↓
VoiceKey Microphone
↓
用户短暂停顿后开始讲话
↓
Conversation
```

用户全过程不触碰电脑。记录从唤醒到实际开始收音的时间，验证停顿后首句是否完整，据此确定所需停顿时长；不要求无停顿连续说出唤醒词与请求。

---

# 25. POC 验收标准

以下验收以第 13 节的主机条件与首次配置已满足为前提。第一阶段原型必须至少满足：

- Voice PE 可被 Mac 识别为 USB 麦克风
- 不需要安装驱动
- 同一 USB 连接同时支持 HID
- F18 可稳定发送
- 本地 Wake Word 可工作
- Wake Word 不依赖网络
- Wake Word 期间 USB Mic 不停止
- 唤醒后 ChatGPT 能开启语音并使用该麦克风
- 用户短暂停顿后讲话，首句能够完整接收；停顿时长由实测确定并记录
- 唤醒 LED 明确表示检测成功，不误示应用已开始收音
- 1 米正常讲话稳定
- 2 米正常讲话可用
- 3 米达到远场使用目标
- 物理 Mute 有效
- 设备长时间运行无明显音频中断

---

# 26. 产品体验目标

第一阶段希望在 Mac 唤醒且已解锁、ChatGPT 已登录并在后台运行、首次配置已完成的条件下达到：

用户坐在书桌、沙发或房间另一侧：

```text
“Hey Chat”
```

设备：

```text
● LED
```

ChatGPT Voice 自动打开。

用户说完唤醒词后短暂停顿，待应用开始收音再讲话：

```text
“帮我总结一下今天收到的重要邮件。”
```

完成首次准备后的日常体验不需要：

- 触摸设备
- 点击 Mac
- 手动将后台运行的 ChatGPT 调到前台
- 切换麦克风
- 手动启动 Voice

最终形成：

> Computer AI 的 Always-Available Voice Interface。

第一阶段的可用范围限定为上述主机条件，不承诺在睡眠、锁屏或应用退出时仍可唤醒。

---

# 27. 后续量产方向

如果 Voice PE 原型验证成功，下一阶段再基于验证结果设计自研硬件。

量产版本重点评估：

```text
ESP32-S3 / 后续 MCU
+
2 Mic / 4 Mic Array
+
XMOS / 独立 Audio DSP
+
USB-C
+
Physical Mute
+
LED Ring
```

如果 2 Mic 无法达到预期远场效果，则升级为：

```text
4 Mic Array
+
Beamforming
```

如果 ChatGPT/macOS 的回声处理不足，则增加：

```text
USB Audio OUT
+
AEC Reference
```

因此 Voice PE 的核心作用是：

> 在不自行设计 PCB 和声学结构之前，快速验证“远场语音唤醒电脑 AI”这一产品体验是否成立。

---

# 28. 核心成功条件

整个项目是否值得继续，最终取决于三个指标：

### 1. 唤得醒

```text
3 米
正常音量
“Hey Chat”
```

能够可靠触发。

### 2. 听得清

```text
3 米
正常讲话
```

ChatGPT Voice 可以稳定理解用户。

### 3. 足够自然

```text
Hey Chat
↓
ChatGPT Voice
```

在第一阶段规定的主机条件下，用户说完唤醒词后可以短暂停顿，再自然讲话，无需触碰设备或电脑。不要求无停顿续说；所需停顿时长与首句完整性由 POC 实测验证，设备亮灯不作为应用已开始收音的证明。

只要这三个条件成立，Voice PE 原型就证明了这个产品方向的核心价值。
