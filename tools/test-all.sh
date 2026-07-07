#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=tools/lib-room.sh
source "${SCRIPT_DIR}/lib-room.sh"

STEP_DELAY="${STEP_DELAY:-6}"

need_cmd mosquitto_pub
need_cmd mosquitto_sub
need_cmd timeout

echo "Checking Pico connectivity before walkthrough..."
wait_for_topics_once "${PICO_STATUS_TIMEOUT}"
echo
echo "Simulating complete walkthrough with ${STEP_DELAY}s between steps."
echo

run_step() {
    local label="$1"
    local topic="$2"
    local payload="$3"
    local expected="$4"
    local hint="$5"
    local tmp_file
    tmp_file="$(mktemp)"

    echo "${label}"
    echo "  publish ${topic} -> ${payload}"
    timeout "${STEP_DELAY}" mosquitto_sub -h "${MQTT_HOST}" -v -t "${MQTT_TOPIC_ROOT}/#" >"${tmp_file}" 2>/dev/null &
    sub_pid=$!
    sleep 1
    publish "${topic}" "${payload}"
    wait "${sub_pid}" || true

    if grep -q "^${expected} " "${tmp_file}"; then
        echo "  ok: observed ${expected}"
    else
        echo "  issue: did not observe expected ${expected}"
        echo "  expected wiring/topic check: ${hint}"
    fi

    rm -f "${tmp_file}"
    echo
}

run_step \
    "1. One-piece copper puzzle completes and should reveal smart film" \
    "escape/pico2/copper_puzzle_complete" \
    "manual walkthrough" \
    "escape/cmd/pico4/reveal_smart_film" \
    "Pico 2 should publish escape/pico2/copper_puzzle_complete; GPIO 15 should connect through the puzzle contact to GND."

run_step \
    "2. Picture rotation should keep color buttons active" \
    "escape/pico3/painting_rotation_complete" \
    "manual walkthrough" \
    "escape/cmd/fire-panel/led" \
    "Pico 3 should publish escape/pico3/painting_rotation_complete when the picture reaches the magnet sensor; the Pi should not wait for any enable command."

run_step \
    "3. Color button sequence completes and should mark oven active" \
    "escape/pico5/color_sequence_complete" \
    "manual walkthrough" \
    "escape/cmd/fire-panel/led" \
    "Pico 5 should publish escape/pico5/color_sequence_complete after 3 red, 4 green, 2 yellow, 3 blue presses; Pico 4 already listens to the oven knob."

run_step \
    "4. Oven target reached and should unlock final lock" \
    "escape/pico4/oven_target_reached" \
    "manual walkthrough" \
    "escape/cmd/pico4/unlock_electromag_lock" \
    "Pico 4 should publish escape/pico4/oven_target_reached; oven pot wiper on GPIO 26, lock relay on GPIO 18."

echo
echo "Walkthrough messages sent. If a step did not propagate, run tools/room-logs.sh or tools/watch-mqtt.sh and verify:"
for entry in "${PICO_TOPICS[@]}"; do
    IFS=: read -r pico topic label wiring <<<"${entry}"
    echo "- ${label}: ${wiring}"
done
