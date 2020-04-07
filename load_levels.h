#pragma once

#include <Box2D/Common/b2Math.h>

#include <vector>
#include <string>

namespace levels
{

struct LevelData
{
    using DoorData = std::tuple<b2Vec2, b2Vec2, b2Vec2>;
    using PathData = std::tuple<std::vector<b2Vec2>, b2Vec2>;
    std::string name;
    std::string map_filename;
    std::vector<DoorData> doors;
    std::vector<PathData> paths;
    b2Vec2 world_camera_position;
    float world_screen_height;
};

struct MainData
{
    using LevelDatas = std::vector<LevelData>;
    LevelDatas levels;
    int default_level;
};

MainData load(const std::string& json_filename);

}
