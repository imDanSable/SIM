// NOLINTBEGIN
#include "../config.hpp"
#ifdef RUNTESTS
#include "../sp/ClockTimer.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#undef INFO
#undef WARN
#include "Catch2/catch_amalgamated.hpp"
using namespace Catch;

TEST_CASE("ClockTimer tests", "[clocktimer]")
{
    SECTION("Constructor")
    {
        sp::ClockTimer timer;
        REQUIRE(timer.getPeriod() == std::nullopt);
        sp::ClockTimer timer2(0.1F);
        REQUIRE(*timer2.getPeriod() == 0.1F);
    }
    SECTION("SetPeriod")
    {
        sp::ClockTimer timer;
        timer.setPeriod(0.1F);
        REQUIRE(*timer.getPeriod() == 0.1F);
    }
    SECTION("Process multiple triggers")
    {
        sp::ClockTimer timer;
        REQUIRE_FALSE(timer.process(1.F, 1.0F));
        REQUIRE_FALSE(timer.process(1.F, 1.0F));
        REQUIRE_FALSE(timer.process(1.F, 0.0F));
        REQUIRE_FALSE(timer.process(1.F, 0.0F));
        // First up
        REQUIRE(timer.process(1.F, 1.0F));
        REQUIRE_FALSE(timer.getPeriod().has_value());
        REQUIRE_FALSE(timer.process(1.F, 0.0F));
        // Second up
        REQUIRE(timer.process(1.F, 1.0F));
        REQUIRE(*timer.getPeriod() == 2.F);
        // No new trigger.
        REQUIRE_FALSE(timer.process(1.F, 1.0F));
        REQUIRE_FALSE(timer.process(1.F, 0.0F));
        // Third up.
        REQUIRE(timer.process(1.F, 1.0F));

        REQUIRE(*timer.getPeriod() == 3.F);
        timer.process(1.F, 0.0F);
        timer.process(1.F, 0.0F);
        timer.process(3.5F, 0.0F);
        timer.process(1.F, 1.0F);

        REQUIRE(*timer.getPeriod() == 6.5F);
        timer.init(true);
        REQUIRE(*timer.getPeriod() == 6.5F);

        REQUIRE_FALSE(timer.process(1.F, .5F));
        REQUIRE_FALSE(timer.process(1.F, -5.0F));
        timer.init();
        REQUIRE_FALSE(timer.getPeriod().has_value());
    }
}
#endif
// NOLINTEND