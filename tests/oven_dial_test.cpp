#include <cassert>

#include "../shared/OvenDial.h"

int main() {
    assert(normalizeDegrees(0) == 0);
    assert(normalizeDegrees(360) == 0);
    assert(normalizeDegrees(370) == 10);
    assert(normalizeDegrees(-10) == 350);
    assert(normalizeDegrees(-720) == 0);
    assert(normalizeDegrees(-721) == 359);

    assert(encoderDegreesFromSteps(35, 10) == 350);
    assert(encoderDegreesFromSteps(-1, 10) == 350);
    assert(encoderDegreesFromSteps(36, 10) == 0);
    assert(encoderDegreesFromSteps(1, -10) == 350);
    assert(encoderDegreesFromSteps(1000, 0) == 0);

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

    assert(shortestDegreeDistance(350, 0) == 10);
    assert(shortestDegreeDistance(5, 355) == 10);
    assert(shortestDegreeDistance(180, 0) == 180);
    assert(shortestDegreeDistance(-5, 5) == 10);
    assert(shortestDegreeDistance(720, 359) == 1);

    assert(dialIsAtTarget(350, 350, 5) == true);
    assert(dialIsAtTarget(346, 350, 5) == true);
    assert(dialIsAtTarget(344, 350, 5) == false);
    assert(dialIsAtTarget(0, 350, 10) == true);
    assert(dialIsAtTarget(5, 355, 9) == false);
    assert(dialIsAtTarget(5, 355, 10) == true);
    assert(dialIsAtTarget(350, 350, -1) == false);

    assert(ovenValueIsAtTarget(350, 350, 10) == true);
    assert(ovenValueIsAtTarget(341, 350, 10) == true);
    assert(ovenValueIsAtTarget(339, 350, 10) == false);
    assert(ovenValueIsAtTarget(0, 350, 10) == false);
    assert(ovenValueIsAtTarget(500, 350, 10) == false);
    assert(ovenValueIsAtTarget(350, 350, -1) == false);

    return 0;
}
