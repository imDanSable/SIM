#pragma once
#include <functional>
#include <rack.hpp>
#include <utility>
#include "Connectable.hpp"
#include "constants.hpp"
#include "plugin.hpp"

class ModuleX : public Connectable {
   public:
    ModuleX(const ModelsListType& leftAllowedModels,
            const ModelsListType& rightAllowedModels,
            int leftLightId,
            int rightLightId);
    ModuleX(const ModuleX& other) = delete;
    ModuleX& operator=(const ModuleX& other) = delete;
    ModuleX(ModuleX&& other) = delete;
    ModuleX& operator=(ModuleX&& other) = delete;
    ~ModuleX() override;
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
    ChainChangeCallbackType chainChangeCallback = nullptr;
};

template <typename T>
class BaseAdapter {
   public:
    BaseAdapter() : ptr_(nullptr) {}
    virtual ~BaseAdapter() = default;
    explicit operator bool() const
    {
        return ptr_ != nullptr;
    }
    explicit operator T*() const
    {
        return ptr_;
    }

    T* operator->() const
    {
        return ptr_;
    }

    void setPtr(T* ptr)
    {
        this->ptr_ = ptr;
    }

   protected:
    T* ptr_;
};
