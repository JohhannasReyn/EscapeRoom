#!/usr/bin/env bash
set -euo pipefail

wpctl set-volume @DEFAULT_AUDIO_SINK@ 10%-
echo "Volume decreased by 10%."
