#include <cassert>
#include <memory>
#include <string>

#include "../src/GameController.h"
#include "../src/effects/Effect.h"
#include "../src/puzzles/CopperPuzzle.h"
#include "../src/puzzles/PlannedPuzzles.h"

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
    RecordingEffect effect;
    CopperPuzzle copper(effect);

    assert(copper.topic() == "escape/puzzle/copper/solved");
    assert(copper.handle("escape/puzzle/unknown/solved", "ignored") == false);
    assert(effect.triggerCount == 0);

    assert(copper.handle("escape/puzzle/copper/solved", "manual test") == true);
    assert(effect.triggerCount == 1);
    assert(effect.lastPayload == "manual test");

    GameController controller;
    controller.addPuzzle(std::make_unique<CopperPuzzle>(effect));
    controller.addPuzzle(std::make_unique<StairsPuzzle>());
    controller.addPuzzle(std::make_unique<DowelsPuzzle>());
    controller.addPuzzle(std::make_unique<WinePuzzle>());
    controller.addPuzzle(std::make_unique<BlenderPuzzle>());
    controller.addPuzzle(std::make_unique<FireplacePuzzle>());
    controller.addPuzzle(std::make_unique<PhonePuzzle>());

    assert(controller.puzzleCount() == 7);
    assert(controller.handleMessage("escape/puzzle/copper/solved", "from controller") == true);
    assert(effect.triggerCount == 2);
    assert(effect.lastPayload == "from controller");

    assert(controller.handleMessage("escape/puzzle/dowels/solved", "future puzzle") == true);
    assert(controller.handleMessage("escape/puzzle/not-real/solved", "ignored") == false);

    return 0;
}
