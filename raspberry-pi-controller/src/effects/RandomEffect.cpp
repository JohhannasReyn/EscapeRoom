#include "RandomEffect.h"

#include <iostream>
#include <utility>

RandomEffect::RandomEffect(std::vector<Effect*> effects)
    : RandomEffect(std::move(effects), std::random_device{}()) {
}

RandomEffect::RandomEffect(std::vector<Effect*> candidateEffects, unsigned int seed)
    : rng(seed) {
    for (Effect* effect : candidateEffects) {
        if (effect != nullptr) {
            effects.push_back(effect);
        }
    }
}

void RandomEffect::trigger(const std::string& payload) {
    if (effects.empty()) {
        std::cout << "Random effect skipped: no effects configured." << std::endl;
        return;
    }

    std::uniform_int_distribution<std::size_t> distribution(0, effects.size() - 1);
    effects[distribution(rng)]->trigger(payload);
}

std::size_t RandomEffect::effectCount() const {
    return effects.size();
}
