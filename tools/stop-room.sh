#!/usr/bin/env bash
set -euo pipefail

sudo systemctl stop escape-room-controller.service
sudo systemctl status escape-room-controller.service
