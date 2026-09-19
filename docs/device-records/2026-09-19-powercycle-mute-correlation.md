# 断电重启后静音恢复再次复现

用户已确认两个应用使用VoiceKey，且普通轻触外壳没有使声音变小。用户断电重启后提供本录音，标注为静音恢复后。5.439秒48kHz单声道，未增强/归一化32bit PCM解码。

遥测捕捉一次静音58.779秒、解除61.834秒，最新AGC为2.63666；63.675秒的新读数为0.0742916，下降约31dB。其后保持约0.069。遥测与录音没有共同绝对时间戳；单个静音边界的对应基于本次用户操作和先前提示，不能把它描述为独立精确同步测量。

录音0–1秒为解码后零信号；1–1.5秒含满幅强瞬态，0.5秒窗口RMS -14.19dBFS，接近满幅样本占该窗口1.75%；后续1.5–2秒RMS -48.04，2–3秒约-34至-36，3秒后-59至-77dBFS。全段RMS -24.48受强瞬态主导，不能作为正常人声音量。未人工听辨，不把尾段自然停顿误判为全部人声被压低。

断电重启并未消除静音恢复附近的增益下降；与此前多次循环相互印证。证据支持优先调查解除静音的瞬态、实体按键机械动作及XMOS内部状态，而不支持泛化为普通轻触外壳导致。具体触发来源仍未确认，不能宣布修复。

录音SHA256和半秒指标：logs/2026-09-19-powercycle-mute-audio.json。当前遥测快照（采样尚可能继续）：logs/2026-09-19-powercycle-recording-partial.jsonl。

```json
{
  "first_nonzero_seconds": 1.172,
  "near_fullscale_sample_count": 420,
  "near_fullscale_first_seconds": 1.2092291666666666,
  "near_fullscale_last_seconds": 1.2327291666666667,
  "telemetry_samples_at_analysis": 594,
  "telemetry_elapsed_at_analysis": 121.913,
  "deltas": {
    "i2c_errors": 0,
    "capture_fault_generation": 0,
    "transport_faults": 0,
    "usb_missing": 62
  }
}
```

完整采样随后完成，见logs/2026-09-19-powercycle-recording.jsonl。除先前分析的第一轮外，后续又捕捉到额外切换，不能把完整窗口描述为仅一次静音：

```json
[
  {
    "elapsed_seconds": 58.779,
    "microphone_muted": true,
    "agc_gain": 3.7203686237335205
  },
  {
    "elapsed_seconds": 61.834,
    "microphone_muted": false,
    "agc_gain": 2.6366615295410156
  },
  {
    "elapsed_seconds": 152.049,
    "microphone_muted": true,
    "agc_gain": 0.07145044207572937
  },
  {
    "elapsed_seconds": 153.27,
    "microphone_muted": false,
    "agc_gain": 0.07145044207572937
  }
]
```
