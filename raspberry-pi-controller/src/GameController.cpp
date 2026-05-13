#include "GameController.h"

#include <iostream>
#include <utility>

void GameController::addPuzzle(std::unique_ptr<PuzzleModule> puzzle) {
    puzzles.push_back(std::move(puzzle));
}

bool GameController::handleMessage(const std::string& topic, const std::string& payload) {
    for (const auto& puzzle : puzzles) {
        if (puzzle->handle(topic, payload)) {
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
