#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
CONFIG_FILE="${BT_CONFIG_FILE:-${PROJECT_ROOT}/bluetooth-speaker.env}"
SCAN_SECONDS="${SCAN_SECONDS:-12}"

echo "Starting Bluetooth scan for ${SCAN_SECONDS} seconds..."
bluetoothctl power on >/dev/null
bluetoothctl agent on >/dev/null || true
bluetoothctl default-agent >/dev/null || true

bluetoothctl scan on >/dev/null 2>&1 &
scan_pid=$!
sleep "${SCAN_SECONDS}"
kill "${scan_pid}" >/dev/null 2>&1 || true
bluetoothctl scan off >/dev/null 2>&1 || true

mapfile -t devices < <(bluetoothctl devices | awk '{ mac=$2; $1=""; $2=""; sub(/^  */, ""); print mac "|" $0 }')

if [ "${#devices[@]}" -eq 0 ]; then
    echo "No Bluetooth devices found. Put the speaker in pairing mode and retry."
    exit 1
fi

echo "Available Bluetooth devices:"
index=1
for device in "${devices[@]}"; do
    mac="${device%%|*}"
    name="${device#*|}"
    echo "  ${index}) ${name} (${mac})"
    index=$((index + 1))
done

printf "Select device number: "
read -r selection

if ! [[ "${selection}" =~ ^[0-9]+$ ]] || [ "${selection}" -lt 1 ] || [ "${selection}" -gt "${#devices[@]}" ]; then
    echo "Invalid selection."
    exit 1
fi

selected="${devices[$((selection - 1))]}"
mac="${selected%%|*}"
name="${selected#*|}"

bluetoothctl pair "${mac}" || true
bluetoothctl trust "${mac}"
bluetoothctl connect "${mac}"

cat >"${CONFIG_FILE}" <<EOF
BT_DEVICE="${mac}"
BT_NAME="${name}"
EOF

echo "Saved Bluetooth speaker config to ${CONFIG_FILE}."
