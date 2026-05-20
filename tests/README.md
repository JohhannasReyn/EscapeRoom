# Host-Side Test Audit

These tests check the logic that can be verified without Raspberry Pi or Pico hardware attached.

Run from the repository root in PowerShell:

```powershell
.\tools\run-host-tests.ps1
```

The tests cover:

- Raspberry Pi controller topic routing and POST red/green behavior
- Whole-room reset hold timing
- Cubby LED segment math
- POST topic and payload naming
- Debounced puzzle input timing
- Timed relay/output pulse behavior

These tests do not replace hardware testing. They are meant to catch code mistakes before upload day so the hardware test is mostly about wiring, power, and sensor placement.
