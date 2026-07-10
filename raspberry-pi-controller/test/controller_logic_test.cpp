#include <cassert>
#include <memory>
#include <string>
#include <vector>

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
        if (eventLog != nullptr) {
            eventLog->push_back(eventName);
        }
    }

    int triggerCount = 0;
    std::string lastPayload;
    std::vector<std::string>* eventLog = nullptr;
    std::string eventName;
};

class RecordingDisplay : public DisplayOutput {
public:
    void show_message(const std::string& text) override {
        lastMessage = text;
        ++showCount;
        recordEvent();
    }

    void flash_message(const std::string& text, int durationSec, double intervalSec) override {
        lastMessage = text;
        lastDuration = durationSec;
        lastInterval = intervalSec;
        ++flashCount;
        recordEvent();
    }

    void clear() override {
        ++clearCount;
        recordEvent();
    }

    int showCount = 0;
    int flashCount = 0;
    int clearCount = 0;
    int lastDuration = 0;
    double lastInterval = 0;
    std::string lastMessage;
    std::vector<std::string>* eventLog = nullptr;
    std::string eventName;

private:
    void recordEvent() {
        if (eventLog != nullptr) {
            eventLog->push_back(eventName);
        }
    }
};

void addActivePuzzles(GameController& controller) {
    controller.addPuzzle(std::make_unique<CopperPuzzle>());
    controller.addPuzzle(std::make_unique<PaintingRotationPuzzle>());
    controller.addPuzzle(std::make_unique<ColorButtonSequencePuzzle>());
    controller.addPuzzle(std::make_unique<ColorButtonSequenceErrorPuzzle>());
    controller.addPuzzle(std::make_unique<OvenTargetPuzzle>());
    controller.addPuzzle(std::make_unique<ElectromagUnlockedPuzzle>());
}

int main() {
    assert(RESET_TOPIC == "escape/game/reset");
    assert(RESET_BUTTON_GPIO == 23);
    assert(RESET_HOLD_MS == 5000);
    assert(resetPressReady(1000, false) == false);
    assert(resetPressReady(4999, true) == false);
    assert(resetPressReady(5000, true) == true);

    CopperPuzzle copper;
    assert(copper.topic() == std::string(EscapeTopic::COPPER_PUZZLE_COMPLETE));
    assert(copper.handle("escape/puzzle/unknown/solved", "ignored") == false);
    assert(copper.handle(EscapeTopic::COPPER_PUZZLE_COMPLETE, "manual test") == true);
    assert(copper.handle("escape/puzzle/copper/solved", "legacy test") == true);

    RecordingEffect paintingAudio;
    RecordingEffect wrongCodeAudio;
    RecordingEffect bakeBuzzer;
    RecordingEffect copperAudio;
    RecordingEffect colorSuccessFirstAudio;
    RecordingEffect colorSuccessSecondAudio;
    RecordingDisplay display;
    GameController controller(
        &paintingAudio,
        &display,
        &wrongCodeAudio,
        &bakeBuzzer,
        &copperAudio,
        &colorSuccessFirstAudio,
        &colorSuccessSecondAudio
    );
    addActivePuzzles(controller);

    assert(controller.puzzleCount() == 6);
    assert(controller.currentState() == RoomState::COPPER_PUZZLE_ACTIVE);

    controller.queueGameReadyCommands();
    bool sawReadyFilm = false;
    bool sawReadySound = false;
    bool sawReadyPicture = false;
    bool sawReadyPot = false;
    while (controller.pendingCommandCount() > 0) {
        MqttCommand command = controller.takeNextPendingCommand();
        sawReadyFilm = sawReadyFilm || (command.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND && command.payload == "film=ready");
        sawReadySound = sawReadySound || (command.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND && command.payload == "sound=ready");
        sawReadyPicture = sawReadyPicture || (command.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND && command.payload == "picture=ready");
        sawReadyPot = sawReadyPot || (command.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND && command.payload == "pot=ready");
    }
    assert(sawReadyFilm == true);
    assert(sawReadySound == true);
    assert(sawReadyPicture == true);
    assert(sawReadyPot == true);

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

    assert(controller.handleMessage(EscapeTopic::COPPER_PUZZLE_COMPLETE, "copper done") == true);
    assert(copperAudio.triggerCount == 1);
    assert(copperAudio.lastPayload == "copper done");
    assert(controller.currentState() == RoomState::COLOR_BUTTON_SEQUENCE_ACTIVE);
    bool sawCopperCubby = false;
    bool sawSmartFilmFromCopper = false;
    bool sawLegacyFilmFromCopper = false;
    bool sawColorButtonsActiveFromCopper = false;
    while (controller.pendingCommandCount() > 0) {
        MqttCommand command = controller.takeNextPendingCommand();
        sawCopperCubby = sawCopperCubby || command.topic == "escape/cubby/2/light_on";
        sawSmartFilmFromCopper = sawSmartFilmFromCopper || command.topic == EscapeTopic::REVEAL_SMART_FILM;
        sawLegacyFilmFromCopper = sawLegacyFilmFromCopper || command.topic == EscapeTopic::LEGACY_PDLC_ON;
        sawColorButtonsActiveFromCopper = sawColorButtonsActiveFromCopper ||
            (command.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND && command.payload == "buttons=active");
    }
    assert(sawCopperCubby == true);
    assert(sawSmartFilmFromCopper == true);
    assert(sawLegacyFilmFromCopper == true);
    assert(sawColorButtonsActiveFromCopper == true);

    assert(controller.handleMessage(EscapeTopic::COLOR_SEQUENCE_ERROR, "wrong code") == true);
    assert(wrongCodeAudio.triggerCount == 1);
    assert(wrongCodeAudio.lastPayload == "wrong code");
    assert(controller.currentState() == RoomState::COLOR_BUTTON_SEQUENCE_ACTIVE);

    assert(controller.handleMessage(EscapeTopic::PAINTING_ROTATION_COMPLETE, "picture") == true);
    assert(paintingAudio.triggerCount == 1);
    assert(paintingAudio.lastPayload == "picture");
    assert(controller.currentState() == RoomState::COLOR_BUTTON_SEQUENCE_ACTIVE);
    bool sawColorButtonsActiveFromPainting = false;
    while (controller.pendingCommandCount() > 0) {
        MqttCommand command = controller.takeNextPendingCommand();
        sawColorButtonsActiveFromPainting = sawColorButtonsActiveFromPainting ||
            (command.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND && command.payload == "buttons=active");
    }
    assert(sawColorButtonsActiveFromPainting == true);
    assert(controller.handleMessage(EscapeTopic::PAINTING_ROTATION_COMPLETE, "picture again") == true);
    assert(paintingAudio.triggerCount == 2);
    assert(paintingAudio.lastPayload == "picture again");
    while (controller.pendingCommandCount() > 0) {
        controller.takeNextPendingCommand();
    }

    assert(controller.handleMessage("escape/telemetry/pico4/oven", "oven_raw=2867,oven_value=350,solved=0,smart_film=0,smart_film_buzzer=0,lock=0") == true);
    MqttCommand physicalResetLed = controller.takeNextPendingCommand();
    assert(physicalResetLed.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND);
    assert(physicalResetLed.payload == "pot=physical-reset");

    std::vector<std::string> successEventOrder;
    display.eventLog = &successEventOrder;
    display.eventName = "display";
    bakeBuzzer.eventLog = &successEventOrder;
    bakeBuzzer.eventName = "bake-buzzer";
    colorSuccessFirstAudio.eventLog = &successEventOrder;
    colorSuccessFirstAudio.eventName = "success-audio-1";
    colorSuccessSecondAudio.eventLog = &successEventOrder;
    colorSuccessSecondAudio.eventName = "success-audio-2";

    assert(controller.handleMessage(EscapeTopic::COLOR_SEQUENCE_COMPLETE, "buttons") == true);
    assert(successEventOrder.size() >= 4);
    assert(successEventOrder[0] == "success-audio-1");
    assert(successEventOrder[1] == "success-audio-2");
    assert(display.flashCount == 1);
    assert(display.lastMessage == "Bake at 350 Degrees");
    assert(bakeBuzzer.triggerCount == 1);
    assert(bakeBuzzer.lastPayload == "buttons");
    assert(colorSuccessFirstAudio.triggerCount == 1);
    assert(colorSuccessFirstAudio.lastPayload == "buttons");
    assert(colorSuccessSecondAudio.triggerCount == 1);
    assert(colorSuccessSecondAudio.lastPayload == "buttons");
    assert(controller.currentState() == RoomState::OVEN_KNOB_ACTIVE);
    bool sawPotActive = false;
    while (controller.pendingCommandCount() > 0) {
        MqttCommand command = controller.takeNextPendingCommand();
        sawPotActive = sawPotActive ||
            (command.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND && command.payload == "pot=active");
    }
    assert(sawPotActive == true);

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

    assert(controller.handleMessage(EscapeTopic::FIRE_STATUS, "button") == true);
    MqttCommand fireStatusLedAll = controller.takeNextPendingCommand();
    assert(fireStatusLedAll.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND);
    assert(fireStatusLedAll.payload == "all=checking");
    MqttCommand fireStatusLedSound = controller.takeNextPendingCommand();
    assert(fireStatusLedSound.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND);
    assert(fireStatusLedSound.payload == "sound=ready");
    MqttCommand fireStatus = controller.takeNextPendingCommand();
    assert(fireStatus.topic == EscapeTopic::STATUS_REQUEST);
    assert(fireStatus.payload == "status");
    while (controller.pendingCommandCount() > 0) {
        controller.takeNextPendingCommand();
    }

    assert(controller.handleMessage(EscapeTopic::FIRE_FILM_ON, "button") == true);
    MqttCommand fireFilmOnLed = controller.takeNextPendingCommand();
    assert(fireFilmOnLed.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND);
    assert(fireFilmOnLed.payload == "film=active");
    MqttCommand fireFilmOn = controller.takeNextPendingCommand();
    assert(fireFilmOn.topic == EscapeTopic::REVEAL_SMART_FILM);
    assert(fireFilmOn.payload == "on");
    MqttCommand fireLegacyFilmOn = controller.takeNextPendingCommand();
    assert(fireLegacyFilmOn.topic == EscapeTopic::LEGACY_PDLC_ON);
    assert(fireLegacyFilmOn.payload == "on");

    assert(controller.handleMessage(EscapeTopic::FIRE_FILM_OFF, "button") == true);
    MqttCommand fireFilmOffLed = controller.takeNextPendingCommand();
    assert(fireFilmOffLed.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND);
    assert(fireFilmOffLed.payload == "film=ready");
    MqttCommand fireFilmOff = controller.takeNextPendingCommand();
    assert(fireFilmOff.topic == EscapeTopic::REVEAL_SMART_FILM);
    assert(fireFilmOff.payload == "off");
    MqttCommand fireLegacyFilmOff = controller.takeNextPendingCommand();
    assert(fireLegacyFilmOff.topic == EscapeTopic::LEGACY_PDLC_ON);
    assert(fireLegacyFilmOff.payload == "off");

    assert(controller.handleMessage(EscapeTopic::FIRE_SOUND_LOOK, "button") == true);
    MqttCommand fireLookLed = controller.takeNextPendingCommand();
    assert(fireLookLed.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND);
    assert(fireLookLed.payload == "sound=playing");
    assert(copperAudio.triggerCount == 2);
    assert(copperAudio.lastPayload == "button");

    assert(controller.handleMessage(EscapeTopic::FIRE_SOUND_CRASH, "button") == true);
    MqttCommand fireCrashLed = controller.takeNextPendingCommand();
    assert(fireCrashLed.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND);
    assert(fireCrashLed.payload == "sound=playing");
    assert(paintingAudio.triggerCount == 3);
    assert(paintingAudio.lastPayload == "button");
    assert(controller.handleMessage(EscapeTopic::FIRE_SOUND_CRASH, "button again") == true);
    controller.takeNextPendingCommand();
    assert(paintingAudio.triggerCount == 4);
    assert(paintingAudio.lastPayload == "button again");

    assert(controller.handleMessage(EscapeTopic::FIRE_SOUND_FAIL, "button") == true);
    MqttCommand fireFailLed = controller.takeNextPendingCommand();
    assert(fireFailLed.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND);
    assert(fireFailLed.payload == "sound=wrong");
    assert(wrongCodeAudio.triggerCount == 2);
    assert(wrongCodeAudio.lastPayload == "button");

    assert(controller.handleMessage(EscapeTopic::FIRE_SOUND_PASS, "button") == true);
    MqttCommand firePassLed = controller.takeNextPendingCommand();
    assert(firePassLed.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND);
    assert(firePassLed.payload == "sound=playing");
    assert(colorSuccessFirstAudio.triggerCount == 2);
    assert(colorSuccessFirstAudio.lastPayload == "button");

    assert(controller.handleMessage(EscapeTopic::FIRE_SOUND_BAKE, "button") == true);
    MqttCommand fireBakeLed = controller.takeNextPendingCommand();
    assert(fireBakeLed.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND);
    assert(fireBakeLed.payload == "sound=playing");
    assert(colorSuccessSecondAudio.triggerCount == 2);
    assert(colorSuccessSecondAudio.lastPayload == "button");

    assert(controller.handleMessage(EscapeTopic::FIRE_UNLOCK, "button") == true);
    MqttCommand fireUnlockLed = controller.takeNextPendingCommand();
    assert(fireUnlockLed.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND);
    assert(fireUnlockLed.payload == "pot=active");
    MqttCommand fireUnlock = controller.takeNextPendingCommand();
    assert(fireUnlock.topic == EscapeTopic::UNLOCK_ELECTROMAG_LOCK);
    assert(fireUnlock.payload == "on");
    MqttCommand fireLegacyUnlock = controller.takeNextPendingCommand();
    assert(fireLegacyUnlock.topic == EscapeTopic::LEGACY_LOCK_TRIGGER);
    assert(fireLegacyUnlock.payload == "on");

    assert(controller.handleMessage(EscapeTopic::FIRE_RESET_ALL, "button") == true);
    MqttCommand fireResetLed = controller.takeNextPendingCommand();
    assert(fireResetLed.topic == EscapeTopic::FIRE_PANEL_LED_COMMAND);
    assert(fireResetLed.payload == "all=checking");
    MqttCommand fireReset = controller.takeNextPendingCommand();
    assert(fireReset.topic == EscapeTopic::RESET_PUZZLE);
    assert(fireReset.payload == "reset");
    MqttCommand fireLegacyReset = controller.takeNextPendingCommand();
    assert(fireLegacyReset.topic == EscapeTopic::LEGACY_GAME_RESET);
    assert(fireLegacyReset.payload == "reset");
    MqttCommand fireResetStatus = controller.takeNextPendingCommand();
    assert(fireResetStatus.topic == EscapeTopic::STATUS_REQUEST);
    assert(fireResetStatus.payload == "status");
    while (controller.pendingCommandCount() > 0) {
        controller.takeNextPendingCommand();
    }

    return 0;
}
