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
    Effect* bakeAttentionEffect
)
    : paintingCrashEffect(paintingCrashEffect),
      displayOutput(displayOutput),
      colorSequenceErrorEffect(colorSequenceErrorEffect),
      bakeAttentionEffect(bakeAttentionEffect) {
}

void GameController::addPuzzle(std::unique_ptr<PuzzleModule> puzzle) {
    puzzles.push_back(std::move(puzzle));
}

bool GameController::handleMessage(const std::string& topic, const std::string& payload) {
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
        transitionTo(RoomState::BOTTLE_LOCK_STAGE, "bread clue and bottle lock stage");
        transitionTo(RoomState::PADLOCK_BOX_STAGE, "bottle message opens padlocked box");
        transitionTo(RoomState::RFID_STAGE, "RFID cards recovered");
        pendingCommands.push_back({EscapeTopic::ENABLE_PAINTING_ROTATION, "on"});
        transitionTo(RoomState::PAINTING_ROTATION_ACTIVE, "painting rotation enabled");
        return false;
    }

    if (topic == EscapeTopic::PAINTING_ROTATION_COMPLETE) {
        transitionTo(RoomState::PAINTING_ROTATION_COMPLETE, topic);

        if (paintingCrashEffect != nullptr) {
            paintingCrashEffect->trigger(payload);
        } else {
            std::cout << "Painting crash audio effect not configured." << std::endl;
        }

        transitionTo(RoomState::CRASHING_PLATES_PLAYED, "crashing plates effect requested");
        transitionTo(RoomState::FINAL_PIECE_ACTIVE, "painting clue points to final piece");
        return true;
    }

    if (topic == EscapeTopic::FINAL_PIECE_PLACED) {
        transitionTo(RoomState::FINAL_PIECE_PLACED, topic);
        pendingCommands.push_back({EscapeTopic::REVEAL_SMART_FILM, "on"});
        pendingCommands.push_back({EscapeTopic::LEGACY_PDLC_ON, "on"});
        transitionTo(RoomState::SMART_FILM_REVEALED, "smart film reveal command queued");
        pendingCommands.push_back({EscapeTopic::ENABLE_COLOR_BUTTON_SEQUENCE, "on"});
        transitionTo(RoomState::COLOR_BUTTON_SEQUENCE_ACTIVE, "color button sequence enabled");
        return true;
    }

    if (topic == EscapeTopic::COLOR_SEQUENCE_COMPLETE) {
        transitionTo(RoomState::COLOR_BUTTON_SEQUENCE_COMPLETE, topic);

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

        transitionTo(RoomState::DISPLAY_BAKE_350, "display bake message requested");
        pendingCommands.push_back({EscapeTopic::ENABLE_OVEN_KNOB, "on"});
        pendingCommands.push_back({EscapeTopic::LEGACY_OVEN_ENABLE, "on"});
        transitionTo(RoomState::OVEN_KNOB_ACTIVE, "oven knob enabled");
        return true;
    }

    if (topic == EscapeTopic::COLOR_SEQUENCE_ERROR) {
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
