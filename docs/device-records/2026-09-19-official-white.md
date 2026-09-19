# Official RGB example, white variant

User requested the official example with white replacing orange. Idle effect 1, speed 1, brightness 255, color 0xFFFFFF. Supersedes the unflashed speed-5 experiment. White does not write or enforce gamma; it retains the current XMOS value, not necessarily a factory default. Rainbow and other states retain speed 1, brightness 30, gamma 0.

Adapter tests, cross-build and partition/image validation passed. All four flashed images passed device hash verification.

- Application SHA256: f7425d63db6856454fffbf5fe58b19c0e32c8bcde71790e16f7676d932d9ef27
- ELF SHA256: da4351d17bc5eafdd0def03230ff81e10ae0aa0a3544fc35df80ca5384f74592
- Runtime ELF prefix matched. Readback: effect 1, brightness 255, speed 1, gamma 0.
- Post-flash baseline: 51 samples over approximately 10 seconds. I2C errors, capture fault generation and transport faults remained zero.

User confirmed the white breathing effect is normal after flashing. Wake/rainbow transition has not been separately reconfirmed for this build. BUG-001 audio gain issue is not fixed by this change.

Source: https://wiki.seeedstudio.com/respeaker_xvf3800_xiao_rgb/
