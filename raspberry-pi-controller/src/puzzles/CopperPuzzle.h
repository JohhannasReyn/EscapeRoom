#ifndef COPPER_PUZZLE_H
#define COPPER_PUZZLE_H

#include "../PuzzleModule.h"

class CopperPuzzle : public PuzzleModule {
public:
    CopperPuzzle() = default;

    std::string name() const override;
    std::string topic() const override;
    bool handle(const std::string& topic, const std::string& payload) override;
};

#endif
