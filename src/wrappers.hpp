#pragma once
#include <cmath>

namespace wrappers {

#ifndef M_PI
#define M_PI 3.14159265358979323846; /* pi */
#endif
#ifndef M_2PI
#define M_2PI 6.283185307179586231941716828464095101  // 2pi
#endif
#ifndef M_1_2PI
#define M_1_2PI 0.159154943091895345554011992339482617  // 1/(2pi)
#endif

/// Returns value wrapped in [lo, hi).
template <class T>
T wrap(T value, T hi = T(1), T lo = T(0));

/// Returns value wrapped in [lo, hi).

/// 'numWraps' reports how many wrappings occured where the sign, + or -,
/// signifies above 'hi' or below 'lo', respectively.
template <class T>
T wrap(T value, int& numWraps, T hi = T(1), T lo = T(0));

/// Returns value incremented by 1 and wrapped into interval [0, max).
template <class T>
T wrapAdd1(T v, T max)
{
    ++v;
    return v == max ? 0 : v;
}

/// Like wrap(), but only adds or subtracts 'hi' once from value.
template <class T>
T wrapOnce(T value, T hi = T(1));

template <class T>
T wrapOnce(T value, T hi, T lo);

template <class T>
T wrapPhase(T radians);  ///< Returns value wrapped in [-pi, pi)
template <class T>
T wrapPhaseOnce(T radians);  ///< Like wrapPhase(), but only wraps once

// ========================

template <class T>
inline T wrap(T v, T hi, T lo)
{
    if (lo == hi) { return lo; }

    // if(v >= hi){
    if (!(v < hi)) {
        T diff = hi - lo;
        v -= diff;
        if (!(v < hi)) { v -= diff * static_cast<T>(static_cast<unsigned>((v - lo) / diff)); }
    }
    else if (v < lo) {
        T diff = hi - lo;
        v += diff;  // this might give diff if range is too large, so check at end of block...
        if (v < lo) { v += diff * static_cast<T>(static_cast<unsigned>(((lo - v) / diff) + 1)); }
        if (v == diff) { return std::nextafter(v, lo); }
    }
    return v;
}

template <class T>
inline T wrap(T v, int& numWraps, T hi, T lo)
{
    if (lo == hi) {
        numWraps = 0xFFFFFFFF;
        return lo;
    }

    T diff = hi - lo;
    numWraps = 0;

    if (v >= hi) {
        v -= diff;
        if (v >= hi) {
            numWraps = int((v - lo) / diff);
            v -= diff * T(numWraps);
        }
        ++numWraps;
    }
    else if (v < lo) {
        v += diff;
        if (v < lo) {
            numWraps = int((v - lo) / diff) - 1;
            v -= diff * T(numWraps);
        }
        --numWraps;
    }
    return v;
}

template <class T>
inline T wrapOnce(T v, T hi)
{
    if (v >= hi) { return v - hi; }
    if (v < T(0)) { return v + hi; }
    return v;
}

template <class T>
inline T wrapOnce(T v, T hi, T lo)
{
    if (v >= hi) { return v - hi + lo; }
    if (v < lo) { return v + hi - lo; }
    return v;
}

template <class T>
inline T wrapPhase(T r)
{
    // The result is		[r+pi - 2pi floor([r+pi] / 2pi)] - pi
    // which simplified is	r - 2pi floor([r+pi] / 2pi) .
    if (r >= T(M_PI)) {
        r -= T(M_2PI);
        if (r < T(M_PI)) { return r; }
        return r - T(int((r + M_PI) * M_1_2PI)) * T(M_2PI);
    }
    if (r < T(-M_PI)) {
        r += T(M_2PI);
        if (r >= T(-M_PI)) { return r; }
        return r - T(int((r + M_PI) * M_1_2PI) - 1) * T(M_2PI);
    }
    return r;
}

template <class T>
inline T wrapPhaseOnce(T r)
{
    if (r >= T(M_PI)) { return r - T(M_2PI); }
    if (r < T(-M_PI)) { return r + T(M_2PI); }
    return r;
}
}  // namespace wrappers