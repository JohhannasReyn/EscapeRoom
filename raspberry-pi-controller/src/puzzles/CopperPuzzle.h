#ifndef COPPER_PUZZLE_H
#define COPPER_PUZZLE_H

#include "../PuzzleModule.h"
#include "../effects/Effect.h"

class CopperPuzzle : public PuzzleModule {
public:
    explicit CopperPuzzle(Effect& solvedEffect);

    std::string name() const override;
    std::string topic() const override;
    bool handle(const std::string& topic, const std::string& payload) override;

private:
    Effect& solvedEffect;
};

#endif
