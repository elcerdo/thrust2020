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

    void addCrate(const b2Vec2& pos, const b2Vec2& velocity, const double angle, const int tag);
    void clearCrates();

    void addWater(const b2Vec2& pos, const b2Vec2& size, const size_t seed, const unsigned int flags);
    void clearWater(const int group_count);

    void addDoor(const b2Vec2& pos, const b2Vec2& size, const b2Vec2& delta);
    void addPath(const std::vector<b2Vec2>& positions, const b2Vec2& size);

    void resetShip(const b2Vec2& pos);
    void resetBall(const b2Vec2& pos);
    void resetParticleSystem();
    void resetGround(const std::string& map_filename);

    void BeginContact(b2Contact* contact) override;

    using UniqueBody = std::unique_ptr<b2Body, std::function<void(b2Body*)>>;
    using UniqueDistanceJoint = std::unique_ptr<b2DistanceJoint, std::function<void(b2Joint*)>>;
    using UniqueSystem = std::unique_ptr<b2ParticleSystem, std::function<void(b2ParticleSystem*)>>;

    static constexpr float ship_scale = 1.8;
    static constexpr float ball_scale = 5;
    static constexpr float crate_scale = 3.5;

    b2World world = b2World({ 0, -8 });
    UniqueBody ground = nullptr;
    UniqueBody ship = nullptr;
    UniqueBody ball = nullptr;
    UniqueDistanceJoint link = nullptr;
    UniqueSystem system = nullptr;

    std::vector<std::tuple<UniqueBody, int>> crates;
    std::vector<std::tuple<UniqueBody, std::vector<b2Vec2>, size_t>> doors;

    struct ShipState
    {
        bool firing_thruster = false;
        bool turning_left = false;
        bool turning_right = false;
        float turning_left_time = 0;
        float turning_right_time = 0;
        bool touched_wall = false;
        float target_angle = 0;
        float thrust_factor = 1;
        unsigned int accum_contact = 0;
    };

    ShipState ship_state;

    unsigned int all_accum_contact = 0;
    bool clean_stuck_in_door = true;
};

