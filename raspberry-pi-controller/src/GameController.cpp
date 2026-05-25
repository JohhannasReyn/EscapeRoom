#include "GameController.h"

#include "../../shared/EncoderDial.h"
#include "../../shared/PostState.h"

#include <cctype>
#include <iostream>
#include <stdexcept>
#include <utility>

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

        if (cubbyNumber < 1 || cubbyNumber > 6) {
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

bool GameController::handleOvenDegreesReport(const std::string& topic, const std::string& payload) {
    if (topic != "escape/oven/degrees") {
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

        ovenDegrees = normalizeDegrees(parsedDegrees);
        std::cout << "Oven dial degrees: " << ovenDegrees << std::endl;
    } catch (const std::exception&) {
        std::cout << "Ignoring malformed oven degrees payload: " << payload << std::endl;
    }

    return true;
}

void GameController::queueCommandsForTopic(const std::string& topic) {
    struct Route {
        const char* puzzleTopic;
        const char* commandTopic;
    };

    static const Route routes[] = {
        {"escape/puzzle/dowels/solved", "escape/cubby/2/light_on"},
        {"escape/puzzle/wine/solved", "escape/cubby/3/light_on"},
        {"escape/puzzle/fireplace/solved", "escape/cubby/4/light_on"},
        {"escape/puzzle/phone/solved", "escape/cubby/5/light_on"},
        {"escape/puzzle/blender/solved", "escape/cubby/6/light_on"},
    };

    for (const Route& route : routes) {
        if (topic == route.puzzleTopic) {
            pendingCommands.push_back({route.commandTopic, "on"});
            return;
        }
    }

    if (topic == "escape/puzzle/oven/solved") {
        pendingCommands.push_back({"escape/game/win", "on"});
    }
}

void GameController::markPuzzleSolved(const std::string& topic) {
    solvedTopics.insert(topic);

    if (!ovenEnabled && allOvenPrerequisitesSolved()) {
        ovenEnabled = true;
        pendingCommands.push_back({"escape/lock/trigger", "on"});
        pendingCommands.push_back({"escape/oven/enable", "on"});
    }
}

bool GameController::allOvenPrerequisitesSolved() const {
    static const char* requiredTopics[] = {
        "escape/puzzle/copper/solved",
        "escape/puzzle/stairs/triggered",
        "escape/puzzle/dowels/solved",
        "escape/puzzle/wine/solved",
        "escape/puzzle/fireplace/solved",
        "escape/puzzle/phone/solved",
        "escape/puzzle/window/triggered",
        "escape/puzzle/blender/solved",
    };

    for (const char* topic : requiredTopics) {
        if (solvedTopics.find(topic) == solvedTopics.end()) {
            return false;
        }
    }

    return true;
}
