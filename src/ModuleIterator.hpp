#pragma once
#include <algorithm>
#include <iterator>
#include <vector>

class ModelPredicate {
   public:
    explicit ModelPredicate(Model* targetModel) : targetModel_(targetModel) {}

    bool operator()(const Module& module) const
    {
        return module.model == targetModel_;
    }

   private:
    Model* targetModel_;
};
/// @brief True while it find a target model
class AnyOf {
   public:
    explicit AnyOf(const std::vector<Model*>& models) : models_(models) {}

    bool operator()(const Module& module) const
    {
        return std::find(models_.begin(), models_.end(), module.model) != models_.end();
    }

   private:
    std::vector<Model*> models_;
};
/// @brief True while it doesn't find a target model
class NoneOf {
   public:
    explicit NoneOf(const std::vector<Model*>& models) : models_(models) {}

    bool operator()(const Module& module) const
    {
        return std::find(models_.begin(), models_.end(), module.model) == models_.end();
    }

   private:
    std::vector<Model*> models_;
};

class ModuleIterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = Module;
    using difference_type = std::ptrdiff_t;
    using pointer = Module*;
    using reference = Module&;

    ModuleIterator(pointer ptr) : ptr_(ptr) {}

    // Dereference operator
    reference operator*() const
    {
        return *static_cast<pointer>(ptr_);
    }
    pointer operator->()
    {
        return static_cast<pointer>(ptr_);
    }

    // Pre-increment operator
    ModuleIterator& operator++()
    {
        if (ptr_) ptr_ = static_cast<pointer>(ptr_->rightExpander.module);
        return *this;
    }

    // Pre-decrement operator
    ModuleIterator& operator--()
    {
        if (ptr_) ptr_ = static_cast<pointer>(ptr_->leftExpander.module);
        return *this;
    }

    // Post-increment operator
    ModuleIterator operator++(int)
    {
        ModuleIterator tmp(*this);
        ++(*this);
        return tmp;
    }

    // Post-decrement operator
    ModuleIterator operator--(int)
    {
        ModuleIterator tmp(*this);
        --(*this);
        return tmp;
    }

    // Equality operator
    bool operator==(const ModuleIterator& other) const
    {
        return ptr_ == other.ptr_;
    }

    // Inequality operator
    bool operator!=(const ModuleIterator& other) const
    {
        return ptr_ != other.ptr_;
    }

   private:
    pointer ptr_;
};
class ModuleReverseIterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = Module;
    using difference_type = std::ptrdiff_t;
    using pointer = Module*;
    using reference = Module&;

    ModuleReverseIterator(Module* ptr) : ptr_(static_cast<pointer>(ptr)) {}

    // Dereference operator
    reference operator*() const
    {
        return *ptr_;
    }
    pointer operator->()
    {
        return ptr_;
    }

    // Pre-increment operator
    ModuleReverseIterator& operator++()
    {
        if (ptr_) ptr_ = static_cast<pointer>(ptr_->leftExpander.module);
        return *this;
    }

    // Pre-decrement operator
    ModuleReverseIterator& operator--()
    {
        if (ptr_) ptr_ = static_cast<pointer>(ptr_->rightExpander.module);
        return *this;
    }

    // Post-increment operator
    ModuleReverseIterator operator++(int)
    {
        ModuleReverseIterator tmp(*this);
        ++(*this);
        return tmp;
    }

    // Post-decrement operator
    ModuleReverseIterator operator--(int)
    {
        ModuleReverseIterator tmp(*this);
        --(*this);
        return tmp;
    }

    // Equality operator
    bool operator==(const ModuleReverseIterator& other) const
    {
        return ptr_ == other.ptr_;
    }

    // Inequality operator
    bool operator!=(const ModuleReverseIterator& other) const
    {
        return ptr_ != other.ptr_;
    }

   private:
    pointer ptr_;
};