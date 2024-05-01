#include "GateWindow.hpp"
#include <algorithm>
#include <cassert>

Gate::Gate(float start, float end)
    : start(start), end(end), reverse(start > end), zero(start == end)
{
    assert(start >= 0.0F && start <= 1.0F && "Start must be normalized");
    assert(end >= 0.0F && end <= 1.0F && "End must be normalized");
    // Initialize trigger so the first HIGH is detected.
    trigger.process(false);
};

bool Gate::get(float normalizedPhase, bool withHitCount)
{
    using rack::dsp::BooleanTrigger::UNTRIGGERED;

    if (zero) { return false; }
    const bool inside = !reverse ? normalizedPhase >= start && normalizedPhase <= end
                                 : normalizedPhase >= start || normalizedPhase <= end;
    if (withHitCount) {
        if (trigger.processEvent(inside) == UNTRIGGERED) { hits++; }
    }
    return inside;
};

float Gate::getStart() const
{
    return start;
};

float Gate::getEnd() const
{
    return end;
};

unsigned int Gate::getHits() const
{
    return hits;
};

bool Gate::getReverse() const
{
    return reverse;
};

bool GateWindow::get(float normalizedPhase, bool withHitCount)
{
    assert(normalizedPhase >= 0.0F && normalizedPhase <= 1.0F && "Phase must be normalized");
    // See if any gate is hit
    if (!withHitCount) {
        return std::any_of(gates.begin(), gates.end(),
                           [normalizedPhase](auto gate) { return gate.get(normalizedPhase); });
    }
    // else return OR-ed result off all get() calls
    bool result = false;
    for (auto& gate : gates) {
        result |= gate.get(normalizedPhase, true);
    }
    return result;
};

void GateWindow::add(float start, float end)
{
    gates.emplace_back(start, end);
};

void GateWindow::add(const Gate& gate)
{
    gates.push_back(gate);
};

void GateWindow::add(const std::vector<Gate>& newGates)
{
    gates.insert(gates.end(), newGates.begin(), newGates.end());
};
// NOTCOVERED
Gate* GateWindow::getGate(float normalizedPhase)
{
    auto it = std::find_if(gates.begin(), gates.end(),
                           [normalizedPhase](auto gate) { return gate.get(normalizedPhase); });
    return it != gates.end() ? &(*it) : nullptr;
};

// NOTCOVERED
GateWindow::Gates GateWindow::getGates(float normalizedPhase)
{
    Gates result;
    std::copy_if(gates.begin(), gates.end(), std::back_inserter(result),
                 [normalizedPhase](auto gate) { return gate.get(normalizedPhase); });
    return result;
};

void GateWindow::clear()
{
    gates.clear();
};

bool GateWindow::allGatesHit() const
{
    return std::all_of(gates.begin(), gates.end(), [](auto gate) { return gate.getHits() > 0; });
};

bool GateWindow::isEmpty() const
{
    return gates.empty();
};

bool GateSequence::get(float totalPhase, bool withHitCount)
{
    auto* gateWindow = getGateWindow(totalPhase);
    if (gateWindow == nullptr) { return false; }
    return gateWindow->get(totalPhase, withHitCount);
};

void GateSequence::add(float start, float end, int distance)
{
    windows[distance].add(start, end);
};

void GateSequence::add(const GateWindow& gateWindow, int distance)
{
    windows[distance] = gateWindow;
};

GateWindow* GateSequence::getGateWindow(int distance)
{
    auto it = windows.find(distance);
    return it != windows.end() ? &it->second : nullptr;
};

Gate* GateSequence::getGate(float totalPhase, bool withHitCount)
{
    auto* gateWindow = getGateWindow(std::floor(totalPhase));
    if (gateWindow == nullptr) { return nullptr; }
    if (gateWindow->isEmpty()) { return nullptr; }

    const float normalizedPhase = totalPhase - ::floorf(totalPhase);
    return gateWindow->getGate(normalizedPhase);
};