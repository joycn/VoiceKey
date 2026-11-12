## Purpose

Provide composable voice input primitives for fixed-seat interactions, separating observed directions, attributed requests, floor ownership and outgoing audio without claiming speaker identity or source separation.

## ADDED Requirements

### Requirement: Calibrated timestamped observations
The system SHALL expose direction and speech observations with validity and monotonic timestamps, allow a calibration offset, and map directions to bounded registered sectors. In enforcing modes, ambiguous overlap, invalid values, future timestamps or samples older than100ms SHALL not authorize audio.

#### Scenario: Circular sector and overlap
- **WHEN** a sector centered at0 has half-width30 and observations are330,0,30,31 degrees
- **THEN** the first three match and31 does not; matching multiple sectors yields unknown rather than arbitrary ownership

#### Scenario: Observation unavailable
- **WHEN** an enforcing mode sees a failed or stale direction read
- **THEN** spatial output closes without muting playback or stopping observation

### Requirement: Explicit floor ownership
The system SHALL offer grant, request, release and revoke operations with a changing generation, independently from silence. An optional first-stable-direction policy SHALL acquire a direction after continuous confirmation and release after configurable inactivity. Manual and keyword policies SHALL retain ownership through silence until explicitly changed or revoked for safety.

#### Scenario: First direction wins
- **WHEN** A finishes stable confirmation before B and B speaks during A's lease
- **THEN** B does not inherit or extend A's lease and output requires both A's sector and locked direction tolerance

#### Scenario: Explicit preemption
- **WHEN** a valid request permits preemption of A by B
- **THEN** ownership and generation change to B and old outgoing audio is discarded

### Requirement: Source-aware keyword events
The system SHALL retain existing keyword detection as an independent listener and expose attribution to a region or unknown. It SHALL reject stale inference results and ambiguous source evidence. In keyword-floor mode, a valid other-seat wake SHALL switch the floor rather than repeatedly toggle the ChatGPT shortcut; an initial acquisition can invoke the existing shortcut.

#### Scenario: Other seat requests while gate closed
- **WHEN** USB forwards A but stable registered B supplies the keyword
- **THEN** B's listener input remains available and a fresh attributed event can acquire the floor

#### Scenario: Direction changes during inference
- **WHEN** source context changes or physical mute occurs before an old detection completes
- **THEN** the old event does not change ownership or trigger HID

### Requirement: Independent output gate
The system SHALL separate physical capture safety, keyword listening and outgoing audio gates. It SHALL preserve physical mute priority, USB enumeration and playback/AEC; reject old capture tokens across routing changes; and produce zero samples while output is denied. It SHALL not claim that gating separates simultaneous voices.

#### Scenario: Physical mute
- **WHEN** hardware mute is active
- **THEN** both keyword admission and outgoing capture close, pending triggers and floor clear, and playback keeps its existing independent behavior

#### Scenario: Normal output rejection
- **WHEN** the observed region differs from the floor owner
- **THEN** output contains silence, keyword listening continues for registered regions, and the capture transport is not reset merely for the gate change

### Requirement: Observable opt-in composition
The system SHALL supply documented C APIs, an opt-in firmware configuration, and backward-compatible diagnostic decoding. Default disabled mode SHALL preserve current VoiceKey behavior. Observation-only mode SHALL collect direction without spatially filtering the original audio or granting floor. Source-selection capabilities SHALL explicitly reject unavailable directional beam selection rather than pretend it exists.

#### Scenario: Disabled mode
- **WHEN** spatial mode is disabled
- **THEN** no direction I2C polling is added and existing audio/wake behavior remains

#### Scenario: Diagnostic inspection
- **WHEN** spatial mode is enabled
- **THEN** diagnostics expose direction validity/age, mapped region, owner, generation and output decision; old reports still decode

#### Scenario: Observe without gating
- **WHEN** observation-only mode receives an unregistered direction
- **THEN** original audio remains available, direction is reported and floor requests are rejected
