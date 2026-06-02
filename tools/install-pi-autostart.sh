#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-/home/admin/escape-room}"
RUN_USER="${RUN_USER:-admin}"
SERVICE_PATH="/etc/systemd/system/escape-room-controller.service"

echo "Installing escape room autostart for project root: ${PROJECT_ROOT}"

sudo chmod +x "${PROJECT_ROOT}"/tools/*.sh

if nmcli connection show escape-room-hotspot >/dev/null 2>&1; then
    sudo nmcli connection modify escape-room-hotspot connection.autoconnect yes
else
    echo "escape-room-hotspot does not exist yet; creating it now."
    bash "${PROJECT_ROOT}/tools/start-wifi.sh"
fi

cat <<SERVICE | sudo tee "${SERVICE_PATH}" >/dev/null
[Unit]
Description=Escape Room Raspberry Pi Controller
After=network-online.target mosquitto.service
Wants=network-online.target
Requires=mosquitto.service

[Service]
Type=simple
User=${RUN_USER}
WorkingDirectory=${PROJECT_ROOT}
Environment=PROJECT_ROOT=${PROJECT_ROOT}
Environment=PLATFORMIO_VENV=/home/${RUN_USER}/.platformio-venv
ExecStart=${PROJECT_ROOT}/tools/monitor-puzzles.sh
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
SERVICE

sudo systemctl daemon-reload
sudo systemctl enable escape-room-controller.service

echo
echo "Autostart installed."
echo "Start now with:"
echo "  sudo systemctl start escape-room-controller.service"
echo
echo "Check status with:"
echo "  sudo systemctl status escape-room-controller.service"
echo
echo "Watch logs with:"
echo "  journalctl -u escape-room-controller.service -f"
