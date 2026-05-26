#include <cassert>

#include "../shared/OvenThermometer.h"

int main() {
    assert(clampOvenValue(-10, 0, 500) == 0);
    assert(clampOvenValue(350, 0, 500) == 350);
    assert(clampOvenValue(999, 0, 500) == 500);

    assert(litThermometerLedCount(0, 0, 500, 10) == 0);
    assert(litThermometerLedCount(1, 0, 500, 10) == 1);
    assert(litThermometerLedCount(250, 0, 500, 10) == 5);
    assert(litThermometerLedCount(350, 0, 500, 10) == 7);
    assert(litThermometerLedCount(500, 0, 500, 10) == 10);
    assert(litThermometerLedCount(500, 500, 500, 10) == 0);
    assert(litThermometerLedCount(250, 0, 500, 0) == 0);

    assert(thermometerBandForIndex(-1, 5) == ThermometerBand::Off);
    assert(thermometerBandForIndex(5, 5) == ThermometerBand::Off);
    assert(thermometerBandForIndex(0, 3) == ThermometerBand::Yellow);
    assert(thermometerBandForIndex(0, 9) == ThermometerBand::Yellow);
    assert(thermometerBandForIndex(3, 9) == ThermometerBand::Orange);
    assert(thermometerBandForIndex(6, 9) == ThermometerBand::Red);

    RgbColor yellow = thermometerColor(ThermometerBand::Yellow);
    RgbColor orange = thermometerColor(ThermometerBand::Orange);
    RgbColor red = thermometerColor(ThermometerBand::Red);
    RgbColor off = thermometerColor(ThermometerBand::Off);

    assert(yellow.r == 255 && yellow.g == 220 && yellow.b == 0);
    assert(orange.r == 255 && orange.g == 90 && orange.b == 0);
    assert(red.r == 255 && red.g == 0 && red.b == 0);
    assert(off.r == 0 && off.g == 0 && off.b == 0);

    return 0;
}
