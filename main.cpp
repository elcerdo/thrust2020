#include "GameWindowOpenGL.h"

#include <QApplication>
#include <QDebug>

#include "Box2D/Dynamics/b2Fixture.h"

int main(int argc, char* argv[])
{
    { // default opengl format
        QSurfaceFormat format;
        format.setVersion(3, 3);
        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
        format.setAlphaBufferSize(0);
        format.setStencilBufferSize(0);
        format.setDepthBufferSize(16);
        format.setProfile(QSurfaceFormat::CoreProfile);
        QSurfaceFormat::setDefaultFormat(format);
    }

    QApplication app(argc, argv);
    GameWindowOpenGL view;    
    view.resetLevel(6); // default level -1

    view.setAnimated(true);
    view.resize(1280, 720);
    view.show();

    view.addSlider("thrust", .5, 20, 2, [&view](const float value) -> void {
        assert(view.state);
        qDebug() << "change thrust" << value;
        view.state->ship_thrust_factor = value;
    });
    view.addSlider("ball density", .1, 2, .2, [&view](const float value) -> void {
        assert(view.state);
        const auto& body = view.state->ball;
        assert(body);
        const auto fixture = body->GetFixtureList();
        assert(fixture);
        qDebug() << "change ball density" << value << fixture->GetDensity();
        fixture->SetDensity(value);
        body->ResetMassData();
    });
    view.addSlider("ship density", .1, 2, 1, [&view](const float value) -> void {
        assert(view.state);
        const auto& body = view.state->ship;
        assert(body);
        const auto fixture = body->GetFixtureList();
        assert(fixture);
        qDebug() << "change ship density" << value << fixture->GetDensity();
        fixture->SetDensity(value);
        body->ResetMassData();
    });

    view.addCheckbox("gravity", Qt::Key_G, true, [&view](const bool checked) -> void {
        assert(view.state);
        view.state->world.SetGravity(checked ? b2Vec2 { 0, -10 } : b2Vec2 {0, 0});
    });
    view.addCheckbox("draw debug", Qt::Key_P, false, [&view](const bool checked) -> void {
        view.draw_debug = checked;
    });
    view.addCheckbox("mute sfx", Qt::Key_M, true, [&view](const bool checked) -> void {
        view.setMuted(checked);
    });
    view.addCheckbox("world view", Qt::Key_W, false, [&view](const bool checked) -> void {
        view.is_zoom_out = checked;
    });
    view.addCheckbox("pause", Qt::Key_O, false, [&view](const bool checked) -> void {
        view.is_paused = checked;
    });


    std::default_random_engine rng;
    view.addButton("drop water", Qt::Key_E, [&view, &rng]() -> void {
        assert(view.state);
        const uint flags = b2_waterParticle | b2_tensileParticle;// | b2_viscousParticle;
        view.state->addWater({ 0, 70 }, { 10, 10 }, rng(), flags);
    });
    view.addButton("clear water", Qt::Key_D, [&view]() -> void {
        assert(view.state);
        view.state->clearWater();
    });
    view.addButton("drop crate", Qt::Key_R, [&view, &rng]() -> void {
        assert(view.state);
        std::uniform_real_distribution<double> dist_angle(0, 2 * M_PI);
        const auto angle = dist_angle(rng);
        std::normal_distribution<double> dist_normal(0, 10);
        const b2Vec2 velocity(dist_normal(rng), dist_normal(rng));
        view.state->addCrate({ 0, 10 }, velocity, angle);
        return;
    });
    view.addButton("clear crates", Qt::Key_F, [&view]() -> void {
        assert(view.state);
        view.state->clearCrates();
    });
    view.addButton("reset ship", Qt::Key_S, [&view]() -> void {
        assert(view.state);
        view.state->resetShip();
    });
    view.addButton("reset ball", Qt::Key_B, [&view]() -> void {
        assert(view.state);
        view.state->resetBall();
    });
    view.addButton("toggle doors", Qt::Key_T, [&view]() -> void {
        assert(view.state);
        using std::get;
        for (auto& door : view.state->doors)
        {
            auto& target = get<2>(door);
            target ++;
            target %= get<1>(door).size();
            qDebug() << "target" << target;
        }
        return;
    });

    return app.exec();
}

