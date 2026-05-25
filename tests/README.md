# Host-Side Test Audit

These tests check the logic that can be verified without Raspberry Pi or Pico hardware attached.

Run from the repository root in PowerShell:

```powershell
.\tools\run-host-tests.ps1
```

The tests cover:

- Raspberry Pi controller topic routing, POST red/green behavior, oven unlock gating, and oven degree telemetry parsing
- Whole-room reset hold timing
- Cubby LED segment math
- POST topic and payload naming
- Debounced puzzle input timing
- Timed relay/output pulse behavior
- Cubby LED current estimates, invalid power settings, and automatic brightness capping math
- Oven encoder degree math for the 350-degree target, wraparound, reverse movement, and tolerance edges

These tests do not replace hardware testing. They are meant to catch code mistakes before upload day so the hardware test is mostly about wiring, power, and sensor placement.
