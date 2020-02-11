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
    ship_touched_wall(false)
{
    cout << "init game state" << endl;

    { // ground
        b2BodyDef def;
        def.type = b2_staticBody;
        def.position.Set(0, -100);

        b2PolygonShape shape;
        shape.SetAsBox(600, 100);

        b2FixtureDef fixture;
        fixture.shape = &shape;
        fixture.density = 0;
        fixture.friction = .9;

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
        def.position.Set(90, 0);

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

        constexpr float ww = 1.8;
        b2PolygonShape shape;
        static const b2Vec2 points[3] {
            { -ww, 0 },
            { ww, 0 },
            { 0, 2*ww }
        };
        shape.Set(points, 3);

        b2FixtureDef fixture;
        fixture.shape = &shape;
        fixture.density = .1;
        fixture.friction = .7;
        fixture.restitution = .1;

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
        shape.m_radius = 5.;

        b2FixtureDef fixture;
        fixture.shape = &shape;
        fixture.density = .1;
        fixture.friction = 1;
        fixture.restitution = 0.1;

        auto body = world.CreateBody(&def);
        body->CreateFixture(&fixture);
        ball = body;
    }

    { // crate tower
        constexpr int nn = 40;
        constexpr float ww = 2;

        for (auto jj=nn; jj; jj--)
        for (auto ii=0; ii<jj; ii++)
        {
            const float xx = 5. + ww * ii - jj + nn;
            addCrate({ xx, 2.5f * ( 1 + nn - jj) }, { 0, 0 }, 0);
        }
    }

    world.SetContactListener(this);
}

void GameState::grab()
{
    assert(!isGrabbed());
    assert(ship);
    assert(ball);

    b2DistanceJointDef def;
    def.Initialize(ship, ball, ship->GetWorldCenter(), ball->GetWorldCenter());
    def.frequencyHz = 25.;
    def.dampingRatio = .0;
    def.collideConnected = true;

    joint = static_cast<b2DistanceJoint*>(world.CreateJoint(&def));
}

void GameState::release()
{
    assert(isGrabbed());
    assert(ship);
    assert(ball);
    world.DestroyJoint(joint);
    joint = nullptr;
}

bool GameState::canGrab() const
{
    if (isGrabbed())
        return false;

    assert(ship);
    assert(ball);
    const auto delta = ship->GetWorldCenter() - ball->GetWorldCenter();
    const double ll = delta.Length();
    return ll > 10 && ll < 25;
}

bool GameState::isGrabbed() const {
    return joint != nullptr;
}

void GameState::step(const float dt)
{
    int velocityIterations = 6;
    int positionIterations = 2;
    const auto angle = ship->GetAngle();
    const auto thrust = (isGrabbed() ? 120. : 20.) * b2Rot(angle).GetYAxis();
    if (ship_firing) ship->ApplyForceToCenter(thrust, true);
    ship_target_angle += ship_target_angular_velocity * dt;
    ship->SetAngularVelocity((ship_target_angle - angle) / .05);
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
    const bool aa_is_wall = aa == left_side || aa == right_side || aa == ground;

    const auto* bb = contact->GetFixtureB()->GetBody();
    assert(bb);
    const bool bb_is_ship = bb == ship;
    const bool bb_is_ball = bb == ball;
    const bool bb_is_wall = bb == left_side || bb == right_side || bb == ground;

    const bool any_ship = aa_is_ship || bb_is_ship;
    const bool any_ball = aa_is_ball || bb_is_ball;
    const bool any_wall = aa_is_wall || bb_is_wall;

    ship_touched_wall |= any_ship && any_wall;
}

void GameState::addCrate(const b2Vec2 pos, const b2Vec2 velocity, const double angle)
{
    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.position.Set(pos.x, pos.y);
    def.angle = angle;

    constexpr float zz = 1.2;
    b2PolygonShape shape;
    static const b2Vec2 points[4] {
        { -zz, -zz },
        { zz, -zz },
        { zz, zz },
        { -zz, zz },
    };
    shape.Set(points, 4);

    b2FixtureDef fixture;
    fixture.shape = &shape;
    fixture.density = .001;
    fixture.friction = .8;
    fixture.restitution = .7;

    auto crate = world.CreateBody(&def);
    crate->CreateFixture(&fixture);
    crate->SetLinearVelocity(velocity);

    crates.push_back(crate);
}


GameState::~GameState()
{
    world.SetContactListener(nullptr);
    //world.DestroyBody(ship);
    //world.DestroyBody(ground);
    //if (joint) world.DestroyJoint(joint);
}
