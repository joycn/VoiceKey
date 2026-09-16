#!/usr/bin/env bash
set -eo pipefail
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
# Validation is deliberately fixed to this project's sdkconfig/build directory.
# Reject overrides before sourcing IDF or allowing any requested hardware write.
for option in "$@"; do
    case "$option" in
        -B*|-C*|-D*|--build-dir*|--project-dir*|--define-cache-entry*)
            echo "Build/project/configuration overrides are unsupported by this validated build wrapper: $option" >&2
            exit 2 ;;
    esac
done
if [[ -n "${SDKCONFIG:-}" || -n "${SDKCONFIG_DEFAULTS:-}" || -n "${IDF_BUILD_DIR:-}" ]]; then
    echo "SDKCONFIG/SDKCONFIG_DEFAULTS/IDF_BUILD_DIR overrides are unsupported by this validated build wrapper." >&2
    exit 2
fi
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
check_images=false
flash_requested=false
for action in "$@"; do
    case "$action" in
        build|all) check_images=true ;;
        flash|app-flash|bootloader-flash|partition-table-flash) check_images=true; flash_requested=true ;;
    esac
done
if $check_images && [[ -f sdkconfig ]]; then
    if ! python "$repo_root/scripts/validate-images.py" --config-only; then
        echo "Cached sdkconfig is not the supported XIAO target. Back it up, remove it, and rebuild from sdkconfig.defaults." >&2
        exit 1
    fi
fi
# Never reach a requested hardware write before complete image-bound checks.
if $flash_requested; then
    idf.py build
    python "$repo_root/scripts/validate-images.py"
fi
idf.py "$@"
if $check_images; then python "$repo_root/scripts/validate-images.py"; fi
