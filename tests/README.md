# Host-Side Test Audit

These tests check the logic that can be verified without Raspberry Pi or Pico hardware attached.

Run from the repository root on macOS, Linux, or Raspberry Pi:

```bash
tools/run-host-tests.sh
```

Run from the repository root in Windows PowerShell:

```powershell
.\tools\run-host-tests.ps1
```

The tests cover:

- Raspberry Pi controller topic routing, five-Pico POST behavior, room state transitions, random activation/reset cue routing, immediate painting/color audio trigger order, display message trigger, oven unlock commands, and oven telemetry parsing
- Whole-room reset hold timing
- Cubby LED segment math
- POST topic and payload naming
- Debounced puzzle input timing
- Timed relay/output pulse behavior
- Cubby LED current estimates, invalid power settings, and automatic brightness capping math
- Component diagnostic test selection names, LED index wrapping, and rainbow color helper behavior
- Oven dial math for potentiometer scaling, the 300-400 degree stepped range, the 350-degree target, and tolerance edges
- Oven thermometer LED band/progression math
- Shared protocol topic names
- Room state names
- Pico startup wiring/status report topic, payload text, and Pi logging path
- Committed portable-router WiFi defaults, Pi rebase flash recommendations, and flash-script coverage
- SSH-wide help installer, executable rebase permissions, fire-panel command wrappers, button-order capture tool, optional send-to-John packaging/sending script, serialized non-blocking audio queue coverage, low-latency compressed-audio playback flags, and activation/reset cue file coverage

These tests do not replace hardware testing. They are meant to catch code mistakes before upload day so the hardware test is mostly about wiring, power, and sensor placement.
