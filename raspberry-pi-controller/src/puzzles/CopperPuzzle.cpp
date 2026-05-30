#include "CopperPuzzle.h"

#include "EscapeRoomProtocol.h"

#include <iostream>

std::string CopperPuzzle::name() const {
    return "Copper puzzle";
}

std::string CopperPuzzle::topic() const {
    return EscapeTopic::COPPER_PUZZLE_COMPLETE;
}

bool CopperPuzzle::handle(const std::string& incomingTopic, const std::string& payload) {
    if (incomingTopic != topic() && incomingTopic != "escape/puzzle/copper/solved") {
        return false;
    }

    std::cout << "Copper puzzle solved event received!" << std::endl;
    std::cout << "Payload: " << payload << std::endl;

    return true;
}
