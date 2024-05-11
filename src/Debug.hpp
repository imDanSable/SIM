#pragma once
#include <chrono>
#include <rack.hpp>
#include <sstream>
#include <utility>

namespace dbg {

const int dbgDivide = 100;
const std::string DebugStreamOutputPath = "./plugins/SIM/build/";
class DebugStream {
    std::ostringstream oss;
    std::string file_name;

   public:
    explicit DebugStream(std::string filename) : file_name(std::move(filename)) {}
    ~DebugStream()
    {
        std::string str = oss.str();
        std::vector<uint8_t> data(str.begin(), str.end());
        rack::system::writeFile(DebugStreamOutputPath + file_name, data);
    }
    template <typename T>
    DebugStream& operator<<(const T& message)
    {
        oss << message;
        return *this;
    }

    // Overload for std::endl
    DebugStream& operator<<(
        std::ostream& (*pf)(std::ostream&))  // cppcheck-suppress constParameterReference
    {
        oss << pf;
        return *this;
    }

    void clear()
    {
        // Create an empty buffer
        std::vector<uint8_t> emptyBuffer;

        // Write the empty buffer to the file, effectively clearing it
        rack::system::writeFile(file_name, emptyBuffer);
    }
};
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

struct DebugDivider : rack::dsp::ClockDivider {
    explicit DebugDivider(int division)
    {
        setDivision(division);
    }
    template <typename... Args>
    void operator()(const std::string& dbg, Args&&... args)
    {
        if (process()) { DEBUG(dbg.c_str(), std::forward<Args>(args)...); }
    }
    template <typename... Args>
    void operator()(int division, const std::string& dbg, Args&&... args)
    {
        setDivision(division);
        if (process()) { DEBUG(dbg.c_str(), std::forward<Args>(args)...); }
    }
};

extern DebugDivider dbg;  // NOLINT

}  // namespace dbg