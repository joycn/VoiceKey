# 音箱无声排查

用户报告灯环不亮并确认音箱无声。CoreAudio默认输出为VoiceKey、48kHz双声道，默认输入为VoiceKey16kHz单声道。系统输出未静音，标量0.49，实际主音量-18dB；此衰减不等于静音。HID可读，正在接收USB播放帧且I2S时钟约48kHz，错误及传输故障0；缓存非空并不证明内容非零或模拟接口发声。

播放一次2秒440Hz、峰值约-12dBFS、首尾50ms渐变的双声道测试音，经系统默认VoiceKey输出，afplay退出成功。未改变系统音量。等待用户确认是否听到；进程成功不证明音箱发声。当前HID不含DAC状态及播放实际PCM峰值，尚不能排除下游路由、供电、模拟输出或音箱问题。

当前快照：

```json
{
  "i2c_errors": 0,
  "i2c_busy": 4936,
  "cache_age_ms": 29,
  "usb_samples": 224,
  "wake_samples": 368,
  "playback_frames": 208,
  "usb_dropped": 0,
  "usb_missing": 0,
  "wake_dropped": 0,
  "playback_dropped": 0,
  "playback_missing": 1200,
  "i2s_consumed_frames": 4515936,
  "schema": 3,
  "xmos_version": "1.0.8",
  "board_valid": true,
  "microphone_muted": false,
  "i2s_active": true,
  "firmware_release": "1.2",
  "runtime_flags": 127,
  "application_elf_sha256_prefix": "3d9d55850e54086c",
  "board_init_error": 0,
  "audio_init_error": 0,
  "wake_init_error": 0,
  "psram_bytes": 8388608,
  "free_heap_bytes": 8336444,
  "minimum_free_heap_bytes": 8326144,
  "capture_fault_generation": 0,
  "transport_faults": 0,
  "wake_ready": true,
  "initialization_complete": true,
  "transport_ready": true,
  "profile_error": "ok",
  "int_profile_compatible": true,
  "capture_clock_qualified": true,
  "output_packed": [
    0,
    0
  ],
  "output_upsample": [
    1,
    1
  ],
  "usb_bit_depth": [
    0,
    0
  ],
  "measured_i2s_rate_hz": 47999,
  "aec_snapshot_valid": true,
  "aec_age_ms": 668,
  "aec_converged": false,
  "aec_bypass": false,
  "reference_gain": 8.0,
  "xmos_build_message": "inthost-lr48-sqr-i2c",
  "led_snapshot_valid": true,
  "led_power_enabled": true,
  "led_speed_fallback": false,
  "led_effect": 5,
  "led_brightness": 127,
  "led_speed": 8,
  "led_gamma": 1,
  "led_age_ms": 1260,
  "agc_snapshot_valid": true,
  "agc_gain": 2.2994420528411865,
  "agc_age_ms": 414,
  "wake_diagnostics_available": true,
  "wake_epoch": 4,
  "wake_inferences": 2931,
  "wake_detections_mod65536": 0,
  "hid_presses_mod65536": 0,
  "wake_pcm_peak": 18,
  "trigger_suppressed_mod65536": 0,
  "capture_gate_closed": false,
  "host_microphone_muted": false
}
```
