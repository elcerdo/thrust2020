#pragma once

#include <Box2D/Common/b2Math.h>

#include <vector>
#include <string>

namespace levels
{

struct LevelData
{
    using DoorData = std::tuple<b2Vec2, b2Vec2, b2Vec2>;
    std::string name;
    std::string map_filename;
    std::vector<DoorData> doors;
};

using LevelDatas = std::vector<LevelData>;

LevelDatas load(const std::string& json_filename);

}
