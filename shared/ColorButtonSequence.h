#ifndef COLOR_BUTTON_SEQUENCE_H
#define COLOR_BUTTON_SEQUENCE_H

inline constexpr const char* COLOR_BUTTON_SEQUENCE = "RRRGGGGYYBBB";
inline constexpr int COLOR_BUTTON_SEQUENCE_LENGTH = 12;

inline bool colorButtonCodeMatchesStep(int stepIndex, char code) {
    if (stepIndex < 0 || stepIndex >= COLOR_BUTTON_SEQUENCE_LENGTH) {
        return false;
    }

    return COLOR_BUTTON_SEQUENCE[stepIndex] == code;
}

inline bool colorFailureUsesTryAgainCue(int failureCount) {
    return failureCount > 0 && ((failureCount - 1) % 3) == 0;
}

#endif
