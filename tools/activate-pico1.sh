#!/usr/bin/env bash
set -euo pipefail

mosquitto_pub -h "${MQTT_HOST:-localhost}" -t 'escape/cmd/pico1/enable_cubby_light' -m "${1:-1}"
