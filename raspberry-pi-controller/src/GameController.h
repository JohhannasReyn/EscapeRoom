#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "PuzzleModule.h"

#include <array>
#include <deque>
#include <memory>
#include <string>
#include <vector>

struct MqttCommand {
    std::string topic;
    std::string payload;
};

class GameController {
public:
    void addPuzzle(std::unique_ptr<PuzzleModule> puzzle);
    bool handleMessage(const std::string& topic, const std::string& payload);
    std::vector<std::string> topics() const;
    std::size_t puzzleCount() const;
    std::size_t pendingCommandCount() const;
    MqttCommand takeNextPendingCommand();
    void queuePostQueryCommand();
    void queueReadyCommand();
    void resetPostState();

private:
    bool handlePostStateReport(const std::string& topic, const std::string& payload);
    void queueCommandsForTopic(const std::string& topic);

    std::vector<std::unique_ptr<PuzzleModule>> puzzles;
    std::deque<MqttCommand> pendingCommands;
    std::array<bool, 6> postReady = {false, false, false, false, false, false};
};

#endif
