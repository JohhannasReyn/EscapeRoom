#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-/home/admin/escape-room}"

cd "${PROJECT_ROOT}"
chmod +x tools/*.sh 2>/dev/null || true
chmod +x fire/* 2>/dev/null || true
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
tools/install-pi-autostart.sh
sudo systemctl restart escape-room-controller.service
tools/room-status.sh
