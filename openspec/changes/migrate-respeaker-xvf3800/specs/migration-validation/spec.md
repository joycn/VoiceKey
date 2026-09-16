## ADDED Requirements

### Requirement: Separate software and device evidence

The project SHALL update current product/technical documents and retain old Voice PE planning/evidence as superseded. It SHALL run production-path protocol/FIR/buffer/drift/lifecycle and actual USB callback/descriptor tests, cross-build and image bounds checks, record fresh hashes and retain hardware/AEC/acoustic acceptance as pending while hardware is absent.

#### Scenario: Software checks pass
- **WHEN** all native tests and the8MB cross-build pass
- **THEN** only software checks are marked complete; actual enumeration, timing, speaker/AEC, far-field quality and Mac behavior remain unchecked.

### Requirement: Development word disclosure

Hi ESP SHALL remain explicitly a development model. Hey Chat/Hello Chat SHALL remain incomplete until compatible authorized models and acceptance evidence exist. A short pause after wake SHALL remain required, and detection LED SHALL not claim application capture readiness.

#### Scenario: Development model detects
- **WHEN** Hi ESP detects and emits F18
- **THEN** documentation shall not declare target-word completion or zero-pause ChatGPT readiness.
