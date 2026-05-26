#ifndef ESCAPE_ROOM_OVEN_THERMOMETER_H
#define ESCAPE_ROOM_OVEN_THERMOMETER_H

#include "LedPowerBudget.h"

enum class ThermometerBand {
    Off,
    Yellow,
    Orange,
    Red,
};

inline int clampOvenValue(int value, int minValue, int maxValue) {
    if (value < minValue) {
        return minValue;
    }

    return value > maxValue ? maxValue : value;
}

inline int litThermometerLedCount(int value, int minValue, int maxValue, int ledCount) {
    if (ledCount <= 0 || maxValue <= minValue) {
        return 0;
    }

    int clamped = clampOvenValue(value, minValue, maxValue);
    int range = maxValue - minValue;
    int progress = clamped - minValue;
    return (progress * ledCount + range - 1) / range;
}

inline ThermometerBand thermometerBandForIndex(int ledIndex, int litCount) {
    if (ledIndex < 0 || ledIndex >= litCount) {
        return ThermometerBand::Off;
    }

    if (litCount <= 3) {
        return ThermometerBand::Yellow;
    }

    int redStart = (litCount * 2) / 3;
    int orangeStart = litCount / 3;

    if (ledIndex >= redStart) {
        return ThermometerBand::Red;
    }

    if (ledIndex >= orangeStart) {
        return ThermometerBand::Orange;
    }

    return ThermometerBand::Yellow;
}

inline RgbColor thermometerColor(ThermometerBand band) {
    switch (band) {
        case ThermometerBand::Yellow: return {255, 220, 0};
        case ThermometerBand::Orange: return {255, 90, 0};
        case ThermometerBand::Red: return {255, 0, 0};
        case ThermometerBand::Off: return {0, 0, 0};
    }

    return {0, 0, 0};
}

#endif
