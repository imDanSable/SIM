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
    private:
        MyAdapter adapter;
};
```

*/

#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <iterator>
#include <map>
#include <set>
#include <type_traits>
#include <vector>
#include "ModuleInstantiationMenu.hpp"
#include "rack.hpp"
#include "sigslot/signal.hpp"

namespace biexpand {
class Connectable : public rack::engine::Module {
   public:
    void setLeftLightId(int id)
    {
        leftLightId = id;
    }

    void setRightLightId(int id)
    {
        rightLightId = id;
    }

    // XXX remove addDefaultConnectionLights
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
            rack::createLightCentered<rack::TinySimpleLight<rack::componentlibrary::GreenLight>>(
                mm2px(Vec((x_offset), y_offset)), this, leftLightId));
        const float width_px = (widget->box.size.x);

        Vec vec = mm2px(Vec(-x_offset, y_offset));
        vec.x += width_px;
        widget->addChild(
            rack::createLightCentered<rack::TinySimpleLight<rack::componentlibrary::GreenLight>>(
                vec, this, rightLightId));
        setLight(true, false);
        setLight(false, false);
    }

    void setLight(bool isRight, bool state)
    {
        if (isRight && rightLightId != -1) { lights[rightLightId].value = state ? 1.0F : 0.0F; }
        else if (!isRight && leftLightId != -1) {
            lights[leftLightId].value = state ? 1.0F : 0.0F;
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

   private:
    bool beingRemoved = false;
    int leftLightId = -1;
    int rightLightId = -1;
};

enum class ExpanderSide { LEFT, RIGHT };
template <ExpanderSide SIDE>
class BiExpander : public Connectable {
   public:
    void onRemove() override
    {
        setBeingRemoved();
        changeSignal();
        // if (changeSignal.slot_count() != 0) { changeSignal.disconnect_all(); }
        assert(changeSignal.slot_count() == 0);
    }

    void onAdd(const AddEvent& e) override;  // forward declaration

    void onExpanderChange(const ExpanderChangeEvent& e) override
    {
        constexpr bool isRightSide = (SIDE == ExpanderSide::RIGHT);
        // XXX I don't trust this. Shouldn't it be like so?
        // auto& currentExpander = isRightSide ? this->leftExpander : this->rightExpander;
        auto& currentExpander = isRightSide ? this->rightExpander : this->leftExpander;
        auto& prevModule = isRightSide ? prevRightModule : prevLeftModule;

        if ((isRightSide && e.side) || (!isRightSide && !e.side)) {
            if (prevModule != currentExpander.module) {
                if (changeSignal.slot_count()) { changeSignal(); }
                prevModule = currentExpander.module;
            }
        }
    }

   private:
    friend class Expandable;
    sigslot::signal_st<> changeSignal;
    Module* prevLeftModule = nullptr;
    Module* prevRightModule = nullptr;
};

using LeftExpander = BiExpander<ExpanderSide::LEFT>;
using RightExpander = BiExpander<ExpanderSide::RIGHT>;

class Adapter {
   public:
    virtual ~Adapter() = default;
    virtual explicit operator bool() const = 0;

    virtual void setPtr(void* ptr) = 0;
};
// template <typename TExpander>
// class BaseAdapter : public Adapter {
//    public:
//     BaseAdapter() : ptr(nullptr) {}
//     explicit operator bool() const
//     {
//         return ptr.load(std::memory_order_acquire) != nullptr;
//     }
//     explicit operator TExpander*() const
//     {
//         return ptr.load(std::memory_order_acquire);
//     }
//     TExpander* operator->() const
//     {
//         return ptr.load(std::memory_order_acquire);
//     }
//     void setPtr(void* ptr) override
//     {
//         this->ptr.store(static_cast<TExpander*>(ptr), std::memory_order_release);
//     }

//    protected:
//     std::atomic<TExpander*> ptr;  // NOLINT
// };

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

   protected:
    T* ptr;  // NOLINT
};

// XXX Also needs some better way to deal with left/right.
using AdapterMap = std::map<rack::Model*, Adapter*>;
class Expandable : public Connectable {
   public:
    Expandable(const AdapterMap& leftAdapters, const AdapterMap& rightAdapters)
        : leftModelsAdapters(leftAdapters), rightModelsAdapters(rightAdapters){};

    // size_t getLeftExpanderCount() const
    // {
    //     return leftExpanders.size();
    // }
    // size_t getRightExpanderCount() const
    // {
    //     return rightExpanders.size();
    // }
    // void onAdd(const AddEvent& e) override
    // {
    //     // if (prevRightModule != this->rightExpander.module) {
    //         refreshExpanders<RightExpander>();
    //         // prevRightModule = this->rightExpander.module;
    //     // }
    //     // if (prevLeftModule != this->leftExpander.module) {
    //         refreshExpanders<LeftExpander>();
    //         // prevLeftModule = this->leftExpander.module;
    //     // }
    // }
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
    virtual void onUpdateExpanders(bool isRight){};

    // template <class T>
    // bool hasExpander(T* expander) const
    // {
    //     if constexpr (std::is_same<T, RightExpander>::value) {
    //         return std::find(rightExpanders.begin(), rightExpanders.end(), expander) !=
    //                rightExpanders.end();
    //     }
    //     else {
    //         return std::find(leftExpanders.begin(), leftExpanders.end(), expander) !=
    //                leftExpanders.end();
    //     }
    // }
    // template <class T>
    // bool hasModel(rack::Model* model) const
    // {
    //     if constexpr (std::is_same<T, RightExpander>::value) {
    //         for (const auto& rightExpander : rightExpanders) {
    //             if (rightExpander->model == model) { return true; }
    //         }
    //     }
    //     else {
    //         for (const auto& leftExpander : leftExpanders) {
    //             if (leftExpander->model == model) { return true; }
    //         }
    //     }
    // }

    template <class T>
    bool connectExpander(T* expander)
    {
        constexpr bool side = std::is_same<T, RightExpander>::value;
        const auto* cadapters = side ? &rightModelsAdapters : &leftModelsAdapters;
        auto adapter = cadapters->find(expander->model);
        if (adapter == cadapters->end()) { return false; }

        if constexpr (side) {
            if (std::find(rightExpanders.begin(), rightExpanders.end(), expander) !=
                rightExpanders.end()) {
                return false;
            }
            rightExpanders.push_back(expander);
            rightAdapters.push_back(adapter->second);
            // XXX Should be checkless. Make assert
            if (expander->changeSignal.slot_count() == 0) {
                expander->changeSignal.connect(&Expandable::refreshExpanders<RightExpander>, this);
            }
        }
        else {
            if (std::find(leftExpanders.begin(), leftExpanders.end(), expander) !=
                leftExpanders.end()) {
                return false;
            }
            leftExpanders.push_back(expander);
            leftAdapters.push_back(adapter->second);
            if (expander->changeSignal.slot_count() == 0) {
                expander->changeSignal.connect(&Expandable::refreshExpanders<LeftExpander>, this);
            }
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
            expander->changeSignal.disconnect_all();

            expander->setLight(true, false);
            auto adapter = leftModelsAdapters.find(expander->model);
            assert(adapter != leftModelsAdapters.end());
            leftExpanders.erase(std::remove(leftExpanders.begin(), leftExpanders.end(), expander),
                                leftExpanders.end());
            leftAdapters.erase(
                std::remove(leftAdapters.begin(), leftAdapters.end(), adapter->second),
                leftAdapters.end());
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
            if (rightExpanders.empty()) {
                setLight(true, false);
            }
        }
        else if constexpr (std::is_same<T, LeftExpander>::value) {
            if (leftExpanders.empty()) {
                setLight(false, false);
            }
        }
    }

    rack::MenuItem* createExpandableSubmenu(rack::ModuleWidget* moduleWidget)
    {
        return rack::createSubmenuItem("Add Expander", "", [=](rack::Menu* menu) {
            for (auto compatible : leftModelsAdapters) {
                auto* item =
                    new ModuleInstantionMenuItem();  // NOLINT(cppcoreguidelines-owning-memory)
                item->text = "Add " + compatible.first->name;
                item->rightText = "←";
                item->module_widget = moduleWidget;
                item->right = false;
                item->model = compatible.first;
                menu->addChild(item);
            }
            for (auto compatible : rightModelsAdapters) {
                auto* item =
                    new ModuleInstantionMenuItem();  // NOLINT(cppcoreguidelines-owning-memory)
                item->text = "Add " + compatible.first->name;
                item->rightText = "→";
                item->module_widget = moduleWidget;
                item->right = true;
                item->model = compatible.first;
                menu->addChild(item);
            }
        });
    };

    std::vector<Adapter*> getLeftAdapters() const
    {
        return leftAdapters;
    }
    std::vector<Adapter*> getRightAdapters() const
    {
        return rightAdapters;
    }

   private:
    friend LeftExpander;
    friend RightExpander;
    Module* prevLeftModule = nullptr;
    Module* prevRightModule = nullptr;
    // vector of pointers to expanders that represents the order of expanders
    // and that is used to keep track of changes in the expander chain
    std::vector<LeftExpander*> leftExpanders;
    std::vector<RightExpander*> rightExpanders;
    // map for compatible models and pointers to adapters
    const AdapterMap leftModelsAdapters;
    const AdapterMap rightModelsAdapters;
    // vector of pointers to adapters that represents the order of expanders
    std::vector<Adapter*> leftAdapters;
    std::vector<Adapter*> rightAdapters;

    /// Will traverse the expander chain and update the expanders.
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
            ++it;
            same = currModule == *it;
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
        // XXX if (!isBeingRemoved()) used to prevent a crash.
        // Since we used erase(range) instead of clear in onRemove() this is no longer necessary.
        // Leaving it here for now in case we need it again.
        // if (!isBeingRemoved()) { onUpdateExpanders(std::is_same<T, RightExpander>::value); }
        onUpdateExpanders(std::is_same<T, RightExpander>::value);
    }
};

///////////////////Implementation/////////////////////
template <ExpanderSide SIDE>
void BiExpander<SIDE>::onAdd(const AddEvent& /*e*/)
{
    // DEBUG("leftExpander.module->id %ld", leftExpander.moduleId);
    // DEBUG("rightExpander.module->id %ld", rightExpander.moduleId);
    // Find our old expander by id on or right if were left and vice versa.
    // Traverse the chain until we find an expandable and ask it to update its expanders.
    // Our callee void Engine::addModule() already has a lock so we'll use the NoLock version of
    // getModule()

    // I believe this used to work, but isn't now. I disabled it and look at it later.

    Module* module{};
    std::function<Module*(Module*)> nextModule;
    if (SIDE == ExpanderSide::LEFT) {
        if (rightExpander.moduleId <= 0) { return; }
        module = APP->engine->getModule_NoLock(rightExpander.moduleId);
        nextModule = [](Module* m) { return m->rightExpander.module; };
    }
    else {
        if (leftExpander.moduleId <= 0) { return; }
        module = APP->engine->getModule_NoLock(leftExpander.moduleId);
        nextModule = [](Module* m) { return m->leftExpander.module; };
    }
    for (;; module = nextModule(module)) {
        if (!module) { return; }
        if (auto* expandable = dynamic_cast<Expandable*>(module)) {
            if constexpr (SIDE == ExpanderSide::LEFT) {
                expandable->leftExpander.module = static_cast<Module*>(this);
                expandable->leftExpander.moduleId = this->id;
                expandable->onExpanderChange(ExpanderChangeEvent{false});
                this->onExpanderChange(ExpanderChangeEvent{true});

                expandable->refreshExpanders<LeftExpander>();
                expandable->refreshExpanders<RightExpander>();
                return;
            }
            else {
                // UNTESTED: since we don't have more that one right expander.
                expandable->rightExpander.module = static_cast<Module*>(this);
                expandable->rightExpander.moduleId = this->id;
                expandable->onExpanderChange(ExpanderChangeEvent{true});
                this->onExpanderChange(ExpanderChangeEvent{false});

                expandable->refreshExpanders<RightExpander>();
                expandable->refreshExpanders<LeftExpander>();
                return;
            }
        }
        auto* connectable = dynamic_cast<Connectable*>(module);
        if (!connectable) { return; }
    }
}
}  // namespace biexpand