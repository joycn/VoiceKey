## Why

Replace the obsolete Voice PE target with reSpeaker XVF3800 USB 4-Mic Array + standard XIAO ESP32S3. A single XIAO USB connection must provide microphone, ChatGPT playback and F18 without resident Mac software. Default active speakers connect to reSpeaker3.5mm, so the playback path supplies the XMOS AEC reference.

## What Changes

- Replace board bindings and firmware assumptions with pinned XVF3800 I2S-master1.0.8, 48k32-bit stereo duplex, 8MB Flash/8MB Octal PSRAM and no UART0/led_strip.
- Implement bounded I2C control cache, physical X0D30 mute authority, epoch recovery and FIR left48k→16k decimation.
- Add UAC2 speaker output, measured explicit feedback, playback master controls, and EP0 HID diagnostics.
- Deliver executable production-path tests, fresh cross-build/image bounds/hash evidence and updated current documentation.
- Preserve old change/evidence as superseded history. Hardware and target-word work remain incomplete.

## Capabilities

### New Capabilities

- `xvf3800-platform`: pinned hardware/protocol and memory layout.
- `duplex-audio`: FIR capture, independent playback, asynchronous USB and diagnostics.
- `migration-validation`: software evidence and pending hardware/target-word acceptance.

### Modified Capabilities

None. Prior Voice PE change was unarchived; this change replaces its current-target assumptions and retains its history.

## Impact

Changes firmware/main, voicekey_core/voicekey_usb, tests, scripts, README, BrainStorm and docs. No hardware writes, automatic XMOS flashing, additional USB endpoints for diagnostics, CDC, resident Mac agent, OTA or network audio. Hi ESP remains development-only; no claim of verified AEC/high fidelity or device acceptance.
