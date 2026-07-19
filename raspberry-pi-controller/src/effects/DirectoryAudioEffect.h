#ifndef DIRECTORY_AUDIO_EFFECT_H
#define DIRECTORY_AUDIO_EFFECT_H

#include "Effect.h"

#include <string>
#include <vector>

class DirectoryAudioEffect : public Effect {
public:
    explicit DirectoryAudioEffect(std::string audioDirectory);

    void trigger(const std::string& payload) override;
    std::vector<std::string> files() const;
    static bool isSupportedAudioFile(const std::string& path);

private:
    std::string audioDirectory;
};

#endif
