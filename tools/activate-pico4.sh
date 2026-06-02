#!/usr/bin/env bash
set -euo pipefail

mosquitto_pub -h "${MQTT_HOST:-localhost}" -t 'escape/cmd/pico4/reveal_smart_film' -m 'on'
mosquitto_pub -h "${MQTT_HOST:-localhost}" -t 'escape/cmd/pico4/enable_oven_knob' -m 'on'
