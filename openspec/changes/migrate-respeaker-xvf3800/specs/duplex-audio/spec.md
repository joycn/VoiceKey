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

### Requirement: HID diagnostics and F18

The firmware SHALL retain F18 keyboard behavior and expose version/build identity, board state, buffers, error counters, wake readiness/init errors and actual PSRAM/heap through an existingEP0 HID Feature Report without CDC or diagnostic endpoints, with a one-shot host script.

#### Scenario: Diagnostic request
- **WHEN** a host requests the96-byte schema2 Feature Report
- **THEN** it receives schema/firmware/XMOS identity and snapshot diagnostics without changing microphone mute or playback state.
