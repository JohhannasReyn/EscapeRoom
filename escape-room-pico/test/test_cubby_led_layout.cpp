#include <cassert>

#include "../src/CubbyLedLayout.h"

int main() {
    assert(totalCubbyLedCount(0, 30, 3) == 0);
    assert(totalCubbyLedCount(1, 30, 3) == 30);
    assert(totalCubbyLedCount(4, 30, 3) == 129);

    CubbyLedSegment first = cubbySegment(1, 30, 3);
    assert(first.start == 0);
    assert(first.count == 30);

    CubbyLedSegment second = cubbySegment(2, 30, 3);
    assert(second.start == 33);
    assert(second.count == 30);

    CubbyLedSegment fourth = cubbySegment(4, 30, 3);
    assert(fourth.start == 99);
    assert(fourth.count == 30);

    return 0;
}
