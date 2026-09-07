# 实现依据

这些来源用于核对接口，不代表实物已验证。

- [Voice PE 配置（固定提交）](https://github.com/esphome/home-assistant-voice-pe/blob/0579e7b9d8504264719c593474c85447253c9dc1/home-assistant-voice.yaml)：GPIO、LED、I²S 16 kHz/32-bit/stereo、静音极性。
- [VoiceKit 控制实现（同一提交）](https://github.com/esphome/home-assistant-voice-pe/blob/0579e7b9d8504264719c593474c85447253c9dc1/esphome/components/voice_kit/voice_kit.cpp)：GPIO4 高脉冲复位、3 秒启动等待、版本请求和通道配置。项目仅使用控制协议，不带 DFU 写入功能。
- [VoiceKit 控制定义](https://github.com/esphome/home-assistant-voice-pe/blob/0579e7b9d8504264719c593474c85447253c9dc1/esphome/components/voice_kit/voice_kit.h)：resource 240/241、version command 88、pipeline register 0x30/0x40。
- [XMOS 配置实现](https://github.com/esphome/voice-kit-xmos-firmware/blob/ef04d4b59d172dfbe5c85ba982bf05fcc85f77f3/src/ffva/src/configuration/configuration_servicer.c)：通道输出处理阶段。固件不被本项目改写。
- [官方 Voice PE 原理图](https://voice-pe.home-assistant.io/resources/home_assistant_voice_pe_schematic_v1.0_241009.pdf)：上板前必须结合实物核对 USB 连接与开关。本轮未完成电气链路验证。
- [Espressif TinyUSB 自定义集成说明](https://components.espressif.com/components/espressif/tinyusb/versions/0.18.0~6)：直接组件与自定义 tusb_config.h。固定 0.18 分支，以使用任务上下文中的音频 pre-load 回调，避免把带锁逻辑搬进新版 ISR 回调。
- [TinyUSB UAC2 定义](https://github.com/hathach/tinyusb/blob/0.18.0/src/device/usbd.h)：单麦 UAC2 描述符结构。本项目仅暴露实际支持的 master mute，不宣告未实现的音量调节。
- [ESP-SR](https://github.com/espressif/esp-sr)：真实 WakeNet 接口与 Flash 模型分区，实际依赖固定 2.2.0。
- [ChatGPT Voice](https://learn.chatgpt.com/docs/features/voice)：主机语音入口与 Voice chat hotkey。具体 F18 绑定、后台行为和延迟仍需在目标安装版本实测。

第三方代码和模型遵循各组件附带许可证，见 `firmware/managed_components/`；量产前需确认模型授权与 USB VID/PID。项目没有把第三方库重标为自有实现。
