#include <cassert>

#include "../shared/PulseTimer.h"

int main() {
    assert(pulseOffAt(1000, 500) == 1500);

    assert(pulseShouldTurnOff(0, 2000) == false);
    assert(pulseShouldTurnOff(1500, 1499) == false);
    assert(pulseShouldTurnOff(1500, 1500) == true);
    assert(pulseShouldTurnOff(1500, 2000) == true);

    return 0;
}
