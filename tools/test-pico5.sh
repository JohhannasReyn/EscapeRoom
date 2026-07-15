#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=tools/lib-room.sh
source "${SCRIPT_DIR}/lib-room.sh"

echo "Testing Pico 5 by simulating the wrong-code try-again/buzzer pattern and color button sequence completion."
print_pico_config pico5

need_cmd mosquitto_pub
need_cmd mosquitto_sub
need_cmd timeout

tmp_file="$(mktemp)"
cleanup() {
    rm -f "${tmp_file}"
}
trap cleanup EXIT

timeout 5 mosquitto_sub \
    -h "${MQTT_HOST}" \
    -v \
    -t "escape/pico5/color_sequence_error" \
    -t "escape/telemetry/pico5/buttons" >"${tmp_file}" 2>/dev/null &
sub_pid=$!

sleep 1
publish "escape/pico5/color_sequence_error" "manual wrong-code test 1 - expect try-again.wav"
publish "escape/pico5/color_sequence_error" "manual wrong-code test 2 - expect buzzer.mp3"
publish "escape/pico5/color_sequence_error" "manual wrong-code test 3 - expect buzzer.mp3"
publish "escape/pico5/color_sequence_error" "manual wrong-code test 4 - expect try-again.wav"
wait "${sub_pid}" || true

error_count="$(grep -c "^escape/pico5/color_sequence_error " "${tmp_file}" || true)"
if [ "${error_count}" -ge 4 ]; then
    echo "ok: wrong-code events were published."
    echo "Expected sounds from the Pi controller: try-again.wav, buzzer.mp3, buzzer.mp3, try-again.wav."
    echo "If no sound played, check Raspberry Pi audio volume/output and assets/audio/try-again.wav plus assets/audio/buzzer.mp3."
else
    echo "issue: expected 4 wrong-code events, observed ${error_count}."
fi

if grep -q "^escape/telemetry/pico5/buttons " "${tmp_file}"; then
    echo "ok: Pico 5 button telemetry is visible."
else
    echo "issue: Pico 5 telemetry was not observed. Check Pico 5 power/WiFi/MQTT."
fi

publish "escape/pico5/color_sequence_complete" "manual color button test"
