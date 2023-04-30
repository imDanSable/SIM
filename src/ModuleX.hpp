#pragma once
#include <functional>
#include <rack.hpp>
#include <utility>
#include "Connectable.hpp"
#include "constants.hpp"
#include "plugin.hpp"

// To create create a new compatible expander you need to subclass ModuleX:
//
// class MyModuleX : public ModuleX;
//
// You also need to provide an adapter for it:
//
// MyAdapter : public BaseAdapter<MyModuleX>
//
// And implement an interface to be used by modules derived from Expandable
// Typically your adapter member functions will check if (ptr == nullptr), which indicates whether
// ModuleX is connected or not, and respond accordingly.
// Say you have a  randomize expander you
// could for example do this:
//
// MyAdapter::randomize(iterator begin, iterator end)
// {
//     if (!ptr) { return; }
//     for (auto it = begin; it != end; ++it) {
//         doRandomize(*it);
// }
//
// And then in your module:
//
// void MyModuleX::process(const ProcessArgs& args)
// {
//     ...
//     adapter.randomize(some.begin(), some.end());
//     ...
// }
//
// The adapter functions similar to an optional.
// You can have direct unchecked access to the Module's pointer
// using the -> operator or by a cast.
//
// And you can weather it is connected using the bool operator.
//
// NOTE: You should not access any member variables of ModuleX directly.
// So don't define ModuleX::process()
// All access should be done by the Expandable module through the adapter.
//
class ModuleX : public Connectable {
   public:
    ModuleX(bool isOutputExpander,
            const ModelsListType& leftAllowedModels,
            const ModelsListType& rightAllowedModels,
            int leftLightId,
            int rightLightId);
    ModuleX(const ModuleX& other) = delete;
    ModuleX& operator=(const ModuleX& other) = delete;
    ModuleX(ModuleX&& other) = delete;
    ModuleX& operator=(ModuleX&& other) = delete;
    ~ModuleX() override;
    void onRemove() override;
    void onExpanderChange(const engine::Module::ExpanderChangeEvent& e) override;

    struct ChainChangeEvent {};
    using ChainChangeCallbackType = std::function<void(const ChainChangeEvent& e)>;

    void setChainChangeCallback(ChainChangeCallbackType callback)
    {
        chainChangeCallback = std::move(callback);
    }

    ChainChangeCallbackType getChainChangeCallback() const
    {
        return chainChangeCallback;
    }

   private:
    bool isOutputExpander;
    // XXX Wrap chainChangeCallback in atomic or use a mutex or use a safe queue or use a lock-free
    // double buffer?
    ChainChangeCallbackType chainChangeCallback = nullptr;
};

template <typename T>
class BaseAdapter {
   public:
    BaseAdapter() : ptr(nullptr) {}
    // virtual ~BaseAdapter()
    // {
    //     ptr = nullptr;
    // };

    explicit operator bool() const
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

    void setPtr(T* ptr)
    {
        this->ptr = ptr;
    }

   protected:
    T* ptr;  // NOLINT
};
