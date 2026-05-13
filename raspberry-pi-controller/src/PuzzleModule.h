#ifndef PUZZLE_MODULE_H
#define PUZZLE_MODULE_H

#include <string>

class PuzzleModule {
public:
    virtual ~PuzzleModule() = default;

    virtual std::string name() const = 0;
    virtual std::string topic() const = 0;
    virtual bool handle(const std::string& topic, const std::string& payload) = 0;
};

#endif
