
#pragma once
#include <vector>
#include "rack.hpp"

namespace iters {

using FloatIter = std::vector<float>::iterator;
using BoolIter = std::vector<bool>::iterator;


// Example usage:
// auto it = CircularIterator(first, last, start, length);
// while (it != true)) {
//     // Use *it to access the current element
//     ++it;
// }
template <typename Iterator>
class CircularIterator {
   public:
    CircularIterator(Iterator first, Iterator last, size_t start, size_t length)
        : first(first), last(last), curr(first + start), length(length)
    {
    }

    typename Iterator::value_type operator*() const
    {
        return *curr;
    }

    CircularIterator& operator++()
    {
        ++curr;
        ++count;
        if (curr == last) { curr = first; }
        return *this;
    }

    bool operator!=(const CircularIterator& other) const
    {
        return count < length;
    }

   private:
    Iterator first;
    Iterator last;
    Iterator curr;
    size_t length;
    size_t count{};
};

// Example usage:
// CircularRange range(first, last, start, length);
// for (auto value : range) {
//     // Use value
// }
template <typename Iterator>
class CircularRange {
   public:
    CircularRange(Iterator first, Iterator last, size_t start, size_t length)
        : beginIterator(first, last, start, length), endIterator(first, last, start, length)
    {
    }

    CircularIterator<Iterator> begin() const
    {
        return beginIterator;
    }

    CircularIterator<Iterator> end() const
    {
        return endIterator;
    }

   private:
    CircularIterator<Iterator> beginIterator;
    CircularIterator<Iterator> endIterator;
};

class PortVoltageIterator {
   private:
    float* it;

   public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = float;
    using difference_type = std::ptrdiff_t;
    using pointer = float*;
    using reference = float&;

    PortVoltageIterator(float* it) : it(it) {}  // NOLINT

    float& operator*()
    {
        return *it;
    }
    const float& operator*() const
    {
        return *it;
    }
    PortVoltageIterator& operator++()
    {
        ++it;  // NOLINT
        return *this;
    }
    PortVoltageIterator operator++(int)  // NOLINT
    {
        PortVoltageIterator tmp(*this);
        ++(*this);
        return tmp;
    }
    PortVoltageIterator& operator--()
    {
        --it;  // NOLINT
        return *this;
    }
    bool operator==(const PortVoltageIterator& rhs) const
    {
        return it == rhs.it;
    }
    bool operator!=(const PortVoltageIterator& rhs) const
    {
        return it != rhs.it;
    }
    // Add addition/subtraction
    PortVoltageIterator operator+(int n) const
    {
        return {it + n};  // NOLINT
    }
    PortVoltageIterator operator-(int n) const
    {
        return {it - n};  // NOLINT
    }
    PortVoltageIterator& operator+=(int n)
    {
        it += n;  // NOLINT
        return *this;
    }
    PortVoltageIterator& operator-=(int n)
    {
        it -= n;  // NOLINT
        return *this;
    }
    // Add less/greater than comparison
    bool operator<(const PortVoltageIterator& other) const
    {
        return it < other.it;
    }
    bool operator>(const PortVoltageIterator& other) const
    {
        return it > other.it;
    }
    bool operator<=(const PortVoltageIterator& other) const
    {
        return it <= other.it;
    }
    bool operator>=(const PortVoltageIterator& other) const
    {
        return it >= other.it;
    }
    // Add offset dereference
    const float& operator[](difference_type n) const
    {
        return *(it + n);  // NOLINT
    }
    float& operator[](difference_type n)
    {
        return *(it + n);  // NOLINT
    }
    difference_type operator-(const PortVoltageIterator& other) const
    {
        return it - other.it;
    }
};
using rack::engine::Param;
class ParamIterator {
   private:
    std::vector<Param>::const_iterator it;

   public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = float;
    using difference_type = std::ptrdiff_t;
    using pointer = float*;
    using reference = float&;
    ParamIterator(std::vector<Param>::const_iterator it) : it(it) {}  // NOLINT
    float& operator*()
    {
        return const_cast<float&>(it->value);  // NOLINT
    }
    const float& operator*() const
    {
        return it->value;
    }

    ParamIterator& operator++()
    {
        ++it;
        return *this;
    }
    ParamIterator& operator--()
    {
        --it;
        return *this;
    }
    // const ParamIterator operator++(int)  // NOLINT
    // {
    //     ParamIterator tmp(*this);
    //     ++(*this);
    //     return tmp;
    // }
    bool operator==(const ParamIterator& rhs) const
    {
        return it == rhs.it;
    }
    bool operator!=(const ParamIterator& rhs) const
    {
        return it != rhs.it;
    }
    // Add addition/subtraction
    ParamIterator operator+(int n) const
    {
        return {it + n};
    }
    ParamIterator operator-(int n) const
    {
        return {it - n};
    }
    ParamIterator& operator+=(int n)
    {
        it += n;
        return *this;
    }
    ParamIterator& operator-=(int n)
    {
        it -= n;
        return *this;
    }
    // Add less/greater than comparison
    bool operator<(const ParamIterator& other) const
    {
        return it < other.it;
    }
    bool operator>(const ParamIterator& other) const
    {
        return it > other.it;
    }
    bool operator<=(const ParamIterator& other) const
    {
        return it <= other.it;
    }
    bool operator>=(const ParamIterator& other) const
    {
        return it >= other.it;
    }
    // Add offset dereference
    const float& operator[](difference_type n) const
    {
        return (it + n)->value;
    }
    difference_type operator-(const ParamIterator& other) const
    {
        return it - other.it;
    }
};

// XXX We'll implement std::conditional to make output ports consts later if needed
template <class PortType>
class PortIterator {
   private:
    typename std::vector<PortType>::iterator it;

   public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = PortType;
    using difference_type = std::ptrdiff_t;
    using pointer = PortType*;
    using reference = PortType&;
    using const_reference = const PortType&;
    PortIterator(typename std::vector<PortType>::iterator it) : it(it) {}  // NOLINT
    reference operator*()
    {
        return *it;
    }
    const_reference operator*() const
    {
        return *it;
    }
    PortIterator operator++()
    {
        ++it;
        return *this;
    }
    PortIterator operator--()
    {
        --it;
        return *this;
    }
    pointer operator->()
    {
        return &(*it);
    }
    bool operator==(const PortIterator& rhs) const
    {
        return it == rhs.it;
    }
    bool operator!=(const PortIterator rhs) const
    {
        return it != rhs.it;
    }
    // Add addition/subtraction
    PortIterator operator+(int n) const
    {
        return {it + n};
    }
    PortIterator operator-(int n) const
    {
        return {it - n};
    }
    PortIterator& operator+=(int n)
    {
        it += n;
        return *this;
    }
    PortIterator& operator-=(int n)
    {
        it -= n;
        return *this;
    }
    // Add less/greater than comparison
    bool operator<(const PortIterator& other) const
    {
        return it < other.it;
    }
    bool operator>(const PortIterator& other) const
    {
        return it > other.it;
    }
    bool operator<=(const PortIterator& other) const
    {
        return it <= other.it;
    }
    bool operator>=(const PortIterator& other) const
    {
        return it >= other.it;
    }
    // Add offset dereference
    reference operator[](difference_type n) const
    {
        return (it + n);
    }
    difference_type operator-(const PortIterator& other) const
    {
        return it - other.it;
    }
};
// typedef  iters::PortIterator<rack::engine::Input> InputIterator; // NOLINT
// typedef  iters::PortIterator<rack::engine::Output> OutputIterator; // NOLINT
}  // namespace iters
