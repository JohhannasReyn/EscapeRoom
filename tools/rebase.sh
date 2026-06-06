#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
REMOTE_REPO="${REMOTE_REPO:-https://github.com/JohhannasReyn/EscapeRoom.git}"
BRANCH="${BRANCH:-main}"
VENV_PATH="${PLATFORMIO_VENV:-${HOME}/.platformio-venv}"
tmp_dir="$(mktemp -d)"

cleanup() {
    rm -rf "${tmp_dir}"
}
trap cleanup EXIT

echo "Fetching latest EscapeRoom from ${REMOTE_REPO} (${BRANCH})..."
git clone --depth 1 --branch "${BRANCH}" "${REMOTE_REPO}" "${tmp_dir}/EscapeRoom"

cd "${PROJECT_ROOT}"
echo "Updating raspberry-pi-controller, tools, assets, and shared..."
rm -rf raspberry-pi-controller tools assets shared
cp -R "${tmp_dir}/EscapeRoom/raspberry-pi-controller" .
cp -R "${tmp_dir}/EscapeRoom/tools" .
cp -R "${tmp_dir}/EscapeRoom/assets" .
cp -R "${tmp_dir}/EscapeRoom/shared" .
chmod +x tools/*.sh tools/connect 2>/dev/null || true

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
