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
        Effect* roomCueEffect = nullptr
    );

    void addPuzzle(std::unique_ptr<PuzzleModule> puzzle);
    bool handleMessage(const std::string& topic, const std::string& payload);
    std::vector<std::string> topics() const;
    std::size_t puzzleCount() const;
    std::size_t pendingCommandCount() const;
    MqttCommand takeNextPendingCommand();
    int lastOvenDegrees() const;
    RoomState currentState() const;
    void queueGameReadyCommands();
    void queuePostQueryCommand();
    void queueReadyCommand();
    void resetPostState();
    void resetGameProgress();
    void triggerRoomCue(const std::string& payload);

private:
    bool handleFireCommand(const std::string& topic, const std::string& payload);
    bool handleSensorTelemetry(const std::string& topic, const std::string& payload);
    bool handlePostStateReport(const std::string& topic, const std::string& payload);
    bool handleOvenDegreesReport(const std::string& topic, const std::string& payload);
    bool handleFlowEvent(const std::string& topic, const std::string& payload);
    void queueFirePanelLedCommand(const std::string& zone, const std::string& mode);
    void queueCommandsForTopic(const std::string& topic);
    void markPuzzleSolved(const std::string& topic);
    void transitionTo(RoomState nextState, const std::string& reason);

    std::vector<std::unique_ptr<PuzzleModule>> puzzles;
    std::deque<MqttCommand> pendingCommands;
    std::array<bool, 5> postReady = {false, false, false, false, false};
    std::set<std::string> solvedTopics;
    RoomState state = RoomState::COPPER_PUZZLE_ACTIVE;
    int ovenDegrees = 0;
    bool ovenPhysicalResetSignaled = false;
    int colorSequenceErrorCount = 0;
    Effect* paintingCrashEffect = nullptr;
    DisplayOutput* displayOutput = nullptr;
    Effect* colorSequenceErrorEffect = nullptr;
    Effect* colorSequenceTryAgainEffect = nullptr;
    Effect* bakeAttentionEffect = nullptr;
    Effect* copperCompleteEffect = nullptr;
    Effect* colorSequenceSuccessFirstEffect = nullptr;
    Effect* colorSequenceSuccessSecondEffect = nullptr;
    Effect* roomCueEffect = nullptr;
};

#endif
