#include <cassert>
#include <string>

#include "../shared/PostState.h"

int main() {
    assert(std::string(postStatePayload(false)) == "ready");
    assert(std::string(postStatePayload(true)) == "completed");

    assert(postStateTopic(1) == "escape/post/cubby/1/state");
    assert(postStateTopic(2) == "escape/post/cubby/2/state");
    assert(postStateTopic(3) == "escape/post/cubby/3/state");
    assert(postStateTopic(4) == "escape/post/cubby/4/state");
    assert(postStateTopic(5) == "escape/post/cubby/5/state");
    assert(postStateTopic(6) == "escape/post/cubby/6/state");

    assert(cubbyStatusTopic(1) == "escape/cubby/1/status");
    assert(cubbyStatusTopic(6) == "escape/cubby/6/status");

    return 0;
}
