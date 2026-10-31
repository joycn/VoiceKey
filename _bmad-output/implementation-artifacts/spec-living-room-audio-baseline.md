---
title: 'Living-room omnidirectional audio baseline'
type: 'chore'
created: '2026-10-31'
status: 'done'
route: 'oneshot'
review_loop_iteration: 0
---

<frozen-after-approval reason="human-owned intent">

## Intent

Configure the existing XVF3800 / XIAO firmware using Seeed's documented audio baseline and the installed WakeNet model's default threshold, with all-direction pickup for a living-room two-metre coverage target. The target requires acoustic validation and is not a software guarantee. Preserve existing USB, mute safety and LED work.

</frozen-after-approval>

## Implementation Notes

- Existing worktree contains user-owned spatial and LED changes; user explicitly requested continuation after being informed. Preserve those changes and make incremental edits.
- Small configuration change with no unresolved intent choices or irreversible operations. No hardware flash or XMOS persistent save in this implementation pass.
- `voicekey_xvf.c`: apply documented MIC_GAIN=90, REF_GAIN=8, SYS_DELAY=12 samples, FMIN_SPEINDEX=1300, AGCMAXGAIN=64, ASROUTGAIN=1 at initialization/reconfiguration with readback. Seed dynamic AGCGAIN=2 once per configuration; permit adaptive readback rather than exact equality. Existing failure path closes capture.
- `sdkconfig.defaults`: explicitly disable experimental spatial boot gate and set PCM gain 1. Existing cached sdkconfig already has matching effective values.
- `wake_engine.c`: retain model creation using DET_MODE_90 and no threshold override, including recreation after mute/fault; reinforce through adapter test.
- Official evidence: Seeed repository host_control/README.md tuning section and python_control/xvf_host.py command/type definitions, checked 2026-10-31. No invented distance parameter or changed gain time constants.
- Verify host tests, USB descriptors/transport tests and firmware build. Record physical two-metre tests separately; do not mark acoustic acceptance passed from software checks.

- Implementation complete: six static baseline values with exact readback; dynamic AGC seed accepts finite nonnegative gain up to 64. Read/write failure prevents capture. Existing physical mute and LED behavior preserved.
- Host regression suite, USB transport and descriptor tests passed under UBSan using an ad-hoc signing compiler wrapper because migrated Apple clang 12 emits unsigned ARM executables killed by this OS. Firmware build initially blocked by Intel ESP-IDF environment; native tools are being installed separately under ~/.espressif-arm64.

## Review Triage Log

- low / patched: cached sdkconfig overrides defaults; added explicit effective-config check and menuconfig correction instructions. Current local cached configuration already matches.
- low / deferred: baseline failures currently report generic VK_XVF_IO. Existing failure handling is safe and consistent, but resource/command-specific diagnostics would ease hardware troubleshooting. Defer diagnostic schema/API expansion from this configuration change.
- medium / patched: added read transport-error and short-response injection for all seven baseline entries, including dynamic AGC; each case verifies fail-closed behavior and successful full reconfiguration on the same controller after fault removal. Focused and complete host regression passed again.

## Verification

- Complete host suite, USB transport and descriptors PASS (UBSan); baseline read faults/recovery added and suite rerun successfully.
- Native ARM xtensa GCC 14.2.0 / ESP-IDF 5.5.2 direct idf.py build PASS; validate-images.py PASS. App SHA256: abb15d393fe06152e34c4643d1e6bd925fafe7df044206c10421756e5306e6bc (422608 bytes). Model SHA256: 56990e6b97faeced7aeea48c89f15b0d0c579c012a8106b91e96141f490051a6.
- Effective sdkconfig: spatial disabled, PCM gain 1, nihaoxiaozhi_tts enabled. No threshold override in either model creation path.
- No device flash, parameter readback on real hardware, or acoustic test performed. Two-metre coverage and existing mute/low-volume issue remain unverified.
- Build used ARM tools under ~/.espressif-arm64, Python requirements installed with uv due to pip SOCKS proxy incompatibility. CMake/Ninja from /tmp/voicekey-build-tools. Old build preserved at the path recorded in /tmp/voicekey-old-build-path.txt.
