#include <cassert>

#include "../shared/OvenDial.h"

int main() {
    assert(clampInt(-1, 0, 10) == 0);
    assert(clampInt(5, 0, 10) == 5);
    assert(clampInt(11, 0, 10) == 10);

    assert(ovenValueFromPotReading(0, 0, 4095, 0, 500) == 0);
    assert(ovenValueFromPotReading(4095, 0, 4095, 0, 500) == 500);
    assert(ovenValueFromPotReading(2867, 0, 4095, 0, 500) == 350);
    assert(ovenValueFromPotReading(-10, 0, 4095, 0, 500) == 0);
    assert(ovenValueFromPotReading(5000, 0, 4095, 0, 500) == 500);
    assert(ovenValueFromPotReading(100, 100, 100, 0, 500) == 0);
    assert(ovenValueFromPotReading(100, 0, 4095, 500, 0) == 500);
    assert(roundToNearestStep(347, 5) == 345);
    assert(roundToNearestStep(348, 5) == 350);
    assert(roundToNearestStep(351, 5) == 350);
    assert(roundToNearestStep(353, 5) == 355);
    assert(roundToNearestStep(353, 0) == 353);
    assert(roundToNearestStepFromBase(347, 170, 15) == 350);
    assert(roundToNearestStepFromBase(342, 170, 15) == 335);
    assert(roundToNearestStepFromBase(358, 170, 15) == 365);
    assert(roundToNearestStepFromBase(353, 170, 0) == 353);
    assert(ovenValueFromPotReading(0, 0, 4095, 170, 440, 15) == 170);
    assert(ovenValueFromPotReading(2730, 0, 4095, 170, 440, 15) == 350);
    assert(ovenValueFromPotReading(4095, 0, 4095, 170, 440, 15) == 440);

    assert(ovenValueIsAtTarget(350, 350, 10) == true);
    assert(ovenValueIsAtTarget(341, 350, 10) == true);
    assert(ovenValueIsAtTarget(339, 350, 10) == false);
    assert(ovenValueIsAtTarget(0, 350, 10) == false);
    assert(ovenValueIsAtTarget(500, 350, 10) == false);
    assert(ovenValueIsAtTarget(350, 350, -1) == false);

    return 0;
}
