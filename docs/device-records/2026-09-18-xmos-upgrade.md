# XMOS upgrade transfer — 2026-09-18

Matched DFU2886:001a serial114993702262500311 with Factory alt0, Upgrade alt1 and DataPartition alt2 before write. Verified pinned official application_xvf3800_i2s_master_v1.0.8_48k.bin:888832 bytes, SHA256d60d0bc2c7f5a67ffa9c9206e066b2d8f1ccb49ebba673f3a7bcff1cf197dfb2.

Executed `dfu-util -d 2886:001a -S 114993702262500311 -R -e -a 1 -D /tmp/application_xvf3800_i2s_master_v1.0.8_48k.bin`.

Exit0;888832 bytes downloaded, DFU manifestation status0 (no error), Done, USB reset issued. dfu-util0.11 warned about missing/invalid DFU suffix; transfer was accepted. Only Upgrade selected; no Factory or DataPartition write requested.

This proves successful DFU transfer/manifestation, not cryptographic readback of Flash or successful1.0.8 runtime. Next requires moving cable back to XIAO maintenance USB, reading VERSION/BLD_MSG and checking target profile. Then restore production firmware and test enumeration/audio. Current XIAO remains maintenance firmware.

[Transfer log](logs/2026-09-18-xmos-upgrade.log).
