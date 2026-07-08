#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

usage() {
    cat <<'EOF'
Usage: tools/flash-pico.sh <pico2|pico3|pico4|pico5|pico7|all>

Flashes active Pico W firmware from macOS, Linux, or Raspberry Pi using PlatformIO.
Examples:
  tools/flash-pico.sh pico2
  tools/flash-pico.sh all

Before each upload, unplug the target Pico, hold BOOTSEL, plug it into USB, then
press Enter when this script prompts you.
EOF
}

if (($# != 1)); then
    usage >&2
    exit 2
fi

if [[ ! -f "${repo_root}/pico-wifi.env" ]]; then
    echo "Missing ${repo_root}/pico-wifi.env." >&2
    echo "Run tools/rebase.sh on the Pi or copy pico-wifi.env.example to pico-wifi.env before flashing." >&2
    exit 1
fi

pio_cmd="${PIO:-}"
if [[ -z "${pio_cmd}" ]]; then
    if command -v pio >/dev/null 2>&1; then
        pio_cmd="$(command -v pio)"
    elif [[ -x "${PLATFORMIO_VENV:-${HOME}/.venv}/bin/pio" ]]; then
        pio_cmd="${PLATFORMIO_VENV:-${HOME}/.venv}/bin/pio"
    elif [[ -x "${repo_root}/.venv/bin/pio" ]]; then
        pio_cmd="${repo_root}/.venv/bin/pio"
    fi
fi

if [[ -z "${pio_cmd}" ]]; then
    echo "Missing PlatformIO command: pio" >&2
    echo "On the Raspberry Pi, run tools/rebase.sh first." >&2
    echo "Install it with: python3 -m pip install -U platformio" >&2
    exit 1
fi

folder_for_target() {
    case "$1" in
        pico2) echo "pico2-copper-final-piece" ;;
        pico3) echo "pico3-painting-rotation" ;;
        pico4) echo "pico4-smart-film-oven" ;;
        pico5) echo "pico5-color-buttons" ;;
        pico7) echo "pico7-fire-panel" ;;
        *)
            echo "Unknown Pico target: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
}

flash_target() {
    local target="$1"
    local folder
    folder="$(folder_for_target "${target}")"

    echo
    echo "Ready to flash ${target}: ${folder}"
    echo "1. Unplug that Pico from USB."
    echo "2. Hold BOOTSEL."
    echo "3. Plug the Pico into this computer or Raspberry Pi."
    echo "4. Release BOOTSEL after it is connected."
    read -r -p "Press Enter to upload ${target}..."

    (
        cd "${repo_root}/${folder}"
        "${pio_cmd}" run --target upload
    )
}

case "$1" in
    all)
        flash_target pico2
        flash_target pico3
        flash_target pico4
        flash_target pico5
        flash_target pico7
        ;;
    pico2|pico3|pico4|pico5|pico7)
        flash_target "$1"
        ;;
    *)
        echo "Unknown Pico target: $1" >&2
        usage >&2
        exit 2
        ;;
esac

echo
echo "Flash complete."
