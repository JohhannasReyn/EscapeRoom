#include <cassert>
#include <string>
#include <vector>

#include "effects/Effect.h"
#include "effects/RandomEffect.h"

class RecordingEffect : public Effect {
public:
    void trigger(const std::string& payload) override {
        lastPayload = payload;
        ++triggerCount;
    }

    int triggerCount = 0;
    std::string lastPayload;
};

int main() {
    RandomEffect empty({});
    empty.trigger("ignored");
    assert(empty.effectCount() == 0);

    RecordingEffect only;
    RandomEffect single({&only}, 1234);
    single.trigger("room activated");
    assert(single.effectCount() == 1);
    assert(only.triggerCount == 1);
    assert(only.lastPayload == "room activated");

    RecordingEffect first;
    RecordingEffect second;
    RandomEffect pair({&first, nullptr, &second}, 5678);
    assert(pair.effectCount() == 2);

    for (int i = 0; i < 20; ++i) {
        pair.trigger("room reset");
    }

    assert(first.triggerCount + second.triggerCount == 20);
    assert(first.lastPayload == "room reset" || second.lastPayload == "room reset");

    return 0;
}
