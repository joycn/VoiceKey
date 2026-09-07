#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
tinyusb_root="$repo_root/firmware/managed_components/espressif__tinyusb"
if [[ ! -f "$tinyusb_root/src/tusb.h" ]]; then
    echo "Run scripts/build.sh reconfigure first to resolve the pinned TinyUSB dependency." >&2
    exit 1
fi
mkdir -p "$repo_root/build-host/descriptor-include"
cat > "$repo_root/build-host/descriptor-include/sdkconfig.h" <<'CONFIG'
#define CONFIG_VOICEKEY_USB_VID 0xCAFE
#define CONFIG_VOICEKEY_USB_PID 0x4014
CONFIG
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -g -fsanitize="${SANITIZERS:-undefined}" \
    -DTUP_DCD_ENDPOINT_MAX=8 -DCFG_TUSB_MCU=OPT_MCU_NONE -DCFG_TUSB_OS=OPT_OS_NONE \
    -I"$repo_root/build-host/descriptor-include" -I"$tinyusb_root/src" \
    -I"$repo_root/firmware/components/voicekey_usb/include" \
    "$repo_root/firmware/components/voicekey_usb/usb_descriptors.c" \
    "$repo_root/tests/test_descriptors.c" -o "$repo_root/build-host/test_descriptors"
"$repo_root/build-host/test_descriptors"
