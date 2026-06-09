#!/usr/bin/env bash
set -euo pipefail

if command -v wpctl >/dev/null 2>&1; then
    wpctl set-mute @DEFAULT_AUDIO_SINK@ 0
    wpctl set-volume @DEFAULT_AUDIO_SINK@ 10%+
elif command -v amixer >/dev/null 2>&1; then
    amixer sset Master 10%+ unmute >/dev/null
else
    echo "Missing audio volume tool: install wireplumber or alsa-utils."
    exit 1
fi

echo "Volume increased by 10%."
