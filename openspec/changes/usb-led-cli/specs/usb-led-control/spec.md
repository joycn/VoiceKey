## ADDED Requirements
### Requirement: CLI LED control
The tool SHALL support RGB hex colors, named colors, off and automatic mode over the existing HID interface and reject unsupported firmware before writing.
#### Scenario: Set a color
- **WHEN** the user requests a valid color on release1.4 firmware
- **THEN** the CLI sends a validated RGB command and reports submission, not physical completion
#### Scenario: Old firmware
- **WHEN** the reported protocol release is older than1.4
- **THEN** the CLI fails without sending a command
### Requirement: Safe LED arbitration
The firmware SHALL process only correctly framed commands and apply user RGB as a static color through the board control task while preserving mute and fault priority.
#### Scenario: Physical mute
- **WHEN** a user color is active and the microphone is physically muted
- **THEN** the mute indication takes precedence and the user color resumes after unmute
#### Scenario: Automatic restoration
- **WHEN** the user requests auto
- **THEN** the existing automatic LED policy resumes
#### Scenario: Invalid command
- **WHEN** a command has invalid length, version, opcode or reserved bytes
- **THEN** no LED override changes
