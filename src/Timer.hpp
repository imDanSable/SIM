#pragma once
#include <chrono>
#include "rack.hpp"

class Profiler {
   public:
    Profiler(const std::string& name)
        : m_name(name), m_start(std::chrono::high_resolution_clock::now())
    {
    }  // NOLINT

    ~Profiler()
    {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> duration = end - m_start;
        DEBUG("%s took %.6f µs", m_name.c_str(), duration.count());
    }

   private:
    std::string m_name;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};
