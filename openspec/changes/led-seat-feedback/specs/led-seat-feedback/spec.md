## ADDED Requirements
### Requirement: Seat layout and temporary previews
The CLI SHALL apply validated seat layouts and timed identify/preview requests without changing audio state.
#### Scenario: Conflicting arcs
- **WHEN** two regions map to any shared LED
- **THEN** reject the whole layout and keep the previous one
#### Scenario: Preview expires
- **WHEN** a timed preview expires after the real owner changes
- **THEN** display the current real owner, not the pre-preview owner
### Requirement: Real state and safety priority
The firmware SHALL render real owner and locally admitted speech independently, with fault and physical mute overriding all user effects.
#### Scenario: Another seat speaks
- **WHEN** A holds the floor and B speaks without acquiring it
- **THEN** keep the owner indicator at A
#### Scenario: Safety override
- **WHEN** a manual or preview request is masked by mute or fault
- **THEN** show the safety indication and report covered
### Requirement: Complete raw parameters and execution status
The CLI SHALL support all documented LED effect/color/brightness/speed/gamma/DOA/ring settings and correlate device execution outcomes by request ID.
#### Scenario: Partial update
- **WHEN** only brightness is specified
- **THEN** preserve other manual parameters
#### Scenario: I2C failure
- **WHEN** the requested LED write or readback fails
- **THEN** report failed instead of claiming applied
