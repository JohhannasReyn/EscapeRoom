#ifndef ESCAPE_ROOM_PUZZLE_DEBOUNCE_H
#define ESCAPE_ROOM_PUZZLE_DEBOUNCE_H

struct DebouncedInputState {
    bool solved;
    int lastState;
    unsigned long stableStartMs;
};

inline bool updateDebouncedSolvedInput(
    DebouncedInputState& input,
    int currentState,
    unsigned long nowMs,
    unsigned long debounceMs
) {
    if (currentState != input.lastState) {
        input.lastState = currentState;
        input.stableStartMs = nowMs;
    }

    if (currentState != 0 && !input.solved && nowMs - input.stableStartMs >= debounceMs) {
        input.solved = true;
        return true;
    }

    return false;
}

#endif
