#ifndef RESET_CONTROL_H
#define RESET_CONTROL_H

#include <string>

constexpr const char* RESET_TOPIC = "escape/game/reset";
constexpr int RESET_BUTTON_GPIO = 23;
constexpr unsigned long RESET_HOLD_MS = 5000;
constexpr unsigned long RESET_POLL_MS = 50;

inline bool resetPressReady(unsigned long heldMs, bool pressed) {
    return pressed && heldMs >= RESET_HOLD_MS;
}

#endif
