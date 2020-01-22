#include "GameState.h"

#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_circle_shape.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_contact.h"

#include <iostream>

using std::cout;
using std::endl;

GameState::GameState() :
    world(b2Vec2(0, -8)),
    ground(nullptr),
    ship(nullptr),
    left_side(nullptr), right_side(nullptr),
    ball(nullptr),
    joint(nullptr),
    ship_firing(false),
    ship_target_angular_velocity(0),
    ship_target_angle(0),
    ship_touched_anything(false)
{
    cout << "init game state" << endl;

    { // ground
        b2BodyDef def;
        def.type = b2_staticBody;
        def.position.Set(0, -10);

        b2PolygonShape shape;
        shape.SetAsBox(50, 10);

        b2FixtureDef fixture;
        fixture.shape = &shape;
        fixture.density = 0;
        fixture.friction = .3;

        auto body = world.CreateBody(&def);
        body->CreateFixture(&fixture);
        ground = body;
    }

    { // left side
        b2BodyDef def;
        def.type = b2_staticBody;
        def.position.Set(-40, 0);

        b2PolygonShape shape;
        shape.SetAsBox(10, 100);

        b2FixtureDef fixture;
        fixture.shape = &shape;
        fixture.density = 0;
        fixture.friction = .3;

        auto body = world.CreateBody(&def);
        body->CreateFixture(&fixture);
        left_side = body;
    }

    { // right side
        b2BodyDef def;
        def.type = b2_staticBody;
        def.position.Set(40, 0);

        b2PolygonShape shape;
        shape.SetAsBox(10, 100);

        b2FixtureDef fixture;
        fixture.shape = &shape;
        fixture.density = 0;
        fixture.friction = .3;

        auto body = world.CreateBody(&def);
        body->CreateFixture(&fixture);
        right_side = body;
    }

    { // ship
        b2BodyDef def;
        def.type = b2_dynamicBody;
        def.position.Set(0, 20);
        def.angle = M_PI / 4 - 2e-2;

        b2PolygonShape shape;
        static const b2Vec2 points[3] {
            { -1, 0 },
            { 1, 0 },
            { 0, 2 }
        };
        shape.Set(points, 3);

        b2FixtureDef fixture;
        fixture.shape = &shape;
        fixture.density = 1;
        fixture.friction = .3;
        fixture.restitution = .4;

        auto body = world.CreateBody(&def);
        body->CreateFixture(&fixture);
        ship = body;
    }

    { // ball
        b2BodyDef def;
        def.type = b2_dynamicBody;
        def.position.Set(0, 1);
        def.angle = 0;
        def.angularVelocity = 0;

        b2CircleShape shape;
        shape.m_radius = 1.;

        b2FixtureDef fixture;
        fixture.shape = &shape;
        fixture.density = .5;
        fixture.friction = .3;
        fixture.restitution = .4;

        auto body = world.CreateBody(&def);
        body->CreateFixture(&fixture);
        ball = body;
    }

    world.SetContactListener(this);
}

void GameState::grab()
{
    assert(!joint);
    assert(ship);
    assert(ball);

    b2DistanceJointDef def;
    def.Initialize(ship, ball, ship->GetWorldCenter(), ball->GetWorldCenter());
    def.frequencyHz = 25.;
    def.dampingRatio = 1.;
    def.collideConnected = true;

    joint = static_cast<b2DistanceJoint*>(world.CreateJoint(&def));
}

void GameState::release()
{
    assert(joint);
    assert(ship);
    assert(ball);
    world.DestroyJoint(joint);
    joint = nullptr;
}

bool GameState::canGrab() const
{
    if (joint)
        return false;

    assert(ship);
    assert(ball);
    const auto delta = ship->GetWorldCenter() - ball->GetWorldCenter();
    return std::abs(delta.Length() - 7.5) < 1;
}

void GameState::step(const float dt)
{
    int velocityIterations = 6;
    int positionIterations = 2;
    const auto angle = ship->GetAngle();
    if (ship_firing) ship->ApplyForceToCenter(100.f * b2Rot(angle).GetYAxis(), true);
    ship_target_angle += ship_target_angular_velocity * dt;
    ship->SetAngularVelocity((ship_target_angle - angle) / .1);
    world.Step(dt, velocityIterations, positionIterations);
    world.ClearForces();
}

void GameState::BeginContact(b2Contact* contact)
{
    assert(contact);

    const auto* aa = contact->GetFixtureA()->GetBody();
    assert(aa);
    const bool aa_is_ship = aa == ship;
    const bool aa_is_ball = aa == ball;

    const auto* bb = contact->GetFixtureB()->GetBody();
    assert(bb);
    const bool bb_is_ship = bb == ship;
    const bool bb_is_ball = bb == ball;

    const bool any_ship = aa_is_ship || bb_is_ship;
    const bool any_ball = aa_is_ball || bb_is_ball;

    ship_touched_anything |= any_ship;
}

GameState::~GameState()
{
    world.SetContactListener(nullptr);
    world.DestroyBody(ship);
    world.DestroyBody(ground);
}
