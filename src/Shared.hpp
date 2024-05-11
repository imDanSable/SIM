#pragma once
#include <limits>
#include <rack.hpp>

std::string getFractionalString(float value, int numerator, int denominator);
float getVoctFromNote(const std::string& noteName, float onErrorVal);
std::string getNoteFromVoct(int rootNote, bool majorScale, int noteNumber);

/**
 * Wraps a value into a specified range.
 *
 * This function takes a value `v` and two range boundaries `lo` and `hi`, and adjusts `v` so that
 * it falls within the range `[lo, hi)`. If `v` is outside the range, it will be adjusted by adding
 * or subtracting multiples of the range length (`hi - lo`) until it falls within the range. The
 * relative distance of `v` from `lo` or `hi` will be maintained.
 *
 * @param v The value to be wrapped.
 * @param lo The lower boundary of the range.
 * @param hi The upper boundary of the range.
 * @return The wrapped value, which will be in the range `[lo, hi)`.
 */
inline float wrap(float v, float lo, float hi)
{
    assert(lo < hi && "Calling wrap with wrong arguments");
    float range = hi - lo;
    float result = rack::eucMod(v - lo, range);
    return result + lo;
}

template <typename T>
inline T limit(T v, T max)
{
    return std::min(v, max - std::numeric_limits<T>::epsilon());
}


/// @brief returns the fractional part of a float
inline float frac(float v)
{
    return v - std::floor(v);
};