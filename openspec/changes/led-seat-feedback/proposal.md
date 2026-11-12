## Why
Tabletop multiplayer users need to see which seat owns the microphone and whether an interruption succeeded. Raw RGB commands do not express these states.
## What Changes
- Complete raw LED controls and add layout, identify, timed preview and automatic real-state rendering.
- Add device request acknowledgements and schema5 diagnostics over existing HID.
- Keep display commands separate from audio ownership; preserve mute/fault priority.
## Capabilities
### New Capabilities
- `led-seat-feedback`: LED seat mapping, render arbitration, CLI and verified command state.
### Modified Capabilities
None.
## Impact
VoiceKey core/board/runtime, HID report size, CLI, diagnostics, tests and documentation. Existing uncommitted voice primitives and basic LED work are preserved.
