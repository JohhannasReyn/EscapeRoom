#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
REMOTE_REPO="${REMOTE_REPO:-https://github.com/JohhannasReyn/EscapeRoom.git}"
BRANCH="${BRANCH:-main}"
VENV_PATH="${PLATFORMIO_VENV:-${HOME}/.venv}"
SELF="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"

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

echo "Rebase complete. Start the room with:"
echo "  cd ${PROJECT_ROOT} && tools/start.sh"
echo "For student-friendly commands, run:"
echo "  help"
echo "If this shell does not know help yet, run:"
echo "  source ${HOME}/.escape-room-help.sh"
