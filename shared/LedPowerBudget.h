#ifndef ESCAPE_ROOM_LED_POWER_BUDGET_H
#define ESCAPE_ROOM_LED_POWER_BUDGET_H

#include <cstdint>

struct RgbColor {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

inline int clampBrightness(int brightness) {
    if (brightness <= 0) {
        return 0;
    }

    return brightness > 255 ? 255 : brightness;
}

inline int colorCurrentAtFullBrightnessMa(RgbColor color, int fullWhiteCurrentMa) {
    if (fullWhiteCurrentMa <= 0) {
        return 0;
    }

    int channelTotal = static_cast<int>(color.r) + static_cast<int>(color.g) + static_cast<int>(color.b);
    return (fullWhiteCurrentMa * channelTotal + 382) / 765;
}

inline int estimateSegmentCurrentMa(
    RgbColor color,
    int ledCount,
    int requestedBrightness,
    int fullWhiteCurrentMa
) {
    if (ledCount <= 0 || requestedBrightness <= 0 || fullWhiteCurrentMa <= 0) {
        return 0;
    }

    int brightness = clampBrightness(requestedBrightness);
    int fullBrightnessCurrent = colorCurrentAtFullBrightnessMa(color, fullWhiteCurrentMa) * ledCount;
    return (fullBrightnessCurrent * brightness + 127) / 255;
}

inline int cappedBrightnessForBudget(
    int requestedBrightness,
    int estimatedCurrentMa,
    int budgetMa
) {
    if (requestedBrightness <= 0 || estimatedCurrentMa <= 0 || budgetMa <= 0) {
        return 0;
    }

    int brightness = clampBrightness(requestedBrightness);

    if (estimatedCurrentMa <= budgetMa) {
        return brightness;
    }

    int capped = (brightness * budgetMa) / estimatedCurrentMa;
    return capped < 1 ? 1 : capped;
}

#endif
