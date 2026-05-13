#include "CopperPuzzle.h"

#include <iostream>

CopperPuzzle::CopperPuzzle(Effect& solvedEffect) : solvedEffect(solvedEffect) {
}

std::string CopperPuzzle::name() const {
    return "Copper puzzle";
}

std::string CopperPuzzle::topic() const {
    return "escape/puzzle/copper/solved";
}

bool CopperPuzzle::handle(const std::string& incomingTopic, const std::string& payload) {
    if (incomingTopic != topic()) {
        return false;
    }

    std::cout << "Copper puzzle solved event received!" << std::endl;
    solvedEffect.trigger(payload);
    std::cout << "Copper puzzle effect complete." << std::endl;

    return true;
}
