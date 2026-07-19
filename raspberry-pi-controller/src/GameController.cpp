#include "GameController.h"

#include "OvenDial.h"
#include "ColorButtonSequence.h"
#include "EscapeRoomProtocol.h"
#include "PostState.h"

#include <cctype>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
inline constexpr const char* FIRE_SOUND_PLAY_ALL = "escape/fire/sound-play-all";
inline constexpr unsigned long FAIL_SAFE_TIMEOUT_MS = 2000;
inline constexpr unsigned long COMPLETED_ROOM_OVEN_RESET_DELAY_MS = 10000;
inline constexpr int OVEN_TARGET_MIN_VALUE = 340;
inline constexpr int OVEN_TARGET_MAX_VALUE = 360;
inline constexpr int OVEN_RESET_LOW_VALUE = 170;

std::string payloadValue(const std::string& payload, const std::string& key) {
    std::size_t start = 0;

    while (start <= payload.size()) {
        std::size_t end = payload.find(',', start);
        std::string part = payload.substr(start, end == std::string::npos ? std::string::npos : end - start);
        std::size_t separator = part.find('=');

        if (separator != std::string::npos && part.substr(0, separator) == key) {
            return part.substr(separator + 1);
        }

        if (end == std::string::npos) {
            break;
        }

        start = end + 1;
    }

    return "";
}

bool payloadHasValue(const std::string& payload, const std::string& key, const std::string& expectedValue) {
    return payloadValue(payload, key) == expectedValue;
}

bool payloadLongValue(const std::string& payload, const std::string& key, long& value) {
    std::string text = payloadValue(payload, key);
    if (text.empty()) {
        return false;
    }

    try {
        std::size_t parsedLength = 0;
        long parsedValue = std::stol(text, &parsedLength);

        if (parsedLength != text.size()) {
            return false;
        }

        value = parsedValue;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool isPicoColorErrorPayload(const std::string& payload) {
    return payload == "incorrect color button entry" ||
        payload == "color button attempt timed out";
}

}

GameController::GameController(
    Effect* paintingCrashEffect,
    DisplayOutput* displayOutput,
    Effect* colorSequenceErrorEffect,
    Effect* colorSequenceTryAgainEffect,
    Effect* bakeAttentionEffect,
    Effect* copperCompleteEffect,
    Effect* colorSequenceSuccessFirstEffect,
    Effect* colorSequenceSuccessSecondEffect,
    Effect* roomCueEffect,
    Effect* playAllAudioEffect,
    Effect* finalVictoryEffect
)
    : paintingCrashEffect(paintingCrashEffect),
      displayOutput(displayOutput),
      colorSequenceErrorEffect(colorSequenceErrorEffect),
      colorSequenceTryAgainEffect(colorSequenceTryAgainEffect),
      bakeAttentionEffect(bakeAttentionEffect),
      copperCompleteEffect(copperCompleteEffect),
      colorSequenceSuccessFirstEffect(colorSequenceSuccessFirstEffect),
      colorSequenceSuccessSecondEffect(colorSequenceSuccessSecondEffect),
      roomCueEffect(roomCueEffect),
      playAllAudioEffect(playAllAudioEffect),
      finalVictoryEffect(finalVictoryEffect) {
}

void GameController::addPuzzle(std::unique_ptr<PuzzleModule> puzzle) {
    puzzles.push_back(std::move(puzzle));
}

bool GameController::handleMessage(const std::string& topic, const std::string& payload, unsigned long nowMs) {
    bool observedFailSafe = observeFailSafe(topic, payload);

    if (handleFireCommand(topic, payload, nowMs)) {
        return true;
    }

    if (handleSensorTelemetry(topic, payload, nowMs)) {
        return true;
    }

    if (handlePostStateReport(topic, payload)) {
        return true;
    }

    if (handleOvenDegreesReport(topic, payload)) {
        return true;
    }

    if (handleFlowEvent(topic, payload, nowMs)) {
        return true;
    }

    if (observedFailSafe) {
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

bool GameController::handleFireCommand(const std::string& topic, const std::string& payload, unsigned long nowMs) {
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
        watchSmartFilmReveal(nowMs);
        return true;
    }

    if (topic == EscapeTopic::FIRE_FILM_OFF) {
        queueFirePanelLedCommand("film", "ready");
        pendingCommands.push_back({EscapeTopic::REVEAL_SMART_FILM, "off"});
        pendingCommands.push_back({EscapeTopic::LEGACY_PDLC_ON, "off"});
        watchSmartFilmHide(nowMs);
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

    if (topic == FIRE_SOUND_PLAY_ALL) {
        queueFirePanelLedCommand("sound", "playing");
        if (playAllAudioEffect != nullptr) {
            playAllAudioEffect->trigger(payload);
        } else {
            std::cout << "Play-all audio effect not configured." << std::endl;
        }
        return true;
    }

    if (topic == EscapeTopic::FIRE_UNLOCK) {
        queueFirePanelLedCommand("pot", "active");
        pendingCommands.push_back({EscapeTopic::UNLOCK_ELECTROMAG_LOCK, "on"});
        pendingCommands.push_back({EscapeTopic::LEGACY_LOCK_TRIGGER, "on"});
        watchLockUnlock(nowMs);
        return true;
    }

    if (topic == EscapeTopic::FIRE_RESET_ALL) {
        triggerRoomCue("room reset");
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

void GameController::processFailSafes(unsigned long nowMs) {
    auto action = failSafes.begin();

    while (action != failSafes.end()) {
        if (nowMs < action->nextCheckAtMs) {
            ++action;
            continue;
        }

        if (action->fallbackRuns < action->maxFallbackRuns) {
            std::ostringstream message;
            message << "FAIL_SAFE retry " << (action->fallbackRuns + 1)
                    << " for " << action->name
                    << " after missing " << action->expectedTopic;
            logFailSafe(message.str());

            for (const MqttCommand& fallbackCommand : action->fallbackCommands) {
                pendingCommands.push_back(fallbackCommand);
            }

            ++action->fallbackRuns;
            ++failSafeRetries;
            action->nextCheckAtMs = nowMs + action->timeoutMs;
            ++action;
            continue;
        }

        std::ostringstream message;
        message << "FAIL_SAFE failure for " << action->name
                << " after retry backup; returning to listening state";
        logFailSafe(message.str());

        if (!action->failureLedCommand.topic.empty()) {
            pendingCommands.push_back(action->failureLedCommand);
        }

        ++failSafeFailures;
        action = failSafes.erase(action);
    }
}

std::size_t GameController::activeFailSafeCount() const {
    return failSafes.size();
}

int GameController::failSafeRetryCount() const {
    return failSafeRetries;
}

int GameController::failSafeFailureCount() const {
    return failSafeFailures;
}

std::vector<std::string> GameController::failSafeLog() const {
    return failSafeMessages;
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
    // Clear transient flags and the state machine so a room reset re-arms the
    // full flow for the next group.
    state = RoomState::COPPER_PUZZLE_ACTIVE;
    colorSequenceErrorCount = 0;
    paintingTelemetryActive = false;
    roomCompletedAtMs = 0;
    lastColorErrorTelemetryCount = -1;
    pendingExplicitColorErrorTelemetryAcks = 0;
    failSafes.clear();
    solvedTopics.clear();
}

void GameController::triggerRoomCue(const std::string& payload) {
    if (roomCueEffect != nullptr) {
        roomCueEffect->trigger(payload);
    }
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

bool GameController::handleSensorTelemetry(const std::string& topic, const std::string& payload, unsigned long nowMs) {
    if (topic.rfind("escape/telemetry/", 0) != 0) {
        return false;
    }

    if (topic == "escape/telemetry/pico2/contacts") {
        if (
            payloadHasValue(payload, "copper", "0") &&
            solvedTopics.find(EscapeTopic::COPPER_PUZZLE_COMPLETE) == solvedTopics.end()
        ) {
            logFailSafe("FAIL_SAFE telemetry fallback: copper puzzle contact is active but completion event was not seen");
            handleFlowEvent(EscapeTopic::COPPER_PUZZLE_COMPLETE, "fail-safe telemetry fallback", nowMs);
            markPuzzleSolved(EscapeTopic::COPPER_PUZZLE_COMPLETE);
        }

        return true;
    }

    if (topic == "escape/telemetry/pico3/painting_sensor") {
        bool magnetPresent = payloadHasValue(payload, "magnet_present", "1") ||
            payloadHasValue(payload, "painting_sensor", "0");

        if (!magnetPresent) {
            paintingTelemetryActive = false;
            return true;
        }

        if (!paintingTelemetryActive) {
            logFailSafe("FAIL_SAFE telemetry fallback: painting magnet is active but rotation event was not seen");
            handleFlowEvent(EscapeTopic::PAINTING_ROTATION_COMPLETE, "fail-safe telemetry fallback", nowMs);
            markPuzzleSolved(EscapeTopic::PAINTING_ROTATION_COMPLETE);
        }

        paintingTelemetryActive = true;
        return true;
    }

    if (topic == "escape/telemetry/pico5/buttons") {
        long telemetryErrorCount = 0;
        if (payloadLongValue(payload, "error_count", telemetryErrorCount)) {
            if (lastColorErrorTelemetryCount < 0 || telemetryErrorCount < lastColorErrorTelemetryCount) {
                lastColorErrorTelemetryCount = telemetryErrorCount;
                pendingExplicitColorErrorTelemetryAcks = 0;
            } else if (telemetryErrorCount > lastColorErrorTelemetryCount) {
                long errorDelta = telemetryErrorCount - lastColorErrorTelemetryCount;
                long explicitAckCount = std::min<long>(errorDelta, pendingExplicitColorErrorTelemetryAcks);
                pendingExplicitColorErrorTelemetryAcks -= static_cast<int>(explicitAckCount);

                long missedErrorCount = errorDelta - explicitAckCount;
                if (missedErrorCount > 0) {
                    std::ostringstream message;
                    message << "FAIL_SAFE telemetry fallback: recovered " << missedErrorCount
                            << " missed color-button error event";
                    if (missedErrorCount != 1) {
                        message << "s";
                    }
                    logFailSafe(message.str());

                    for (long i = 0; i < missedErrorCount; ++i) {
                        triggerColorSequenceErrorCue("fail-safe telemetry fallback");
                    }
                }

                lastColorErrorTelemetryCount = telemetryErrorCount;
            }
        }

        if (
            payloadHasValue(payload, "solved", "1") &&
            currentState() != RoomState::OVEN_KNOB_ACTIVE &&
            currentState() != RoomState::OVEN_TARGET_REACHED &&
            currentState() != RoomState::ELECTROMAGNETIC_LOCK_RELEASED &&
            currentState() != RoomState::ROOM_KEY_AVAILABLE
        ) {
            logFailSafe("FAIL_SAFE telemetry fallback: color sequence is solved but completion event was not seen");
            handleFlowEvent(EscapeTopic::COLOR_SEQUENCE_COMPLETE, "fail-safe telemetry fallback", nowMs);
            markPuzzleSolved(EscapeTopic::COLOR_SEQUENCE_COMPLETE);
        }

        return true;
    }

    if (topic != "escape/telemetry/pico4/oven") {
        return true;
    }

    long telemetryOvenValue = 0;
    if (!payloadLongValue(payload, "oven_value", telemetryOvenValue)) {
        return true;
    }

    bool atTarget = telemetryOvenValue >= OVEN_TARGET_MIN_VALUE && telemetryOvenValue <= OVEN_TARGET_MAX_VALUE;
    bool atResetLow = telemetryOvenValue <= OVEN_RESET_LOW_VALUE;

    if (
        currentState() == RoomState::ROOM_KEY_AVAILABLE &&
        roomCompletedAtMs != 0 &&
        nowMs >= roomCompletedAtMs + COMPLETED_ROOM_OVEN_RESET_DELAY_MS &&
        atResetLow
    ) {
        resetRoomFromCompletedOvenDial(nowMs);
        return true;
    }

    if (
        currentState() == RoomState::ELECTROMAGNETIC_LOCK_RELEASED &&
        payloadHasValue(payload, "lock", "1")
    ) {
        handleFlowEvent(EscapeTopic::ELECTROMAG_LOCK_UNLOCKED, "fail-safe telemetry verified lock", nowMs);
        return true;
    }

    if (currentState() == RoomState::OVEN_KNOB_ACTIVE && atTarget) {
        logFailSafe("FAIL_SAFE telemetry fallback: oven is at target but target-reached event was not seen");
        handleFlowEvent(EscapeTopic::OVEN_TARGET_REACHED, "fail-safe telemetry fallback", nowMs);
        markPuzzleSolved(EscapeTopic::OVEN_TARGET_REACHED);
        return true;
    }

    return true;
}

bool GameController::handleFlowEvent(const std::string& topic, const std::string& payload, unsigned long nowMs) {
    if (topic == EscapeTopic::CUBBY_APPROACH_DETECTED || topic == "escape/puzzle/stairs/triggered") {
        transitionTo(RoomState::CUBBY_APPROACH_DETECTED, topic);
        pendingCommands.push_back({EscapeTopic::ENABLE_CUBBY_LIGHT, "1"});
        pendingCommands.push_back({EscapeTopic::LEGACY_CUBBY_1_LIGHT_ON, "on"});
        transitionTo(RoomState::FIRST_CUBBY_LIT, "first cubby light command queued");
        transitionTo(RoomState::COPPER_PUZZLE_ACTIVE, "copper puzzle already listening");
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
        watchSmartFilmReveal(nowMs);
        queueFirePanelLedCommand("film", "active");
        transitionTo(RoomState::SMART_FILM_REVEALED, "smart film reveal command queued");
        queueFirePanelLedCommand("buttons", "active");
        transitionTo(RoomState::COLOR_BUTTON_SEQUENCE_ACTIVE, "color buttons already listening");
        return false;
    }

    if (topic == EscapeTopic::PAINTING_ROTATION_COMPLETE) {
        paintingTelemetryActive = true;
        transitionTo(RoomState::PAINTING_ROTATION_COMPLETE, topic);
        queueFirePanelLedCommand("picture", "triggered");

        if (paintingCrashEffect != nullptr) {
            paintingCrashEffect->trigger(payload);
        } else {
            std::cout << "Painting crash audio effect not configured." << std::endl;
        }

        transitionTo(RoomState::CRASHING_PLATES_PLAYED, "crashing plates effect requested");
        queueFirePanelLedCommand("buttons", "active");
        transitionTo(RoomState::COLOR_BUTTON_SEQUENCE_ACTIVE, "color buttons already listening");
        return true;
    }

    if (topic == EscapeTopic::COLOR_SEQUENCE_COMPLETE) {
        transitionTo(RoomState::COLOR_BUTTON_SEQUENCE_COMPLETE, topic);
        queueFirePanelLedCommand("buttons", "ready");

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

        if (bakeAttentionEffect != nullptr) {
            bakeAttentionEffect->trigger(payload);
        } else {
            std::cout << "Bake attention buzzer not configured." << std::endl;
        }

        if (displayOutput != nullptr) {
            displayOutput->flash_message("Bake at 350 Degrees", 6, 0.5);
        } else {
            std::cout << "Display output not configured. Message: Bake at 350 Degrees" << std::endl;
        }

        transitionTo(RoomState::DISPLAY_BAKE_350, "display bake message requested");
        pendingCommands.push_back({EscapeTopic::ARM_OVEN_POTENTIOMETER, "on"});
        watchOvenArm(nowMs);
        queueFirePanelLedCommand("pot", "active");
        transitionTo(RoomState::OVEN_KNOB_ACTIVE, "oven knob armed");
        return true;
    }

    if (topic == EscapeTopic::COLOR_SEQUENCE_ERROR) {
        if (isPicoColorErrorPayload(payload)) {
            ++pendingExplicitColorErrorTelemetryAcks;
        }
        triggerColorSequenceErrorCue(payload);
        return true;
    }

    if (topic == EscapeTopic::OVEN_TARGET_REACHED || topic == "escape/puzzle/oven/solved") {
        if (currentState() != RoomState::OVEN_KNOB_ACTIVE) {
            logFailSafe("FAIL_SAFE ignored oven target while oven potentiometer is inactive");
            return true;
        }

        transitionTo(RoomState::OVEN_TARGET_REACHED, topic);
        pendingCommands.push_back({EscapeTopic::UNLOCK_ELECTROMAG_LOCK, "on"});
        pendingCommands.push_back({EscapeTopic::LEGACY_LOCK_TRIGGER, "on"});
        watchLockUnlock(nowMs);
        queueFirePanelLedCommand("pot", "ready");
        transitionTo(RoomState::ELECTROMAGNETIC_LOCK_RELEASED, "oven target reached");
        return true;
    }

    if (topic == EscapeTopic::ELECTROMAG_LOCK_UNLOCKED) {
        if (currentState() != RoomState::ROOM_KEY_AVAILABLE) {
            roomCompletedAtMs = nowMs;
            if (finalVictoryEffect != nullptr) {
                finalVictoryEffect->trigger(payload);
            } else {
                std::cout << "Final victory audio effect not configured." << std::endl;
            }
        }
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

void GameController::triggerColorSequenceErrorCue(const std::string& payload) {
    queueFirePanelLedCommand("buttons", "wrong");
    ++colorSequenceErrorCount;

    Effect* selectedErrorEffect = colorSequenceErrorEffect;
    if (colorFailureUsesTryAgainCue(colorSequenceErrorCount) && colorSequenceTryAgainEffect != nullptr) {
        selectedErrorEffect = colorSequenceTryAgainEffect;
    }

    if (selectedErrorEffect != nullptr) {
        selectedErrorEffect->trigger(payload);
    } else {
        std::cout << "Color sequence error audio effect not configured." << std::endl;
    }
}

void GameController::resetRoomFromCompletedOvenDial(unsigned long nowMs) {
    (void)nowMs;
    logFailSafe("FAIL_SAFE completed-room reset: oven dial returned to 170 after completion threshold");
    triggerRoomCue("room reset");
    resetGameProgress();
    queueFirePanelLedCommand("all", "checking");
    pendingCommands.push_back({EscapeTopic::RESET_PUZZLE, "reset"});
    pendingCommands.push_back({EscapeTopic::LEGACY_GAME_RESET, "reset"});
    pendingCommands.push_back({EscapeTopic::STATUS_REQUEST, "status"});
    queueGameReadyCommands();
}

void GameController::watchSmartFilmReveal(unsigned long nowMs) {
    failSafes.erase(
        std::remove_if(failSafes.begin(), failSafes.end(), [](const FailSafeAction& action) {
            return action.name == "smart film reveal";
        }),
        failSafes.end()
    );

    failSafes.push_back({
        "smart film reveal",
        EscapeTopic::SMART_FILM_READY,
        "transparent",
        {
            {EscapeTopic::REVEAL_SMART_FILM, "on"},
            {EscapeTopic::LEGACY_PDLC_ON, "on"},
        },
        {EscapeTopic::FIRE_PANEL_LED_COMMAND, "film=error"},
        nowMs + FAIL_SAFE_TIMEOUT_MS,
        FAIL_SAFE_TIMEOUT_MS,
        0,
        1,
    });
}

void GameController::watchSmartFilmHide(unsigned long nowMs) {
    failSafes.erase(
        std::remove_if(failSafes.begin(), failSafes.end(), [](const FailSafeAction& action) {
            return action.name == "smart film hide";
        }),
        failSafes.end()
    );

    failSafes.push_back({
        "smart film hide",
        EscapeTopic::SMART_FILM_READY,
        "opaque",
        {
            {EscapeTopic::REVEAL_SMART_FILM, "off"},
            {EscapeTopic::LEGACY_PDLC_ON, "off"},
        },
        {EscapeTopic::FIRE_PANEL_LED_COMMAND, "film=error"},
        nowMs + FAIL_SAFE_TIMEOUT_MS,
        FAIL_SAFE_TIMEOUT_MS,
        0,
        1,
    });
}

void GameController::watchOvenArm(unsigned long nowMs) {
    failSafes.erase(
        std::remove_if(failSafes.begin(), failSafes.end(), [](const FailSafeAction& action) {
            return action.name == "oven potentiometer arm";
        }),
        failSafes.end()
    );

    failSafes.push_back({
        "oven potentiometer arm",
        "escape/telemetry/pico4/oven",
        "oven_armed=1",
        {
            {EscapeTopic::ARM_OVEN_POTENTIOMETER, "on"},
        },
        {EscapeTopic::FIRE_PANEL_LED_COMMAND, "pot=error"},
        nowMs + FAIL_SAFE_TIMEOUT_MS,
        FAIL_SAFE_TIMEOUT_MS,
        0,
        1,
    });
}

void GameController::watchLockUnlock(unsigned long nowMs) {
    failSafes.erase(
        std::remove_if(failSafes.begin(), failSafes.end(), [](const FailSafeAction& action) {
            return action.name == "lock unlock";
        }),
        failSafes.end()
    );

    failSafes.push_back({
        "lock unlock",
        EscapeTopic::ELECTROMAG_LOCK_UNLOCKED,
        "",
        {
            {EscapeTopic::UNLOCK_ELECTROMAG_LOCK, "on"},
            {EscapeTopic::LEGACY_LOCK_TRIGGER, "on"},
        },
        {EscapeTopic::FIRE_PANEL_LED_COMMAND, "pot=error"},
        nowMs + FAIL_SAFE_TIMEOUT_MS,
        FAIL_SAFE_TIMEOUT_MS,
        0,
        1,
    });
}

bool GameController::observeFailSafe(const std::string& topic, const std::string& payload) {
    bool observed = false;
    auto action = failSafes.begin();

    while (action != failSafes.end()) {
        bool matched = topic == action->expectedTopic &&
            (action->expectedPayloadFragment.empty() || payload.find(action->expectedPayloadFragment) != std::string::npos);

        if (!matched && topic == "escape/telemetry/pico4/oven") {
            matched =
                (action->name == "smart film reveal" && payloadHasValue(payload, "smart_film", "1")) ||
                (action->name == "smart film hide" && payloadHasValue(payload, "smart_film", "0")) ||
                (action->name == "lock unlock" && payloadHasValue(payload, "lock", "1"));
        }

        if (matched) {
            logFailSafe("FAIL_SAFE verified " + action->name + " from " + topic);
            action = failSafes.erase(action);
            observed = true;
        } else {
            ++action;
        }
    }

    return observed;
}

void GameController::logFailSafe(const std::string& message) {
    std::cout << message << std::endl;
    failSafeMessages.push_back(message);

    if (failSafeMessages.size() > 50) {
        failSafeMessages.erase(failSafeMessages.begin());
    }
}

void GameController::queueCommandsForTopic(const std::string& topic) {
    struct Route {
        const char* puzzleTopic;
        const char* commandTopic;
    };

    static const Route routes[] = {
        {EscapeTopic::COPPER_PUZZLE_COMPLETE, "escape/cubby/2/light_on"},
        {EscapeTopic::PAINTING_ROTATION_COMPLETE, "escape/cubby/3/light_on"},
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
