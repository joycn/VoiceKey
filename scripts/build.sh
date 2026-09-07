#!/usr/bin/env bash
set -eo pipefail
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
idf_checkout="${IDF_PATH:-$HOME/esp/esp-idf-v5.5.2}"
if [[ ! -f "$idf_checkout/export.sh" ]]; then
    echo "ESP-IDF v5.5.2 not found. See docs/build.md or set IDF_PATH." >&2
    exit 1
fi
# Prefer an explicitly selected Python. On this workstation uv provides Python 3.11.
if [[ -n "${VOICEKEY_PYTHON:-}" ]]; then
    export PATH="$(dirname "$VOICEKEY_PYTHON"):$PATH"
elif [[ -x "$HOME/.local/bin/uv" ]]; then
    if python_exe="$($HOME/.local/bin/uv python find 3.11 2>/dev/null)"; then
        export PATH="$(dirname "$python_exe"):$PATH"
    fi
fi
. "$idf_checkout/export.sh"
cd "$repo_root/firmware"
# sdkconfig.defaults fixes the target; normal builds never flash hardware.
if [[ $# == 0 ]]; then set -- build; fi
idf.py "$@"
