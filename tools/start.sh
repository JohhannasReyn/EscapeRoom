#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
VENV_PATH="${PLATFORMIO_VENV:-${HOME}/.venv}"

cd "${PROJECT_ROOT}"

if systemctl is-active --quiet escape-room-tv-dashboard.service 2>/dev/null; then
    sudo systemctl stop escape-room-tv-dashboard.service || true
fi

if systemctl is-active --quiet escape-room-controller.service 2>/dev/null; then
    sudo systemctl stop escape-room-controller.service || true
fi

tools/connect.sh || true

if [ ! -f "${VENV_PATH}/bin/activate" ]; then
    echo "PlatformIO venv not found at ${VENV_PATH}."
    echo "Run: python3 -m venv ${VENV_PATH} && source ${VENV_PATH}/bin/activate && python -m pip install -U pip platformio"
    exit 1
fi

# shellcheck source=/dev/null
source "${VENV_PATH}/bin/activate"

echo "Starting escape room controller in foreground."
echo "Logs are filtered in the controller: telemetry prints only when it changes."
cd "${PROJECT_ROOT}/raspberry-pi-controller"
exec pio run -e raspberry_pi_controller -t exec
