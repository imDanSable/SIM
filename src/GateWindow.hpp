#include <cassert>
#include <rack.hpp>
#include <vector>

struct Gate {
   public:
    Gate(float start, float end);
    bool get(float normalizedPhase, bool withHitCount = false);
    float getStart() const;
    float getEnd() const;
    unsigned int getHits() const;
    bool getReverse() const;

   private:
    float start;
    float end;
    bool reverse;
    bool zero;
    uint8_t hits = 0;
    rack::dsp::BooleanTrigger trigger;
};

struct ExtendedGate {};

/// @brief places gates within a window of [0, 1]
class GateWindow {
   public:
    using Gates = std::vector<Gate>;
    /// @brief Checks if a normalized phase is within any gate range.
    /// @param normalizedPhase A float between 0.0F and 1.0F.
    /// @param withHitCount If true, the hit count of the gate will be incremented.
    /// @return true if the phase is within a gate range, false otherwise.
    /// A gate range is [start, end] or [start, 1.0F] and [0.0F, end] if start > end.
    bool get(float normalizedPhase, bool withHitCount = false, bool autoRemove = false);
    /// @brief Set a gate range without copy constructor
    void add(float start, float end);
    /// @brief Set a gate range with copy constructor
    void add(const Gate& gate);
    /// @brief Set multiple gate ranges
    void add(const Gates& gates);
    /// @brief Get the gate at phase if any
    Gate* getGate(float normalizedPhase);
    /// @brief Get all gates at phase
    Gates getGates(float normalizedPhase);
    /// @brief Checks if all gates have been hit and unhit at least once
    bool allGatesHit() const;
    /// @brief Remove all gates that have been hit and unhit at least once
    void removeHitGates();
    /// @brief Checks if all gates are empty
    bool isEmpty() const;
    /// @brief Clears all gates
    void clear();
    /// @brief Get all gates as vector
    Gates& getGates();

   private:
    Gates gates;
};

/// @brief places gatewindows on a timeline
class GateSequence {
   public:
    using Windows = std::map<int, GateWindow>;
    /// @brief Checks if a total phase is within any gate range.
    bool get(float totalPhase, bool withHitCount = false);
    /// @brief Add a gate window to the sequence (no copyconstructor)
    void add(float start, float end, int distance = 0);
    /// @brief Add a gate window to the sequence
    void add(const GateWindow& gateWindow, int distance = 0);

    /// @brief Get a gate window by distance
    GateWindow* getGateWindow(int distance);
    /// @brief Get a gate by total phase
    Gate* getGate(float totalPhase, bool withHitCount = false);

   private:
    Windows windows;
};