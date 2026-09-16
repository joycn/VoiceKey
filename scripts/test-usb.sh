#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
tinyusb_root="$repo_root/firmware/managed_components/espressif__tinyusb"
if [[ ! -f "$tinyusb_root/src/tusb.h" ]]; then
    echo "Run scripts/build.sh reconfigure first to resolve TinyUSB." >&2
    exit 1
fi
mkdir -p "$repo_root/build-host"
"${CC:-cc}" -std=gnu11 -Wall -Wextra -Werror -g -fsanitize="${SANITIZERS:-undefined}" \
    -DTUP_DCD_ENDPOINT_MAX=8 -DCFG_TUSB_MCU=OPT_MCU_NONE -DCFG_TUSB_OS=OPT_OS_NONE \
    -I"$repo_root/tests/usb_shim" -I"$tinyusb_root/src" \
    -I"$repo_root/firmware/components/voicekey_usb/include" \
    -I"$repo_root/firmware/components/voicekey_core/include" \
    "$repo_root/firmware/components/voicekey_usb/usb_device.c" \
    "$repo_root/firmware/components/voicekey_core/voicekey_core.c" \
    "$repo_root/firmware/components/voicekey_core/voicekey_audio.c" \
    "$repo_root/tests/test_usb.c" -lm -o "$repo_root/build-host/test_usb"
"$repo_root/build-host/test_usb"
