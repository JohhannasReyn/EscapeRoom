#ifndef DISPLAY_OUTPUT_H
#define DISPLAY_OUTPUT_H

#include <string>

class DisplayOutput {
public:
    virtual ~DisplayOutput() = default;

    virtual void show_message(const std::string& text);
    virtual void flash_message(const std::string& text, int durationSec, double intervalSec);
    virtual void clear();
};

#endif
