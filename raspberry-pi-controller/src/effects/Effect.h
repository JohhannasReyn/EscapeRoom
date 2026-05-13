#ifndef EFFECT_H
#define EFFECT_H

#include <string>

class Effect {
public:
    virtual ~Effect() = default;

    virtual void trigger(const std::string& payload) = 0;
};

#endif
