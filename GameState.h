#pragma once

#include "Box2D/Dynamics/b2World.h"
#include "Box2D/Dynamics/b2Body.h"
#include "Box2D/Dynamics/Joints/b2DistanceJoint.h"
#include "Box2D/Dynamics/Joints/b2PrismaticJoint.h"
#include "Box2D/Particle/b2ParticleSystem.h"

#include <memory>
#include <vector>
#include <random>

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
    void addDoor(const b2Vec2 pos, const b2Vec2 size, const b2Vec2 direction);
    void flop();

    void BeginContact(b2Contact* contact) override;

    b2World world;
    b2Body* ground;
    b2Body* ship;
    b2Body* ball;

    std::vector<b2Body*> crates;
    std::vector<std::tuple<b2Body*, b2PrismaticJoint*>> doors;
    b2DistanceJoint* joint;

    b2ParticleSystem* system;

    bool ship_firing;
    double ship_target_angular_velocity;
    double ship_target_angle;
    bool ship_touched_wall;
    double ship_thrust_factor;
    int ship_accum_contact;
    int all_accum_contact;
    double all_energy;

    std::default_random_engine color_rng;
};

