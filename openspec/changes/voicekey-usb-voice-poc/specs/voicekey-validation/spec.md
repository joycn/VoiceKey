## Purpose

为本项目建立可重复的固件构建与逻辑测试，并明确记录设备和主机实测证据，避免将开发环境验证误认为远场效果、USB 兼容性或对话体验已经达到产品验收要求。

## ADDED Requirements

### Requirement: Reproducible development validation
项目 SHALL 提供固定工具链与组件版本、构建说明、脱离硬件的关键逻辑测试，以及可执行的分阶段验收步骤。

#### Scenario: Build without hardware
- **WHEN** 开发者没有连接设备但完成依赖安装
- **THEN** 可以构建 ESP32-S3 固件并测试缓存、静音、HID 状态逻辑，不需连接云端 AI 服务

### Requirement: Explicit evidence boundary
项目 SHALL 分别记录软件测试、烧录、USB 枚举、录音、远场唤醒、目标词、回声以及 ChatGPT 闭环的状态。

#### Scenario: No physical device
- **WHEN** 仅编译和逻辑测试完成而没有 Voice PE
- **THEN** 硬件与产品验收保持待验证，不能标记完成或归档为产品验收通过

### Requirement: Supported host interaction
闭环验收 SHALL 使用已解锁且保持唤醒的 Mac，ChatGPT 已登录在后台运行且 Voice 可用，权限、输入设备和快捷键均已配置；允许唤醒词后的短暂停顿。

#### Scenario: First spoken request
- **WHEN** 用户说完唤醒词、按实测时长短暂停顿后讲话
- **THEN** 记录实际启动延迟及首句完整性，不以 LED 时长替代应用收音时间
