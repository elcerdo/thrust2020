#pragma once

#include "box2d/b2_world.h"
#include "box2d/b2_body.h"

#include <memory>

struct GameState
{
    GameState();
    ~GameState();

    void step(const float dt);

    b2World world;
    b2Body* ground;
    b2Body* ship;
};

