#ifndef PLANNED_PUZZLES_H
#define PLANNED_PUZZLES_H

#include "../PuzzleModule.h"
#include "EscapeRoomProtocol.h"

#include <iostream>
#include <string>
#include <utility>

class PlannedPuzzle : public PuzzleModule {
public:
    PlannedPuzzle(std::string puzzleName, std::string puzzleTopic)
        : puzzleName(std::move(puzzleName)), puzzleTopic(std::move(puzzleTopic)) {
    }

    std::string name() const override {
        return puzzleName;
    }

    std::string topic() const override {
        return puzzleTopic;
    }

    bool handle(const std::string& incomingTopic, const std::string& payload) override {
        if (incomingTopic != puzzleTopic) {
            return false;
        }

        std::cout << puzzleName << " event received." << std::endl;
        std::cout << "Payload: " << payload << std::endl;
        std::cout << "No effect has been assigned to this puzzle yet." << std::endl;
        return true;
    }

private:
    std::string puzzleName;
    std::string puzzleTopic;
};

class StairsPuzzle : public PlannedPuzzle {
public:
    StairsPuzzle() : PlannedPuzzle("Cubby approach sensor", EscapeTopic::CUBBY_APPROACH_DETECTED) {}
};

class PaintingRotationPuzzle : public PlannedPuzzle {
public:
    PaintingRotationPuzzle() : PlannedPuzzle("Painting rotation puzzle", EscapeTopic::PAINTING_ROTATION_COMPLETE) {}
};

class ColorButtonSequencePuzzle : public PlannedPuzzle {
public:
    ColorButtonSequencePuzzle() : PlannedPuzzle("Color button sequence", EscapeTopic::COLOR_SEQUENCE_COMPLETE) {}
};

class ColorButtonSequenceErrorPuzzle : public PlannedPuzzle {
public:
    ColorButtonSequenceErrorPuzzle() : PlannedPuzzle("Color button sequence error", EscapeTopic::COLOR_SEQUENCE_ERROR) {}
};

class OvenTargetPuzzle : public PlannedPuzzle {
public:
    OvenTargetPuzzle() : PlannedPuzzle("Oven target reached", EscapeTopic::OVEN_TARGET_REACHED) {}
};

class ElectromagUnlockedPuzzle : public PlannedPuzzle {
public:
    ElectromagUnlockedPuzzle() : PlannedPuzzle("Electromagnetic lock unlocked", EscapeTopic::ELECTROMAG_LOCK_UNLOCKED) {}
};

#endif
