---
title: 'Single-sector directional capture gate'
type: 'feature'
created: '2026-09-20'
status: 'in-progress'
route: 'dispatch'
baseline_commit: '55bfc45a5948d3ec779bfb87c6ae520b653030b2'
review_loop_iteration: 0
context: []
---

<frozen-after-approval reason="User explicitly requested implementation of the narrowed directional gate">

## Intent

Implement the user's request: allow pickup from a specified direction and reject other directions where practical, on existing VoiceKey hardware. Simultaneous-speaker isolation is an experimental outcome, not a promised capability. Deliver buildable firmware, observable diagnostics, tests and a manual acoustic experiment. No Home Assistant functionality.

## Boundaries & Constraints

**Always:** Preserve XVF3800 1.0.8 INT/I2S -> XIAO -> USB architecture, speaker playback/AEC reference, physical mute priority and failure-closed safety. Keep polling direction while the software gate is closed. Use existing I2C task and bounded retries, never audio/USB-thread bus I/O. Apply the sector to USB microphone and WakeNet; reject pending/stale wake triggers on gate closure. Keep HID release working. Expose compile-time enable, center and half-width; defaults enabled, device-native 0 degrees +/-30 degrees inclusive, physically calibrated by diagnostics. No claim device-native zero equals enclosure front. Provide disabled build for A/B control.

**Never:** HA, Wi-Fi, MQTT, new Mac background service, voiceprints, source separation, fixed-beam tuning, AGC fixes, hardware mute writes, deployment/flashing, or a first-speaker lock in this increment. Do not silently cure or obscure BUG-001. Keep source and currently flashed firmware distinguished. No commit or push.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|---|---|---|---|
| Allowed | Fresh valid speech direction in inclusive sector | After short continuous confirmation, new PCM reaches USB and wake | Candidate reset if interrupted |
| Rejected | Fresh speech outside sector | Immediately close gate, clear queued PCM/pending wake | No previous audio replay |
| Silence | Valid no-speech | Gate closed; keep observing | Do not use undefined silence DOA to open |
| Unknown | Failed/short/busy-exhausted read, invalid direction/speech, stale/future timestamp | Gate closed | New valid sustained speech required to reopen |
| Wrap | Center 0, half-width30; 330,0,30 vs329,31 | First three inside; last two outside | Circular arithmetic |
| Mute/fault | Physical mute or unsafe board/clock | Capture and new trigger remain blocked regardless of direction | Playback follows existing independent safety rules |
| Disabled | Direction feature off | Existing omnidirectional behavior | No extra direction I2C traffic |
| Reopen | Block captured across direction transition / old inference | Reject old data/result, admit only fresh capture | No hardware mute or global transport reset |

</frozen-after-approval>

## Code Map

- `firmware/components/voicekey_core/voicekey_xvf.c`, `include/voicekey_xvf.h`: vk_xvf_read validates status/length, three busy attempts; control profile pins1.0.8. Official Seeed DOA example uses res20/cmd18, four payload bytes: little-endian uint16 angle and speech flag. Reject angle>=360 or speech>1. Source: https://wiki.seeedstudio.com/respeaker_xvf3800_xiao_doa_vad/ .
- `firmware/main/board.c`, `board.h`: single I2C task, 20ms loop, mutex snapshot, existing100ms safety freshness. Publish separate direction validity/timestamp; telemetry failure must not set physical mute or reconfigure XMOS. Poll direction before final required safety sample if needed to preserve safety freshness; account for transaction duration.
- `firmware/components/voicekey_core/voicekey_core.c`, `include/voicekey_core.h`: audio/wake FIFO and epochs, pending HID state. Direction gate should be distinct from physical mute. Existing invalidate bumps audio epoch, and capture resets RX on that epoch; avoid repeated hardware RX resets for normal sector transitions. Add a separate generation/token if necessary to reject in-flight old blocks.
- `firmware/main/runtime.c`, `audio_input.c`: callbacks enforce gate at publish, USB dequeue, wake read and trigger. Read board snapshot coherently; no lock inversion. Playback untouched. Existing warmup/rate qualification stays.
- `firmware/main/Kconfig.projbuild`: user compile-time settings. `sdkconfig.defaults` and build wrapper must actually enable default feature in real builds with persisted sdkconfig considered.
- `scripts/diagnose.py`, USB diagnostic header/descriptors, `tests/test_diagnostics.py`: existing192-byte schema3 report full. Extend length with explicit schema compatibility, preserve old decoder support; keep HID descriptor size<256 and host requests consistent. Existing offsets can be retained with appended direction fields.
- `tests/test_core.c`, `test_audio.c`, `test_runtime.c`, `test_audio_adapter.c`, `test_board_adapter.c`: portable tests and actual adapter simulation; new config must not accidentally bypass active-feature tests. `scripts/test.sh`, `test-usb.sh`, `test-descriptors.sh`, `build.sh`, `validate-images.py` are checks.

## Tasks & Acceptance

**Execution:**
- [ ] Core direction logic/protocol files: implement fresh-sample circular sector gate with short configurable confirmation (default60ms), no silence hold/pre-roll; timestamps, malformed data and disable behavior tested.
- [ ] Board/runtime/audio files: integrate independent software gate, enforce freshness on consumers and invalidate pending/in-flight audio and wake without resetting playback or physical capture on each direction transition.
- [ ] Kconfig and diagnostics/decoder: expose enable/center/width/confirmation, angle/speech validity+age and gate/candidate state; backward-compatible old diagnostics decode.
- [ ] Tests/scripts: cover every matrix row, real adapter integration, gate disabled baseline, failed reads and delayed polling; register all new sources/tests with build scripts and CMake.
- [ ] `docs/directional-capture.md`, `README.md`: describe settings, device-native calibration, A/B recording procedure, shared-speaker limitation, polling/confirmation latency and clipped beginnings, BUG-001 confound; distinguish compiled from flashed/verified.

**Acceptance Criteria:**
- Given active USB and qualified clock, when direction becomes rejected/stale, then USB supplies silence and wake cannot trigger while playback still progresses.
- Given recovery to allowed speech, when confirmation completes, then only fresh audio is accepted without replay of rejected or pre-transition samples.
- Given default and disabled configurations, when test suites and cross-build run, then both feature paths are covered and production firmware builds.
- Given two simultaneous speakers, when running manual experiments, then record observed leakage and target retention; software checks must not be labeled proof of acoustic isolation.

## Implementation Notes

Scope authorized by the latest user message. Defaults taken from the discussed single-sector proposal; compile-time tunable. No device write in this task. Investigation and review use subagents as directed by bmad-build.

## Spec Change Log

## Review Triage Log

## Verification

Run scripts/test.sh, scripts/test-usb.sh, scripts/test-descriptors.sh, scripts/build.sh and scripts/validate-images.py. Add focused meaningful gate and protocol tests, not merely metadata checks. Record firmware build identity and explicit pending physical calibration/acoustic validation.
