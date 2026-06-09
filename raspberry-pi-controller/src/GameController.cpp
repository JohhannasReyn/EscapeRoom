#include "GameController.h"

#include "OvenDial.h"
#include "EscapeRoomProtocol.h"
#include "PostState.h"

#include <cctype>
#include <iostream>
#include <stdexcept>
#include <utility>

GameController::GameController(
    Effect* paintingCrashEffect,
    DisplayOutput* displayOutput,
    Effect* colorSequenceErrorEffect,
    Effect* bakeAttentionEffect,
    Effect* copperCompleteEffect,
    Effect* colorSequenceSuccessFirstEffect,
    Effect* colorSequenceSuccessSecondEffect
)
    : paintingCrashEffect(paintingCrashEffect),
      displayOutput(displayOutput),
      colorSequenceErrorEffect(colorSequenceErrorEffect),
      bakeAttentionEffect(bakeAttentionEffect),
      copperCompleteEffect(copperCompleteEffect),
      colorSequenceSuccessFirstEffect(colorSequenceSuccessFirstEffect),
      colorSequenceSuccessSecondEffect(colorSequenceSuccessSecondEffect) {
}

void GameController::addPuzzle(std::unique_ptr<PuzzleModule> puzzle) {
    puzzles.push_back(std::move(puzzle));
}

bool GameController::handleMessage(const std::string& topic, const std::string& payload) {
    if (handleFireCommand(topic, payload)) {
        return true;
    }

    if (handleSensorTelemetry(topic, payload)) {
        return true;
    }

    if (handlePostStateReport(topic, payload)) {
        return true;
    }

    if (handleOvenDegreesReport(topic, payload)) {
        return true;
    }

    if (handleFlowEvent(topic, payload)) {
        return true;
    }

    for (const auto& puzzle : puzzles) {
        if (puzzle->handle(topic, payload)) {
            markPuzzleSolved(topic);
            queueCommandsForTopic(topic);
            return true;
        }
    }

    std::cout << "No puzzle handler registered for topic: " << topic << std::endl;
    return false;
}

bool GameController::handleFireCommand(const std::string& topic, const std::string& payload) {
    if (topic == EscapeTopic::FIRE_STATUS) {
        queueFirePanelLedCommand("all", "checking");
        queueFirePanelLedCommand("sound", "ready");
        pendingCommands.push_back({EscapeTopic::STATUS_REQUEST, "status"});
        queueGameReadyCommands();
        return true;
    }

    if (topic == EscapeTopic::FIRE_FILM_ON) {
        queueFirePanelLedCommand("film", "active");
        pendingCommands.push_back({EscapeTopic::REVEAL_SMART_FILM, "on"});
        pendingCommands.push_back({EscapeTopic::LEGACY_PDLC_ON, "on"});
        return true;
    }

    if (topic == EscapeTopic::FIRE_FILM_OFF) {
        queueFirePanelLedCommand("film", "ready");
        pendingCommands.push_back({EscapeTopic::REVEAL_SMART_FILM, "off"});
        pendingCommands.push_back({EscapeTopic::LEGACY_PDLC_ON, "off"});
        return true;
    }

    if (topic == EscapeTopic::FIRE_SOUND_LOOK) {
        queueFirePanelLedCommand("sound", "playing");
        if (copperCompleteEffect != nullptr) {
            copperCompleteEffect->trigger(payload);
        }
        return true;
    }

    if (topic == EscapeTopic::FIRE_SOUND_CRASH) {
        queueFirePanelLedCommand("sound", "playing");
        if (paintingCrashEffect != nullptr) {
            paintingCrashEffect->trigger(payload);
        }
        return true;
    }

    if (topic == EscapeTopic::FIRE_SOUND_FAIL) {
        queueFirePanelLedCommand("sound", "wrong");
        if (colorSequenceErrorEffect != nullptr) {
            colorSequenceErrorEffect->trigger(payload);
        }
        return true;
    }

    if (topic == EscapeTopic::FIRE_SOUND_PASS) {
        queueFirePanelLedCommand("sound", "playing");
        if (colorSequenceSuccessFirstEffect != nullptr) {
            colorSequenceSuccessFirstEffect->trigger(payload);
        }
        return true;
    }

    if (topic == EscapeTopic::FIRE_SOUND_BAKE) {
        queueFirePanelLedCommand("sound", "playing");
        if (colorSequenceSuccessSecondEffect != nullptr) {
            colorSequenceSuccessSecondEffect->trigger(payload);
        }
        return true;
    }

    if (topic == EscapeTopic::FIRE_UNLOCK) {
        queueFirePanelLedCommand("pot", "active");
        pendingCommands.push_back({EscapeTopic::UNLOCK_ELECTROMAG_LOCK, "on"});
        pendingCommands.push_back({EscapeTopic::LEGACY_LOCK_TRIGGER, "on"});
        return true;
    }

    if (topic == EscapeTopic::FIRE_RESET_ALL) {
        resetGameProgress();
        queueFirePanelLedCommand("all", "checking");
        pendingCommands.push_back({EscapeTopic::RESET_PUZZLE, "reset"});
        pendingCommands.push_back({EscapeTopic::LEGACY_GAME_RESET, "reset"});
        pendingCommands.push_back({EscapeTopic::STATUS_REQUEST, "status"});
        queueGameReadyCommands();
        return true;
    }

    return false;
}

std::vector<std::string> GameController::topics() const {
    std::vector<std::string> registeredTopics;

    for (const auto& puzzle : puzzles) {
        registeredTopics.push_back(puzzle->topic());
    }

    return registeredTopics;
}

std::size_t GameController::puzzleCount() const {
    return puzzles.size();
}

std::size_t GameController::pendingCommandCount() const {
    return pendingCommands.size();
}

MqttCommand GameController::takeNextPendingCommand() {
    if (pendingCommands.empty()) {
        return {"", ""};
    }

    MqttCommand command = pendingCommands.front();
    pendingCommands.pop_front();
    return command;
}

int GameController::lastOvenDegrees() const {
    return ovenDegrees;
}

RoomState GameController::currentState() const {
    return state;
}

void GameController::queueGameReadyCommands() {
    queueFirePanelLedCommand("film", "ready");
    queueFirePanelLedCommand("sound", "ready");
    queueFirePanelLedCommand("picture", "ready");
    queueFirePanelLedCommand("buttons", "ready");
    queueFirePanelLedCommand("pot", "ready");
    pendingCommands.push_back({EscapeTopic::ENABLE_COPPER_PUZZLE, "on"});
    pendingCommands.push_back({EscapeTopic::ENABLE_PAINTING_ROTATION, "on"});
    pendingCommands.push_back({EscapeTopic::ENABLE_COLOR_BUTTON_SEQUENCE, "on"});
}

void GameController::queuePostQueryCommand() {
    resetPostState();
    pendingCommands.push_back({"escape/post/query", "status"});
}

void GameController::queueReadyCommand() {
    pendingCommands.push_back({"escape/cubby/all/status", "off"});
}

void GameController::resetPostState() {
    postReady.fill(false);
}

void GameController::resetGameProgress() {
    // Clear the one-shot flags and the state machine so a room reset re-arms the
    // full flow for the next group. Without this the painting crash sound (a
    // once-per-game cue) would never replay after the first playthrough.
    state = RoomState::COPPER_PUZZLE_ACTIVE;
    paintingRotationHandled = false;
    ovenPhysicalResetSignaled = false;
    solvedTopics.clear();
}

bool GameController::handlePostStateReport(const std::string& topic, const std::string& payload) {
    const std::string prefix = "escape/post/cubby/";
    const std::string suffix = "/state";

    if (topic.rfind(prefix, 0) != 0 || topic.size() <= prefix.size() + suffix.size()) {
        return false;
    }

    if (topic.compare(topic.size() - suffix.size(), suffix.size(), suffix) != 0) {
        return false;
    }

    std::string cubbyText = topic.substr(prefix.size(), topic.size() - prefix.size() - suffix.size());

    try {
        int cubbyNumber = std::stoi(cubbyText);

        if (cubbyNumber < 1 || cubbyNumber > 5) {
            std::cout << "Ignoring POST report for cubby " << cubbyNumber << "." << std::endl;
            return true;
        }

        if (payload == "completed") {
            postReady[cubbyNumber - 1] = false;
            pendingCommands.push_back({cubbyStatusTopic(cubbyNumber), "red"});
        } else if (payload == "ready") {
            postReady[cubbyNumber - 1] = true;
            pendingCommands.push_back({cubbyStatusTopic(cubbyNumber), "green"});

            bool allReady = true;
            for (bool ready : postReady) {
                allReady = allReady && ready;
            }

            if (allReady) {
                queueReadyCommand();
            }
        } else {
            std::cout << "Unknown POST payload for cubby " << cubbyNumber << ": " << payload << std::endl;
        }
    } catch (const std::exception&) {
        std::cout << "Ignoring malformed POST topic: " << topic << std::endl;
    }

    return true;
}

bool GameController::handleSensorTelemetry(const std::string& topic, const std::string& payload) {
    if (topic != "escape/telemetry/pico4/oven") {
        return false;
    }

    const std::string key = "oven_value=";
    std::size_t start = payload.find(key);
    if (start == std::string::npos) {
        return true;
    }

    start += key.size();
    std::size_t end = payload.find(',', start);
    std::string valueText = payload.substr(start, end == std::string::npos ? std::string::npos : end - start);

    try {
        int telemetryOvenValue = std::stoi(valueText);
        bool atTarget = telemetryOvenValue >= 340 && telemetryOvenValue <= 360;

        if (currentState() != RoomState::OVEN_KNOB_ACTIVE && atTarget && !ovenPhysicalResetSignaled) {
            queueFirePanelLedCommand("pot", "physical-reset");
            ovenPhysicalResetSignaled = true;
        } else if (!atTarget && ovenPhysicalResetSignaled && currentState() != RoomState::OVEN_KNOB_ACTIVE) {
            queueFirePanelLedCommand("pot", "ready");
            ovenPhysicalResetSignaled = false;
        }
    } catch (const std::exception&) {
    }

    return true;
}

bool GameController::handleFlowEvent(const std::string& topic, const std::string& payload) {
    if (topic == EscapeTopic::CUBBY_APPROACH_DETECTED || topic == "escape/puzzle/stairs/triggered") {
        transitionTo(RoomState::CUBBY_APPROACH_DETECTED, topic);
        pendingCommands.push_back({EscapeTopic::ENABLE_CUBBY_LIGHT, "1"});
        pendingCommands.push_back({EscapeTopic::LEGACY_CUBBY_1_LIGHT_ON, "on"});
        transitionTo(RoomState::FIRST_CUBBY_LIT, "first cubby light command queued");
        pendingCommands.push_back({EscapeTopic::ENABLE_COPPER_PUZZLE, "on"});
        transitionTo(RoomState::COPPER_PUZZLE_ACTIVE, "copper puzzle enabled");
        return true;
    }

    if (topic == EscapeTopic::COPPER_PUZZLE_COMPLETE || topic == "escape/puzzle/copper/solved") {
        transitionTo(RoomState::COPPER_PUZZLE_COMPLETE, topic);

        if (copperCompleteEffect != nullptr) {
            copperCompleteEffect->trigger(payload);
        } else {
            std::cout << "Copper completion audio effect not configured." << std::endl;
        }

        pendingCommands.push_back({EscapeTopic::REVEAL_SMART_FILM, "on"});
        pendingCommands.push_back({EscapeTopic::LEGACY_PDLC_ON, "on"});
        queueFirePanelLedCommand("film", "active");
        transitionTo(RoomState::SMART_FILM_REVEALED, "smart film reveal command queued");
        pendingCommands.push_back({EscapeTopic::ENABLE_COLOR_BUTTON_SEQUENCE, "on"});
        queueFirePanelLedCommand("buttons", "active");
        transitionTo(RoomState::COLOR_BUTTON_SEQUENCE_ACTIVE, "color button sequence enabled");
        return false;
    }

    if (topic == EscapeTopic::PAINTING_ROTATION_COMPLETE) {
        if (paintingRotationHandled) {
            return true;
        }

        transitionTo(RoomState::PAINTING_ROTATION_COMPLETE, topic);
        queueFirePanelLedCommand("picture", "triggered");

        if (paintingCrashEffect != nullptr) {
            paintingCrashEffect->trigger(payload);
        } else {
            std::cout << "Painting crash audio effect not configured." << std::endl;
        }

        paintingRotationHandled = true;

        transitionTo(RoomState::CRASHING_PLATES_PLAYED, "crashing plates effect requested");
        transitionTo(RoomState::FINAL_PIECE_ACTIVE, "painting clue points to final piece");
        return true;
    }

    if (topic == EscapeTopic::FINAL_PIECE_PLACED) {
        transitionTo(RoomState::FINAL_PIECE_PLACED, topic);
        pendingCommands.push_back({EscapeTopic::REVEAL_SMART_FILM, "on"});
        pendingCommands.push_back({EscapeTopic::LEGACY_PDLC_ON, "on"});
        queueFirePanelLedCommand("film", "active");
        transitionTo(RoomState::SMART_FILM_REVEALED, "smart film reveal command queued");
        pendingCommands.push_back({EscapeTopic::ENABLE_COLOR_BUTTON_SEQUENCE, "on"});
        queueFirePanelLedCommand("buttons", "active");
        transitionTo(RoomState::COLOR_BUTTON_SEQUENCE_ACTIVE, "color button sequence enabled");
        return true;
    }

    if (topic == EscapeTopic::COLOR_SEQUENCE_COMPLETE) {
        transitionTo(RoomState::COLOR_BUTTON_SEQUENCE_COMPLETE, topic);
        queueFirePanelLedCommand("buttons", "ready");

        if (displayOutput != nullptr) {
            displayOutput->flash_message("Bake at 350 Degrees", 6, 0.5);
        } else {
            std::cout << "Display output not configured. Message: Bake at 350 Degrees" << std::endl;
        }

        if (bakeAttentionEffect != nullptr) {
            bakeAttentionEffect->trigger(payload);
        } else {
            std::cout << "Bake attention buzzer not configured." << std::endl;
        }

        if (colorSequenceSuccessFirstEffect != nullptr) {
            colorSequenceSuccessFirstEffect->trigger(payload);
        } else {
            std::cout << "Color success first audio effect not configured." << std::endl;
        }

        if (colorSequenceSuccessSecondEffect != nullptr) {
            colorSequenceSuccessSecondEffect->trigger(payload);
        } else {
            std::cout << "Color success second audio effect not configured." << std::endl;
        }

        transitionTo(RoomState::DISPLAY_BAKE_350, "display bake message requested");
        pendingCommands.push_back({EscapeTopic::ENABLE_OVEN_KNOB, "on"});
        pendingCommands.push_back({EscapeTopic::LEGACY_OVEN_ENABLE, "on"});
        queueFirePanelLedCommand("pot", "active");
        transitionTo(RoomState::OVEN_KNOB_ACTIVE, "oven knob enabled");
        return true;
    }

    if (topic == EscapeTopic::COLOR_SEQUENCE_ERROR) {
        queueFirePanelLedCommand("buttons", "wrong");
        if (colorSequenceErrorEffect != nullptr) {
            colorSequenceErrorEffect->trigger(payload);
        } else {
            std::cout << "Color sequence error audio effect not configured." << std::endl;
        }

        return true;
    }

    if (topic == EscapeTopic::OVEN_TARGET_REACHED || topic == "escape/puzzle/oven/solved") {
        transitionTo(RoomState::OVEN_TARGET_REACHED, topic);
        pendingCommands.push_back({EscapeTopic::UNLOCK_ELECTROMAG_LOCK, "on"});
        pendingCommands.push_back({EscapeTopic::LEGACY_LOCK_TRIGGER, "on"});
        queueFirePanelLedCommand("pot", "ready");
        transitionTo(RoomState::ELECTROMAGNETIC_LOCK_RELEASED, "oven target reached");
        return true;
    }

    if (topic == EscapeTopic::ELECTROMAG_LOCK_UNLOCKED) {
        transitionTo(RoomState::ROOM_KEY_AVAILABLE, topic);
        return true;
    }

    return false;
}

bool GameController::handleOvenDegreesReport(const std::string& topic, const std::string& payload) {
    if (topic != EscapeTopic::OVEN_POSITION_UPDATE && topic != EscapeTopic::LEGACY_OVEN_DEGREES) {
        return false;
    }

    try {
        std::size_t parsedLength = 0;
        int parsedDegrees = std::stoi(payload, &parsedLength);

        while (parsedLength < payload.size() && std::isspace(static_cast<unsigned char>(payload[parsedLength]))) {
            ++parsedLength;
        }

        if (parsedLength != payload.size()) {
            throw std::invalid_argument("trailing characters");
        }

        ovenDegrees = clampInt(parsedDegrees, 0, 500);
        std::cout << "Oven dial value: " << ovenDegrees << std::endl;
    } catch (const std::exception&) {
        std::cout << "Ignoring malformed oven dial payload: " << payload << std::endl;
    }

    return true;
}

void GameController::queueFirePanelLedCommand(const std::string& zone, const std::string& mode) {
    pendingCommands.push_back({EscapeTopic::FIRE_PANEL_LED_COMMAND, zone + "=" + mode});
}

void GameController::queueCommandsForTopic(const std::string& topic) {
    struct Route {
        const char* puzzleTopic;
        const char* commandTopic;
    };

    static const Route routes[] = {
        {EscapeTopic::COPPER_PUZZLE_COMPLETE, "escape/cubby/2/light_on"},
        {EscapeTopic::PAINTING_ROTATION_COMPLETE, "escape/cubby/3/light_on"},
        {EscapeTopic::FINAL_PIECE_PLACED, "escape/cubby/4/light_on"},
        {EscapeTopic::COLOR_SEQUENCE_COMPLETE, "escape/cubby/5/light_on"},
    };

    for (const Route& route : routes) {
        if (topic == route.puzzleTopic) {
            pendingCommands.push_back({route.commandTopic, "on"});
            return;
        }
    }
}

void GameController::markPuzzleSolved(const std::string& topic) {
    solvedTopics.insert(topic);
}

void GameController::transitionTo(RoomState nextState, const std::string& reason) {
    if (state == nextState) {
        return;
    }

    std::cout << "STATE " << roomStateName(state) << " -> " << roomStateName(nextState);
    if (!reason.empty()) {
        std::cout << " (" << reason << ")";
    }
    std::cout << std::endl;
    state = nextState;
}
