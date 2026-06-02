#ifndef GPIO_BUZZER_EFFECT_H
#define GPIO_BUZZER_EFFECT_H

#include "Effect.h"

#include <string>

class GpioBuzzerEffect : public Effect {
public:
    GpioBuzzerEffect(int gpioPin, int durationMs);

    void trigger(const std::string& payload) override;

private:
    int gpioPin;
    int durationMs;
};

#endif
