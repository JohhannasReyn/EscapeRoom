#!/usr/bin/env bash
set -euo pipefail

sudo systemctl start escape-room-controller.service
sudo systemctl start escape-room-tv-dashboard.service
tools/room-status.sh
