#include "../Phasor.hpp"

// NOLINTBEGIN
#include "../config.hpp"
#ifdef RUNTESTS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#include <random>
#include "catch_amalgamated.hpp"
using namespace Catch;

TEST_CASE("wrap function", "[phasor][wrap][lowlevel]")
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

TEST_CASE("Phasor::setPeriod", "[phasor]")
{
    Phasor phasor;

    SECTION("Phasor basics")
    {
        // Two steps to zero
        phasor.setPeriod(1.F);
        phasor.process(0.2F);
        REQUIRE(phasor.get() == Approx(0.2F));
        phasor.process(0.8F);
        REQUIRE(phasor.get() == Approx(0.F));
        REQUIRE(phasor.getDistance() == 1);

        // Set period
        phasor.reset();
        phasor.setPeriod(0.1F);
        phasor.process(0.4F);
        REQUIRE(phasor.get() == Approx(0.F));
        REQUIRE(phasor.getDistance() == 4);

        // Not rounded number
        phasor.setPeriod(10.10F);
        phasor.process(5.05F);
        REQUIRE(phasor.get() == Approx(0.5F));

        // Phasor reset
        phasor.reset(true);
        REQUIRE(phasor.getPeriod() == Approx(10.10F));
        REQUIRE(phasor.isPeriodSet() == true);
        REQUIRE(phasor.get() == Approx(0.0F));
        phasor.reset();
        REQUIRE(std::isnan(phasor.getPeriod()));
        REQUIRE(phasor.isPeriodSet() == false);

        // Adjust period
        phasor.setPeriod(1.F);
        phasor.set(0.3F);
        phasor.setPeriod(.233F);
        REQUIRE(phasor.get() == Approx(0.3F));
    }

    SECTION("setTotalPhase")
    {
        Phasor phasor;

        phasor.setTotalPhase(5.7f);
        REQUIRE(phasor.get() == Approx(0.7f));
        REQUIRE(phasor.getDistance() == 5);

        phasor.setTotalPhase(-5.7f);
        REQUIRE(phasor.get() == Approx(0.3f));
        REQUIRE(phasor.getDistance() == -6);

        phasor.setTotalPhase(0.0f);
        REQUIRE(phasor.get() == Approx(0.0f));
        REQUIRE(phasor.getDistance() == 0);
    }

    SECTION("Phasor direction")
    {
        // Small phase change
        phasor.set(0.3F);
        phasor.set(0.2F);
        REQUIRE(phasor.getDirection() == false);
        REQUIRE(phasor.getSmartDirection() == false);
        phasor.set(0.4F);
        REQUIRE(phasor.getDirection() == true);
        REQUIRE(phasor.getSmartDirection() == true);

        // Wrapped phase change
        phasor.set(0.9F);
        phasor.set(0.1F);
        REQUIRE(phasor.getDirection() == false);
        REQUIRE(phasor.getSmartDirection() == true);

        // Negative wrapped phase change
        phasor.set(0.1F);
        phasor.set(0.9F);
        REQUIRE(phasor.getDirection() == true);
        REQUIRE(phasor.getSmartDirection() == false);
    }

    SECTION("Phasor getDistance/getTotalPhase")
    {
        phasor.setPeriod(1.F);
        phasor.process(0.5F);
        REQUIRE(phasor.getDistance() == 0);
        REQUIRE(phasor.getTotalPhase() == Approx(0.5F));
        REQUIRE(phasor.get() == Approx(0.5F));
        phasor.process(0.5F);
        REQUIRE(phasor.getDistance() == 1);
        REQUIRE(phasor.getTotalPhase() == Approx(1.0F));
        REQUIRE(phasor.get() == Approx(0.0F));
        phasor.process(-1.0F);
        REQUIRE(phasor.getDistance() == 0);
        REQUIRE(phasor.getTotalPhase() == Approx(0.0F));
        REQUIRE(phasor.get() == Approx(0.0F));
        phasor.process(-0.5F);
        REQUIRE(phasor.getDistance() == -1);
        REQUIRE(phasor.getTotalPhase() == Approx(-0.5F));
        REQUIRE(phasor.get() == Approx(0.5F));
        phasor.process(-0.499F);
        REQUIRE(phasor.getDistance() == -1);
        REQUIRE(phasor.getTotalPhase() == Approx(-0.999F));
        REQUIRE(phasor.get() == Approx(0.001F).margin(1e-6f));
        phasor.process(-0.001F);
        REQUIRE(phasor.getDistance() == -2);
        REQUIRE(phasor.getTotalPhase() == Approx(-2.0F));
        phasor.process(10.5);
        REQUIRE(phasor.getDistance() == 8);
        REQUIRE(phasor.getTotalPhase() == Approx(8.500F));
    }

    SECTION("FP error test")
    {
        // TODO: Implement
    }
}
#endif
// NOLINTEND