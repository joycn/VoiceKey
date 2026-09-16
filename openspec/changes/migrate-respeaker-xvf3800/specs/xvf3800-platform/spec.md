## ADDED Requirements

### Requirement: Pinned XVF3800 platform

The firmware SHALL target only reSpeaker XVF3800 with standard XIAO ESP32S3,8MB Flash/8MB Octal PSRAM, pinned IDF5.5.2/TinyUSB0.18.0~6/ESP-SR2.2.0 and XMOS I2S-master1.0.8_48k image with recorded commit and SHA256. It SHALL retain NVS/PHY,4MB application and3MB model within8MB, without OTA, led_strip or UART0 console.

#### Scenario: Build validation
- **WHEN** firmware is built
- **THEN** actual sdkconfig, image headers, binary partitions and image bounds are validated against8MB and the pinned manifest.

### Requirement: Bounded control and mute authority

The firmware SHALL use I2C SDA5/SCL6/address0x2C,VERSION(48,0), left processed auto-select beam(6,3) with readback and GPO_READ_VALUES X0D30 high as mute. A dedicated20ms task SHALL validate status/length, bound busy64 retries and timeout, and close capture/new wake on failure or cache age>100ms. It SHALL never command unmute or auto-flash XMOS.

#### Scenario: Fault and recovery
- **WHEN** a control read fails, returns malformed status or becomes stale
- **THEN** capture and new wake close, old inference is invalidated, and recovery begins with fresh samples while playback remains independent.
