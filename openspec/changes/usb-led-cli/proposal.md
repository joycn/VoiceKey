## Why
Mac users need to set the VoiceKey LED through the existing XIAO USB connection.
## What Changes
- Add validated HID LED commands, a Python CLI and automatic-mode restoration.
- Keep physical mute and fault indications above user colors; no automatic flashing.
## Capabilities
### New Capabilities
- `usb-led-control`: USB LED color override and CLI.
### Modified Capabilities
None.
## Impact
USB callbacks, board LED arbitration, protocol release 1.4, host scripts and tests. No new endpoints or dependencies beyond existing hidapi.
