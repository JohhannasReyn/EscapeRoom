#ifndef ESCAPE_ROOM_OVEN_DIAL_H
#define ESCAPE_ROOM_OVEN_DIAL_H

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
