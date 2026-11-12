#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$repo_root/build-host"
for name in runtime spatial_runtime spatial_boot audio_adapter board_adapter; do
  test_source="$name"
  extra_cflags=(-UCONFIG_VOICEKEY_SPATIAL)
  if [[ "$name" == spatial_boot ]]; then
    test_source=spatial_runtime
    extra_cflags=(-DCONFIG_VOICEKEY_SPATIAL=1)
  fi
  "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -g -fsanitize="${SANITIZERS:-undefined}" \
    "${extra_cflags[@]}" \
    -I"$repo_root/tests/runtime_shim" -I"$repo_root/tests/usb_shim" \
    -I"$repo_root/firmware/components/voicekey_core/include" \
    -I"$repo_root/firmware/components/voicekey_usb/include" \
    "$repo_root/firmware/components/voicekey_core/voicekey_core.c" \
    "$repo_root/firmware/components/voicekey_core/voicekey_audio.c" \
    "$repo_root/firmware/components/voicekey_core/voicekey_xvf.c" \
    "$repo_root/firmware/components/voicekey_core/voicekey_spatial.c" \
    "$repo_root/firmware/components/voicekey_core/voicekey_led.c" \
    "$repo_root/tests/test_${test_source}.c" -lm -o "$repo_root/build-host/test_${name}"
  "$repo_root/build-host/test_${name}"
done
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -g -fsanitize="${SANITIZERS:-undefined}" \
  -I"$repo_root/tests/wake_shim" -I"$repo_root/tests/runtime_shim" -I"$repo_root/tests/usb_shim" \
  -I"$repo_root/firmware/components/voicekey_core/include" -I"$repo_root/firmware/components/voicekey_usb/include" \
  "$repo_root/tests/test_wake_adapter.c" -o "$repo_root/build-host/test_wake_adapter"
"$repo_root/build-host/test_wake_adapter"
