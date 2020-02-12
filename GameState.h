#pragma once

#include "Box2D/Dynamics/b2World.h"
#include "Box2D/Dynamics/b2Body.h"
#include "Box2D/Dynamics/Joints/b2DistanceJoint.h"
#include "Box2D/Particle/b2ParticleSystem.h"

#include <memory>
#include <vector>

struct GameState : public b2ContactListener
{
    GameState();
    ~GameState();

    void step(const float dt);
    bool canGrab() const;
    bool isGrabbed() const;
    void grab();
    void release();
    void addCrate(const b2Vec2 pos, const b2Vec2 velocity, const double angle);

    void BeginContact(b2Contact* contact) override;

    b2World world;
    b2Body* ground;
    b2Body* ship;
    b2Body* left_side;
    b2Body* right_side;
    b2Body* ball;

    std::vector<b2Body*> crates;
    b2DistanceJoint* joint;

    b2ParticleSystem* system;

    bool ship_firing;
    double ship_target_angular_velocity;
    double ship_target_angle;
    bool ship_touched_wall;
    double ship_thrust_factor;
    int accum_contact;
};

