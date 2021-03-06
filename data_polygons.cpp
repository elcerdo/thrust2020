#include "data_polygons.h"
#include <algorithm>

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

float colorDistance(const polygons::Color& aa, const polygons::Color& bb)
{
    const auto xx = std::fabs(aa.x - bb.x);
    const auto yy = std::fabs(aa.y - bb.y);
    const auto zz = std::fabs(aa.z - bb.z);
    const auto ww = 0.f; //std::fabs(aa.w - bb.w);
    return std::max(std::max(xx, yy), std::max(zz, ww));
}

bool polygons::isForeground(const Color& aa)
{
    const polygons::Color foreground_color { 0, 0, 0, 1 };
    return colorDistance(aa, foreground_color) == 0;
}

size_t polygons::PolyHasher::operator()(const Poly& poly) const
{
    size_t seed = 0x1fac1e5b;
    for (const auto& point : poly)
    {
        hash_combine(seed, point.x);
        hash_combine(seed, point.y);
    }
    return seed;
}

