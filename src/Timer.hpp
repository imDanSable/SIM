#pragma once
#include <chrono>
#include <utility>
#include "logger.hpp"

class Profiler {
   public:
    explicit Profiler(std::string name)
        : m_name(std::move(name)), m_start(std::chrono::high_resolution_clock::now())
    {
    }  // NOLINT

    ~Profiler()
    {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> duration = end - m_start;
        DEBUG("%s took %.6f µs", m_name.c_str(), duration.count());  // NOLINT
    }

   private:
    std::string m_name;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};
