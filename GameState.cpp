#include "GameState.h"

#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_fixture.h"

#include <iostream>

using std::cout;
using std::endl;

GameState::GameState() :
    world(b2Vec2(0, -10)),
    ground(nullptr)
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

        auto body = world.CreateBody(&def);
        body->CreateFixture(&fixture);
        ship = body;
    }
}

void GameState::step(const float dt)
{
    int velocityIterations = 6;
    int positionIterations = 2;
    world.Step(dt, velocityIterations, positionIterations);
}

GameState::~GameState()
{
    world.DestroyBody(ship);
    world.DestroyBody(ground);
}
