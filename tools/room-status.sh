#!/usr/bin/env bash
set -euo pipefail

sudo systemctl status escape-room-controller.service escape-room-tv-dashboard.service
