#ifndef ESCAPE_ROOM_OVEN_DIAL_H
#define ESCAPE_ROOM_OVEN_DIAL_H

inline int normalizeDegrees(int degrees) {
    int normalized = degrees % 360;
    return normalized < 0 ? normalized + 360 : normalized;
}

inline int encoderDegreesFromSteps(int steps, int degreesPerStep) {
    return normalizeDegrees(steps * degreesPerStep);
}

inline int clampInt(int value, int minValue, int maxValue) {
    if (value < minValue) {
        return minValue;
    }

    if (value > maxValue) {
        return maxValue;
    }

    return value;
}

inline int ovenValueFromPotReading(
    int rawReading,
    int rawMin,
    int rawMax,
    int ovenMinValue,
    int ovenMaxValue
) {
    if (rawMax <= rawMin || ovenMaxValue <= ovenMinValue) {
        return ovenMinValue;
    }

    int clampedReading = clampInt(rawReading, rawMin, rawMax);
    long readingSpan = static_cast<long>(rawMax) - rawMin;
    long ovenSpan = static_cast<long>(ovenMaxValue) - ovenMinValue;
    long scaled = (static_cast<long>(clampedReading) - rawMin) * ovenSpan;

    return ovenMinValue + static_cast<int>((scaled + (readingSpan / 2)) / readingSpan);
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

inline bool ovenValueIsAtTarget(int ovenValue, int targetValue, int tolerance) {
    if (tolerance < 0) {
        return false;
    }

    int diff = ovenValue - targetValue;
    if (diff < 0) {
        diff = -diff;
    }

    return diff <= tolerance;
}

#endif
