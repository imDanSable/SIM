#pragma once
#include <algorithm>
#include <iterator>
#include <vector>
#include "Connectable.hpp"
#include "plugin.hpp"

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

    explicit ModuleIterator(pointer ptr) : ptr_(ptr) {}

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

   protected:
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

   protected:
    pointer ptr_;
};

class ConnectableIterator : public ModuleIterator {
   public:
    using ModuleIterator::ModuleIterator;

    // Default constructor for the end iterator
    ConnectableIterator() : ModuleIterator(nullptr), is_end_iterator_(true) {}

    ConnectableIterator& operator++()
    {
        do {
            ModuleIterator::operator++();
            if (ptr_ == nullptr || !dynamic_cast<Connectable*>(&**this)) {
                is_end_iterator_ = true;
                break;
            }
        } while (!dynamic_cast<Connectable*>(&**this));
        return *this;
    }

    ConnectableIterator operator++(int)
    {
        ConnectableIterator temp(*this);
        operator++();
        return temp;
    }

    // Override the equality operator to consider is_end_iterator_
    bool operator==(const ConnectableIterator& other) const
    {
        return is_end_iterator_ == other.is_end_iterator_ && ptr_ == other.ptr_;
    }

    // Override the inequality operator to consider is_end_iterator_
    bool operator!=(const ConnectableIterator& other) const
    {
        return !(*this == other);
    }

   private:
    bool is_end_iterator_ = false;
};

class ConnectableReverseIterator : public ModuleReverseIterator {
public:
    using ModuleReverseIterator::ModuleReverseIterator;

    // Default constructor for the end iterator
    ConnectableReverseIterator() : ModuleReverseIterator(nullptr), is_end_iterator_(true) {}

    ConnectableReverseIterator& operator++() {
        do {
            ModuleReverseIterator::operator++();
            if (ptr_ == nullptr || !dynamic_cast<Connectable*>(&**this)) {
                is_end_iterator_ = true;
                break;
            }
        } while (!dynamic_cast<Connectable*>(&**this));
        return *this;
    }

    ConnectableReverseIterator& operator--() {
        do {
            ModuleReverseIterator::operator--();
            if (ptr_ == nullptr || !dynamic_cast<Connectable*>(&**this)) {
                is_end_iterator_ = true;
                break;
            }
        } while (!dynamic_cast<Connectable*>(&**this));
        return *this;
    }
   private:
    bool is_end_iterator_ = false;
};
