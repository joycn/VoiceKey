# 只读增益诊断版实机验证

同一ESP32-S3身份核对通过；四段写入哈希验证通过。运行ELF前缀6983a56b368cdaf2与本机构建一致。应用SHA-256为169a1fdd168542e6d525d7422456a6d813d3f0beb64006d6fb93a40fbcea96be。

30秒基线：

```json
{
  "samples": 151,
  "all_board_valid": true,
  "all_agc_valid": true,
  "gain_range": [
    0.7034512162208557,
    0.7765701413154602
  ],
  "deltas": {
    "i2c_errors": 0,
    "capture_fault_generation": 0,
    "transport_faults": 0,
    "usb_missing": 0,
    "wake_inferences": 967,
    "wake_epoch": 0
  }
}
```

仅证明诊断访问和该窗口的运行状态，未调AGC，不证明低音量修复。等待四轮静音和同步录音。
