#pragma once

#include <Box2D/Common/b2Math.h>

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <string>

namespace polygons
{

using Color = b2Vec4;

float colorDistance(const Color& aa, const Color& bb);

using Poly = std::vector<b2Vec2>;

struct PolyHasher
{
    size_t operator()(const Poly& poly) const;
};


using PolyToColors = std::unordered_map<Poly, Color, PolyHasher>;

}

