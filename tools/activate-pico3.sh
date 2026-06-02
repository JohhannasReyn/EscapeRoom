#!/usr/bin/env bash
set -euo pipefail

mosquitto_pub -h "${MQTT_HOST:-localhost}" -t 'escape/cmd/pico3/enable_painting_rotation' -m 'on'
