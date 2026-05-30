# Pico 6 Unused / Future Puzzle Archive

Pico 6 is no longer part of the active five-Pico runtime layout.

This folder preserves firmware and config snapshots for puzzle modules that were removed from the active flow but may be useful later:

- `old-pico-cabinet-dowels-wine-*`: dowels and wine contact puzzles
- `old-pico-tv-wall-*`: TV wall trigger relay
- `old-pico-phone-window-props-*`: phone and window prop inputs
- `old-pico-back-room-blender-final-*`: blender flour puzzle and final output relay
- `legacy-oven-encoder-home-reference.h`: archived rotary encoder plus magnetic home sensor helpers from the older Pico 4 oven dial design

To restore one of these modules later, copy the archived `main.cpp` and `platformio.ini` back into a Pico project folder, then update MQTT topics, pin constants, and the Raspberry Pi controller state machine before wiring it into the room.

Pico 6 should stay out of the active runtime unless a future puzzle is intentionally assigned to it.
