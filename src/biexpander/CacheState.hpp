#include <rack.hpp>
/// @brief equality operator for Param for cache comparison
inline bool operator!=(const rack::engine::Param& lhs, const rack::engine::Param& rhs)
{
    return lhs.value != rhs.value;
}
// @brief equality operator for Input for cache comparison
inline bool operator!=(const rack::engine::Input& lhs, const rack::engine::Input& rhs)
{
    // Check number of channels
    if (lhs.channels != rhs.channels) { return true; }  // NOLINT
    // Check individual channel voltages
    for (uint8_t i = 0; i < lhs.channels; i++) {                  // NOLINT
        if (lhs.voltages[i] != rhs.voltages[i]) { return true; }  // NOLINT
    }
    return false;
}

/// @brief Mixin class for indicating of invalid cache
/// @details This class is used internally by Connectable
class CacheState {
   public:
    /// @brief Pass inputs that don't invalidate the internal state of the adapter
    void setIgnoreInputIds(std::vector<size_t> ignoreInputIds)
    {
        // Pass inputs that don't invalidate readBuffer
        inputIndices.clear();
        for (size_t i = 0; i < static_cast<size_t>(module->getNumInputs()); i++) {
            if (std::find(ignoreInputIds.begin(), ignoreInputIds.end(), i) ==
                ignoreInputIds.end()) {
                // if we don't find the index in the ignore list, we add it to the inputIndices
                inputIndices.push_back(i);
            }
        }
    }
    /// @brief Pass parameters that don't invalidate the internal state of the adapter
    void setIgnoreParamIds(std::vector<size_t> ignoreParamIds)
    {
        paramIndices.clear();
        for (size_t i = 0; i < static_cast<size_t>(module->getNumParams()); i++) {
            if (std::find(ignoreParamIds.begin(), ignoreParamIds.end(), i) ==
                ignoreParamIds.end()) {
                paramIndices.push_back(i);
            }
        }
    }

    explicit CacheState(rack::Module* module) : module(module) {
        // Use APP->engine->getSampleRate() to set the paramDivider so that it checks 15 times per second
        float sampleRate = APP->engine->getSampleRate();
        paramDivider.setDivision(sampleRate / 15.F);
    }

    /// @brief Just returns the dirty flag without updating the cache
    bool isDirty() const
    {
        return dirty;
    }

    /// @brief Checks if the module is dirty and updates the cache if needed
    /// @details Either because it was set dirty, or because of change in input, param
    /// @details A change in connection state is not checked here but in the module's onPortChange
    bool needsRefreshing()
    {
        if (dirty) { return true; }
        {  // With cache enabled, this block is the main CPU consumer

            // Is it time to check the params?
            if (paramDivider.process()) {
                // Check if any parameter has changed
                // For all indices in paramIndices (the ones that are not ignored)
                if (std::any_of(paramIndices.begin(), paramIndices.end(), [&](int paramIndice) {
                        return module->params[paramIndice] != paramCache[paramIndice];
                    })) {
                    return true;
                }
            }
            // Compare inputs (using the != operator defined in iters.hpp)
            if (std::any_of(inputIndices.begin(), inputIndices.end(), [&](int inputIndice) {
                    return module->inputs[inputIndice] != inputCache[inputIndice];
                })) {
                return true;
            }
        }
        // If none of the above conditions are met, the adapter is not dirty
        return false;
    }
    void setDirty()
    {
        dirty = true;
    }
    void paramRefresh() const
    {
        paramCache = module->params;
    }
    void inputRefresh() const
    {
        inputCache = module->inputs;
    }
    /// @brief Updates the cache with the current state of the module and resets the dirty flag
    void refresh()
    {
        // An expensive copy step, but only if things change
        paramRefresh();
        inputRefresh();
        dirty = false;
    }

   private:
    rack::Module* module;
    bool dirty = true;
    mutable std::vector<rack::Param> paramCache;
    mutable std::vector<rack::Input> inputCache;
    std::vector<size_t> paramIndices;
    std::vector<size_t> inputIndices;
    rack::dsp::ClockDivider paramDivider;
};