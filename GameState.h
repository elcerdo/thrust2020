#pragma once

#include "Box2D/Dynamics/b2World.h"
#include "Box2D/Dynamics/b2Body.h"
#include "Box2D/Dynamics/Joints/b2DistanceJoint.h"
#include "Box2D/Dynamics/Joints/b2PrismaticJoint.h"
#include "Box2D/Particle/b2ParticleSystem.h"

#include <memory>
#include <vector>
#include <random>
#include <functional>

struct GameState : public b2ContactListener
{
    GameState();
    ~GameState();

    void step(const float dt);
    void dumpCollisionData() const;

    bool canGrab() const;
    bool isGrabbed() const;
    void grab();
    void release();

    void addCrate(const b2Vec2 pos, const b2Vec2 velocity, const double angle);
    void clearCrates();

    void addWater(const b2Vec2 pos, const b2Vec2 size, const size_t seed, const uint flags);
    void clearWater();

    void addDoor(const b2Vec2 pos, const b2Vec2 size, const b2Vec2 delta);
    void addPath(const std::vector<b2Vec2>& positions, const b2Vec2 size);

    void resetShip();
    void resetBall();
    void resetParticleSystem();
    void resetGround(const std::string& map_filename);

    void BeginContact(b2Contact* contact) override;

    using UniqueBody = std::unique_ptr<b2Body, std::function<void(b2Body*)>>;
    using UniqueDistanceJoint = std::unique_ptr<b2DistanceJoint, std::function<void(b2Joint*)>>;
    using UniqueSystem = std::unique_ptr<b2ParticleSystem, std::function<void(b2ParticleSystem*)>>;

    b2World world;
    UniqueBody ground;
    UniqueBody ship;
    UniqueBody ball;

    std::vector<UniqueBody> crates;
    std::vector<std::tuple<UniqueBody, std::vector<b2Vec2>, size_t>> doors;
    UniqueDistanceJoint link;

    UniqueSystem system;

    bool ship_firing;
    double ship_target_angular_velocity;
    double ship_target_angle;
    bool ship_touched_wall;
    double ship_thrust_factor;
    int ship_accum_contact;
    int all_accum_contact;
    double all_energy;
};

