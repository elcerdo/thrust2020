#include "data_polygons.h"

#include <boost/functional/hash.hpp>

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

