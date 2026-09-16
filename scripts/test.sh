#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$repo_root/build-host"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -g -fsanitize="${SANITIZERS:-undefined}" \
  -I"$repo_root/firmware/components/voicekey_core/include" \
  "$repo_root/firmware/components/voicekey_core/voicekey_core.c" \
  "$repo_root/tests/test_core.c" -o "$repo_root/build-host/test_core"
"$repo_root/build-host/test_core"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -g -fsanitize="${SANITIZERS:-undefined}" \
  -I"$repo_root/firmware/components/voicekey_core/include" \
  "$repo_root/firmware/components/voicekey_core/voicekey_audio.c" \
  "$repo_root/firmware/components/voicekey_core/voicekey_xvf.c" \
  "$repo_root/tests/test_audio.c" -lm -o "$repo_root/build-host/test_audio"
"$repo_root/build-host/test_audio"
"$repo_root/scripts/test-adapters.sh"
python3 "$repo_root/tests/test_image_validation.py"
python3 "$repo_root/tests/test_build_wrapper.py"
python3 "$repo_root/tests/test_diagnostics.py"
