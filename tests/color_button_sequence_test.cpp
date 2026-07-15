#include <cassert>
#include <string>

#include "ColorButtonSequence.h"

int main() {
    assert(std::string(COLOR_BUTTON_SEQUENCE) == "RRRGGGGYYBBB");
    assert(COLOR_BUTTON_SEQUENCE_LENGTH == 12);

    assert(colorButtonCodeMatchesStep(0, 'R') == true);
    assert(colorButtonCodeMatchesStep(1, 'R') == true);
    assert(colorButtonCodeMatchesStep(2, 'R') == true);
    assert(colorButtonCodeMatchesStep(3, 'G') == true);
    assert(colorButtonCodeMatchesStep(6, 'G') == true);
    assert(colorButtonCodeMatchesStep(7, 'Y') == true);
    assert(colorButtonCodeMatchesStep(8, 'Y') == true);
    assert(colorButtonCodeMatchesStep(9, 'B') == true);
    assert(colorButtonCodeMatchesStep(11, 'B') == true);

    assert(colorButtonCodeMatchesStep(0, 'G') == false);
    assert(colorButtonCodeMatchesStep(3, 'R') == false);
    assert(colorButtonCodeMatchesStep(12, 'B') == false);
    assert(colorButtonCodeMatchesStep(-1, 'R') == false);

    assert(colorFailureUsesTryAgainCue(1) == true);
    assert(colorFailureUsesTryAgainCue(2) == false);
    assert(colorFailureUsesTryAgainCue(3) == false);
    assert(colorFailureUsesTryAgainCue(4) == true);
    assert(colorFailureUsesTryAgainCue(5) == false);
    assert(colorFailureUsesTryAgainCue(6) == false);
    assert(colorFailureUsesTryAgainCue(7) == true);
    assert(colorFailureUsesTryAgainCue(0) == false);

    return 0;
}
