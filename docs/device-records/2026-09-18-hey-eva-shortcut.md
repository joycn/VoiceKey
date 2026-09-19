# Hey Eva / Mac组合键需求更新

用户指定Hey Eva及Shift+Option+Command+S（⇧⌥⌘S）。固件现发送USB HID modifier0x0e（左Shift/Alt/GUI）与S usage0x16，释放报告全部清零；GET_REPORT同步最后成功发送状态。沿用30ms按下、冷却、重试及过期丢弃机制。

实际USB回调测试覆盖精确按键/修饰符、发送失败重试、释放失败重试、释放后8字节全零读回和过期事件；完整test.sh、交叉编译、8MB镜像校验与OpenSpec严格校验通过。应用SHA256 `25f0abd26822c59c98e191c1b8aef6acdf0ecbaa3a3655908eac0b2fdb0f8c8f`，412304字节。未烧录，Mac组合键实测尚未完成。

Hey Eva模型尚未取得，本地2.2.0组件与官方列表未找到匹配词；仍使用Hi ESP开发模型，不能说Hey Eva已实现。详情见[模型接入状态](../wake-word-model.md)。BUG-001保持待修复，未恢复任何此前撤回的音频调参。
