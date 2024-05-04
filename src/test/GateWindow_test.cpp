// NOLINTBEGIN
#include "../config.hpp"
#ifdef RUNTESTS
#include "../sp/GateWindow.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#undef INFO
#undef WARN
#include "Catch2/catch_amalgamated.hpp"
using namespace Catch;

#define eps 0.00001f

TEST_CASE("Test the gate class", "[Gate]")
{
    SECTION("Regular gates")
    {
        sp::Gate gate(0.1F, 0.2F);
        REQUIRE(gate.get(0.15F));
        REQUIRE(gate.get(0.1F));
        REQUIRE(gate.get(0.2F));
        REQUIRE_FALSE(gate.get(0.09F));
        REQUIRE_FALSE(gate.get(0.21F));
    }
    SECTION("Reverse gates")
    {
        sp::Gate gate(0.9F, 0.1F);
        REQUIRE(gate.get(0.95F));
        REQUIRE(gate.get(0.05F));
        REQUIRE(gate.get(0.0F));
        REQUIRE(gate.get(1.0F));
        REQUIRE(gate.get(0.1F));
        REQUIRE(gate.get(0.9F));
        REQUIRE_FALSE(gate.get(0.15F));
        REQUIRE_FALSE(gate.get(0.85F));
    }
    SECTION("Hitcount testing")
    {
        sp::Gate gate(0.1F, 0.2F);
        // No hits
        REQUIRE(gate.getHits() == 0);
        // first hit
        gate.get(0.15F, true);
        REQUIRE(gate.getHits() == 0);
        // second hit
        gate.get(0.16F, true);
        REQUIRE(gate.getHits() == 0);
        // third hit
        gate.get(0.17F, true);
        REQUIRE(gate.getHits() == 0);
        // non hits
        gate.get(0.09F, true);
        REQUIRE(gate.getHits() == 1);
        gate.get(0.21F, true);
        REQUIRE(gate.getHits() == 1);
        // hit after non hit
        gate.get(0.15F, true);
        REQUIRE(gate.getHits() == 1);
        SECTION("Multiple hits")
        {
            sp::Gate gate(0.2F, 0.9F);
            for (int i = 0; i < 100; i++) {
                // Hit
                gate.get(0.5F, true);
                gate.get(0.6F, true);
                // Non hit
                gate.get(0.1F, true);
                gate.get(0.95F, true);
            }
            REQUIRE(gate.getHits() == 100);
        }
    }
}

TEST_CASE("Test gates and phase values against GateWindow", "[GateWindow]")
{
    // Cartesian product of two ranges, each with 3 rows in the table totalling 9 test cases
    auto randomNumber1 = GENERATE(take(2, random(0.F, 1.F)));
    auto randomNumber2 = GENERATE(take(2, random(0.F, 1.F)));

    std::vector<float> sorted = {std::min(randomNumber1, randomNumber2),
                                 std::max(randomNumber1, randomNumber2)};

    auto inside1 = GENERATE_COPY(take(1, random(sorted[0] + eps, sorted[1] - eps)));
    auto outside1 = GENERATE_COPY(take(1, random(0.F, sorted[0] - eps)));
    auto outside2 = GENERATE_COPY(take(1, random(sorted[1] + eps, 1.F - eps)));

    // Testing the test
    REQUIRE(sorted[0] < sorted[1]);
    REQUIRE(inside1 >= sorted[0]);
    REQUIRE(inside1 <= sorted[1]);
    REQUIRE(outside1 <= sorted[0]);
    REQUIRE(outside2 >= sorted[1]);

    sp::GateWindow gateWindow;
    // Normal gates
    gateWindow.add(sorted[0], sorted[1]);
    REQUIRE(gateWindow.get(inside1) == true);
    REQUIRE(gateWindow.get(outside1) == false);
    REQUIRE(gateWindow.get(outside2) == false);

    // Wrap-around gates
    gateWindow.clear();
    gateWindow.add(sorted[1], sorted[0]);
    REQUIRE(gateWindow.get(inside1) == false);
    REQUIRE(gateWindow.get(outside1) == true);
    REQUIRE(gateWindow.get(outside2) == true);

    // Non random wrap-around
    gateWindow.clear();
    gateWindow.add(0.9F, 0.1F);
    REQUIRE(gateWindow.get(0.5F) == false);
    REQUIRE(gateWindow.get(0.05F) == true);
    REQUIRE(gateWindow.get(0.95F) == true);

    // Adding using two floats
    gateWindow.clear();
    gateWindow.add(0.9F, 0.1F);
    REQUIRE(gateWindow.get(0.5F) == false);
    REQUIRE(gateWindow.get(0.05F) == true);
    REQUIRE(gateWindow.get(0.95F) == true);

    // Adding another as a single Gate
    sp::Gate gate(0.45F, 0.55F);
    gateWindow.add(gate);
    REQUIRE(gateWindow.get(0.5F) == true);
    REQUIRE(gateWindow.get(0.05F) == true);
    REQUIRE(gateWindow.get(0.95F) == true);

    // Adding a vector of gates
    gateWindow.clear();
    sp::GateWindow::Gates gates = {{0.8F, 0.2F}, {0.45F, 0.55F}};
    gateWindow.add(gates);
    REQUIRE(gateWindow.get(0.5F) == true);
    REQUIRE(gateWindow.get(0.4F) == false);
    REQUIRE(gateWindow.get(0.05F) == true);
    REQUIRE(gateWindow.get(0.6F) == false);
    REQUIRE(gateWindow.get(0.95F) == true);

    // Adding a vector of overlapping gates
    gateWindow.clear();
    gates = {{0.8F, 0.2F}, {0.1F, 0.3F}};
    gateWindow.add(gates);
    REQUIRE(gateWindow.get(0.5F) == false);
    REQUIRE(gateWindow.get(0.0F) == true);
    REQUIRE(gateWindow.get(0.25F) == true);

    SECTION("Gatewindow Hit test")
    {
        sp::GateWindow gateWindow;
        // No gates
        REQUIRE(gateWindow.allGatesHit());
        // single gate
        gateWindow.add(0.1F, 0.2F);
        REQUIRE_FALSE(gateWindow.allGatesHit());
        // not hit it
        gateWindow.get(0.3F, true);
        REQUIRE_FALSE(gateWindow.allGatesHit());
        // hit it
        gateWindow.get(0.15F, true);
        REQUIRE_FALSE(gateWindow.allGatesHit());
        // unhit it
        gateWindow.get(0.3F, true);
        REQUIRE(gateWindow.allGatesHit());
        // add another gate
        gateWindow.add(0.3F, 0.4F);
        REQUIRE_FALSE(gateWindow.allGatesHit());
        // hit first gate
        gateWindow.get(0.15F, true);
        REQUIRE_FALSE(gateWindow.allGatesHit());
        // hit second gate
        gateWindow.get(0.35F, true);
        REQUIRE_FALSE(gateWindow.allGatesHit());
        // unhit second gate
        gateWindow.get(0.45F, true);
        REQUIRE(gateWindow.allGatesHit());
    }
}

TEST_CASE("GateSequence operations", "[GateSequence]")
{
    sp::GateSequence gs;

    SECTION("Adding and retrieving gate windows")
    {
        sp::GateWindow gw1;
        sp::GateWindow gw2;
        sp::GateWindow gw3;

        gs.add(gw1, 0);
        gs.add(gw2, 1);
        gs.add(gw3, 2);
        gs.add(gw3, -10);
        REQUIRE(gs.getGateWindow(0) != nullptr);
        REQUIRE(gs.getGateWindow(0) != nullptr);
        REQUIRE(gs.getGateWindow(1) != nullptr);
        REQUIRE(gs.getGateWindow(2) != nullptr);
        REQUIRE(gs.getGateWindow(-10) != nullptr);
    }

    SECTION("Asking for non-existent gate windows")
    {
        REQUIRE(gs.getGateWindow(100) == nullptr);
    }

    SECTION("Adding/Getting and retrieving gates")
    {
        gs.add(0.1f, 0.2f, -1);
        REQUIRE(gs.getGate(-.85f) != nullptr);

        // gs.add(0.1f, 0.2f, 1);
        // gs.add(0.3f, 0.4f, 2);
        // gs.add(0.2f, 0.3f, -10);

        // REQUIRE(gs.getGate(1.15f) != nullptr);

        // REQUIRE(gs.getGate(2.35f) != nullptr);
        // REQUIRE(gs.getGate(-10.75f) != nullptr);

        // REQUIRE(gs.getGate(1.15f)->getStart() == Approx(0.1f));
        // REQUIRE(gs.getGate(1.16f)->getEnd() == Approx(0.2f));

        // REQUIRE(gs.getGate(-1.35f)->getStart() == Approx(0.4f));
        // REQUIRE(gs.getGate(-1.35f)->getEnd() == Approx(0.3f));

        SECTION("Asking for non-existent gates")
        {
            REQUIRE(gs.getGate(0.5f) == nullptr);
        }
    }
}
#endif
#pragma GCC diagnostic pop
// NOLINTEND