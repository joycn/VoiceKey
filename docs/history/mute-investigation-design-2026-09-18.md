# 已撤回的静音排查设计（历史）

## Control turnaround timing repair

Live HID snapshots showed repeated busy responses and control failures while I2S remained near48kHz. The board transport now yields two FreeRTOS ticks at1kHz after a successful command write, before response reception or the next setting readback request. Every busy retry therefore also yields; the existing three-attempt and10ms driver-operation limits remain. This is a measured-hardware repair candidate, not a claim that2ms is a vendor-specified minimum. Transport failure still closes capture; cache timestamps remain conservatively anchored before the poll and the100ms freshness gate is unchanged. No GPIO/unmute writes or gain changes are added. Actual error-rate and mute recovery acceptance require flashing and repeat measurements.

The initial2tick-only device trial still produced control errors. A second candidate additionally waits10ms after busy64 before each remaining attempt (at most twice), retaining fail-closed behavior and conservative cache timestamps.

The10ms-backoff device run eliminated error increments over60seconds but observed cache ages102/147/113ms during batched housekeeping. The next candidate spreads steady-state format verification (one rotating parameter per100ms when AEC is not pending) and AEC collection (one of three parameters per poll, publishing only a complete set) across polls. Safety GPIO/I2S reads follow housekeeping; cache time is sampled immediately before these safety reads, never stamped after them. Configuration/recovery still validates the complete profile before opening capture. Poll overruns and retries can extend format sweep duration; no instantaneous format-change detection is claimed.100ms stale protection remains unchanged.


## Read-only post-mute amplitude diagnostics

User supplied before/after recordings under similar conditions show a large recorded-level difference despite reported mute release. Add extension1 in reserved bytes174–191 of the existing192-byte HID schema3:174=extension version,175 flags(valid gain snapshot,AGC enabled,USB host mic mute,runtime capture gate),176/180/184/188 float32 mic gain/current AGC gain/AGC target power/AGC time. Unknown extension or stale gain snapshots decode as null. Read35/0 and17/10,13,12,14 without writing tuning or mute parameters; collect one value only in an otherwise idle housekeeping slot, commit all five together, and invalidate on control failure. Query every2seconds and mark snapshots older than3seconds invalid. These temporally staggered values are diagnostic snapshots, not atomic hardware captures. No new endpoints or resident host software. Firmware state is not proof of acoustic recovery.

## Bounded low-gain recovery candidate

Device telemetry and user listening show low volume preceding mute and recovery with AGCGAIN rising from below0.05 to above13 while capture remains valid. Configure PP_AGCALPHAFASTGAIN(17,16,float32)=1.0 on initial/recovery configuration with exact readback. This selects the existing XMOS fast recovery path only below unity; do not change current AGCGAIN, target, maximum, time constants, physical mute or playback. Do not reset gain on mute release or persist XMOS tuning to flash. A failed write/readback closes admission. No extra steady-state transactions. The chosen threshold is an experimental project value, not a vendor-recommended preset, and acoustic acceptance remains pending. Rollback removes this setting and power-cycles both chips with the prior application, as XIAO reset alone does not restore XMOS volatile settings.

Reference: https://www.xmos.com/documentation/XM-014888-PC/html/modules/fwk_xvf/doc/user_guide/04_tuning_the_application.html (AGC Configuration). XMOS discourages arbitrary time-constant changes because speech levels may fluctuate. Fast-path selection must be verified against pinned Seeed1.0.8 on hardware; native tests cannot emulate that DSP.

### Candidate withdrawn after hardware failure

The below-unity fast-recovery candidate was rejected after the user reported no audible voice. Its17/16 write has been removed from current code. The preceding candidate section records the historical experiment only; current configuration preserves XMOS AGC settings. Restore the prior read-only diagnostics application and power-cycle both chips before any new acoustic baseline. The failed trial does not establish that AGC alone caused the original issue.
