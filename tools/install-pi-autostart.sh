#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-/home/admin/escape-room}"
RUN_USER="${RUN_USER:-admin}"
SERVICE_PATH="/etc/systemd/system/escape-room-controller.service"

echo "Installing escape room controller autostart for project root: ${PROJECT_ROOT}"

sudo chmod +x "${PROJECT_ROOT}"/tools/*.sh 2>/dev/null || true

cat <<SERVICE | sudo tee "${SERVICE_PATH}" >/dev/null
[Unit]
Description=Escape Room Raspberry Pi Controller
After=network-online.target bluetooth.service mosquitto.service
Wants=network-online.target bluetooth.service
Requires=mosquitto.service

[Service]
Type=simple
User=${RUN_USER}
WorkingDirectory=${PROJECT_ROOT}
Environment=PROJECT_ROOT=${PROJECT_ROOT}
Environment=PLATFORMIO_VENV=/home/${RUN_USER}/.venv
ExecStartPre=-${PROJECT_ROOT}/tools/connect.sh
ExecStart=${PROJECT_ROOT}/tools/monitor-puzzles.sh
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
SERVICE

if systemctl list-unit-files escape-room-tv-dashboard.service >/dev/null 2>&1; then
    sudo systemctl disable --now escape-room-tv-dashboard.service >/dev/null 2>&1 || true
    sudo rm -f /etc/systemd/system/escape-room-tv-dashboard.service
fi

sudo systemctl daemon-reload
sudo systemctl enable escape-room-controller.service

echo
echo "Autostart installed."
echo "Start now with:"
echo "  tools/start-room.sh"
echo
echo "Run in foreground with:"
echo "  tools/start.sh"
