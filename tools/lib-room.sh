#!/usr/bin/env bash

PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
MQTT_HOST="${MQTT_HOST:-localhost}"
MQTT_TOPIC_ROOT="${MQTT_TOPIC_ROOT:-escape}"
PICO_STATUS_TIMEOUT="${PICO_STATUS_TIMEOUT:-6}"

PICO_TOPICS=(
    "pico2:escape/telemetry/pico2/contacts:Pico 2 one-piece copper puzzle:GPIO 15 copper input to puzzle contact to GND; internal pull-up enabled"
    "pico3:escape/telemetry/pico3/painting_sensor:Pico 3 painting rotation:GPIO 15 magnetic/reed/hall sensor output, GPIO 14 reset"
    "pico4:escape/telemetry/pico4/oven:Pico 4 smart film / oven:GPIO 15 smart film relay, GPIO 18 lock relay, GPIO 26 oven pot wiper"
    "pico5:escape/telemetry/pico5/buttons:Pico 5 color buttons:GPIO 15 red, GPIO 16 green, GPIO 17 yellow, GPIO 18 blue, buttons to GND"
    "pico7:escape/telemetry/fire-panel/status:Pico 7 fire panel:GP2-GP11 buttons to GND; GP12-GP21 red/green status LEDs"
)

need_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Missing required command: $1"
        return 1
    fi
}

publish() {
    local topic="$1"
    local payload="${2:-test}"
    mosquitto_pub -h "${MQTT_HOST}" -t "${topic}" -m "${payload}"
}

request_status() {
    publish "${MQTT_TOPIC_ROOT}/cmd/all/status_request" "status"
}

print_pico_config() {
    local id="$1"

    for entry in "${PICO_TOPICS[@]}"; do
        IFS=: read -r pico topic label wiring <<<"${entry}"
        if [ "${pico}" = "${id}" ]; then
            echo "${label}"
            echo "Expected telemetry topic: ${topic}"
            echo "Expected wiring: ${wiring}"
            return 0
        fi
    done

    return 1
}

wait_for_topics_once() {
    local timeout_sec="${1:-${PICO_STATUS_TIMEOUT}}"
    local tmp_file
    tmp_file="$(mktemp)"

    request_status || true
    timeout "${timeout_sec}" mosquitto_sub -h "${MQTT_HOST}" -v -t "${MQTT_TOPIC_ROOT}/telemetry/#" -t "${MQTT_TOPIC_ROOT}/post/cubby/+/state" >"${tmp_file}" 2>/dev/null || true

    for entry in "${PICO_TOPICS[@]}"; do
        IFS=: read -r pico topic label wiring <<<"${entry}"
        if grep -q "^${topic} " "${tmp_file}"; then
            echo "${label}: connected"
        else
            echo "${label}: not found"
        fi
    done

    rm -f "${tmp_file}"
}
