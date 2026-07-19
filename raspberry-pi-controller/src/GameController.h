#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "PuzzleModule.h"
#include "effects/DisplayOutput.h"
#include "effects/Effect.h"

#include "RoomState.h"

#include <array>
#include <deque>
#include <memory>
#include <set>
#include <string>
#include <vector>

struct MqttCommand {
    std::string topic;
    std::string payload;
};

class GameController {
public:
    GameController(
        Effect* paintingCrashEffect = nullptr,
        DisplayOutput* displayOutput = nullptr,
        Effect* colorSequenceErrorEffect = nullptr,
        Effect* colorSequenceTryAgainEffect = nullptr,
        Effect* bakeAttentionEffect = nullptr,
        Effect* copperCompleteEffect = nullptr,
        Effect* colorSequenceSuccessFirstEffect = nullptr,
        Effect* colorSequenceSuccessSecondEffect = nullptr,
        Effect* roomCueEffect = nullptr,
        Effect* playAllAudioEffect = nullptr,
        Effect* finalVictoryEffect = nullptr
    );

    void addPuzzle(std::unique_ptr<PuzzleModule> puzzle);
    bool handleMessage(const std::string& topic, const std::string& payload, unsigned long nowMs = 0);
    std::vector<std::string> topics() const;
    std::size_t puzzleCount() const;
    std::size_t pendingCommandCount() const;
    MqttCommand takeNextPendingCommand();
    void processFailSafes(unsigned long nowMs);
    std::size_t activeFailSafeCount() const;
    int failSafeRetryCount() const;
    int failSafeFailureCount() const;
    std::vector<std::string> failSafeLog() const;
    int lastOvenDegrees() const;
    RoomState currentState() const;
    void queueGameReadyCommands();
    void queuePostQueryCommand();
    void queueReadyCommand();
    void resetPostState();
    void resetGameProgress();
    void triggerRoomCue(const std::string& payload);

private:
    bool handleFireCommand(const std::string& topic, const std::string& payload, unsigned long nowMs);
    bool handleSensorTelemetry(const std::string& topic, const std::string& payload, unsigned long nowMs);
    bool handlePostStateReport(const std::string& topic, const std::string& payload);
    bool handleOvenDegreesReport(const std::string& topic, const std::string& payload);
    bool handleFlowEvent(const std::string& topic, const std::string& payload, unsigned long nowMs);
    void queueFirePanelLedCommand(const std::string& zone, const std::string& mode);
    void watchSmartFilmReveal(unsigned long nowMs);
    void watchSmartFilmHide(unsigned long nowMs);
    void watchOvenArm(unsigned long nowMs);
    void watchLockUnlock(unsigned long nowMs);
    bool observeFailSafe(const std::string& topic, const std::string& payload);
    void triggerColorSequenceErrorCue(const std::string& payload);
    void resetRoomFromCompletedOvenDial(unsigned long nowMs);
    void logFailSafe(const std::string& message);
    void queueCommandsForTopic(const std::string& topic);
    void markPuzzleSolved(const std::string& topic);
    void transitionTo(RoomState nextState, const std::string& reason);

    std::vector<std::unique_ptr<PuzzleModule>> puzzles;
    std::deque<MqttCommand> pendingCommands;
    struct FailSafeAction {
        std::string name;
        std::string expectedTopic;
        std::string expectedPayloadFragment;
        std::vector<MqttCommand> fallbackCommands;
        MqttCommand failureLedCommand;
        unsigned long nextCheckAtMs = 0;
        unsigned long timeoutMs = 2000;
        int fallbackRuns = 0;
        int maxFallbackRuns = 1;
    };
    std::vector<FailSafeAction> failSafes;
    std::vector<std::string> failSafeMessages;
    std::array<bool, 5> postReady = {false, false, false, false, false};
    std::set<std::string> solvedTopics;
    RoomState state = RoomState::COPPER_PUZZLE_ACTIVE;
    int ovenDegrees = 0;
    bool paintingTelemetryActive = false;
    unsigned long roomCompletedAtMs = 0;
    int colorSequenceErrorCount = 0;
    long lastColorErrorTelemetryCount = -1;
    int pendingExplicitColorErrorTelemetryAcks = 0;
    int failSafeRetries = 0;
    int failSafeFailures = 0;
    Effect* paintingCrashEffect = nullptr;
    DisplayOutput* displayOutput = nullptr;
    Effect* colorSequenceErrorEffect = nullptr;
    Effect* colorSequenceTryAgainEffect = nullptr;
    Effect* bakeAttentionEffect = nullptr;
    Effect* copperCompleteEffect = nullptr;
    Effect* colorSequenceSuccessFirstEffect = nullptr;
    Effect* colorSequenceSuccessSecondEffect = nullptr;
    Effect* roomCueEffect = nullptr;
    Effect* playAllAudioEffect = nullptr;
    Effect* finalVictoryEffect = nullptr;
};

#endif
