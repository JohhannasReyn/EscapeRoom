#ifndef COMPONENT_DIAGNOSTICS_H
#define COMPONENT_DIAGNOSTICS_H

#include <cstring>

struct RgbColor {
    int r;
    int g;
    int b;
};

enum class ComponentTest {
    Idle,
    Ws2812,
    Pir,
    MagneticSwitch,
    CopperContact,
    Button,
    Potentiometer,
    RelayLock,
    SmartFilmRelay,
    Unknown
};

inline const char* componentTestName(ComponentTest test) {
    switch (test) {
        case ComponentTest::Idle: return "idle";
        case ComponentTest::Ws2812: return "ws2812";
        case ComponentTest::Pir: return "pir";
        case ComponentTest::MagneticSwitch: return "magnetic_switch";
        case ComponentTest::CopperContact: return "copper_contact";
        case ComponentTest::Button: return "button";
        case ComponentTest::Potentiometer: return "potentiometer";
        case ComponentTest::RelayLock: return "relay_lock";
        case ComponentTest::SmartFilmRelay: return "smart_film_relay";
        case ComponentTest::Unknown: return "unknown";
    }

    return "unknown";
}

inline ComponentTest componentTestFromName(const char* name) {
    if (std::strcmp(name, "idle") == 0) return ComponentTest::Idle;
    if (std::strcmp(name, "ws2812") == 0) return ComponentTest::Ws2812;
    if (std::strcmp(name, "pir") == 0) return ComponentTest::Pir;
    if (std::strcmp(name, "magnetic_switch") == 0) return ComponentTest::MagneticSwitch;
    if (std::strcmp(name, "copper_contact") == 0) return ComponentTest::CopperContact;
    if (std::strcmp(name, "button") == 0) return ComponentTest::Button;
    if (std::strcmp(name, "potentiometer") == 0) return ComponentTest::Potentiometer;
    if (std::strcmp(name, "relay_lock") == 0) return ComponentTest::RelayLock;
    if (std::strcmp(name, "smart_film_relay") == 0) return ComponentTest::SmartFilmRelay;
    return ComponentTest::Unknown;
}

inline int wrapLedIndex(int step, int ledCount) {
    if (ledCount <= 0) {
        return 0;
    }

    int wrapped = step % ledCount;
    return wrapped < 0 ? wrapped + ledCount : wrapped;
}

inline RgbColor rainbowColorForStep(int step) {
    constexpr int wheelSteps = 21;
    constexpr int bandSteps = 7;
    int position = wrapLedIndex(step, wheelSteps);

    if (position < bandSteps) {
        int green = (255 * position) / (bandSteps - 1);
        int red = 255 - green;
        return {red, green, 0};
    }

    if (position < bandSteps * 2) {
        int offset = position - bandSteps;
        int blue = (255 * offset) / (bandSteps - 1);
        int green = 255 - blue;
        return {0, green, blue};
    }

    int offset = position - bandSteps * 2;
    int red = (255 * offset) / (bandSteps - 1);
    int blue = 255 - red;
    return {red, 0, blue};
}

#endif
