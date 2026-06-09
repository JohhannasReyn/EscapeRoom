#!/usr/bin/env bash
set -uo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
VOLUME="${VOLUME:-80%}"
TEST_SOUND="${TEST_SOUND:-${PROJECT_ROOT}/assets/audio/yeah-you-did-it.mp3}"

if command -v wpctl >/dev/null 2>&1; then
    wpctl set-volume @DEFAULT_AUDIO_SINK@ "${VOLUME}" || true
    wpctl set-mute @DEFAULT_AUDIO_SINK@ 0 || true
elif command -v amixer >/dev/null 2>&1; then
    amixer sset Master "${VOLUME}" unmute >/dev/null 2>&1 || true
fi

echo "Wired audio output ready. Plug the speaker into the Raspberry Pi 3.5mm jack."

if [ -f "${TEST_SOUND}" ]; then
    if command -v ffplay >/dev/null 2>&1; then
        ffplay -nodisp -autoexit -loglevel quiet "${TEST_SOUND}" >/dev/null 2>&1 || true
    elif command -v pw-play >/dev/null 2>&1; then
        pw-play "${TEST_SOUND}" >/dev/null 2>&1 || true
    elif command -v aplay >/dev/null 2>&1; then
        aplay "${TEST_SOUND}" >/dev/null 2>&1 || true
    fi
else
    echo "Test sound not found: ${TEST_SOUND}"
fi
