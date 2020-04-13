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
    float ship_screen_height;
    b2Vec2 ship_spawn;
    b2Vec2 ball_spawn;
    b2Vec2 crate_spawn;
    b2Vec2 water_spawn;
    b2Vec2 water_drop_size;
};

struct MainData
{
    using LevelDatas = std::vector<LevelData>;
    LevelDatas levels;
    int default_level;
};

MainData load(const std::string& json_filename);

}
