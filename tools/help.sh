#!/usr/bin/env bash
set -euo pipefail

cat <<'EOF'
Escape room tools

Setup / operation:
  tools/start.sh              Start the controller in the terminal, without TV dashboard.
  tools/connect.sh            Test wired speaker output through the Raspberry Pi 3.5mm jack.
  tools/pair.sh               Bluetooth is retired; prints wired-audio setup guidance.
  tools/rebase.sh             Pull latest public repo folders, prepare venv, and build controller.

Testing:
  tools/test-connection.sh    Print one-line connected/not-found status for each Pico.
  tools/test-all.sh           Simulate the full walkthrough with delays between steps.
  tools/test-pico1.sh         Retired Pico 1 manual legacy test.
  tools/test-pico2.sh         Simulate one-piece copper puzzle completion.
  tools/test-pico3.sh         Simulate painting rotation completion.
  tools/test-pico4.sh         Reveal smart film and enable oven knob.
  tools/test-pico5.sh         Enable color button sequence.

Audio:
  tools/set-volume.sh -v 50   Set speaker volume to 50%.
  tools/volume-up.sh          Increase volume by 10%.
  tools/volume-down.sh        Decrease volume by 10%.

Debug:
  tools/watch-mqtt.sh         Watch raw MQTT messages.
  tools/room-logs.sh          Watch systemd logs if running as a service.

Fire panel fallbacks:
  fire/status                 Ask all Picos to report state.
  fire/film-on                Reveal smart film.
  fire/film-off               Hide/reset smart film.
  fire/sound-look             Play check-the-oven audio.
  fire/sound-crash            Play crashing plates audio.
  fire/sound-fail             Play buzzer audio.
  fire/sound-pass             Play success audio.
  fire/sound-bake             Play bake-at-350 audio.
  fire/unlock                 Release final lock.
  fire/reset-all              Reset all puzzles; physical panel reset requires a 5-second hold.
EOF
