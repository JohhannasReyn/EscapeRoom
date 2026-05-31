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

inline const char* componentTestWiring(ComponentTest test) {
    switch (test) {
        case ComponentTest::Idle:
            return "No active test selected. Wire one component, then publish a test name to escape/debug/pico0/set_test.";
        case ComponentTest::Ws2812:
            return "WS2812B: 5V supply + -> LED 5V+, 5V supply - -> LED GND, Pico GND -> LED GND, Pico GPIO17 -> 330-470 ohm resistor -> LED DIN.";
        case ComponentTest::Pir:
            return "PIR: PIR VCC -> 5V supply or Pico 3V3 if supported, PIR GND -> Pico GND and supply GND, PIR OUT -> Pico GPIO6. Confirm OUT is not 5V before connecting.";
        case ComponentTest::MagneticSwitch:
            return "Magnetic switch: Pico GPIO15 -> one side of switch, Pico GND -> other side. Uses INPUT_PULLUP, idle=1, closed/triggered=0.";
        case ComponentTest::CopperContact:
            return "Copper contact: Pico GPIO15 -> one copper contact, Pico GND -> other contact. Uses INPUT_PULLUP, open=1, connected=0.";
        case ComponentTest::Button:
            return "Button: Pico GPIO15 -> one side of button, Pico GND -> other side. Uses INPUT_PULLUP, released=1, pressed=0.";
        case ComponentTest::Potentiometer:
            return "Potentiometer: Pico 3V3 -> one outer leg, Pico GND -> other outer leg, Pico GPIO26/ADC0 -> center wiper.";
        case ComponentTest::RelayLock:
            return "Relay lock output: Pico GPIO18 -> relay IN, Pico GND -> relay GND, relay VCC -> module supply. Test relay before connecting a real lock.";
        case ComponentTest::SmartFilmRelay:
            return "Smart film relay output: Pico GPIO15 -> relay/driver IN, Pico GND -> relay/driver GND, driver VCC -> module supply. Test driver before connecting smart film.";
        case ComponentTest::Unknown:
            return "Unknown test. Valid names: idle, ws2812, pir, magnetic_switch, copper_contact, button, potentiometer, relay_lock, smart_film_relay.";
    }

    return "Unknown test.";
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
