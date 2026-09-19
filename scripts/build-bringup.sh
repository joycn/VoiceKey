#!/usr/bin/env bash
set -eo pipefail
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
if [[ $# != 0 ]]; then echo 'Build only; no flash arguments.' >&2; exit 2; fi
idf_checkout="${IDF_PATH:-$HOME/esp/esp-idf-v5.5.2}"
if [[ -n "${VOICEKEY_PYTHON:-}" ]]; then
    export PATH="$(dirname "$VOICEKEY_PYTHON"):$PATH"
elif [[ -x "$HOME/.local/bin/uv" ]]; then
    if python_exe="$($HOME/.local/bin/uv python find 3.11 2>/dev/null)"; then export PATH="$(dirname "$python_exe"):$PATH"; fi
fi
. "$idf_checkout/export.sh"
cd "$repo_root/firmware/bringup"
idf.py build
python "$repo_root/scripts/validate-bringup.py"
