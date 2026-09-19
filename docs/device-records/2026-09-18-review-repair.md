# Review repair: 2026-09-18

Status: software repaired and builds validated; NOT flashed. Mac had no ESP32 USB/ROM serial device; user explicitly reported being temporarily unable to operate BOOT/RESET. No hardware write was attempted in this repair turn. Prior first-flash results remain historical evidence only.

Changes:
- Playback now shares >=250ms measured48k clock qualification with capture, separately from microphone mute. Unqualified packets are discarded, qualification loss clears FIFO/DMA, and recovery accepts new data only. Detection remains windowed, not instantaneous.
- Added standalone internal-RAM USB Serial/JTAG maintenance project. Read-only XMOS queries, no PSRAM/I2S/model/TinyUSB startup, no DAC/XMOS writes. Added build/partition/image validator and negative tests.
- Updated OpenSpec/current product scope and added playback/DAC/AEC commissioning contract. No new analog/AEC success claimed.

Executed successfully: scripts/test.sh, scripts/test-usb.sh, scripts/test-descriptors.sh, scripts/build.sh, scripts/build-diagnostic.sh, tests/test_diagnostic_images.py, strict OpenSpec validation, shell/Python syntax and git diff whitespace checks. Adapter regression executes production runtime and audio callbacks; added16k rejection,48k recovery, rate-change rejection and no old sample replay. Maintenance negative tests use actual images under optimized Python and reject UART0, changed partitions, incorrect8MB headers and model-region writes.

Native tests do not reproduce electrical clocks, USB enumeration, PSRAM behavior or acoustics. Hardware tasks remain open. Maintenance build success does not prove USB console enumeration.

## Artifacts

| Image | Bytes | SHA256 |
| --- | ---: | --- |
| `firmware/build/voicekey.bin` | 412176 | `3c21c147f0a275ad658ce3d6d9bd62310208f804967016db03c1f0da10ea2864` |
| `firmware/build/bootloader/bootloader.bin` | 22464 | `85ab3f61e5d257cb9d1be152ae3a0e2111460b407df352991f8e60456765c71d` |
| `firmware/build/partition_table/partition-table.bin` | 3072 | `2b20bd22ecb09964c233a82504470c6818863aea98e4250ea4321d9e88fe8a10` |
| `firmware/build/srmodels/srmodels.bin` | 291142 | `2a162e78cb30938d9c55172df2a1b0f9bee307f8ee27ddf90e3ffbcd8ece736a` |
| `firmware/diagnostic/build/voicekey_diagnostic.bin` | 202032 | `7ba2a3e99ac27cdb888a7a843f205fa17bd1a27c05eceffa36f4a94c40852d7d` |
| `firmware/diagnostic/build/bootloader/bootloader.bin` | 22496 | `8474d6f7354ae7cd70876ab7071b864d80419a62f28805ed22818e5de3001118` |
| `firmware/diagnostic/build/partition_table/partition-table.bin` | 3072 | `2b20bd22ecb09964c233a82504470c6818863aea98e4250ea4321d9e88fe8a10` |

Next physical action: identify the ROM download device, run the documented maintenance flashing procedure, save console results, and only then restore/test the production image. See [maintenance](../maintenance.md) and [playback/AEC contract](../playback-aec-contract.md).
