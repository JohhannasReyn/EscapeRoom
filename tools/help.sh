#!/usr/bin/env bash
set -euo pipefail

cat <<'EOF'
Escape room tools

Setup / operation:
  tools/start.sh              Start the controller in the terminal, without TV dashboard.
  tools/connect or connect.sh Connect paired Bluetooth speaker and play a success sound.
  tools/pair.sh               Scan, select, pair, trust, and save a Bluetooth speaker.
  tools/rebase.sh             Pull latest public repo folders, prepare venv, and build controller.

Testing:
  tools/test-connection.sh    Print one-line connected/not-found status for each Pico.
  tools/test-all.sh           Simulate the full walkthrough with delays between steps.
  tools/test-pico1.sh         Simulate motion sensor trigger to test cubby LEDs.
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
EOF
