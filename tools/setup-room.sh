#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-/home/admin/escape-room}"

cd "${PROJECT_ROOT}"
tools/install-pi-autostart.sh
sudo systemctl start escape-room-controller.service
sudo systemctl status escape-room-controller.service
