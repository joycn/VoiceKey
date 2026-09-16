> SUPERSEDED by `migrate-respeaker-xvf3800`. Retained as historical Voice PE planning/evidence; not current hardware instructions.

## Purpose

将本地语音检测和物理按钮转换为桌面应用可接收的键盘触发，使用户在规定主机条件下短暂停顿后进入语音会话，并清楚区分唤醒反馈与应用收音就绪。

## ADDED Requirements

### Requirement: Local detection
设备 SHALL 在没有网络和主机推理服务时运行真实唤醒模型，并明确报告当前模型名称与唤醒词；不能用音量阈值代替唤醒模型或声称内置模型支持未经验证的词。

#### Scenario: Offline wake
- **WHEN** 设备输入当前已安装模型支持的唤醒词
- **THEN** 本地检测成功可产生一次触发，不调用云端或主机唤醒服务

### Requirement: Bounded HID pulse
设备 SHALL 在连接可用且未静音时，将一次有效唤醒或按钮按下转换为 F18 的按下及释放，正常传输的按下间隔为 20–50 ms，并抑制短时间内的重复触发。

#### Scenario: Repeated detection
- **WHEN** 同一唤醒在配置的冷却周期内重复命中
- **THEN** 不追加键盘按下，已发出的按键仍正常释放

#### Scenario: Lost USB connection
- **WHEN** USB 断连或暂停
- **THEN** 丢弃待发触发，恢复时先清除按键状态，不重放离线事件，也不远程唤醒主机

### Requirement: Detection feedback semantics
设备 SHALL 提供唤醒 LED 反馈，其含义仅为检测成功，不能代表应用已经开始收音。

#### Scenario: Application start is delayed
- **WHEN** HID 已发出但 ChatGPT 尚未开始收音
- **THEN** 设备不宣称应用就绪；用户按实测停顿时间后讲话
