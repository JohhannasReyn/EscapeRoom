#!/usr/bin/env bash

VENV_PATH="${PLATFORMIO_VENV:-${HOME}/.venv}"

if [ ! -f "${VENV_PATH}/bin/activate" ]; then
    echo "PlatformIO venv not found at ${VENV_PATH}."
    echo "Create it with:"
    echo "  python3 -m venv ${VENV_PATH}"
    echo "  source ${VENV_PATH}/bin/activate"
    echo "  python3 -m pip install -U pip platformio"
    return 1 2>/dev/null || exit 1
fi

# shellcheck source=/dev/null
source "${VENV_PATH}/bin/activate"
echo "Activated PlatformIO venv: ${VENV_PATH}"

if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    echo "Starting a shell with the venv active. Use 'exit' to leave it."
    exec "${SHELL:-/bin/bash}" -i
fi
