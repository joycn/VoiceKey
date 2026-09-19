## ADDED Requirements

### Requirement: Separate software and device evidence

The project SHALL update current product/technical documents and retain old Voice PE planning/evidence as superseded. It SHALL run production-path protocol/FIR/buffer/drift/lifecycle and actual USB callback/descriptor tests, cross-build and image bounds checks, record fresh hashes and retain hardware/AEC/acoustic acceptance as pending until supported by actual device evidence.

#### Scenario: Software checks pass
- **WHEN** all native tests and the8MB cross-build pass
- **THEN** only software checks are marked complete; actual enumeration, timing, speaker/AEC, far-field quality and Mac behavior remain unchecked.

### Requirement: 你好小智 model and acceptance

The firmware SHALL select bundled wn9_nihaoxiaozhi_tts for 你好小智, reject missing or mismatched models, and package the matching model partition. Recognition SHALL trigger Shift+Option+Command+S. Model availability and software checks SHALL NOT imply verified on-device recognition or acoustic quality. A short pause after wake remains required.

#### Scenario: Missing target model
- **WHEN** the model partition does not contain wn9_nihaoxiaozhi_tts
- **THEN** wake initialization fails explicitly instead of selecting a different wake word.

#### Scenario: Packaged target model
- **WHEN** the 你好小智 model is built successfully
- **THEN** recognition accuracy, far-field quality and Mac shortcut delivery remain pending until device evidence is recorded.

### Requirement: Independent maintenance diagnostics

A separate maintenance image SHALL report startup/reset and memory information over USB Serial/JTAG without production TinyUSB, I2S, PSRAM or model initialization, and perform only read-only XMOS queries. It SHALL preserve the production partition layout and provide a documented restoration path. Physical USB enumeration and audio/AEC acceptance SHALL remain required.

#### Scenario: Production USB unavailable
- **WHEN** the device can enter ROM download mode but production USB fails
- **THEN** an identified device can receive a validated maintenance image and expose startup/control evidence independently of production HID.
