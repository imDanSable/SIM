#include "../Shared.hpp"

// NOLINTBEGIN
#include "../config.hpp"
#ifdef RUNTESTS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#include "catch_amalgamated.hpp"
using namespace Catch;

TEST_CASE("wrap function", "[shared][wrap][lowlevel]")
{
    REQUIRE(wrap(0.5f, 0.0f, 1.0f) == Approx(0.5f));
    REQUIRE(wrap(-0.5f, 0.0f, 1.0f) == Approx(0.5f));
    REQUIRE(wrap(1.5f, 0.0f, 1.0f) == Approx(0.5f));
    REQUIRE(wrap(-0.5f, -1.0f, 0.0f) == Approx(-0.5f));
    REQUIRE(wrap(100.5f, 0.0f, 1.0f) == Approx(0.5f));
    REQUIRE(wrap(-100.5f, 0.0f, 1.0f) == Approx(0.5f));
    REQUIRE(wrap(100.5f, -1.0f, 0.0f) == Approx(-.5f));
    REQUIRE(wrap(-100.5f, -1.0f, 0.0f) == Approx(-.5f));
    REQUIRE(wrap(0.0f, 0.5f, 1.0f) == Approx(0.5f));
    REQUIRE(wrap(0.5f, -2.0f, -1.0f) == Approx(-1.5f));
    REQUIRE(wrap(-1.5f, -2.0f, -1.0f) == Approx(-1.5f));
}

// R 3
TEST_CASE("ClockTracker", "[shared][clocktracker][lowlevel]")
{
    ClockTracker clockTracker;
    ClockTracker init_clockTracker;
    ClockTracker init_with_period_clockTracker;
    init_clockTracker.init();
    init_with_period_clockTracker.init(0.1);
    SECTION("isPeriodDetected")
    {
        REQUIRE(clockTracker.isPeriodDetected() == false);
        clockTracker.init();
        REQUIRE(clockTracker.isPeriodDetected() == false);
        clockTracker.process(10.F, 0.F);
        clockTracker.process(10.F, 8.F);
        REQUIRE(clockTracker.isPeriodDetected() == false);
        clockTracker.process(10.F, -3.F);
        clockTracker.process(10.F, 10.F);
        REQUIRE(clockTracker.isPeriodDetected() == false);
        clockTracker.process(10.F, 0.F);
        clockTracker.process(10.F, 11.F);
        REQUIRE(clockTracker.isPeriodDetected() == true);
        REQUIRE(clockTracker.getPeriod() == 20.F);
        clockTracker.init();
        REQUIRE(clockTracker.isPeriodDetected() == false);
        clockTracker.init(0.1);
        REQUIRE(clockTracker.isPeriodDetected() == true);
        REQUIRE(clockTracker.getPeriod() == 0.1F);
        REQUIRE(init_clockTracker.isPeriodDetected() == false);
        REQUIRE(init_with_period_clockTracker.isPeriodDetected() == true);
        REQUIRE(init_with_period_clockTracker.getPeriod() == 0.1F);
    }
    SECTION("getTimePassed & getTimeFraction")
    {
        ClockTracker clockTracker;
        float expectedTimePassed = 0.F;
        auto randomTime = GENERATE(take(5, random(-1.F, 1.F)));
        auto randomVolt = GENERATE(take(5, random(-10.F, 10.F)));
        for (int i = 0; i < 100; ++i) {
            clockTracker.process(randomTime, randomVolt);
            expectedTimePassed += randomTime;
        }
        REQUIRE(clockTracker.getTimePassed() == Approx(expectedTimePassed));
        REQUIRE(clockTracker.getTimeFraction() ==
                Approx(expectedTimePassed / clockTracker.getPeriod()));
    }
}

TEST_CASE("Note to Voltage", "[shared][notetovoltage][lowlevel]")
{
    SECTION("getVoctFromNote")
    {
        REQUIRE(getVoctFromNote("C4", 0.F) == Approx(0.F));
        REQUIRE(getVoctFromNote("C#4", 0.F) == Catch::Approx(0.0833333F));
        REQUIRE(getVoctFromNote("db4", NAN) == Catch::Approx(0.0833333F));
        REQUIRE(getVoctFromNote("D4", 0.F) == Catch::Approx(0.1666667F));
        REQUIRE(getVoctFromNote("D#4", 0.F) == Catch::Approx(0.25F));
        REQUIRE(getVoctFromNote("E-1", 0.F) == Catch::Approx(-4.6666667F));
        REQUIRE(getVoctFromNote("e1", 0.F) == Catch::Approx(-2.6666667F));
        REQUIRE(getVoctFromNote("E1", 0.F) == Catch::Approx(-2.6666667F));
        REQUIRE(getVoctFromNote("E2", 0.F) == Catch::Approx(-1.6666667F));
        REQUIRE(getVoctFromNote("G#-3", 0.F) == Catch::Approx(-6.3333333F));
        float res = getVoctFromNote("asdf", NAN);
        REQUIRE(std::isnan(res));
    }
    SECTION("getNoteFromVoct")
    {
        REQUIRE(getNoteFromVoct(0, true, -12) == "C3");
        REQUIRE(getNoteFromVoct(0, true, 0) == "C4");
        REQUIRE(getNoteFromVoct(0, true, 1) == "C♯4");
        REQUIRE(getNoteFromVoct(0, true, 2) == "D4");
        REQUIRE(getNoteFromVoct(0, true, 3) == "D♯4");
        REQUIRE(getNoteFromVoct(0, true, 13) == "C♯5");
        REQUIRE(getNoteFromVoct(0, true, 20) == "G♯5");
        REQUIRE(getNoteFromVoct(0, true, 21) == "A5");
        REQUIRE(getNoteFromVoct(0, true, 22) == "A♯5");
        REQUIRE(getNoteFromVoct(0, true, 23) == "B5");
        REQUIRE(getNoteFromVoct(1, true, 23) == "B5");
        REQUIRE(getNoteFromVoct(4, true, 24) == "C6");

        REQUIRE(getNoteFromVoct(0, false, 5) == "F4");
        REQUIRE(getNoteFromVoct(0, false, -10) == "D3");
        REQUIRE(getNoteFromVoct(0, false, -11) == "D♭3");
        REQUIRE(getNoteFromVoct(3, false, -11) == "D♭3");
    }
}

TEST_CASE("getFractionalString", "[shared][getfractionalstring][lowlevel]")
{
    REQUIRE(getFractionalString(3.F, 1, 3) == "9 * 1/3");
    REQUIRE(getFractionalString(3.F, 1, 4) == "12 * 1/4");
    REQUIRE(getFractionalString(-73.F, 51, -99) == "142 * 51/-99");
}
#endif
// NOLINTEND