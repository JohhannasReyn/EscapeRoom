#!/usr/bin/env bash
set -uo pipefail

BT_DEVICE="${BT_DEVICE:-00:22:6C:12:1D:C3}"
BT_NAME="${BT_NAME:-Yamaha ATS-1090}"
VOLUME="${VOLUME:-80%}"

echo "Connecting Bluetooth device ${BT_NAME} (${BT_DEVICE})..."
bluetoothctl power on || true

if ! bluetoothctl connect "${BT_DEVICE}"; then
    echo "Bluetooth connect did not complete. The controller can still run; check the speaker manually if audio is missing."
fi

echo
echo "Bluetooth device info:"
bluetoothctl info "${BT_DEVICE}" || true

if command -v wpctl >/dev/null 2>&1; then
    echo
    echo "Audio devices:"
    wpctl status

    sink_id="$(wpctl status | awk -v name="${BT_NAME}" 'index($0, name) { gsub(/[^0-9]/, " ", $0); print $1; exit }')"

    if [ -n "${sink_id}" ]; then
        echo
        echo "Setting ${BT_NAME} as default sink ${sink_id} at volume ${VOLUME}."
        wpctl set-default "${sink_id}" || true
        wpctl set-volume @DEFAULT_AUDIO_SINK@ "${VOLUME}" || true
        wpctl set-mute @DEFAULT_AUDIO_SINK@ 0 || true
    else
        echo
        echo "Could not auto-find '${BT_NAME}' in wpctl output."
        echo "Run 'wpctl status', then set the sink manually:"
        echo "  wpctl set-default SINK_ID"
        echo "  wpctl set-volume @DEFAULT_AUDIO_SINK@ ${VOLUME}"
    fi
else
    echo "wpctl is not installed; Bluetooth connected, but audio sink was not adjusted."
fi
