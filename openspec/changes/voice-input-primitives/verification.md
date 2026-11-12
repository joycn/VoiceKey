# 软件验证（2026-09-20）

- `scripts/test.sh`：通过，含空间纯逻辑、协议异常、实际runtime/board/audio/wake适配器、默认/开启启动分支、旧诊断兼容。
- `scripts/test-usb.sh`、`scripts/test-descriptors.sh`：通过，含224字节Feature Report及原USB音频/HID行为。
- `scripts/build.sh`：空间开启与默认关闭配置均构建通过，镜像校验通过；最终工作区配置及构建产物恢复默认关闭。
- `openspec validate voice-input-primitives --strict`、`git diff --check`：通过。
- 自查修复：发言权切换使旧关键词epoch失效；RX排队块在空间切换后丢弃；USB断开清除发言权并拒绝关键词抢占；观察模式不改变音频。对应回归已运行。

构建产物：

| 配置 | 应用字节数 | SHA256 |
|---|---:|---|
| enabled | 418256 | 948c8736f44a4a0564644f9e45b73d3a233dfcf6964ca6e236b2ddf9f6417930 |
| default | 417968 | 9e6ae0a76033a9affabe9755357989bfeb57c1a81d145aa8715b4f5f77a0f5de |

未烧录设备。座位校准、真实关键词归属、I2C/音频时序、AEC、句首损失和多人重叠效果尚未验收；BUG-001保持未解决。固定方向波束未实现，仅暴露不支持结果。7项软件任务完成，实机验收任务保留未完成。
