#ifndef AUDIO_EFFECT_H
#define AUDIO_EFFECT_H

#include "Effect.h"

#include <string>

class AudioEffect : public Effect {
public:
    explicit AudioEffect(std::string audioFile);

    void trigger(const std::string& payload) override;
    std::string file() const;

private:
    std::string audioFile;
};

#endif
