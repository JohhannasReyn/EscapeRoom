#ifndef CUBBY_LED_LAYOUT_H
#define CUBBY_LED_LAYOUT_H

struct CubbyLedSegment {
    int start;
    int count;
};

inline constexpr int totalCubbyLedCount(int cubbyCount, int ledsPerCubby, int ledsBetweenCubbies) {
    if (cubbyCount <= 0 || ledsPerCubby <= 0) {
        return 0;
    }

    int gaps = cubbyCount > 1 ? cubbyCount - 1 : 0;
    return cubbyCount * ledsPerCubby + gaps * ledsBetweenCubbies;
}

inline constexpr CubbyLedSegment cubbySegment(
    int cubbyNumber,
    int ledsPerCubby,
    int ledsBetweenCubbies
) {
    if (cubbyNumber <= 0 || ledsPerCubby <= 0) {
        return {0, 0};
    }

    int start = (cubbyNumber - 1) * (ledsPerCubby + ledsBetweenCubbies);
    return {start, ledsPerCubby};
}

#endif
