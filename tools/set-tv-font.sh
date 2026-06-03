#!/usr/bin/env bash
set -euo pipefail

TV_CONSOLE="${TV_CONSOLE:-/dev/tty1}"
TV_FONT="${TV_FONT:-Lat2-Terminus32x16.psf.gz}"

font_candidates=()

add_font_candidate() {
    local font="$1"

    [ -n "${font}" ] || return 0
    font_candidates+=("${font}")

    if [[ "${font}" != /* ]]; then
        font_candidates+=("/usr/share/consolefonts/${font}")
        font_candidates+=("/usr/share/consolefonts/${font}.psf.gz")
        font_candidates+=("/usr/share/consolefonts/${font}.psf")
    fi
}

add_font_candidate "${TV_FONT}"
add_font_candidate "Lat2-Terminus32x16.psf.gz"
add_font_candidate "Lat15-Terminus32x16.psf.gz"
add_font_candidate "Uni3-Terminus32x16.psf.gz"
add_font_candidate "Lat2-TerminusBold32x16.psf.gz"
add_font_candidate "Lat15-TerminusBold32x16.psf.gz"
add_font_candidate "Uni3-TerminusBold32x16.psf.gz"

if ! command -v setfont >/dev/null 2>&1; then
    echo "setfont is not installed; TV dashboard will use the default console font."
    exit 0
fi

for font in "${font_candidates[@]}"; do
    if [[ "${font}" == /* && ! -f "${font}" ]]; then
        continue
    fi

    if setfont -C "${TV_CONSOLE}" "${font}" >/dev/null 2>&1; then
        echo "TV console font set to ${font} on ${TV_CONSOLE}."
        exit 0
    fi
done

echo "Could not set a larger TV console font; dashboard will use the default font."
exit 0
