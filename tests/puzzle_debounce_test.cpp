#include <cassert>

#include "../shared/PuzzleDebounce.h"

int main() {
    DebouncedInputState input{false, 0, 0};

    assert(updateDebouncedSolvedInput(input, 0, 100, 750) == false);

    assert(updateDebouncedSolvedInput(input, 1, 200, 750) == false);
    assert(input.lastState == 1);
    assert(input.stableStartMs == 200);

    assert(updateDebouncedSolvedInput(input, 1, 949, 750) == false);
    assert(input.solved == false);

    assert(updateDebouncedSolvedInput(input, 1, 950, 750) == true);
    assert(input.solved == true);

    assert(updateDebouncedSolvedInput(input, 1, 2000, 750) == false);

    DebouncedInputState bounce{false, 0, 0};
    assert(updateDebouncedSolvedInput(bounce, 1, 100, 750) == false);
    assert(updateDebouncedSolvedInput(bounce, 0, 200, 750) == false);
    assert(updateDebouncedSolvedInput(bounce, 1, 300, 750) == false);
    assert(updateDebouncedSolvedInput(bounce, 1, 1049, 750) == false);
    assert(updateDebouncedSolvedInput(bounce, 1, 1050, 750) == true);

    return 0;
}
