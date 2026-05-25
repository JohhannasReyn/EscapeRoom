#include <cassert>

#include "../shared/EncoderDial.h"

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

    return 0;
}
