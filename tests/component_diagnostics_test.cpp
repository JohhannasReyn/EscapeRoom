#include <cassert>
#include <string>

#include "../pico-0-component-tests/src/ComponentDiagnostics.h"

int main() {
    assert(std::string(componentTestName(ComponentTest::Ws2812)) == "ws2812");
    assert(std::string(componentTestName(ComponentTest::Pir)) == "pir");
    assert(std::string(componentTestName(ComponentTest::MagneticSwitch)) == "magnetic_switch");
    assert(std::string(componentTestName(ComponentTest::CopperContact)) == "copper_contact");
    assert(std::string(componentTestName(ComponentTest::Button)) == "button");
    assert(std::string(componentTestName(ComponentTest::Potentiometer)) == "potentiometer");
    assert(std::string(componentTestName(ComponentTest::RelayLock)) == "relay_lock");
    assert(std::string(componentTestName(ComponentTest::SmartFilmRelay)) == "smart_film_relay");
    assert(std::string(componentTestName(ComponentTest::Unknown)) == "unknown");

    assert(componentTestFromName("ws2812") == ComponentTest::Ws2812);
    assert(componentTestFromName("pir") == ComponentTest::Pir);
    assert(componentTestFromName("magnetic_switch") == ComponentTest::MagneticSwitch);
    assert(componentTestFromName("copper_contact") == ComponentTest::CopperContact);
    assert(componentTestFromName("button") == ComponentTest::Button);
    assert(componentTestFromName("potentiometer") == ComponentTest::Potentiometer);
    assert(componentTestFromName("relay_lock") == ComponentTest::RelayLock);
    assert(componentTestFromName("smart_film_relay") == ComponentTest::SmartFilmRelay);
    assert(componentTestFromName("nope") == ComponentTest::Unknown);

    assert(std::string(componentTestWiring(ComponentTest::Idle)).find("No active test") != std::string::npos);
    assert(std::string(componentTestWiring(ComponentTest::Ws2812)).find("GPIO17") != std::string::npos);
    assert(std::string(componentTestWiring(ComponentTest::Pir)).find("GPIO6") != std::string::npos);
    assert(std::string(componentTestWiring(ComponentTest::MagneticSwitch)).find("GPIO15") != std::string::npos);
    assert(std::string(componentTestWiring(ComponentTest::CopperContact)).find("GPIO15") != std::string::npos);
    assert(std::string(componentTestWiring(ComponentTest::Button)).find("GPIO15") != std::string::npos);
    assert(std::string(componentTestWiring(ComponentTest::Potentiometer)).find("GPIO26") != std::string::npos);
    assert(std::string(componentTestWiring(ComponentTest::RelayLock)).find("GPIO18") != std::string::npos);
    assert(std::string(componentTestWiring(ComponentTest::SmartFilmRelay)).find("GPIO15") != std::string::npos);
    assert(std::string(componentTestWiring(ComponentTest::Unknown)).find("Valid names") != std::string::npos);

    assert(rainbowColorForStep(0).r == 255);
    assert(rainbowColorForStep(0).g == 0);
    assert(rainbowColorForStep(0).b == 0);
    assert(rainbowColorForStep(21).r == 255);
    assert(rainbowColorForStep(21).g == 0);
    assert(rainbowColorForStep(21).b == 0);

    RgbColor greenish = rainbowColorForStep(7);
    assert(greenish.r == 0);
    assert(greenish.g == 255);
    assert(greenish.b == 0);

    RgbColor blueish = rainbowColorForStep(14);
    assert(blueish.r == 0);
    assert(blueish.g == 0);
    assert(blueish.b == 255);

    assert(wrapLedIndex(0, 21) == 0);
    assert(wrapLedIndex(20, 21) == 20);
    assert(wrapLedIndex(21, 21) == 0);
    assert(wrapLedIndex(44, 21) == 2);
    assert(wrapLedIndex(5, 0) == 0);

    return 0;
}
