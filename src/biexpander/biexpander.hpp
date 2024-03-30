/**
Derive your Expanders from either LeftExpander or RightExpander.

Create an adapter for your expander.
The adapter functions as a safe proxy to your actual expander for modules that are compatible with
this expander.
Adapters should derive from BaseAdapter<YourExpander>.
Adapters behave like optionals: When no expander is connected, casting it to bool will yield false
and casting it to your expander or dereferencing it will yield the nullptr. Otherwise it will yield
true or a direct pointer to your expander. Typically you would create functions in your adapter
class that do nothing or return defaults when no expander is present and do the actual work or
return values of the expander when one is.

Derive your modules that can have expanders from Expandable.
For each compatible expander model the module can interact with, create an adapter member variable
corresponding with the compatible expander. On construction pass a map of compatible models with
corresponding adapters for both the left and right side. The map should be of the form:
{{modelLeftExpander, &this->leftAdapter}, {modelRightExpander, &this->rightAdapter}, {...,...}}.

Note that an expander can be compatible with multiple expandables.
A connection between an expander and an expandable is made when the expander is directly attached to
the the expandable on the right side or when all modules between the expander and the expandable are
compatible with the expandable for that side.

The Expandables should do all reading and writing from and to their adapters or by accessing the
expander directly through the adapter. Typically this is done in process() and typically the
expanders themselves do not have their own process() functions. If they do, they should not read and
write to the same ports, params, lights, etc. as the Expandable.

If you want connection lights to lit up when an expander is connected, add them to your widget and
call setLeftLightId(lightId) and setRightLightId(lightId) with their ids.
Note that for LeftExpanders only the right light will be lit when connected and for RightExpanders
only the left.

Example code:
```
class MyExpander : public biexpand::LeftExpander {
    public:
        void actuallyDoSomething();
        int getSomething();
        void setSomething();
};

class MyAdapter : public biexpand::BaseAdapter<MyExpander> {
    public:
        void maybeDoSomething() {
            if (!this) { return; }
            MyExpander* expander = this->operator->();
            expander->actuallyDoSomething();
        }

        int getSomething() {
            if (!this) { return 0; } // Or whatever default value you want.
            MyExpander* expander = this->operator->();
            int anInt = expander->getSomething();
            //use anInt
        }

        void setSomething() {
            if (!this) { return; }
            MyExpander* expander = this->operator->();
            expander->setSomething();
        }
};

class MyExpandable : public biexpand::Expandable {
    public:
        MyExpandable() : biexpand::Expandable({{modelMyExpander, &this->adapter}}, {}) {}
        void process(const ProcessArgs& args) override {
                *adapter->maybeDoSometing(); // No checking needed here.
            }
        }

    // Override this function to do something when the expander chain changes.
    virtual void onUpdateExpanders(bool isRight){};
    private:
        MyAdapter adapter;
};
```

*/

#pragma once
#include <functional>
#include <iterator>
#include <map>
#include <rack.hpp>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>
#include "ModuleInstantiationMenu.hpp"
#include "sigslot/signal.hpp"

namespace biexpand {

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

    explicit CacheState(rack::Module* module) : module(module) {}

    /// @brief Checks if the adapter is dirty
    /// @details Either because it was set dirty, or because of change in input, param
    /// @details A change in connection state is not checked here but in the module's onPortChange
    bool needsRefresh() const
    {
        if (dirty) { return true; }
        if (paramCache.size() != module->params.size() ||
            inputCache.size() != module->inputs.size()) {
            paramCache = module->params;
            inputCache = module->inputs;
            return true;
        }
        // Check if any parameter has changed
        // For all indices in paramIndices (the ones that are not ignored)
        if (std::any_of(paramIndices.begin(), paramIndices.end(), [&](int paramIndice) {
                return module->params[paramIndice] != paramCache[paramIndice];
            })) {
            paramCache = module->params;
            return true;
        }
        // Compare inputs (using the != operator defined in iters.hpp)
        if (std::any_of(inputIndices.begin(), inputIndices.end(), [&](int inputIndice) {
                return module->inputs[inputIndice] != inputCache[inputIndice];
            })) {
            inputCache = module->inputs;  // Deep copy
            return true;
        }
        // If none of the above conditions are met, the adapter is not dirty
        return false;
    }
    void setDirty()
    {
        dirty = true;
    }
    /// @brief Updates the cache with the current state of the module and resets the dirty flag
    void refresh()
    {
        // An expensive copy step, but only if things change
        paramCache = module->params;
        inputCache = module->inputs;
        dirty = false;
    }

   private:
    rack::Module* module;
    bool dirty = true;
    mutable std::vector<rack::Param> paramCache;
    mutable std::vector<rack::Input> inputCache;
    std::vector<size_t> paramIndices;
    std::vector<size_t> inputIndices;
};

class Connectable : public rack::engine::Module {
   public:
    Connectable() : cacheState(this) {}

    /// @brief call this after configuring inputs, outputs and params of the module in its
    /// constructor
    /// @param ignoreInputIds indices of inputs that don't invalidate the internal state of the
    /// adapter
    /// @param ignoreParamIds indices of parameters that don't invalidate the internal state of the
    /// adapter
    void configCache(std::vector<size_t> ignoreInputIds = {},
                     std::vector<size_t> ignoreParamIds = {})
    {
        cacheState.setIgnoreInputIds(std::move(ignoreInputIds));
        cacheState.setIgnoreParamIds(std::move(ignoreParamIds));
    }
    void setLeftLightId(int id)
    {
        leftLightId = id;
    }

    void setRightLightId(int id)
    {
        rightLightId = id;
    }

    void addDefaultConnectionLights(rack::widget::Widget* widget,
                                    int leftLightId,
                                    int rightLightId,
                                    float x_offset = 1.7F,
                                    float y_offset = 2.F)
    {
        setLeftLightId(leftLightId);
        setRightLightId(rightLightId);

        using rack::math::Vec;
        using rack::window::mm2px;
        widget->addChild(
            rack::createLightCentered<rack::SmallLight<rack::componentlibrary::GreenLight>>(
                mm2px(Vec((x_offset), y_offset)), this, leftLightId));
        const float width_px = (widget->box.size.x);

        Vec vec = mm2px(Vec(-x_offset, y_offset));
        vec.x += width_px;
        widget->addChild(
            rack::createLightCentered<rack::SmallLight<rack::componentlibrary::GreenLight>>(
                vec, this, rightLightId));
        setLight(true, false);
        setLight(false, false);
    }

    void setLight(bool isRight, bool state)
    {
        if (isRight && rightLightId != -1) {
            lights[rightLightId].setBrightness(state ? 1.0F : 0.0F);
        }
        else if (!isRight && leftLightId != -1) {
            lights[leftLightId].setBrightness(state ? 1.0F : 0.0F);
        }
    }

    bool isBeingRemoved() const
    {
        return beingRemoved;
    }
    void setBeingRemoved()
    {
        beingRemoved = true;
    }

    bool isDirty() const
    {
        return cacheState.needsRefresh();
    }
    void refresh()
    {
        cacheState.refresh();
    }
    void setDirty()
    {
        cacheState.setDirty();
    }

    /// @brief Invalidate the cache of a connectable when a port changes
    void onPortChange(const rack::Module::PortChangeEvent& e) override
    {
        // XXX This could be more precise (like params and inputs we ignore but opposite)
        setDirty();
    }

   private:
    CacheState cacheState;
    bool beingRemoved = false;
    int leftLightId = -1;
    int rightLightId = -1;
};
// forward declaration of specilized templates for BiExpander friendship
template <typename F>
class Expandable;

enum class ExpanderSide { LEFT, RIGHT };
template <ExpanderSide SIDE>
class BiExpander : public Connectable {
   public:
    void onRemove() override
    {
        setBeingRemoved();
        changeSignal();
        assert(changeSignal.slot_count() == 0);
    }

    void onExpanderChange(const ExpanderChangeEvent& e) override
    {
        constexpr bool isRightSide = (SIDE == ExpanderSide::RIGHT);

        if ((isRightSide && e.side) || (!isRightSide && !e.side)) {
            auto& currentExpander = isRightSide ? this->rightExpander : this->leftExpander;
            auto& prevModule = isRightSide ? prevRightModule : prevLeftModule;
            if (prevModule != currentExpander.module) {
                if (changeSignal.slot_count()) { changeSignal(); }
                prevModule = currentExpander.module;
            }
        }
    }

   private:
    friend class Expandable<bool>;
    friend class Expandable<float>;
    sigslot::signal_st<> changeSignal;
    Module* prevLeftModule = nullptr;
    Module* prevRightModule = nullptr;
};

using LeftExpander = BiExpander<ExpanderSide::LEFT>;
using RightExpander = BiExpander<ExpanderSide::RIGHT>;

class Adapter {
    using FloatIter = std::vector<float>::iterator;
    using BoolIter = std::vector<bool>::iterator;

   public:
    virtual ~Adapter() = default;
    virtual explicit operator bool() const = 0;

    virtual void setPtr(void* ptr) = 0;

    virtual bool inPlace(int /*length*/, int /*channel*/) const
    {
        return false;
    }

    virtual BoolIter transform(BoolIter first, BoolIter last, BoolIter out, int /*channel*/) const
    {
        return std::copy(first, last, out);
    }
    virtual FloatIter transform(FloatIter first,
                                FloatIter last,
                                FloatIter out,
                                int /*channel*/) const
    {
        return std::copy(first, last, out);
    }
    virtual void transformInPlace(FloatIter first, FloatIter last, int channel) const
    {
        // Do nothing
    }
    virtual void transformInPlace(BoolIter first, BoolIter last, int channel) const
    {
        // Do nothing
    }
    virtual void setDirty() = 0;
    virtual bool isDirty() const = 0;
    virtual void refresh() = 0;
};

template <typename T>
class BaseAdapter : public Adapter {
   public:
    BaseAdapter() : ptr(nullptr) {}
    // virtual ~BaseAdapter()
    // {
    //     ptr = nullptr;
    // };

    explicit operator bool() const override
    {
        return ptr != nullptr;
    }
    explicit operator T*() const
    {
        return ptr;
    }
    T* operator->() const
    {
        return ptr;
    }
    void setPtr(void* ptr) override
    {
        this->ptr = static_cast<T*>(ptr);
    }
    void setDirty() override
    {
        ptr->setDirty();
    }
    bool isDirty() const override
    {
        return ptr->isDirty();
    }
    void refresh() override
    {
        ptr->refresh();
    }

   protected:
    T* ptr;  // NOLINT /// XXX Make private one day
};

using AdapterMap = std::map<rack::Model*, Adapter*>;

template <typename F>  // F: The underlying datatype of the buffer bool or float for now
class Expandable : public Connectable {
   public:
    Expandable(AdapterMap leftAdapters, AdapterMap rightAdapters)
        : leftModelsAdapters(std::move(leftAdapters)), rightModelsAdapters(std::move(rightAdapters))
    {
        v1.resize(16);
        v2.resize(16);
    };

    void onRemove() override
    {
        setBeingRemoved();
        disconnectExpanders<RightExpander>(rightExpanders.begin(), rightExpanders.end());
        disconnectExpanders<LeftExpander>(leftExpanders.begin(), leftExpanders.end());
    }
    void onExpanderChange(const ExpanderChangeEvent& e) override
    {
        if (e.side && (prevRightModule != this->rightExpander.module)) {
            refreshExpanders<RightExpander>();
            prevRightModule = this->rightExpander.module;
        }
        else if (!e.side && (prevLeftModule != this->leftExpander.module)) {
            refreshExpanders<LeftExpander>();
            prevLeftModule = this->leftExpander.module;
        }
    };

    /// Implement instead of onExpanderChange() if you need to do something when the expander chain
    virtual void onUpdateExpanders(bool isRight) {};

    template <class T>
    bool connectExpander(T* expander)
    {
        constexpr bool side = std::is_same<T, RightExpander>::value;
        const auto* cadapters = side ? &rightModelsAdapters : &leftModelsAdapters;
        auto adapter = cadapters->find(expander->model);
        if (adapter == cadapters->end()) { return false; }  // Not compatible

        if constexpr (side) {
            if (std::find(rightExpanders.begin(), rightExpanders.end(), expander) !=
                rightExpanders.end()) {
                return false;
            }
            rightExpanders.push_back(expander);
            rightAdapters.push_back(adapter->second);
            assert(expander->changeSignal.slot_count() == 0);
            expander->changeSignal.connect(&Expandable::refreshExpanders<RightExpander>, this);
        }
        else {
            if (std::find(leftExpanders.begin(), leftExpanders.end(), expander) !=
                leftExpanders.end()) {
                // assert(false);  // How did we get here? Putting this in for debugging purposes
                // XXX We crashed here twice, but it could be due to a bug in the module
                return false;
            }
            leftExpanders.push_back(expander);
            leftAdapters.push_back(
                adapter->second);  // NOTE: this is where we expect a single adapter/expander
                                   // instance, by using ->second
            // if (expander->changeSignal.slot_count() == 0) {
            // BUG:  We had a crash here. Moving arr right to left over rex/spike combo with smart
            // enabled. reproducable
            // XXX I suspect that the order of disconnecting and connecting makes signals causes to
            // be connected to two modules for a short time.
            // Changing the assert from == 0 to < 2 seems to not throw.

            assert(expander->changeSignal.slot_count() < 2);
            expander->changeSignal.connect(&Expandable::refreshExpanders<LeftExpander>, this);
            // }
        }
        setLight(side, true);
        expander->setLight(!side, true);
        adapter->second->setPtr(expander);
        return true;
    }

    template <class T>
    void disconnectExpander(T* expander)
    {
        if constexpr (std::is_same<T, RightExpander>::value) {
            prevRightModule = nullptr;
            expander->changeSignal.disconnect_all();

            expander->setLight(false, false);
            auto adapter = rightModelsAdapters.find(expander->model);
            assert(adapter != rightModelsAdapters.end());
            rightExpanders.erase(
                std::remove(rightExpanders.begin(), rightExpanders.end(), expander),
                rightExpanders.end());
            rightAdapters.erase(
                std::remove(rightAdapters.begin(), rightAdapters.end(), adapter->second),
                rightAdapters.end());

            auto it = rightModelsAdapters.find(expander->model);
            if (it != rightModelsAdapters.end()) { it->second->setPtr(nullptr); }
        }
        else if constexpr (std::is_same<T, LeftExpander>::value) {
            prevLeftModule = nullptr;
            // Tell the disconnected module to disconnect from the expandable
            expander->changeSignal.disconnect_all();

            // Turn off the light
            expander->setLight(true, false);
            // Is it compatible?
            auto adapter = leftModelsAdapters.find(expander->model);
            assert(adapter != leftModelsAdapters.end());
            leftExpanders.erase(std::remove(leftExpanders.begin(), leftExpanders.end(), expander),
                                leftExpanders.end());
            leftAdapters.erase(
                std::remove(leftAdapters.begin(), leftAdapters.end(), adapter->second),
                leftAdapters.end());  // NOTE: Here we asume that there is only one adapter/expander
                                      // instance
            auto it = leftModelsAdapters.find(expander->model);
            if (it != leftModelsAdapters.end()) { it->second->setPtr(nullptr); }
        }
    }

    template <class T>
    void disconnectExpanders(typename std::vector<T*>::iterator begin,
                             typename std::vector<T*>::iterator end)
    {
        auto currExpander = begin;
        while (currExpander != end) {
            disconnectExpander(*currExpander);
            currExpander++;
        }
        if constexpr (std::is_same<T, RightExpander>::value) {
            if (rightExpanders.empty()) { setLight(true, false); }
        }
        else if constexpr (std::is_same<T, LeftExpander>::value) {
            if (leftExpanders.empty()) { setLight(false, false); }
        }
    }

    rack::MenuItem* createExpandableSubmenu(rack::ModuleWidget* moduleWidget)
    {
        const auto* expandable = dynamic_cast<Expandable*>(moduleWidget->module);
        assert(expandable != nullptr);
        auto hasLeftModel = [expandable](const std::string& name) {
            // look through leftExpanders vector<Module> and check each Module->Model->name
            const auto& leftExpanders = expandable->getLeftExpanders();
            return std::any_of(leftExpanders.begin(), leftExpanders.end(),
                               [name](Module* module) { return module->model->name == name; });
        };
        auto hasRightModel = [expandable](const std::string& name) {
            const auto& rightExpanders = expandable->getRightExpanders();
            return std::any_of(rightExpanders.begin(), rightExpanders.end(),
                               [name](Module* module) { return module->model->name == name; });
        };
        bool hasExpanders =
            !expandable->getLeftExpanders().empty() || !expandable->getRightExpanders().empty();
        return rack::createSubmenuItem(
            "Add Expander", "",
            [this, moduleWidget, hasLeftModel, hasRightModel, hasExpanders](rack::Menu* menu) {
                for (auto compatible : leftModelsAdapters) {
                    auto* item = new ModuleInstantionMenuItem();
                    item->text = "Add " + compatible.first->name;
                    item->rightText = "←";
                    item->module_widget = moduleWidget;
                    item->right = false;
                    item->model = compatible.first;
                    menu->addChild(item);
                    if (hasLeftModel(compatible.first->name)) { item->disabled = true; }
                }
                for (auto compatible : rightModelsAdapters) {
                    auto* item = new ModuleInstantionMenuItem();
                    item->text = "Add " + compatible.first->name;
                    item->rightText = "→";
                    item->module_widget = moduleWidget;
                    item->right = true;
                    item->model = compatible.first;
                    menu->addChild(item);
                    if (hasRightModel(compatible.first->name)) { item->disabled = true; }
                }
                // Add an "Add all compatible" menu item
                auto* item = new ModuleInstantionMenuItem();
                item->text = "Add all compatible";
                item->all = true;
                item->module_widget = moduleWidget;
                menu->addChild(item);
                if (hasExpanders) { item->disabled = true; }
            });
    };

    std::vector<LeftExpander*> getLeftExpanders() const
    {
        return leftExpanders;
    }
    std::vector<RightExpander*> getRightExpanders() const
    {
        return rightExpanders;
    }

    std::vector<Adapter*> getLeftAdapters() const
    {
        return leftAdapters;
    }
    std::vector<Adapter*> getRightAdapters() const
    {
        return rightAdapters;
    }

   protected:
    bool dirtyAdapters()
    {
        if (std::any_of(leftAdapters.begin(), leftAdapters.end(),  // Use the stored variable here
                        [](const auto* adapter) { return adapter->isDirty(); })) {
            return true;
        }
        if (std::any_of(rightAdapters.begin(), rightAdapters.end(),
                        [](const auto* adapter) { return adapter->isDirty(); })) {
            return true;
        }
        return false;
    }

   private:
    friend LeftExpander;
    friend RightExpander;
    Module* prevLeftModule = nullptr;
    Module* prevRightModule = nullptr;
    /// @brief vector of pointers to expanders that represents the order of expanders and that is
    /// used to keep track of changes in the expander chain
    std::vector<LeftExpander*> leftExpanders;
    std::vector<RightExpander*> rightExpanders;
    // map for compatible models and pointers to adapters
    AdapterMap leftModelsAdapters;
    AdapterMap rightModelsAdapters;
    /// @brief vector of pointers to adapters that represents the order of expanders
    std::vector<Adapter*> leftAdapters;
    std::vector<Adapter*> rightAdapters;

    /// @brief Will traverse the expander chain and update the expanders.
    template <class T>
    void refreshExpanders()
    {
        std::vector<T*>* expanders = nullptr;
        std::function<T*(Connectable*)> nextModule;

        if constexpr (std::is_same<T, RightExpander>::value) {
            nextModule = [this](Connectable* currModule) -> T* {
                T* expander = dynamic_cast<T*>(currModule->rightExpander.module);
                if (!expander || expander->isBeingRemoved()) { return nullptr; }
                auto it = rightModelsAdapters.find(expander->model);
                if (it == rightModelsAdapters.end()) { return nullptr; }
                return expander;
            };
            expanders = &rightExpanders;
        }
        else {
            nextModule = [this](Connectable* currModule) -> T* {
                T* expander = dynamic_cast<T*>(currModule->leftExpander.module);
                if (!expander || expander->isBeingRemoved()) { return nullptr; }
                auto it = leftModelsAdapters.find(expander->model);
                if (it == leftModelsAdapters.end()) { return nullptr; }
                return expander;
            };
            expanders = &leftExpanders;
        }
        T* currModule = nextModule(dynamic_cast<Connectable*>(this));
        // Don't touch the expanders that are unchanged
        // Keep list of connected models to avoid connecting the same model twice
        std::set<rack::plugin::Model*>
            connectedModels;  // IMPROVE: we could reuse the expanders vector icw any_of.
        auto it = expanders->begin();
        bool same = !expanders->empty() && currModule == *it;
        while (currModule && it != expanders->end() && same) {
            connectedModels.insert(currModule->model);
            currModule = nextModule(currModule);
            same = currModule == *it;
            ++it;
        }
        // Delete and disconnect all expanders after the first one that is different from expanders
        disconnectExpanders<T>(it, expanders->end());

        // Then add and connect expanders from here until currModule == nullptr
        // or if we try to add the same model twice
        while (currModule) {
            if ((connectedModels.find(currModule->model) == connectedModels.end()) &&
                connectExpander(currModule)) {
                connectedModels.insert(currModule->model);
                currModule = nextModule(currModule);
            }
            else {
                break;
            }
        }
        // while (currModule) {
        //     if (connectExpander(currModule)) { currModule = nextModule(currModule); }
        //     else {
        //         break;
        //     }
        // }
        onUpdateExpanders(std::is_same<T, RightExpander>::value);
    }

   protected:
    // Double Buffer begin
    std::vector<F>& readBuffer()
    {
        return *voltages[0];
    }
    std::vector<F>& writeBuffer()
    {
        return *voltages[1];
    }

    template <typename Adapter>
    void transform(Adapter& adapter)
    {
        if (adapter) {
            writeBuffer().resize(16);
            if (adapter.inPlace(readBuffer().size(), 0)) {
                adapter.transformInPlace(readBuffer().begin(), readBuffer().end(), 0);
            }
            else {
                auto newEnd = adapter.transform(readBuffer().begin(), readBuffer().end(),
                                                writeBuffer().begin(), 0);
                const int outputLength = std::distance(writeBuffer().begin(), newEnd);
                writeBuffer().resize(outputLength);
                swap();
                assert((outputLength <= 16) && (outputLength >= 0));  // NOLINT
            }
            adapter.refresh();
        }
    }

   private:
    /// @brief Buffers for adapters to operate on
    /// @details The buffers are swapped when the operation could not take place in place.
    std::vector<F> v1, v2;
    std::array<std::vector<F>*, 2> voltages{&v1, &v2};
    void swap()
    {
        std::swap(voltages[0], voltages[1]);
    }
};

}  // namespace biexpand