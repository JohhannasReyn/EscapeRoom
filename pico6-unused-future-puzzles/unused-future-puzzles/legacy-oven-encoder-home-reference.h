#ifndef LEGACY_OVEN_ENCODER_HOME_REFERENCE_H
#define LEGACY_OVEN_ENCODER_HOME_REFERENCE_H

// Archived from the active Pico 4 oven dial when the room moved to a
// potentiometer. Keep this here if a future puzzle needs a rotary encoder with
// a magnetic home/reference sensor.

namespace LegacyOvenEncoderHome {
inline constexpr int OVEN_ENCODER_CLK_PIN = 19;
inline constexpr int OVEN_ENCODER_DT_PIN = 20;
inline constexpr int OVEN_HOME_SENSOR_PIN = 21;
inline constexpr int OVEN_DEGREES_PER_STEP = 10;
inline constexpr const char* OVEN_HOME_DETECTED_TOPIC = "escape/pico4/oven_home_detected";

inline int normalizeDegrees(int degrees) {
    int normalized = degrees % 360;
    return normalized < 0 ? normalized + 360 : normalized;
}

inline int encoderDegreesFromSteps(int steps, int degreesPerStep) {
    return normalizeDegrees(steps * degreesPerStep);
}

inline int shortestDegreeDistance(int a, int b) {
    int diff = normalizeDegrees(a) - normalizeDegrees(b);

    if (diff < -180) {
        diff += 360;
    } else if (diff > 180) {
        diff -= 360;
    }

    return diff < 0 ? -diff : diff;
}

inline bool encoderDialIsAtTarget(int currentDegrees, int targetDegrees, int toleranceDegrees) {
    if (toleranceDegrees < 0) {
        return false;
    }

    return shortestDegreeDistance(currentDegrees, targetDegrees) <= toleranceDegrees;
}
} // namespace LegacyOvenEncoderHome

#endif
