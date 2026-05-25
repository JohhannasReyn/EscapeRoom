#include <cassert>

#include "../shared/LedPowerBudget.h"

int main() {
    assert(colorCurrentAtFullBrightnessMa({255, 255, 255}, 60) == 60);
    assert(colorCurrentAtFullBrightnessMa({255, 0, 0}, 60) == 20);
    assert(colorCurrentAtFullBrightnessMa({0, 255, 0}, 60) == 20);
    assert(colorCurrentAtFullBrightnessMa({0, 0, 255}, 60) == 20);
    assert(colorCurrentAtFullBrightnessMa({0, 0, 0}, 60) == 0);

    assert(estimateSegmentCurrentMa({255, 255, 255}, 30, 255, 60) == 1800);
    assert(estimateSegmentCurrentMa({255, 255, 255}, 30, 128, 60) == 904);
    assert(estimateSegmentCurrentMa({255, 210, 120}, 30, 80, 60) == 433);
    assert(estimateSegmentCurrentMa({255, 255, 255}, 1, 999, 60) == 60);
    assert(estimateSegmentCurrentMa({255, 255, 255}, 1, -10, 60) == 0);
    assert(estimateSegmentCurrentMa({255, 255, 255}, -1, 255, 60) == 0);
    assert(estimateSegmentCurrentMa({255, 255, 255}, 1, 255, -60) == 0);

    assert(cappedBrightnessForBudget(80, 1200, 3000) == 80);
    assert(cappedBrightnessForBudget(300, 1200, 3000) == 255);
    assert(cappedBrightnessForBudget(255, 10800, 2500) == 59);
    assert(cappedBrightnessForBudget(80, 3600, 2500) == 55);
    assert(cappedBrightnessForBudget(80, 0, 2500) == 0);
    assert(cappedBrightnessForBudget(80, 3600, -1) == 0);

    return 0;
}
