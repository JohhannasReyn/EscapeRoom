#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-/home/admin/escape-room}"
VENV_PATH="${PLATFORMIO_VENV:-${HOME}/.platformio-venv}"

if [ ! -f "${VENV_PATH}/bin/activate" ]; then
    echo "PlatformIO venv not found at ${VENV_PATH}."
    exit 1
fi

# shellcheck source=/dev/null
source "${VENV_PATH}/bin/activate"

cd "${PROJECT_ROOT}/raspberry-pi-controller"
exec pio run -e raspberry_pi_controller -t exec
