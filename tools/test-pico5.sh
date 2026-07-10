#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=tools/lib-room.sh
source "${SCRIPT_DIR}/lib-room.sh"

echo "Testing Pico 5 by simulating wrong-code buzzer and color button sequence completion."
print_pico_config pico5

need_cmd mosquitto_pub
need_cmd mosquitto_sub
need_cmd timeout

tmp_file="$(mktemp)"
cleanup() {
    rm -f "${tmp_file}"
}
trap cleanup EXIT

timeout 4 mosquitto_sub \
    -h "${MQTT_HOST}" \
    -v \
    -t "escape/pico5/color_sequence_error" \
    -t "escape/telemetry/pico5/buttons" >"${tmp_file}" 2>/dev/null &
sub_pid=$!

sleep 1
publish "escape/pico5/color_sequence_error" "manual wrong-code buzzer test"
wait "${sub_pid}" || true

if grep -q "^escape/pico5/color_sequence_error " "${tmp_file}"; then
    echo "ok: wrong-code buzzer event was published."
    echo "If no buzzer sound played, check Raspberry Pi audio volume/output and assets/audio/buzzer.mp3."
else
    echo "issue: wrong-code buzzer event was not observed on MQTT."
fi

if grep -q "^escape/telemetry/pico5/buttons " "${tmp_file}"; then
    echo "ok: Pico 5 button telemetry is visible."
else
    echo "issue: Pico 5 telemetry was not observed. Check Pico 5 power/WiFi/MQTT."
fi

publish "escape/pico5/color_sequence_complete" "manual color button test"
