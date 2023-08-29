#pragma once
#include <chrono>
#include "rack.hpp"
class Timer {
    using high_resolution_clock = std::chrono::high_resolution_clock;
    using milliseconds = std::chrono::milliseconds;

   public:
    explicit Timer(bool run = false)
    {
        if (run) { Reset(); }
    }

    void Reset()
    {
        _start = high_resolution_clock::now();
    }

    milliseconds Elapsed() const
    {
        return std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - _start);
    }

    template <typename T, typename Traits>
    friend std::basic_ostream<T, Traits>& operator<<(std::basic_ostream<T, Traits>& out,
                                                     const Timer& timer)
    {
        return out << timer.Elapsed().count();
    }

   private:
    high_resolution_clock::time_point _start;
};