#pragma once

#include "box2d/b2_world.h"
#include "box2d/b2_body.h"

#include <memory>

struct GameState
{
    GameState();
    ~GameState();

    void step(const float dt);
    bool canGrab() const;

    b2World world;
    b2Body* ground;
    b2Body* ship;
    b2Body* left_side;
    b2Body* right_side;
    b2Body* ball;
    b2Joint* joint;

    bool ship_firing;
    double ship_target_angular_velocity;
    double ship_target_angle;
};

