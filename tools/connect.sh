#!/usr/bin/env bash
set -uo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
CONFIG_FILE="${BT_CONFIG_FILE:-${PROJECT_ROOT}/bluetooth-speaker.env}"
VOLUME="${VOLUME:-80%}"
TEST_SOUND="${TEST_SOUND:-${PROJECT_ROOT}/assets/audio/yeah-you-did-it.mp3}"

if [ -f "${CONFIG_FILE}" ]; then
    # shellcheck source=/dev/null
    source "${CONFIG_FILE}"
fi

if [ -z "${BT_DEVICE:-}" ]; then
    echo "No Bluetooth speaker configured."
    echo "Run: tools/pair.sh"
    exit 1
fi

echo "Connecting Bluetooth speaker ${BT_NAME:-speaker} (${BT_DEVICE})..."
bluetoothctl power on >/dev/null 2>&1 || true

if ! bluetoothctl connect "${BT_DEVICE}"; then
    echo "Bluetooth connect did not complete."
    exit 1
fi

if command -v wpctl >/dev/null 2>&1; then
    sink_id="$(wpctl status | awk -v name="${BT_NAME:-}" 'name != "" && index($0, name) { gsub(/[^0-9]/, " ", $0); print $1; exit }')"

    if [ -n "${sink_id}" ]; then
        wpctl set-default "${sink_id}" || true
    fi

    wpctl set-volume @DEFAULT_AUDIO_SINK@ "${VOLUME}" || true
    wpctl set-mute @DEFAULT_AUDIO_SINK@ 0 || true
fi

echo "Bluetooth speaker connected."

if [ -f "${TEST_SOUND}" ]; then
    if command -v ffplay >/dev/null 2>&1; then
        ffplay -nodisp -autoexit -loglevel quiet "${TEST_SOUND}" >/dev/null 2>&1 || true
    elif command -v pw-play >/dev/null 2>&1; then
        pw-play "${TEST_SOUND}" >/dev/null 2>&1 || true
    elif command -v aplay >/dev/null 2>&1; then
        aplay "${TEST_SOUND}" >/dev/null 2>&1 || true
    fi
fi
