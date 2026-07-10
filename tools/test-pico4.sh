#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=tools/lib-room.sh
source "${SCRIPT_DIR}/lib-room.sh"

echo "Testing Pico 4 by revealing smart film and simulating oven target reached."
print_pico_config pico4

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
    -t "escape/pico4/smart_film_ready" >"${tmp_file}" 2>/dev/null &
sub_pid=$!

sleep 1
publish "escape/cmd/pico4/reveal_smart_film" "on"
wait "${sub_pid}" || true

if grep -q "^escape/pico4/smart_film_ready transparent" "${tmp_file}"; then
    echo "ok: Pico 4 acknowledged smart film transparent."
else
    echo "issue: Pico 4 did not acknowledge smart film reveal."
    echo "Check Pico 4 power/WiFi/MQTT first, then GPIO 15 -> smart-film relay IN and relay power."
fi

>"${tmp_file}"
echo "Turn the oven knob slowly across its range for the next 8 seconds."
timeout 8 mosquitto_sub \
    -h "${MQTT_HOST}" \
    -v \
    -t "escape/telemetry/pico4/oven" >"${tmp_file}" 2>/dev/null || true

raw_values="$(sed -n 's/.*oven_raw=\([0-9][0-9]*\).*/\1/p' "${tmp_file}")"
if [ -z "${raw_values}" ]; then
    echo "issue: no Pico 4 oven telemetry observed."
    echo "Check Pico 4 power/WiFi/MQTT before rewiring the potentiometer."
else
    min_raw="$(printf '%s\n' "${raw_values}" | sort -n | head -n 1)"
    max_raw="$(printf '%s\n' "${raw_values}" | sort -n | tail -n 1)"
    raw_span=$((max_raw - min_raw))
    echo "oven_raw range: ${min_raw}-${max_raw} (span ${raw_span})"

    if [ "${raw_span}" -lt 50 ]; then
        echo "issue: oven_raw barely changed. Check 3V3/GND on the outer pot legs and GPIO 26 on the wiper."
    else
        echo "ok: Pico 4 sees the potentiometer changing."
    fi
fi

publish "escape/pico4/oven_target_reached" "manual oven test"
