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

- Raspberry Pi controller topic routing, five-Pico POST behavior, room state transitions, painting audio trigger, wrong-code audio trigger, display message trigger, oven unlock commands, and oven telemetry parsing
- Whole-room reset hold timing
- Cubby LED segment math
- POST topic and payload naming
- Debounced puzzle input timing
- Timed relay/output pulse behavior
- Cubby LED current estimates, invalid power settings, and automatic brightness capping math
- Component diagnostic test selection names, LED index wrapping, and rainbow color helper behavior
- Oven dial math for potentiometer scaling, the 350-degree target, and tolerance edges
- Oven thermometer LED band/progression math
- Shared protocol topic names
- Room state names

These tests do not replace hardware testing. They are meant to catch code mistakes before upload day so the hardware test is mostly about wiring, power, and sensor placement.
