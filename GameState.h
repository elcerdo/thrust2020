#pragma once

#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_distance_joint.h"

#include <memory>

struct GameState : public b2ContactListener
{
    GameState();
    ~GameState();

    void step(const float dt);
    bool canGrab() const;
    void grab();
    void release();

    void BeginContact(b2Contact* contact) override;

    b2World world;
    b2Body* ground;
    b2Body* ship;
    b2Body* left_side;
    b2Body* right_side;
    b2Body* ball;
    b2DistanceJoint* joint;

    bool ship_firing;
    double ship_target_angular_velocity;
    double ship_target_angle;
    bool ship_touched_anything;
};

