#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
REMOTE_REPO="${REMOTE_REPO:-https://github.com/JohhannasReyn/EscapeRoom.git}"
BRANCH="${BRANCH:-main}"
VENV_PATH="${PLATFORMIO_VENV:-${HOME}/.venv}"
SELF="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
REBASE_STATE_FILE="${REBASE_STATE_FILE:-${PROJECT_ROOT}/.escape-room-rebase-state}"
ACTIVE_PICOS=(pico2 pico3 pico4 pico5 pico7)

pico_project_dir() {
    case "$1" in
        pico2) echo "pico2-copper-final-piece" ;;
        pico3) echo "pico3-painting-rotation" ;;
        pico4) echo "pico4-smart-film-oven" ;;
        pico5) echo "pico5-color-buttons" ;;
        pico7) echo "pico7-fire-panel" ;;
        *)
            echo "Unknown Pico target: $1" >&2
            exit 1
            ;;
    esac
}

pico_label() {
    case "$1" in
        pico2) echo "Pico 2 - Copper Puzzle Piece Detection" ;;
        pico3) echo "Pico 3 - Painting Rotation Sensor" ;;
        pico4) echo "Pico 4 - Smart Film, Oven Dial, and Lock" ;;
        pico5) echo "Pico 5 - Color Button Sequence" ;;
        pico7) echo "Pico 7 - Fire Panel Controller" ;;
        *)
            echo "Unknown Pico target: $1" >&2
            exit 1
            ;;
    esac
}

pico_flash_command() {
    case "$1" in
        pico2) echo "tools/flash-pico.sh pico2" ;;
        pico3) echo "tools/flash-pico.sh pico3" ;;
        pico4) echo "tools/flash-pico.sh pico4" ;;
        pico5) echo "tools/flash-pico.sh pico5" ;;
        pico7) echo "tools/flash-pico.sh pico7" ;;
        *)
            echo "Unknown Pico target: $1" >&2
            exit 1
            ;;
    esac
}

pico_shared_inputs() {
    case "$1" in
        pico2|pico3)
            echo "shared/EscapeRoomProtocol.h"
            echo "shared/PicoStatusReport.h"
            echo "shared/PostState.h"
            ;;
        pico4)
            echo "shared/OvenDial.h"
            echo "shared/EscapeRoomProtocol.h"
            echo "shared/PicoStatusReport.h"
            echo "shared/PostState.h"
            ;;
        pico5)
            echo "shared/ColorButtonSequence.h"
            echo "shared/EscapeRoomProtocol.h"
            echo "shared/PicoStatusReport.h"
            echo "shared/PostState.h"
            ;;
        pico7)
            echo "shared/EscapeRoomProtocol.h"
            echo "shared/PicoStatusReport.h"
            ;;
        *)
            echo "Unknown Pico target: $1" >&2
            exit 1
            ;;
    esac
}

pico_hash_inputs() {
    local pico="$1"

    pico_project_dir "${pico}"
    pico_shared_inputs "${pico}"
    echo "pico-wifi.env"
    echo "tools/platformio_pico_wifi.py"
}

hash_file() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1"
    else
        shasum -a 256 "$1"
    fi
}

hash_stream() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum
    else
        shasum -a 256
    fi
}

hash_pico_inputs() {
    local source_root="$1"
    local pico="$2"
    local file_list
    local input

    file_list="$(mktemp)"
    (
        cd "${source_root}"
        for input in $(pico_hash_inputs "${pico}"); do
            if [ -e "${input}" ]; then
                find "${input}" -type f -print
            fi
        done | sort > "${file_list}"

        if [ ! -s "${file_list}" ]; then
            echo "missing"
        else
            while IFS= read -r input; do
                hash_file "${input}"
            done < "${file_list}" | hash_stream | awk '{print $1}'
        fi
    )
    rm -f "${file_list}"
}

read_previous_pico_hash() {
    local pico="$1"

    if [ ! -f "${REBASE_STATE_FILE}" ]; then
        return 0
    fi

    sed -n "s/^${pico}=//p" "${REBASE_STATE_FILE}" | tail -n 1
}

print_flash_recommendations() {
    local source_root="$1"
    local source_commit="$2"
    local pico
    local previous_hash
    local current_hash
    local first_rebase=0
    local changed_picos=()

    if [ ! -f "${REBASE_STATE_FILE}" ]; then
        first_rebase=1
    fi

    for pico in "${ACTIVE_PICOS[@]}"; do
        previous_hash="$(read_previous_pico_hash "${pico}")"
        current_hash="$(hash_pico_inputs "${source_root}" "${pico}")"
        if [ -z "${previous_hash}" ] || [ "${previous_hash}" != "${current_hash}" ]; then
            changed_picos+=("${pico}")
        fi
    done

    echo
    echo "Pico flash check for source ${source_commit}:"

    if [ "${first_rebase}" -eq 1 ]; then
        echo "No previous Pico firmware rebase state was found."
        echo "If these Picos have not been flashed from this repo yet, flash all active Picos:"
        echo "  tools/flash-pico.sh all"
        echo "Or flash them one at a time:"
        for pico in "${ACTIVE_PICOS[@]}"; do
            echo "  $(pico_flash_command "${pico}")  # $(pico_label "${pico}")"
        done
        return 0
    fi

    if [ "${#changed_picos[@]}" -eq 0 ]; then
        echo "No Pico firmware changes detected since the last rebase."
        echo "No Pico flashing needed."
        return 0
    fi

    echo "Pico firmware changed since the last rebase. Flash these Pico(s):"
    for pico in "${changed_picos[@]}"; do
        echo "  $(pico_flash_command "${pico}")  # $(pico_label "${pico}")"
    done
}

write_rebase_state() {
    local source_root="$1"
    local source_commit="$2"
    local pico

    {
        echo "# Local state written by tools/rebase.sh. Do not commit."
        echo "commit=${source_commit}"
        for pico in "${ACTIVE_PICOS[@]}"; do
            echo "${pico}=$(hash_pico_inputs "${source_root}" "${pico}")"
        done
    } > "${REBASE_STATE_FILE}"
}

# Reuse the clone handed over by a self-update re-exec; otherwise make a new one.
if [ -n "${REBASE_TMP_DIR:-}" ] && [ -d "${REBASE_TMP_DIR}/EscapeRoom" ]; then
    tmp_dir="${REBASE_TMP_DIR}"
    cloned_here=0
else
    tmp_dir="$(mktemp -d)"
    cloned_here=1
fi

cleanup() {
    rm -rf "${tmp_dir}"
}
trap cleanup EXIT

if [ "${cloned_here}" -eq 1 ]; then
    echo "Fetching latest EscapeRoom from ${REMOTE_REPO} (${BRANCH})..."
    git clone --depth 1 --branch "${BRANCH}" "${REMOTE_REPO}" "${tmp_dir}/EscapeRoom"
fi

# Self-update: if rebase.sh changed upstream, atomically replace this file and
# re-run the fresh copy (guarded so it happens at most once per run). The clone
# is handed to the re-exec via REBASE_TMP_DIR so the second pass does not
# re-download. mv is atomic, so the running shell keeps reading the old inode
# safely until exec swaps in the new process image.
new_self="${tmp_dir}/EscapeRoom/tools/rebase.sh"
if [ "${REBASE_SELF_UPDATED:-0}" != "1" ] && [ -f "${new_self}" ] && ! cmp -s "${new_self}" "${SELF}"; then
    echo "rebase.sh changed upstream; updating it and re-running the new version..."
    cp "${new_self}" "${SELF}.tmp.$$"
    chmod +x "${SELF}.tmp.$$"
    mv -f "${SELF}.tmp.$$" "${SELF}"
    trap - EXIT
    REBASE_SELF_UPDATED=1 REBASE_TMP_DIR="${tmp_dir}" exec "${SELF}" "$@"
fi

SOURCE_COMMIT="$(git -C "${tmp_dir}/EscapeRoom" rev-parse --short=12 HEAD)"

cd "${PROJECT_ROOT}"
echo "Updating controller, tools, active Pico firmware, tests, assets, shared code, and room WiFi defaults..."
rm -rf raspberry-pi-controller tools fire assets shared tests
rm -rf pico2-copper-final-piece pico3-painting-rotation pico4-smart-film-oven pico5-color-buttons pico7-fire-panel
cp -R "${tmp_dir}/EscapeRoom/raspberry-pi-controller" .
cp -R "${tmp_dir}/EscapeRoom/tools" .
cp -R "${tmp_dir}/EscapeRoom/fire" .
cp -R "${tmp_dir}/EscapeRoom/assets" .
cp -R "${tmp_dir}/EscapeRoom/shared" .
cp -R "${tmp_dir}/EscapeRoom/tests" .
cp -R "${tmp_dir}/EscapeRoom/pico2-copper-final-piece" .
cp -R "${tmp_dir}/EscapeRoom/pico3-painting-rotation" .
cp -R "${tmp_dir}/EscapeRoom/pico4-smart-film-oven" .
cp -R "${tmp_dir}/EscapeRoom/pico5-color-buttons" .
cp -R "${tmp_dir}/EscapeRoom/pico7-fire-panel" .
cp "${tmp_dir}/EscapeRoom/pico-wifi.env" .
cp "${tmp_dir}/EscapeRoom/pico-wifi.env.example" .
cp "${tmp_dir}/EscapeRoom/john-contact.env.example" .
cp "${tmp_dir}/EscapeRoom/README.md" .
chmod +x tools/*.sh 2>/dev/null || true
chmod +x fire/* 2>/dev/null || true

if [ -x "${PROJECT_ROOT}/tools/install-help-command.sh" ]; then
    "${PROJECT_ROOT}/tools/install-help-command.sh"
    # If this script was sourced, make the help function available immediately.
    # If this script was executed normally, this only affects the child shell;
    # future SSH sessions still get the function from ~/.bashrc.
    if [ -f "${HOME}/.escape-room-help.sh" ]; then
        # shellcheck source=/dev/null
        source "${HOME}/.escape-room-help.sh"
    fi
fi

if [ ! -d "${VENV_PATH}" ]; then
    echo "Creating PlatformIO venv at ${VENV_PATH}..."
    python3 -m venv "${VENV_PATH}"
fi

# shellcheck source=/dev/null
source "${VENV_PATH}/bin/activate"

echo "Installing/updating PlatformIO..."
python -m pip install -U pip platformio

echo "Building Raspberry Pi controller..."
cd "${PROJECT_ROOT}/raspberry-pi-controller"
pio run -e raspberry_pi_controller

print_flash_recommendations "${tmp_dir}/EscapeRoom" "${SOURCE_COMMIT}"
write_rebase_state "${tmp_dir}/EscapeRoom" "${SOURCE_COMMIT}"

echo "Rebase complete. Start the room with:"
echo "  cd ${PROJECT_ROOT} && tools/start.sh"
echo "For student-friendly commands, run:"
echo "  help"
echo "If this shell does not know help yet, run:"
echo "  source ${HOME}/.escape-room-help.sh"
