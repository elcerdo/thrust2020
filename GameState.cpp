#include "GameState.h"

#include "Box2D/Dynamics/b2Fixture.h"
#include "Box2D/Collision/Shapes/b2PolygonShape.h"
#include "Box2D/Collision/Shapes/b2CircleShape.h"
#include "Box2D/Dynamics/Contacts/b2Contact.h"
#include "Box2D/Particle/b2ParticleGroup.h"

#include <iostream>
#include <bitset>

#include "extract_polygons.h"
#include "decompose_polygons.h"

using std::cout;
using std::endl;

constexpr uint16 ground_category = 1 << 0;
constexpr uint16 object_category = 1 << 1;
constexpr uint16 door_category = 1 << 2;

GameState::GameState() :
    world(b2Vec2(0, -8)),
    ground(nullptr),
    ship(nullptr),
    ball(nullptr),
    joint(nullptr),
    system(nullptr),
    ship_firing(false),
    ship_target_angular_velocity(0),
    ship_target_angle(0),
    ship_touched_wall(false),
    ship_thrust_factor(1.),
    ship_accum_contact(0),
    all_accum_contact(0),
    all_energy(0),
    color_rng(0x12345678)
{
    cout << "init game state" << endl;

    { // ground
        b2BodyDef def;
        def.type = b2_staticBody;
        def.position.Set(0, 0);
        auto body = world.CreateBody(&def);

        const auto push_fixture = [&body](const polygons::Poly& poly)
        {
            b2PolygonShape shape;
            shape.Set(poly.data(), poly.size());

            b2FixtureDef fixture;
            fixture.shape = &shape;
            fixture.density = 0;
            fixture.friction = .9;
            fixture.filter.categoryBits = ground_category;
            fixture.filter.maskBits = object_category | door_category;

            body->CreateFixture(&fixture);

        };

        const auto polys_to_colors = polygons::extract(":map.svg");
        const polygons::Color foreground_color { 0, 1, 0, 1};

        const auto foreground_transform = [](const polygons::Poly& poly) -> polygons::Poly
        {
            polygons::Poly poly_;
            poly_.reserve(poly.size());
            for (const auto& point : poly)
            {
                const b2Vec2 point_ { point.x - .5f , .25f - point.y };
                poly_.emplace_back(600 * point_);
            }
            return poly_;
        };

        for (const auto& poly_color : std::get<1>(polys_to_colors))
        {
            if (!polygons::isForeground(poly_color.second))
                continue;

            const auto subpolys = polygons::decompose(polygons::ensure_cw(poly_color.first), 1e-5);
            cout << "foreground subpolys " << subpolys.size() << endl;
            for (const auto& subpoly : subpolys)
                push_fixture(foreground_transform(subpoly));
        }

        ground = body;
    }

    resetShip();
    resetBall();

    /*
    { // crate tower
        constexpr int nn = 10;
        constexpr float ww = 2;

        for (auto jj=nn; jj; jj--)
        for (auto ii=0; ii<jj; ii++)
        {
            const float xx = 10. + ww * ii - jj + nn;
            addCrate({ xx, 2.5f * ( 1 + nn - jj) }, { 0, 0 }, 0);
        }
    }
    */

    { // particle system
        //const uint32 particle_type = b2_powderParticle;

        b2ParticleSystemDef system_def;
        system_def.density = 5e-2;
        system_def.radius = .5; // FIXME 1.2;
        //system_def.elasticStrength = 1;
        system_def.surfaceTensionPressureStrength = .4;
        system_def.surfaceTensionNormalStrength = .4;
        system = world.CreateParticleSystem(&system_def);

        /*
        b2PolygonShape shape;
        shape.SetAsBox(4, 22);

        b2ParticleGroupDef group_def;
        group_def.shape = &shape;
        //group_def.flags = b2_powderParticle;
        //group_def.flags = b2_elasticParticle;
        group_def.groupFlags = b2_rigidParticleGroup | b2_solidParticleGroup;
        //group_def.groupFlags = b2_solidParticleGroup;
        group_def.position.Set(-15, 30);
        group_def.color.Set(0, 0xffu, 0xffu, 127u);
        system->CreateParticleGroup(group_def);
        */
    }

    addDoor({ 115, -170 }, {1, 10}, {0, -20});
    addDoor({ 15, -230 }, {1, 10}, {20, 0});
    addDoor({ -86, -65 }, {1, 10}, {-8, -15});


    const auto dump_filter_data = [](const b2Body& body) -> void
    {
        for (auto fixture=body.GetFixtureList(); fixture; fixture = fixture->GetNext())
        {
            const auto& filter = fixture->GetFilterData();
            const std::bitset<8> category_bits(filter.categoryBits);
            const std::bitset<8> mask_bits(filter.maskBits);
            cout << "  " << category_bits.to_string() << " " << mask_bits.to_string() << endl;
        }
    };

    cout << "ground" << endl;
    dump_filter_data(*ground);

    cout << "ship" << endl;
    dump_filter_data(*ship);

    cout << "ball" << endl;
    dump_filter_data(*ball);

    if (!doors.empty())
    {
        cout << "door" << endl;
        dump_filter_data(*std::get<0>(doors.front()));
    }

    if (!crates.empty())
    {
        cout << "crate" << endl;
        dump_filter_data(*crates.front());
    }

    world.SetContactListener(this);
}

void GameState::resetBall()
{ // ball
    if (ball)
    {
        world.DestroyBody(ball);
        ball = nullptr;
    }

    assert(!ball);

    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.position.Set(20, 0);
    def.angle = 0;
    def.angularVelocity = 0;

    b2CircleShape shape;
    shape.m_radius = 5.;

    b2FixtureDef fixture;
    fixture.shape = &shape;
    fixture.density = .1;
    fixture.friction = 1;
    fixture.restitution = 0.1;
    fixture.filter.categoryBits = object_category;
    fixture.filter.maskBits = object_category | ground_category | door_category;

    auto body = world.CreateBody(&def);
    body->CreateFixture(&fixture);
    ball = body;
}

void GameState::resetShip()
{ // ship
    if (ship)
    {
        world.DestroyBody(ship);
        ship = nullptr;
    }

    assert(!ship);

    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.position.Set(0, 0);
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
    fixture.filter.categoryBits = object_category;
    fixture.filter.maskBits = object_category | ground_category | door_category;

    auto body = world.CreateBody(&def);
    body->CreateFixture(&fixture);
    ship = body;
}

void GameState::flop()
{
    cout << "flop ";
    std::uniform_real_distribution<float32> dist(0, 255u);
    const float32 rr = dist(color_rng);
    const float32 gg = dist(color_rng);
    const float32 bb = 127u; //dist(color_rng);
    cout << rr << " ";
    cout << gg << " ";
    cout << bb << endl;

    assert(system);

    b2PolygonShape shape;
    shape.SetAsBox(10, 10);

    b2ParticleGroupDef group_def;
    group_def.shape = &shape;
    group_def.flags = b2_tensileParticle | b2_viscousParticle; //b2_powderParticle;
    //group_def.flags = b2_elasticParticle;
    //group_def.groupFlags = b2_solidParticleGroup;
    group_def.position.Set(0, 70);
    group_def.color.Set(rr, gg, bb, 255u);
    system->CreateParticleGroup(group_def);
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
    using std::get;
    int velocityIterations = 6;
    int positionIterations = 2;
    const auto angle = ship->GetAngle();
    const auto thrust = ship_thrust_factor * (isGrabbed() ? 120. : 20.) * b2Rot(angle).GetYAxis();
    for (auto& door : doors)
    {
        assert(get<2>(door) < get<1>(door).size());
        const auto delta = get<1>(door)[get<2>(door)] - get<0>(door)->GetWorldCenter();
        const auto delta_length = delta.Length();
        const auto delta_norm = delta_length > 2 ? 2 * delta / delta_length : delta;
        get<0>(door)->SetLinearVelocity(20 * delta_norm);
    }
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
    const bool aa_is_wall = aa == ground;
    const double aa_energy = .5 * aa->GetMass() * aa->GetLinearVelocity().LengthSquared();

    const auto* bb = contact->GetFixtureB()->GetBody();
    assert(bb);
    const bool bb_is_ship = bb == ship;
    const bool bb_is_ball = bb == ball;
    const bool bb_is_wall = bb == ground;
    const double bb_energy = .5 * bb->GetMass() * bb->GetLinearVelocity().LengthSquared();

    const bool any_ship = aa_is_ship || bb_is_ship;
    const bool any_ball = aa_is_ball || bb_is_ball;
    const bool any_wall = aa_is_wall || bb_is_wall;

    ship_touched_wall |= any_ship && any_wall;
    if (any_ship) ship_accum_contact++;
    all_accum_contact++;
    all_energy += aa_energy + bb_energy;
}

void GameState::addDoor(const b2Vec2 pos, const b2Vec2 size, const b2Vec2 delta)
{
    assert(ground);

    b2Body* door = nullptr;
    {
        b2BodyDef def;
        def.type = b2_kinematicBody;
        def.position = pos;
        def.angle = atan2(delta.y, delta.x) + M_PI / 2.f;

        b2PolygonShape shape;
        shape.SetAsBox(size.x, size.y);

        b2FixtureDef fixture;
        fixture.shape = &shape;
        fixture.density = 0;
        fixture.friction = .8;
        fixture.filter.categoryBits = door_category;
        fixture.filter.maskBits = object_category;

        door = world.CreateBody(&def);
        door->CreateFixture(&fixture);
    }
    assert(door);

    /*
    b2PrismaticJoint* joint = nullptr;
    {
        b2PrismaticJointDef def;
        def.Initialize(ground, door, door->GetWorldCenter(), direction);
        def.lowerTranslation = 0;
        def.upperTranslation = 2*size.y;
        def.enableLimit = true;
        def.maxMotorForce = 1e9f;
        def.motorSpeed = -10.f;
        def.enableMotor = true;

        joint = static_cast<b2PrismaticJoint*>(world.CreateJoint(&def));
    }
    assert(joint);
    */

    const auto origin = door->GetWorldCenter();
    doors.push_back({ door, { origin, origin + delta }, 0 });

}

void GameState::addCrate(const b2Vec2 pos, const b2Vec2 velocity, const double angle)
{
    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.position = pos;
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
    fixture.density = .01;
    fixture.friction = .8;
    fixture.restitution = .7;
    fixture.filter.categoryBits = object_category;
    fixture.filter.maskBits = object_category | ground_category | door_category;

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
