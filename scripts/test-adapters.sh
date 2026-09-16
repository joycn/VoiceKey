#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$repo_root/build-host"
for name in runtime audio_adapter board_adapter; do
  "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -g -fsanitize="${SANITIZERS:-undefined}" \
    -I"$repo_root/tests/runtime_shim" -I"$repo_root/tests/usb_shim" \
    -I"$repo_root/firmware/components/voicekey_core/include" \
    -I"$repo_root/firmware/components/voicekey_usb/include" \
    "$repo_root/firmware/components/voicekey_core/voicekey_core.c" \
    "$repo_root/firmware/components/voicekey_core/voicekey_audio.c" \
    "$repo_root/firmware/components/voicekey_core/voicekey_xvf.c" \
    "$repo_root/tests/test_${name}.c" -lm -o "$repo_root/build-host/test_${name}"
  "$repo_root/build-host/test_${name}"
done
