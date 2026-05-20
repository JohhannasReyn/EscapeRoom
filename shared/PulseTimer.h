#ifndef ESCAPE_ROOM_PULSE_TIMER_H
#define ESCAPE_ROOM_PULSE_TIMER_H

inline unsigned long pulseOffAt(unsigned long nowMs, unsigned long durationMs) {
    return nowMs + durationMs;
}

inline bool pulseShouldTurnOff(unsigned long offAtMs, unsigned long nowMs) {
    return offAtMs != 0 && nowMs >= offAtMs;
}

#endif
