#ifndef ESCAPE_ROOM_ENCODER_DIAL_H
#define ESCAPE_ROOM_ENCODER_DIAL_H

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

inline bool dialIsAtTarget(int currentDegrees, int targetDegrees, int toleranceDegrees) {
    return shortestDegreeDistance(currentDegrees, targetDegrees) <= toleranceDegrees;
}

#endif
