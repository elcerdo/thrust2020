#include "GameState.h"

#include "Box2D/Dynamics/b2Fixture.h"
#include "Box2D/Collision/Shapes/b2PolygonShape.h"
#include "Box2D/Collision/Shapes/b2CircleShape.h"
#include "Box2D/Dynamics/Contacts/b2Contact.h"
#include "Box2D/Particle/b2ParticleGroup.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "extract_polygons.h"
#include "decompose_polygons.h"

constexpr uint16 ground_category = 1 << 0;
constexpr uint16 object_category = 1 << 1;
constexpr uint16 door_category = 1 << 2;
const float default_density = 0.2;
const float default_friction = 0.1;
const float default_restitution = 0.5;

GameState::GameState()
{
    resetShip();
    resetBall();
    resetParticleSystem();

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


    world.SetContactListener(this);
}

void GameState::resetGround(const std::string& map_filename)
{
    using std::cout;
    using std::endl;

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

    cout << "** svg loading" << endl;
    cout << "filename " << std::quoted(map_filename) << endl;

    const auto polys_to_colors = polygons::extract(map_filename);
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

    cout << "foreground";
    cout.flush();
    for (const auto& poly_color : std::get<1>(polys_to_colors))
    {
        if (!polygons::isForeground(poly_color.second))
            continue;

        const auto subpolys = polygons::decompose(polygons::ensure_cw(poly_color.first), 1e-5);

        cout << " " << subpolys.size();
        cout.flush();

        for (const auto& subpoly : subpolys)
            push_fixture(foreground_transform(subpoly));
    }
    cout << endl;

    ground = UniqueBody(body, [this](b2Body* body) -> void { world.DestroyBody(body); });
}

void GameState::resetParticleSystem()
{
    b2ParticleSystemDef system_def;
    system_def.density = .5;
    system_def.radius = .5;
    //system_def.elasticStrength = 1;
    system_def.surfaceTensionPressureStrength = .4;
    system_def.surfaceTensionNormalStrength = .4;
    auto system_ = world.CreateParticleSystem(&system_def);
    assert(system_);
    system_->SetStuckThreshold(4);

    system = UniqueSystem(system_, [this](b2ParticleSystem* system_) -> void { world.DestroyParticleSystem(system_); });

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

void GameState::dumpCollisionData() const
{
    using std::cout;
    using std::endl;

    const auto dump_filter_data = [](const b2Body& body) -> void
    {
        size_t count = 5;
        for (auto fixture=body.GetFixtureList(); fixture && count; fixture = fixture->GetNext())
        {
            const auto& filter = fixture->GetFilterData();
            const std::bitset<8> category_bits(filter.categoryBits);
            const std::bitset<8> mask_bits(filter.maskBits);
            cout << "  " << category_bits.to_string() << " " << mask_bits.to_string() << endl;
            count--;
        }
        if (!count) cout << "  ...." << endl;
    };

    cout << "** category & collision mask" << endl;

    if (ground)
    {
        cout << "ground" << endl;
        dump_filter_data(*ground);
    }

    if (ship)
    {
        cout << "ship" << endl;
        dump_filter_data(*ship);
    }

    if (ball)
    {
        cout << "ball" << endl;
        dump_filter_data(*ball);
    }

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
}

void GameState::resetBall()
{ // ball
    if (ball)
    {
        if (isGrabbed())
            release();

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
    fixture.density = default_density;
    fixture.friction = default_friction;
    fixture.restitution = default_restitution;
    fixture.filter.categoryBits = object_category;
    fixture.filter.maskBits = object_category | ground_category | door_category;

    auto body = world.CreateBody(&def);
    body->CreateFixture(&fixture);

    ball = UniqueBody(body, [this](b2Body* body) -> void { world.DestroyBody(body); });
}

void GameState::resetShip()
{ // ship
    if (ship)
    {
        if (isGrabbed())
            release();

        ship = nullptr;
    }

    assert(!ship);

    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.position.Set(0, 0);
    def.angularVelocity = 0;
    def.angle = 0;

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
    fixture.density = default_density * 5;
    fixture.friction = default_friction;
    fixture.restitution = default_restitution;
    fixture.filter.categoryBits = object_category;
    fixture.filter.maskBits = object_category | ground_category | door_category;

    auto body = world.CreateBody(&def);
    body->CreateFixture(&fixture);

    ship = UniqueBody(body, [this](b2Body* body) -> void { world.DestroyBody(body); });
    ship_state = ShipState();
}

void GameState::addWater(const b2Vec2 position, const b2Vec2 size, const size_t seed, const unsigned int flags)
{
    using std::cout;
    using std::endl;

    cout << "** addWater ";

    std::default_random_engine rng(seed);
    std::uniform_real_distribution<float32> dist(0, 255u);
    const float32 rr = dist(rng);
    const float32 gg = dist(rng);
    const float32 bb = dist(rng);
    cout << rr << " ";
    cout << gg << " ";
    cout << bb << " ";

    assert(system);

    b2PolygonShape shape;
    shape.SetAsBox(size.x, size.y);

    b2ParticleGroupDef group_def;
    group_def.shape = &shape;
    group_def.flags = flags;
    cout << std::bitset<32>(group_def.flags).to_string() << " ";
    cout << std::bitset<32>(group_def.groupFlags).to_string() << endl;
    group_def.position.Set(position.x, position.y);
    group_def.color.Set(rr, gg, bb, 255u);
    system->CreateParticleGroup(group_def);
}

void GameState::clearWater(const int group_count)
{
    using std::cout;
    using std::endl;

    cout << "** clearWater " << group_count << endl;

    assert(system);
    unsigned int count = 0;
    for (auto* group = system->GetParticleGroupList(); group; group = group->GetNext())
    {
        if (group_count >= 0 && count >= group_count)
            break;
        group->DestroyParticles(false);
        count++;
    }
}

void GameState::grab()
{
    assert(!isGrabbed());
    assert(ship);
    assert(ball);

    b2DistanceJointDef def;
    def.Initialize(ship.get(), ball.get(), ship->GetWorldCenter(), ball->GetWorldCenter());
    def.frequencyHz = 25.;
    def.dampingRatio = .2;
    def.collideConnected = true;

    auto joint = static_cast<b2DistanceJoint*>(world.CreateJoint(&def));
    link = UniqueDistanceJoint(joint, [this](b2Joint* joint) -> void { world.DestroyJoint(joint); });
}

void GameState::release()
{
    assert(isGrabbed());
    assert(ship);
    assert(ball);
    link = nullptr;
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
    return static_cast<bool>(link);
}

void GameState::step(const float dt)
{
    using std::get;

    for (auto& door : doors)
    {
        assert(get<0>(door));
        assert(get<2>(door) < get<1>(door).size());
        const auto delta = get<1>(door)[get<2>(door)] - get<0>(door)->GetWorldCenter();
        const auto delta_length = delta.Length();
        const auto delta_norm = delta_length > 2 ? 2 * delta / delta_length : delta;
        get<0>(door)->SetLinearVelocity(20 * delta_norm);
    }

    { // ship
        assert(ship);
        const auto angle = ship->GetAngle();
        if (ship_state.firing_thruster)
        {
            const auto thrust = ship_state.thrust_factor * 2 * (isGrabbed() ? 50. : 40.) * b2Rot(angle).GetYAxis();
            ship->ApplyForceToCenter(ship->GetMass() * thrust, true);
        }

        const auto target_angular_velocity = (
            ship_state.turning_right && !ship_state.turning_left ? -1 :
            !ship_state.turning_right && ship_state.turning_left ? 1 :
            0) * (isGrabbed() ? 2. : 2.6) * M_PI / 2.;

        ship_state.target_angle += target_angular_velocity * dt;
        ship->SetAngularVelocity((ship_state.target_angle - angle) / .05);
    }

    { // step
        ship_state.accum_contact = 0;
        all_accum_contact = 0;
        int velocityIterations = 6;
        int positionIterations = 2;
        int particleIterations = std::min(world.CalculateReasonableParticleIterations(dt), 4);
        world.Step(dt, velocityIterations, positionIterations, particleIterations);
        world.ClearForces();
    }

    if (clean_stuck_in_door)
    {
        assert(system);
        const auto candidates = system->GetStuckCandidates();
        const auto positions = system->GetPositionBuffer();
        for (auto kk=0, kk_max=system->GetStuckCandidateCount(); kk<kk_max; kk++)
            for (auto& door : doors)
            {
                assert(get<0>(door));
                const auto fixture = get<0>(door)->GetFixtureList();
                assert(fixture);
                const auto candidate = candidates[kk];
                if (fixture->TestPoint(positions[candidate]))
                {
                    system->DestroyParticle(candidate);
                    break;
                }
            }
    }

}

void GameState::BeginContact(b2Contact* contact)
{
    assert(contact);

    const auto* aa = contact->GetFixtureA()->GetBody();
    assert(aa);
    const bool aa_is_ship = aa == ship.get();
    //const bool aa_is_ball = aa == ball.get();
    const bool aa_is_wall = aa == ground.get();
    //const double aa_energy = .5 * aa->GetMass() * aa->GetLinearVelocity().LengthSquared();

    const auto* bb = contact->GetFixtureB()->GetBody();
    assert(bb);
    const bool bb_is_ship = bb == ship.get();
    //const bool bb_is_ball = bb == ball.get();
    const bool bb_is_wall = bb == ground.get();
    //const double bb_energy = .5 * bb->GetMass() * bb->GetLinearVelocity().LengthSquared();

    const bool any_ship = aa_is_ship || bb_is_ship;
    //const bool any_ball = aa_is_ball || bb_is_ball;
    const bool any_wall = aa_is_wall || bb_is_wall;

    ship_state.touched_wall |= any_ship && any_wall;
    if (any_ship) ship_state.accum_contact++;
    all_accum_contact++;
}

void GameState::addDoor(const b2Vec2 pos, const b2Vec2 size, const b2Vec2 delta)
{
    UniqueBody door = nullptr;
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

        auto body = world.CreateBody(&def);
        body->CreateFixture(&fixture);

        door = UniqueBody(body, [this](b2Body* body) { world.DestroyBody(body); });
    }
    assert(door);

    /*
    b2PrismaticJoint* joint = nullptr;
    {
        assert(ground);
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
    doors.emplace_back(std::move(door), std::vector<b2Vec2> { origin, origin + delta }, 0);
}


void GameState::addPath(const std::vector<b2Vec2>& positions, const b2Vec2 size)
{
    assert(!positions.empty());
    const auto& p0 = positions.front();

    UniqueBody door = nullptr;
    {
        b2BodyDef def;
        def.type = b2_kinematicBody;
        def.position = p0;
        def.angle = 0;

        b2PolygonShape shape;
        shape.SetAsBox(size.x, size.y);

        b2FixtureDef fixture;
        fixture.shape = &shape;
        fixture.density = 0;
        fixture.friction = .8;
        fixture.filter.categoryBits = door_category;
        fixture.filter.maskBits = object_category;

        auto body = world.CreateBody(&def);
        body->CreateFixture(&fixture);

        door = UniqueBody(body, [this](b2Body* body) { world.DestroyBody(body); });
    }
    assert(door);

    doors.emplace_back(std::move(door), positions, 0);
}

void GameState::addCrate(const b2Vec2 pos, const b2Vec2 velocity, const double angle)
{
    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.position = pos;
    def.angle = angle;

    constexpr float zz = 3; // 1.2
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
    fixture.density = default_density;
    fixture.friction = default_friction;
    fixture.restitution = default_restitution;
    fixture.filter.categoryBits = object_category;
    fixture.filter.maskBits = object_category | ground_category | door_category;

    auto crate = world.CreateBody(&def);
    crate->CreateFixture(&fixture);
    crate->SetLinearVelocity(velocity);

    auto foo = UniqueBody(crate, [this](b2Body* body) -> void { world.DestroyBody(body); });
    crates.emplace_back(std::move(foo));
}

void GameState::clearCrates()
{
    crates.clear();
}

GameState::~GameState()
{
    world.SetContactListener(nullptr);
}
