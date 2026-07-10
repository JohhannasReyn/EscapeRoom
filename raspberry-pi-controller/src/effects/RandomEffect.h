#ifndef RANDOM_EFFECT_H
#define RANDOM_EFFECT_H

#include "Effect.h"

#include <cstddef>
#include <random>
#include <vector>

class RandomEffect : public Effect {
public:
    explicit RandomEffect(std::vector<Effect*> effects);
    RandomEffect(std::vector<Effect*> effects, unsigned int seed);

    void trigger(const std::string& payload) override;
    std::size_t effectCount() const;

private:
    std::vector<Effect*> effects;
    std::mt19937 rng;
};

#endif
