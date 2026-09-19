# 当前实现依据

- [官方reSpeaker源码固定commit](https://github.com/respeaker/reSpeaker_XVF3800_USB_4MIC_ARRAY/tree/a652fe79da3a292b25decc0e1e7f267d29bb0284)：`python_control/xvf_host.py`为命令、类型和版本权威；VERSION(48,0)、GPO_READ_VALUES(20,0)返回5个uint8，X0D30是第二项；AUDIO_MGR_OP_L(35,15)的(6,3)；I2S_INPUT_PACKED(35,10)；I2S_INACTIVE(35,24)；LED_EFFECT(20,12)、LED_COLOR(20,16)。不使用旧bit-packed示例。
- [官方I2C说明](https://wiki.seeedstudio.com/respeaker_xvf_3800_i2c_list/)：写发送`[resource,command,length,payload]`，无单独写状态读取；设置通过readback确认。读取发送`[resource,command|0x80,payload_length+1]`后独立接收status+payload，0成功/64 busy。本实现每次IDF传输10 ms超时、busy最多3次。
- [I2S时钟与Home Assistant说明](https://wiki.seeedstudio.com/respeaker_xvf3800_xiao_home_assistant/)、[XIAO I2S](https://wiki.seeedstudio.com/respeaker_xvf3800_xiao_i2s/)、[GPIO](https://wiki.seeedstudio.com/respeaker_xvf3800_xiao_gpio/)、[RGB](https://wiki.seeedstudio.com/respeaker_xvf3800_xiao_rgb/)：官方板级使用依据；仍需实物版本核对。
- XMOS固定镜像：`xmos_firmwares/i2s/application_xvf3800_i2s_master_v1.0.8_48k.bin`，888832字节，SHA256 `d60d0bc2c7f5a67ffa9c9206e066b2d8f1ccb49ebba673f3a7bcff1cf197dfb2`。详见manifest和verify脚本。维护口不能代替日常XIAO连接。
- [TinyUSB0.18.0](https://github.com/hathach/tinyusb/tree/0.18.0/src/class/audio)：使用组件0.18.0~6的实际代码核对post-read、控制、feedback接口；FS反馈API输入16.16，启用格式纠正后发送10.14。手动反馈由USB任务更新，不依赖未启用的SOF ISR。
- [ESP-IDF5.5.2 I2S](https://github.com/espressif/esp-idf/tree/v5.5.2/components/esp_driver_i2s)：TX on_sent使用event.dma_buf、清零后回调、整数ISR渲染；不是event.data指针间接值。计数取实际DMA完成，不用入队量估算消费。
- [ESP-SR](https://github.com/espressif/esp-sr)：固定2.2.0，保留真实WakeNet接口/clean与Hi ESP模型。目标词状态见wake-word-model.md。

依赖与模型许可证见各managed component；USB VID/PID为开发值。历史Voice PE证据在`docs/history`和已标记superseded的旧OpenSpec，不能证明当前硬件有效。

## 2026-09-18 配置核验补充

- [固定版本命令表](https://github.com/respeaker/reSpeaker_XVF3800_USB_4MIC_ARRAY/blob/a652fe79da3a292b25decc0e1e7f267d29bb0284/python_control/xvf_host.py)：BLD_MSG(48,1)最多50字节；USB_BIT_DEPTH(48,8)在INT模式为(0,0)；OP_PACKED(35,13)、OP_UPSAMPLE(35,14)；AEC_AECCONVERGED(33,3)为int32，SHF_BYPASS(33,70)为uint8，AUDIO_MGR_REF_GAIN(35,1)为float。构建信息作为有界诊断元数据，不假定未经实机确认的名称。
- [XMOS输出调谐说明](https://www.xmos.com/documentation/XM-014888-PC/html/modules/fwk_xvf/doc/user_guide/04_tuning_the_application.html)：内部处理后的16 kHz语音在48 kHz输出总线上需要上采样；不能把host loopback示例的上采样关闭设置用于处理后语音。
- [XVF3800介绍](https://wiki.seeedstudio.com/respeaker_xvf3800_introduction/)：安装需关注进音孔一面和朝向。[XIAO GPIO说明](https://wiki.seeedstudio.com/respeaker_xvf3800_xiao_gpio/)用于X0D31功放低有效、X0D33灯电源与X0D30静音辨识；功放优化留待实测。
- ReSpeaker Lite/XU316、XVF3000、Pi HAT及旧阵列资料只作产品差异参考，其固件、GPIO、驱动与AEC能力不作为本XVF3800+XIAO实现参数。
