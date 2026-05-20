#include <cassert>
#include <memory>
#include <string>

#include "../src/GameController.h"
#include "../src/ResetControl.h"
#include "../src/effects/Effect.h"
#include "../src/puzzles/CopperPuzzle.h"
#include "../src/puzzles/PlannedPuzzles.h"
#include "../../shared/PostState.h"

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
    assert(RESET_TOPIC == "escape/game/reset");
    assert(RESET_BUTTON_GPIO == 23);
    assert(RESET_HOLD_MS == 1000);
    assert(resetPressReady(1000, false) == false);
    assert(resetPressReady(999, true) == false);
    assert(resetPressReady(1000, true) == true);

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
    controller.addPuzzle(std::make_unique<WindowPuzzle>());

    assert(controller.puzzleCount() == 8);
    controller.queuePostQueryCommand();
    assert(controller.pendingCommandCount() == 1);
    MqttCommand queryCommand = controller.takeNextPendingCommand();
    assert(queryCommand.topic == "escape/post/query");
    assert(queryCommand.payload == "status");

    assert(controller.handleMessage("escape/post/cubby/2/state", "completed") == true);
    assert(controller.pendingCommandCount() == 1);
    MqttCommand redPostCommand = controller.takeNextPendingCommand();
    assert(redPostCommand.topic == cubbyStatusTopic(2));
    assert(redPostCommand.payload == "red");

    assert(controller.handleMessage("escape/post/cubby/2/state", "ready") == true);
    assert(controller.pendingCommandCount() == 1);
    MqttCommand greenPostCommand = controller.takeNextPendingCommand();
    assert(greenPostCommand.topic == cubbyStatusTopic(2));
    assert(greenPostCommand.payload == "green");

    for (int cubbyNumber = 1; cubbyNumber <= 6; ++cubbyNumber) {
        assert(controller.handleMessage("escape/post/cubby/" + std::to_string(cubbyNumber) + "/state", "ready") == true);
    }

    assert(controller.pendingCommandCount() == 7);

    for (int cubbyNumber = 1; cubbyNumber <= 6; ++cubbyNumber) {
        MqttCommand postReadyCommand = controller.takeNextPendingCommand();
        assert(postReadyCommand.topic == cubbyStatusTopic(cubbyNumber));
        assert(postReadyCommand.payload == "green");
    }

    MqttCommand autoReadyCommand = controller.takeNextPendingCommand();
    assert(autoReadyCommand.topic == "escape/cubby/all/status");
    assert(autoReadyCommand.payload == "off");

    controller.queueReadyCommand();
    assert(controller.pendingCommandCount() == 1);
    MqttCommand readyCommand = controller.takeNextPendingCommand();
    assert(readyCommand.topic == "escape/cubby/all/status");
    assert(readyCommand.payload == "off");

    assert(controller.handleMessage("escape/puzzle/copper/solved", "from controller") == true);
    assert(effect.triggerCount == 2);
    assert(effect.lastPayload == "from controller");

    assert(controller.handleMessage("escape/puzzle/dowels/solved", "future puzzle") == true);
    assert(controller.pendingCommandCount() == 1);
    MqttCommand firstCubbyCommand = controller.takeNextPendingCommand();
    assert(firstCubbyCommand.topic == "escape/cubby/2/light_on");
    assert(firstCubbyCommand.payload == "on");

    assert(controller.handleMessage("escape/puzzle/window/triggered", "right wall prop") == true);
    assert(controller.handleMessage("escape/puzzle/not-real/solved", "ignored") == false);

    return 0;
}
