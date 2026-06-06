#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: tools/set-volume.sh -v 50"
}

volume=""
while getopts ":v:" opt; do
    case "${opt}" in
        v) volume="${OPTARG}" ;;
        *) usage; exit 1 ;;
    esac
done

if ! [[ "${volume}" =~ ^[0-9]+$ ]] || [ "${volume}" -lt 0 ] || [ "${volume}" -gt 100 ]; then
    usage
    exit 1
fi

wpctl set-mute @DEFAULT_AUDIO_SINK@ 0
wpctl set-volume @DEFAULT_AUDIO_SINK@ "${volume}%"
echo "Volume set to ${volume}%."
