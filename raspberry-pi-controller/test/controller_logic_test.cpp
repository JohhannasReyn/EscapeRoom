#include <cassert>
#include <memory>
#include <string>

#include "../src/GameController.h"
#include "../src/ResetControl.h"
#include "../src/effects/Effect.h"
#include "../src/effects/DisplayOutput.h"
#include "../src/puzzles/CopperPuzzle.h"
#include "../src/puzzles/PlannedPuzzles.h"
#include "EscapeRoomProtocol.h"
#include "PostState.h"
#include "RoomState.h"

class RecordingEffect : public Effect {
public:
    void trigger(const std::string& payload) override {
        lastPayload = payload;
        ++triggerCount;
    }

    int triggerCount = 0;
    std::string lastPayload;
};

class RecordingDisplay : public DisplayOutput {
public:
    void show_message(const std::string& text) override {
        lastMessage = text;
        ++showCount;
    }

    void flash_message(const std::string& text, int durationSec, double intervalSec) override {
        lastMessage = text;
        lastDuration = durationSec;
        lastInterval = intervalSec;
        ++flashCount;
    }

    void clear() override {
        ++clearCount;
    }

    int showCount = 0;
    int flashCount = 0;
    int clearCount = 0;
    int lastDuration = 0;
    double lastInterval = 0;
    std::string lastMessage;
};

void addActivePuzzles(GameController& controller) {
    controller.addPuzzle(std::make_unique<StairsPuzzle>());
    controller.addPuzzle(std::make_unique<CopperPuzzle>());
    controller.addPuzzle(std::make_unique<FinalPiecePuzzle>());
    controller.addPuzzle(std::make_unique<PaintingRotationPuzzle>());
    controller.addPuzzle(std::make_unique<ColorButtonSequencePuzzle>());
    controller.addPuzzle(std::make_unique<ColorButtonSequenceErrorPuzzle>());
    controller.addPuzzle(std::make_unique<OvenTargetPuzzle>());
    controller.addPuzzle(std::make_unique<ElectromagUnlockedPuzzle>());
}

int main() {
    assert(RESET_TOPIC == "escape/game/reset");
    assert(RESET_BUTTON_GPIO == 23);
    assert(RESET_HOLD_MS == 1000);
    assert(resetPressReady(1000, false) == false);
    assert(resetPressReady(999, true) == false);
    assert(resetPressReady(1000, true) == true);

    CopperPuzzle copper;
    assert(copper.topic() == std::string(EscapeTopic::COPPER_PUZZLE_COMPLETE));
    assert(copper.handle("escape/puzzle/unknown/solved", "ignored") == false);
    assert(copper.handle(EscapeTopic::COPPER_PUZZLE_COMPLETE, "manual test") == true);
    assert(copper.handle("escape/puzzle/copper/solved", "legacy test") == true);

    RecordingEffect paintingAudio;
    RecordingEffect wrongCodeAudio;
    RecordingEffect bakeBuzzer;
    RecordingDisplay display;
    GameController controller(&paintingAudio, &display, &wrongCodeAudio, &bakeBuzzer);
    addActivePuzzles(controller);

    assert(controller.puzzleCount() == 8);
    assert(controller.currentState() == RoomState::WAITING_FOR_CUBBY_APPROACH);

    controller.queuePostQueryCommand();
    MqttCommand queryCommand = controller.takeNextPendingCommand();
    assert(queryCommand.topic == "escape/post/query");
    assert(queryCommand.payload == "status");

    assert(controller.handleMessage("escape/post/cubby/2/state", "completed") == true);
    MqttCommand redPostCommand = controller.takeNextPendingCommand();
    assert(redPostCommand.topic == cubbyStatusTopic(2));
    assert(redPostCommand.payload == "red");

    for (int cubbyNumber = 1; cubbyNumber <= 5; ++cubbyNumber) {
        assert(controller.handleMessage("escape/post/cubby/" + std::to_string(cubbyNumber) + "/state", "ready") == true);
    }

    assert(controller.pendingCommandCount() == 6);
    for (int cubbyNumber = 1; cubbyNumber <= 5; ++cubbyNumber) {
        MqttCommand postReadyCommand = controller.takeNextPendingCommand();
        assert(postReadyCommand.topic == cubbyStatusTopic(cubbyNumber));
        assert(postReadyCommand.payload == "green");
    }

    MqttCommand autoReadyCommand = controller.takeNextPendingCommand();
    assert(autoReadyCommand.topic == "escape/cubby/all/status");
    assert(autoReadyCommand.payload == "off");

    assert(controller.handleMessage("escape/post/cubby/6/state", "ready") == true);
    assert(controller.pendingCommandCount() == 0);

    assert(controller.handleMessage(EscapeTopic::CUBBY_APPROACH_DETECTED, "approach") == true);
    assert(controller.currentState() == RoomState::COPPER_PUZZLE_ACTIVE);
    MqttCommand cubbyLight = controller.takeNextPendingCommand();
    assert(cubbyLight.topic == EscapeTopic::ENABLE_CUBBY_LIGHT);
    MqttCommand legacyCubbyLight = controller.takeNextPendingCommand();
    assert(legacyCubbyLight.topic == EscapeTopic::LEGACY_CUBBY_1_LIGHT_ON);
    MqttCommand enableCopper = controller.takeNextPendingCommand();
    assert(enableCopper.topic == EscapeTopic::ENABLE_COPPER_PUZZLE);

    assert(controller.handleMessage(EscapeTopic::COPPER_PUZZLE_COMPLETE, "copper done") == true);
    assert(controller.currentState() == RoomState::PAINTING_ROTATION_ACTIVE);
    bool sawPaintingEnable = false;
    bool sawCopperCubby = false;
    while (controller.pendingCommandCount() > 0) {
        MqttCommand command = controller.takeNextPendingCommand();
        sawPaintingEnable = sawPaintingEnable || command.topic == EscapeTopic::ENABLE_PAINTING_ROTATION;
        sawCopperCubby = sawCopperCubby || command.topic == "escape/cubby/2/light_on";
    }
    assert(sawPaintingEnable == true);
    assert(sawCopperCubby == true);

    assert(controller.handleMessage(EscapeTopic::PAINTING_ROTATION_COMPLETE, "painting") == true);
    assert(paintingAudio.triggerCount == 1);
    assert(paintingAudio.lastPayload == "painting");
    assert(controller.currentState() == RoomState::FINAL_PIECE_ACTIVE);

    assert(controller.handleMessage(EscapeTopic::PAINTING_ROTATION_COMPLETE, "painting again") == true);
    assert(paintingAudio.triggerCount == 2);
    assert(paintingAudio.lastPayload == "painting again");
    assert(controller.currentState() == RoomState::FINAL_PIECE_ACTIVE);

    assert(controller.handleMessage(EscapeTopic::FINAL_PIECE_PLACED, "piece") == true);
    assert(controller.currentState() == RoomState::COLOR_BUTTON_SEQUENCE_ACTIVE);
    bool sawSmartFilm = false;
    bool sawLegacyFilm = false;
    bool sawColorEnable = false;
    while (controller.pendingCommandCount() > 0) {
        MqttCommand command = controller.takeNextPendingCommand();
        sawSmartFilm = sawSmartFilm || command.topic == EscapeTopic::REVEAL_SMART_FILM;
        sawLegacyFilm = sawLegacyFilm || command.topic == EscapeTopic::LEGACY_PDLC_ON;
        sawColorEnable = sawColorEnable || command.topic == EscapeTopic::ENABLE_COLOR_BUTTON_SEQUENCE;
    }
    assert(sawSmartFilm == true);
    assert(sawLegacyFilm == true);
    assert(sawColorEnable == true);

    assert(controller.handleMessage(EscapeTopic::COLOR_SEQUENCE_ERROR, "wrong code") == true);
    assert(wrongCodeAudio.triggerCount == 1);
    assert(wrongCodeAudio.lastPayload == "wrong code");
    assert(controller.currentState() == RoomState::COLOR_BUTTON_SEQUENCE_ACTIVE);

    assert(controller.handleMessage(EscapeTopic::COLOR_SEQUENCE_COMPLETE, "buttons") == true);
    assert(display.flashCount == 1);
    assert(display.lastMessage == "Bake at 350 Degrees");
    assert(bakeBuzzer.triggerCount == 1);
    assert(bakeBuzzer.lastPayload == "buttons");
    assert(controller.currentState() == RoomState::OVEN_KNOB_ACTIVE);
    bool sawOvenEnable = false;
    bool sawLegacyOvenEnable = false;
    while (controller.pendingCommandCount() > 0) {
        MqttCommand command = controller.takeNextPendingCommand();
        sawOvenEnable = sawOvenEnable || command.topic == EscapeTopic::ENABLE_OVEN_KNOB;
        sawLegacyOvenEnable = sawLegacyOvenEnable || command.topic == EscapeTopic::LEGACY_OVEN_ENABLE;
    }
    assert(sawOvenEnable == true);
    assert(sawLegacyOvenEnable == true);

    assert(controller.handleMessage(EscapeTopic::OVEN_POSITION_UPDATE, "370") == true);
    assert(controller.lastOvenDegrees() == 370);
    assert(controller.handleMessage(EscapeTopic::OVEN_POSITION_UPDATE, "-1") == true);
    assert(controller.lastOvenDegrees() == 0);
    assert(controller.handleMessage(EscapeTopic::OVEN_POSITION_UPDATE, "720") == true);
    assert(controller.lastOvenDegrees() == 500);
    assert(controller.handleMessage(EscapeTopic::OVEN_POSITION_UPDATE, "120abc") == true);
    assert(controller.lastOvenDegrees() == 500);

    assert(controller.handleMessage(EscapeTopic::OVEN_TARGET_REACHED, "350") == true);
    assert(controller.currentState() == RoomState::ELECTROMAGNETIC_LOCK_RELEASED);
    bool sawUnlock = false;
    bool sawLegacyUnlock = false;
    while (controller.pendingCommandCount() > 0) {
        MqttCommand command = controller.takeNextPendingCommand();
        sawUnlock = sawUnlock || command.topic == EscapeTopic::UNLOCK_ELECTROMAG_LOCK;
        sawLegacyUnlock = sawLegacyUnlock || command.topic == EscapeTopic::LEGACY_LOCK_TRIGGER;
    }
    assert(sawUnlock == true);
    assert(sawLegacyUnlock == true);

    assert(controller.handleMessage(EscapeTopic::ELECTROMAG_LOCK_UNLOCKED, "open") == true);
    assert(controller.currentState() == RoomState::ROOM_KEY_AVAILABLE);

    return 0;
}
