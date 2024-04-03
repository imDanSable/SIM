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
#ifdef DEBUGSTATE
#include <iostream>
#endif
#include <iterator>
#include <map>
#include <rack.hpp>
#include <set>
#include <utility>
#include <vector>
#include "../Debug.hpp"
#include "CacheState.hpp"
#include "ConnectionLights.hpp"
#include "ModuleInstantiationMenu.hpp"
#include "sigslot/signal.hpp"
namespace biexpand {
#define DEBUGSTATE

#ifdef DEBUGSTATE
class DebugState {
   public:
    DebugState()
    {
        instances.push_back(this);  // Add this instance to the collection
    }

    virtual ~DebugState()
    {
        instances.erase(std::remove(instances.begin(), instances.end(), this),
                        instances.end());  // Remove this instance from the collection
    }

    virtual void DebugStateReport() {};

    static void DebugStateReportAll()
    {
        for (DebugState* instance : instances) {
            instance->DebugStateReport();
        }
    }

   private:
    inline static std::vector<DebugState*> instances = {};  // Collection of all instances
};
class Connectable : public rack::engine::Module, public DebugState {
#else
class Connectable : public rack::engine::Module {
#endif
   public:
    Connectable() : connectionLights(this), cacheState(this) {}

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

    bool isBeingRemoved() const
    {
        return beingRemoved;
    }
    void setBeingRemoved()
    {
        beingRemoved = true;
    }

    /// @brief Invalidate the cache of a connectable when a port changes
    void onPortChange(const rack::Module::PortChangeEvent& e) override
    {
        // XXX This could be more precise (like params and inputs we ignore but opposite)
        cacheState.setDirty();
    }

    ConnectionLights connectionLights;  // NOLINT
    CacheState cacheState;              // NOLINT

   private:
    bool beingRemoved = false;
};
// forward declaration of specilized templates for BiExpander friendship
template <typename F>
class Expandable;

class BiExpander : public Connectable {
   public:
#ifdef DEBUGSTATE
    void DebugStateReport() override
    {
        dbg::DebugStream dbg(model->name + ".txt");
        dbg << "BiExpander: " << model->name << "(" << imright << ")" << std::endl;
        dbg << "Slot count: " << changeSignal.slot_count() << std::endl;
    }
#endif
    explicit BiExpander(bool right) : imright(right) {}
    void onRemove() override
    {
        DEBUG("BiExpander(%s)::onRemove", model->name.c_str());
        setBeingRemoved();
        changeSignal(imright);
        assert(changeSignal.slot_count() == 0);
    }

    void onExpanderChange(const ExpanderChangeEvent& e) override
    {
        DEBUG("BiExpander(%s)::onExpanderChange %s", model->name.c_str(),
              std::to_string(e.side).c_str());
        if ((imright && e.side) || (!imright && !e.side)) {
            auto& currentExpander = imright ? this->rightExpander : this->leftExpander;
            auto& prevModule = imright ? prevRightModule : prevLeftModule;
            if (prevModule != currentExpander.module) {
                if (changeSignal.slot_count()) {
                    DEBUG("    Sending signal %s", std::to_string(imright).c_str());
                    changeSignal(imright);
                }
                prevModule = currentExpander.module;
            }
        }
    }

   private:
    bool imright;
    friend class Expandable<bool>;
    friend class Expandable<float>;
    sigslot::signal_st<bool> changeSignal;
    Module* prevLeftModule = nullptr;
    Module* prevRightModule = nullptr;
};

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
    virtual void transformInPlace(FloatIter first, FloatIter last, int channel) const {}
    virtual void transformInPlace(BoolIter first, BoolIter last, int channel) const {}
    virtual void setDirty() = 0;
    virtual bool needsRefresh() const = 0;
    virtual void refresh() = 0;
};

template <typename T>
class BaseAdapter : public Adapter {
   public:
    BaseAdapter() : ptr(nullptr), boolTransformFunction(nullptr), floatTransformFunction(nullptr) {}
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
        ptr->cacheState.setDirty();
    }
    bool needsRefresh() const override
    {
        return ptr->cacheState.needsRefreshing();
    }
    void refresh() override
    {
        ptr->cacheState.refresh();
    }

    void setBoolValueFunction(std::function<bool(bool)> func)
    {
        floatTransformFunction = std::move(func);
    }
    void setFloatValueFunction(std::function<float(float)> func)
    {
        floatTransformFunction = std::move(func);
    }
    std::function<bool(bool)> getBoolValueFunction() const
    {
        return boolTransformFunction;
    }
    std::function<float(float)> getFloatValueFunction() const
    {
        return floatTransformFunction;
    }

   protected:
    T* ptr;  // NOLINT
   private:
    std::function<bool(bool)> boolTransformFunction;
    std::function<float(float)> floatTransformFunction;
};

using AdapterMap = std::map<rack::Model*, Adapter*>;

/// @brief Expandable is a module that can have expanders attached to it.
/// @param F is the underlying datatype (float or bool for now)
template <typename F>
class Expandable : public Connectable {
   public:
    Expandable(AdapterMap leftAdapters, AdapterMap rightAdapters)
        : leftModelsAdapters(std::move(leftAdapters)), rightModelsAdapters(std::move(rightAdapters))
    {
        v1.resize(16);
        v2.resize(16);
    };
#ifdef DEBUGSTATE
    void DebugStateReport() override
    {
        dbg::DebugStream dbg(model->name + ".txt");
        dbg << "Expandable: " << model->name << std::endl;
        dbg << "Left expanders:";
        if (leftExpanders.empty()) { dbg << "None" << std::endl; }
        else {
            dbg << "[";
            for (auto it = leftExpanders.begin(); it != leftExpanders.end(); ++it) {
                dbg << (*it)->model->name;
                if (std::next(it) != leftExpanders.end()) { dbg << ", "; }
            }
            dbg << "]" << std::endl;
        }
        if (rightExpanders.empty()) { dbg << "None" << std::endl; }
        else {
            dbg << "Right expanders: [";
            for (auto it = rightExpanders.begin(); it != rightExpanders.end(); ++it) {
                dbg << (*it)->model->name;
                if (std::next(it) != rightExpanders.end()) { dbg << ", "; }
            }
            dbg << "]" << std::endl;
        }

        dbg << "Left adapters: " << leftAdapters.size() << std::endl;
        dbg << "Right adapters: " << rightAdapters.size() << std::endl;
    }
#endif
    void onRemove() override
    {
        DEBUG("BiExpander(%s)::onRemove", model->name.c_str());
        setBeingRemoved();
        disconnectExpanders(true, rightExpanders.begin(), rightExpanders.end());
        disconnectExpanders(false, leftExpanders.begin(), leftExpanders.end());
    }
    void onExpanderChange(const ExpanderChangeEvent& e) override
    {
        DEBUG("Expandable(%s)::onExpanderChange %s", model->name.c_str(),
              std::to_string(e.side).c_str());
        if (e.side) {  //&& (prevRightModule != this->rightExpander.module)) {
            refreshExpanders(true);
            prevRightModule = this->rightExpander.module;
        }
        else if (!e.side) {  //&& (prevLeftModule != this->leftExpander.module)) {
            refreshExpanders(false);
            prevLeftModule = this->leftExpander.module;
        }
    };

    /// @brief Override onUpdateExpanders instead of onExpanderChange() if you need to do something
    /// when the expander chain, or to avoid the VCV Bug (<=v2.4.1) where onExpanderChange is not
    /// called when the expander is deleted.
    virtual void onUpdateExpanders(bool isRight) {};

    bool connectExpander(bool right, BiExpander* expander)
    {
        DEBUG("Expandable(%s)::connectExpander %s", model->name.c_str(),
              std::to_string(right).c_str());
        const auto* cadapters = right ? &rightModelsAdapters : &leftModelsAdapters;
        auto adapter = cadapters->find(expander->model);
        if (adapter == cadapters->end()) { return false; }  // Not compatible
        auto* expanders = right ? &rightExpanders : &leftExpanders;
        auto* adapters = right ? &rightAdapters : &leftAdapters;
        if (std::find(expanders->begin(), expanders->end(), expander) != expanders->end()) {
            return false;
        }
        expanders->push_back(expander);
        adapters->push_back(adapter->second);
        assert(expander->changeSignal.slot_count() == 0);
        expander->changeSignal.connect(&Expandable::refreshExpanders, this);
        connectionLights.setLight(right, true);
        expander->connectionLights.setLight(!right, true);
        adapter->second->setPtr(expander);
        return true;
    }

    void disconnectExpander(bool right, BiExpander* expander)
    {
        DEBUG("Expandable(%s)::disconnectExpander side: %d", model->name.c_str(), right);
        auto* prevModule = right ? &prevRightModule : &prevLeftModule;
        auto* expanders = right ? &rightExpanders : &leftExpanders;
        auto* adapters = right ? &rightAdapters : &leftAdapters;
        auto* modelsAdapters = right ? &rightModelsAdapters : &leftModelsAdapters;
        *prevModule = nullptr;
        // Tell the disconnected module to disconnect from the expandable
        expander->changeSignal.disconnect_all();
        // Turn off the light
        expander->connectionLights.setLight(!right, false);
        // Remove the expander from our list
        expanders->erase(std::remove(expanders->begin(), expanders->end(), expander),
                         expanders->end());
        // find and remove the adapter from the list
        auto adapter = right ? rightModelsAdapters.find(expander->model)
                             : leftModelsAdapters.find(expander->model);
        adapters->erase(std::remove(adapters->begin(), adapters->end(), adapter->second),
                        adapters->end());
        // Set the adapter to nullptr
        auto it = modelsAdapters->find(expander->model);
        if (it != modelsAdapters->end()) { it->second->setPtr(nullptr); }
    }

    void disconnectExpanders(bool right,
                             typename std::vector<BiExpander*>::iterator begin,
                             typename std::vector<BiExpander*>::iterator end)
    {
        DEBUG("Expandable(%s)::disconnectExpanders side: %d", model->name.c_str(), right);
        // Create a copy of the pointers to the expanders
        std::vector<BiExpander*> expandersCopy(begin, end);
        DEBUG("span size: %zu", expandersCopy.size());
        // Disconnect each expander (which will remove themselves from the vectors and update
        // the adapters to empty)
        for (BiExpander* expander : expandersCopy) {
            DEBUG("About to disconnect %s", expander->model->name.c_str());
            disconnectExpander(right, expander);
        }
        if (rightExpanders.empty()) { connectionLights.setLight(right, false); }
    }

    std::vector<BiExpander*> getLeftExpanders() const
    {
        return leftExpanders;
    }
    std::vector<BiExpander*> getRightExpanders() const
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

    rack::MenuItem* createExpandableSubmenu(rack::ModuleWidget* moduleWidget)
    {
        const auto* expandable = dynamic_cast<Expandable*>(moduleWidget->module);
        assert(expandable != nullptr);
        auto hasModel = [expandable](bool right, const std::string& name) {
            auto expanders =
                right ? expandable->getRightExpanders() : expandable->getLeftExpanders();
            return std::any_of(expanders.begin(), expanders.end(),
                               [name](Module* module) { return module->model->name == name; });
        };
        bool hasExpanders =
            !expandable->getLeftExpanders().empty() || !expandable->getRightExpanders().empty();
        return rack::createSubmenuItem(
            "Add Expander", "", [this, moduleWidget, hasModel, hasExpanders](rack::Menu* menu) {
                for (auto compatible : leftModelsAdapters) {
                    auto* item = new ModuleInstantionMenuItem();
                    item->text = "Add " + compatible.first->name;
                    item->rightText = "←";
                    item->module_widget = moduleWidget;
                    item->right = false;
                    item->model = compatible.first;
                    menu->addChild(item);
                    if (hasModel(false, compatible.first->name)) { item->disabled = true; }
                }
                for (auto compatible : rightModelsAdapters) {
                    auto* item = new ModuleInstantionMenuItem();
                    item->text = "Add " + compatible.first->name;
                    item->rightText = "→";
                    item->module_widget = moduleWidget;
                    item->right = true;
                    item->model = compatible.first;
                    menu->addChild(item);
                    if (hasModel(true, compatible.first->name)) { item->disabled = true; }
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

   protected:
    bool dirtyAdapters()
    {
        if (std::any_of(leftAdapters.begin(),
                        leftAdapters.end(),  // Use the stored variable here
                        [](const auto* adapter) { return adapter->needsRefresh(); })) {
            return true;
        }
        if (std::any_of(rightAdapters.begin(), rightAdapters.end(),
                        [](const auto* adapter) { return adapter->needsRefresh(); })) {
            return true;
        }
        return false;
    }

   private:
    friend BiExpander;
    Module* prevLeftModule = nullptr;
    Module* prevRightModule = nullptr;
    /// @brief vector of pointers to expanders that represents the order of expanders and that
    /// is used to keep track of changes in the expander chain
    std::vector<BiExpander*> leftExpanders;
    std::vector<BiExpander*> rightExpanders;
    // map for compatible models and pointers to adapters
    AdapterMap leftModelsAdapters;
    AdapterMap rightModelsAdapters;
    /// @brief vector of pointers to adapters that represents the order of expanders
    std::vector<Adapter*> leftAdapters;
    std::vector<Adapter*> rightAdapters;

    /// @brief Will traverse the expander chain and update the expanders.
    void refreshExpanders(bool right)
    {
        DEBUG("Expandable(%s)::refreshExpanders side: %d", model->name.c_str(), right);
        std::vector<BiExpander*>* expanders = nullptr;
        std::function<BiExpander*(Connectable*)> nextModule;

        nextModule = [this, right](Connectable* currModule) -> BiExpander* {
            auto* expander = right ? dynamic_cast<BiExpander*>(currModule->rightExpander.module)
                                   : dynamic_cast<BiExpander*>(currModule->leftExpander.module);
            if (!expander || expander->isBeingRemoved()) { return nullptr; }
            auto* modelsAdapters = right ? &rightModelsAdapters : &leftModelsAdapters;
            auto it = modelsAdapters->find(expander->model);
            if (it == modelsAdapters->end()) { return nullptr; }
            return expander;
        };
        expanders = right ? &rightExpanders : &leftExpanders;
        BiExpander* currModule = nextModule(dynamic_cast<Connectable*>(this));
        // Don't touch the expanders that are unchanged
        // Keep list of connected models to avoid connecting the same model twice
        std::set<rack::plugin::Model*>
            connectedModels;  // IMPROVE: we could reuse the expanders vector icw any_of.
        auto it = expanders->begin();
        bool same = !expanders->empty() && currModule == *it;
        DEBUG("Checking sameness");
        while (currModule && it != expanders->end() && same) {
            connectedModels.insert(currModule->model);
            currModule = nextModule(currModule);
            same = currModule == *it;
            if (currModule) {
                DEBUG("Model: %s is %s", currModule->model->name.c_str(),
                      same ? "same" : "not same");
            }
            ++it;
        }
        DEBUG("Done Checking sameness");
        // Delete and disconnect all expanders after the first one that is different from
        // expanders
        if (it != expanders->end()) {
            DEBUG("About to disconnect from %s to %s", (*it)->model->name.c_str(),
                  expanders->back()->model->name.c_str());
            disconnectExpanders(right, it, expanders->end());
        }

        // Then add and connect expanders from here until currModule == nullptr
        while (currModule) {
            if ((connectedModels.find(currModule->model) == connectedModels.end()) &&
                connectExpander(right, currModule)) {
                connectedModels.insert(currModule->model);
                currModule = nextModule(currModule);
            }
            else {
                break;
            }
        }
        // If this side is empty, turn off the light
        if (expanders->empty()) { connectionLights.setLight(right, false); }
        onUpdateExpanders(right);
#ifdef DEBUGSTATE
        DEBUG("Done refreshing expanders THE FINAL LIST IS:");
        for (auto* expander : *expanders) {
            DEBUG("    %s", expander->model->name.c_str());
        }
#endif
    }

   protected:
    // Buffer&transform section
    std::vector<F>& readBuffer() const
    {
        return *voltages[0];
    }
    std::vector<F>& writeBuffer()
    {
        return *voltages[1];
    }
    template <typename Adapter>
    void transform(Adapter& adapter, std::function<void(F)> func = [](F) {})
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
    std::array<std::vector<F>*, 3> voltages{&v1, &v2};
    void swap()
    {
        std::swap(voltages[0], voltages[1]);
    }
    // end Buffer&Transform section
};

}  // namespace biexpand