## Why

VoiceKey needs reusable direction and floor-control capabilities for fixed-seat multiplayer voice interaction. Its current global capture/wake mute cannot express listening for requests from all seats while forwarding only the floor holder.

## What Changes

- Catalog observation, calibration, region mapping, stable direction acquisition, event attribution, floor ownership, source capability and audio gating primitives with explicit limits.
- Add bounded, allocation-free C APIs and integrate fresh XVF direction observations and independent USB/wake routing into VoiceKey.
- Allow manual seat selection/release and keyword-origin requests through firmware APIs; retain existing wake keyword detector and separate ownership changes from ChatGPT shortcut actions.
- Add opt-in spatial mode, diagnostics and tests; normal default mode preserves existing behavior.
- Provide acoustic experiment instructions, without claiming source separation or physical validation. HA, game rules, remote UI, new keyword models and flashing are outside this change.

## Capabilities

### New Capabilities
- `voice-input-primitives`: Timestamped direction, calibrated regions, source attribution, floor generations and independent capture routing APIs.

### Modified Capabilities

None; no canonical existing specs are present. The active XVF migration remains independent.

## Impact

Portable voicekey_core, XVF protocol reader, board snapshots, runtime/audio/wake integration, diagnostic HID report and decoder, host tests, documentation. Existing USB audio formats, physical mute, playback/AEC and pinned XMOS firmware remain. No new runtime dependencies.
