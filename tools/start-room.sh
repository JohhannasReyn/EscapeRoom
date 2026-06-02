#!/usr/bin/env bash
set -euo pipefail

sudo systemctl start escape-room-controller.service
sudo systemctl status escape-room-controller.service
