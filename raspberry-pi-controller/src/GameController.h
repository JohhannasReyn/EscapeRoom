#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "PuzzleModule.h"

#include <memory>
#include <string>
#include <vector>

class GameController {
public:
    void addPuzzle(std::unique_ptr<PuzzleModule> puzzle);
    bool handleMessage(const std::string& topic, const std::string& payload);
    std::vector<std::string> topics() const;
    std::size_t puzzleCount() const;

private:
    std::vector<std::unique_ptr<PuzzleModule>> puzzles;
};

#endif
