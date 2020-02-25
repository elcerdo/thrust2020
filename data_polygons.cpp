#include "data_polygons.h"

#include <boost/functional/hash.hpp>

float polygons::colorDistance(const Color& aa, const Color& bb)
{
    const auto xx = std::fabs(aa.x - bb.x);
    const auto yy = std::fabs(aa.y - bb.y);
    const auto zz = std::fabs(aa.z - bb.z);
    const auto ww = std::fabs(aa.w - bb.w);
    return std::max(std::max(xx, yy), std::max(zz, ww));
}

size_t polygons::PolyHasher::operator()(const Poly& poly) const
{
    size_t seed = 0x1fac1e5b;
    for (const auto& point : poly)
    {
        boost::hash_combine(seed, point.x);
        boost::hash_combine(seed, point.y);
    }
    return seed;
}

