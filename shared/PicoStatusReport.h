#ifndef PICO_STATUS_REPORT_H
#define PICO_STATUS_REPORT_H

namespace EscapePicoStatus {
inline constexpr const char* PICO2_REPORT =
    "Pico2 - Copper Puzzle Piece Detection\n"
    "Connected. Wiring: GPIO 15 to puzzle contact 1 and GND to contact 2";

inline constexpr const char* PICO3_REPORT =
    "Pico3 - Painting Rotation Sensor\n"
    "Connected. Wiring: GPIO 15 to sensor signal/output, 3V3 to sensor VCC/red, and GND to sensor GND/black";

inline constexpr const char* PICO4_REPORT =
    "Pico4 - Smart Film, Oven Potentiometer, and Lock\n"
    "Connected. Wiring: GPIO 15 to smart-film relay IN, GPIO 16 to buzzer +, GPIO 18 to lock relay IN, GPIO 26 to oven pot wiper, pot outer legs to 3V3 and GND";

inline constexpr const char* PICO5_REPORT =
    "Pico5 - Color Button Sequence\n"
    "Connected. Wiring: GPIO 15 red, GPIO 16 green, GPIO 17 yellow, GPIO 18 blue; each button connects its GPIO to GND";

inline constexpr const char* PICO7_REPORT =
    "Pico7 - Fire Panel Remote\n"
    "Connected. Wiring: GP2-GP11 buttons to GND; GP12-GP21 status LEDs with resistors";
} // namespace EscapePicoStatus

#endif
