# XVF3800 + XIAO 实机验收（全部待执行）

当前没有硬件。代码测试、镜像哈希和官方资料不能替代以下实测。

## 接线与恢复

- [ ] 确认标准XIAO ESP32S3、8 MB Flash/8 MB Octal PSRAM和reSpeaker PCB版本，保存原固件与恢复方法。
- [ ] 确认XIAO USB为唯一日常连接；XMOS USB仅维护，3.5 mm有源音箱接reSpeaker。
- [ ] 检查BCLK8、WS7、RX43、TX44、SDA5、SCL6、0x2C；无旧reset/reference/LED/power绑定，无UART0冲突。
- [ ] 按build.md完全断电、按住Mute经XMOS口上电进入红灯闪烁Factory Safe Mode，只写Upgrade alt1并保留Factory alt0；核验固定XMOS1.0.8镜像及SHA256；用BOOT/RESET验证ESP ROM恢复。不可自动刷XMOS。

## 控制与采集

- [ ] Feature Report核对schema、VoiceKey release、应用hash、XMOS1.0.8；确认beam(6,3)读回。
- [ ] 测量48 kHz/32-bit stereo主从时序、Philips格式、左右顺序、有效位、连续双工。录音检查直流、削波和音量，不用增益掩盖格式错误。
- [ ] 静音X0D30时红灯、USB零样本、无新唤醒；播放继续。解除后只用新样本，旧推理结果丢弃。
- [ ] 注入I2C NACK/短回复/busy/超时、状态>100 ms、I2S中断与RX溢出（包括ISR后另一核推理立即返回），检查故障紫灯、采集关闭、恢复清理。记录最坏响应时间和在途尾音，覆盖5 ms检查边界附近的短时钟中断，区分DMA完成回调前已发送的数据与故障确认后的清空。

## USB与播放

- [ ] macOS枚举VoiceKey XVF3800 Audio输入16 kHz/s16/mono、输出48 kHz/s16/stereo及HID键盘；无CDC、无需驱动。
- [ ] 同时录放，检查异步显式反馈、主音量(-60..0 dB)/主静音、采集host mute独立性。
- [ ] alternate0/1、停止、欠载、溢出、暂停/恢复、拔插和快速重连均不重放旧音频；不足补零。检查超过5ms无TX进展后DMA预填缓冲被清除，主机在故障期间继续送包以及TX早于RX恢复均不重放，并区分已送入DAC的不可撤回样本。
- [ ] 真正独立时钟漂移、USB主机停顿、WakeNet繁忙时长运行：先30分钟再8小时，记录队列水位、丢帧/欠载、PSRAM、堆、任务时延。时长为实验建议，非已批准可靠性门槛。
- [ ] 在实际macOS通过一次性HID脚本取得完整96-byte schema2报告，验证应用hash与当次ELF一致，并记录wake ready、board/audio/wake初始化错误、实际PSRAM与free/minimum heap。测量USB总电流及suspend电流。

## WakeNet、F18与ChatGPT

- [ ] Hi ESP离线检测、USB录放并发；BOOT只作为开发触发。按键去抖30 ms、按下30 ms、冷却2 s、事件TTL200 ms；测试长按、发送失败、静音中按键释放、断连不重放。
- [ ] 独立取得并验证Hey Chat/Hello Chat模型、授权与准确性；不能把Hi ESP测试记为目标词完成。
- [ ] Mac唤醒已解锁、ChatGPT登录后台、权限与输入输出已选定；先验证F18在该版本的可配置性、后台行为及重复触发语义。
- [ ] 短暂停顿后讲话，记录检测/HID/应用开始收音时刻与首句完整性；蓝灯不作为应用就绪证明。

## 声学与AEC

- [ ] 0.5/1/2/3米，0/45/90/180度，多说话人，自然音量；安静、空调、键盘、说话、音乐背景。
- [ ] 记录成功次数/总次数、负样本小时及误触发、首句完整率、延迟分布、削波与原始WAV。
- [ ] reSpeaker3.5 mm有源音箱播放ChatGPT，同时2–3米打断/双讲；记录输出被重识别、误唤醒、AEC收敛和残余回声，并与耳机对比。
- [ ] 另接Mac/蓝牙音箱作为无此参考路径的对照，不能假设同样AEC效果。不作高保真宣称。

每项在执行前写明通过标准，填入`device-test-record.md`。一轮演示成功不能代表远场、AEC或产品验收完成。
