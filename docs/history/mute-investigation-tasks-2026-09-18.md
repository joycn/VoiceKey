# 静音排查历史任务（当前实现已撤回）

## 6. Control turnaround repair

- [x] 6.1 Add bounded task-side command settling and regression coverage for asynchronous readiness/busy retries; cross-build and validate images.
- [ ] 6.2 Flash the repair and compare control error rates, cache validity, repeated physical mute recovery and capture quality on hardware.

- [x] 6.3 Add busy backoff and staggered housekeeping with real safety-read timestamps; verify bounded steady-state transactions and cross-build.
- [ ] 6.4 Flash staggered candidate and verify zero control faults/stale closures over a bounded device run, then repeat physical mute and capture tests.


## 7. Post-mute low recording level

- [x] 7.1 Add read-only gain/host-mute/capture-gate diagnostics with stale/partial snapshot tests, cross-build and image validation.
- [ ] 7.2 Flash and compare gain/state before mute, during mute and after release with controlled audio; identify and repair the actual attenuation cause.

- [x] 7.3 Evaluate a below-unity AGC fast-recovery candidate: software checks passed, hardware trial failed (no audible voice), and candidate source changes were withdrawn.
- [ ] 7.4 Restore read-only diagnostic firmware, power-cycle XMOS to remove volatile candidate tuning, then isolate actual PCM/recording levels before further repair.
