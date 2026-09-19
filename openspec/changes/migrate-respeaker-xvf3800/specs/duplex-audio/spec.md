## ADDED Requirements

### Requirement: Shared filtered capture

The firmware SHALL continuously receive48k32-bit stereo I2S with XVF3800 master/XIAO slave on BCLK8/WS7/RX43/TX44, filter only left with a stateful decimate3 FIR before saturating s16 conversion, and feed independent USB/WakeNet queues from the same16k result. FIR SHALL meet≤0.2dB ripple through6.5kHz and≥60dB attenuation from8kHz. Mute/failure/recovery SHALL reset history, queues and inference epochs.

#### Scenario: Chunking and mute
- **WHEN** identical input arrives with different chunk boundaries or mute interrupts inference
- **THEN** continuous output is chunk-equivalent and mute recovery cannot replay pre-mute filter history, samples or inference.

### Requirement: Independent speaker and measured feedback

UAC2 SHALL expose16k16-bit mono IN and48k16-bit stereo OUT with asynchronous explicit feedback based on actual I2S consumption and bounded queue correction. Playback SHALL apply master volume/mute before32-bit XMOS output and AEC reference. Physical microphone mute SHALL never stop playback. Stop/underflow/disconnect/reconnect SHALL zero unavailable output and discard historical audio.

#### Scenario: Drift and host stall
- **WHEN** independent clocks drift or the host stops supplying packets
- **THEN** queues remain bounded, feedback follows measured consumption, shortages become silence and WakeNet is not blocked by playback.

### Requirement: HID diagnostics and Shift+Option+Command+S

The firmware SHALL retain Shift+Option+Command+S keyboard behavior and expose version/build identity, board state, buffers, error counters, wake readiness/init errors and actual PSRAM/heap through an existingEP0 HID Feature Report without CDC or diagnostic endpoints, with a one-shot host script.

#### Scenario: Diagnostic request
- **WHEN** a host requests the192-byte schema3 Feature Report
- **THEN** it receives schema/firmware/XMOS identity and snapshot diagnostics without changing microphone mute or playback state.

### Requirement: Cached AEC observability

The firmware SHALL expose schema3 diagnostics with build metadata, format/profile status, measured sample rate and low-rate cached AEC convergence/bypass/reference gain. USB callbacks SHALL perform no I2C. Invalid or stale AEC data SHALL be distinguishable from valid zero values.

#### Scenario: AEC query failure
- **WHEN** an AEC snapshot cannot be completed
- **THEN** the snapshot is invalid, control failure closes capture, and later recovery uses fresh samples without changing physical mute or AEC tuning parameters.


### Requirement: Playback clock qualification

Playback SHALL require a fresh measured48k qualification independently of microphone mute. Unqualified or lost qualification SHALL clear queued/DMA playback and reject new packets until qualification recovers. Rate changes SHALL be detected within the measurement window; instantaneous detection is not promised.

#### Scenario: Wrong clock and recovery
- **WHEN** DMA progresses at16k rather than48k, or a qualified clock becomes incompatible
- **THEN** playback is closed after detection, emits zero samples, and recovery cannot replay packets supplied while unqualified.
