## Context

See proposal.md. Runtime currently routes the same16k stream to independent USB/WakeNet FIFOs but has one capture-mute state. Every capture epoch change resets RX; spatial gating must not use that path. Board I2C already has a100ms safety age and bounded retries. Existing BUG-001 is not fixed here.

## Goals / Non-Goals

**Goals:** Portable allocation-free primitives plus real board/runtime integration, callable firmware controls, explicit uncertainty and experimentally testable behavior.

**Non-Goals:** Game engine, host control UI, automatic seat enrollment conversation, identity, multi-source transcription, new wake model, fixed-beam DSP or physical deployment.

## Decisions

- Add voicekey_spatial module: up to8 sectors, integer circular angles, monotonic observations, independent validity, stable candidate, floor owner/angle/generation, manual/first/keyword policies. Region IDs are indices; overlapping sectors return unknown. Calibration adds a signed offset to device angles.
- Observation-only configuration reads direction without changing the legacy USB/listener stream or granting floor, enabling paired baseline recordings.
- Configuration APIs are transactional; invalid settings leave the current configuration intact. Runtime defaults disabled, optional build default enables a single calibrated0+/-30 sector with first-speaker policy. Callers can install multiple seats and request/grant/release through C APIs; no remote protocol is implied.
- First policy defaults250ms confirmation,20-degree lock tolerance,2s matching-speech lease. Manual/keyword floor persists through silence. A generation change invalidates USB backlog and the listener epoch, including pending old actions. Direction failure or physical safety failure revokes ownership, requiring reacquisition.
- Read res20/cmd18 four-byte DOA/speech using existing protocol helper, timestamp before transaction; treat invalid/suspicious payload as unavailable. Do not change XMOS routing or physical mute. Add observation read before safety poll so existing mute sample remains fresh; test actual adapter schedule. Official Seeed example: https://wiki.seeedstudio.com/respeaker_xvf3800_xiao_doa_vad/ .
- Core output gate is separate from capture mute. Capture carries a routing token in addition to existing audio epoch; changes discard in-flight old blocks and drain three RX descriptors plus the currently filling descriptor without resetting I2S. Wake observes all registered regions independently of output owner. Fresh speech region transitions or unknown source invalidate wake queues/model epoch; silence is zero-fed while preserving source continuity. Detection uses that bound epoch/source segment, never only the DOA at inference completion. Overlap remains uncertain in the physical source itself.
- Expose bounded timestamp-history attribution API for callers that know event start/end. Runtime WakeNet has no exact word boundaries, so use conservative continuous single-region model context; document missed keywords during switching as a tradeoff.
- Preserve the automatic processed beam as the only supported source selection. Expose unsupported result for requested isolated/fixed beam. Future hardware adapter can implement it without changing floor policy.
- Extend diagnostic report under a new length/schema while preserving original offsets and legacy decoding; return actual output permission, not just a UI color. No I2C in USB or audio callbacks.

## Risks / Trade-offs

- Aggressive fail-close and source-epoch resets can clip words or reduce wake sensitivity -> opt-in, measured experiments and explicit source-binding diagnostics.
- Extra I2C transactions may exceed safety budget -> timestamp observations independently; bounded failure closes gate; host timing tests plus required physical measurements.
- Software cannot prove acoustic source identity -> distinguish direction attribution from physical isolation; record leakage, ambiguity and misses.
- Silence does not finish a game turn -> manual/keyword owners persist, first-policy lease is optional.

## Migration Plan

Build/test default and enabled paths, document API composition and experiments. Do not flash. Keep spatial feature disabled for legacy behavior until seats are calibrated. Deploy and physical/acoustic acceptance remain explicit unchecked tasks.
