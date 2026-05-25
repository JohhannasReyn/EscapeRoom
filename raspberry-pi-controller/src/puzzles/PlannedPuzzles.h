#ifndef PLANNED_PUZZLES_H
#define PLANNED_PUZZLES_H

#include "../PuzzleModule.h"

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
    StairsPuzzle() : PlannedPuzzle("Stairs puzzle", "escape/puzzle/stairs/triggered") {}
};

class DowelsPuzzle : public PlannedPuzzle {
public:
    DowelsPuzzle() : PlannedPuzzle("Dowels puzzle", "escape/puzzle/dowels/solved") {}
};

class WinePuzzle : public PlannedPuzzle {
public:
    WinePuzzle() : PlannedPuzzle("Wine puzzle", "escape/puzzle/wine/solved") {}
};

class BlenderPuzzle : public PlannedPuzzle {
public:
    BlenderPuzzle() : PlannedPuzzle("Blender puzzle", "escape/puzzle/blender/solved") {}
};

class FireplacePuzzle : public PlannedPuzzle {
public:
    FireplacePuzzle() : PlannedPuzzle("Fireplace puzzle", "escape/puzzle/fireplace/solved") {}
};

class PhonePuzzle : public PlannedPuzzle {
public:
    PhonePuzzle() : PlannedPuzzle("Phone puzzle", "escape/puzzle/phone/solved") {}
};

class WindowPuzzle : public PlannedPuzzle {
public:
    WindowPuzzle() : PlannedPuzzle("Right wall/window prop", "escape/puzzle/window/triggered") {}
};

class OvenPuzzle : public PlannedPuzzle {
public:
    OvenPuzzle() : PlannedPuzzle("Oven dial puzzle", "escape/puzzle/oven/solved") {}
};

#endif
