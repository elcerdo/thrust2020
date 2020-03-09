#include "GameWindowOpenGL.h"

#include <QApplication>
#include <QDebug>

#include "Box2D/Dynamics/b2Fixture.h"

int main(int argc, char* argv[])
{
    QSurfaceFormat glFormat;
    glFormat.setVersion(3, 3);
    //glFormat.setSamples(2);
    glFormat.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(glFormat);

    QApplication app(argc, argv);

    GameWindowOpenGL view_opengl;
    view_opengl.setAnimated(true);
    view_opengl.resize(1280, 720);
    view_opengl.show();

    GameState& state = view_opengl.state;

    view_opengl.addSlider("thrust", .5, 3, 1.5, [&state](const float value) -> void {
        qDebug() << "change thrust" << value;
        state.ship_thrust_factor = value;
    });

    view_opengl.addSlider("ball density", .01, .1, .04, [&state](const float value) -> void {
        b2MassData mass_data;
        const auto body = state.ball;
        assert(body);
        const auto fixture = body->GetFixtureList();
        assert(fixture);
        qDebug() << "change ball mass" << value << fixture->GetDensity();
        fixture->SetDensity(value);
        body->ResetMassData();
    });
    view_opengl.addSlider("liquid damping", 0, 1, .5, [&state](const float value) -> void {
        assert(state.system);
        state.system->SetDamping(1 - value);
    });

    view_opengl.addCheckbox("gravity", Qt::Key_G, true, [&state](const bool checked) -> void {
        qDebug() << "gravity" << checked;
        state.world.SetGravity(checked ? b2Vec2 { 0, -10 } : b2Vec2 {0, 0});
    });
    view_opengl.addCheckbox("draw debug", Qt::Key_D, false, [&view_opengl](const bool checked) -> void {
        const auto before = view_opengl.draw_debug;
        view_opengl.draw_debug = checked;
        qDebug() << "draw debug" << before << checked << &view_opengl << view_opengl.draw_debug;
    });
    view_opengl.addCheckbox("mute sfx", Qt::Key_M, true, [&view_opengl](const bool checked) -> void {
        view_opengl.setMuted(checked);
    });
    view_opengl.addCheckbox("world view", Qt::Key_W, false, [&view_opengl](const bool checked) -> void {
        view_opengl.is_zoom_out = checked;
    });

    std::default_random_engine rng;
    view_opengl.addButton("reset ship", Qt::Key_S, [&state]() -> void { state.resetShip(); });
    view_opengl.addButton("reset ball", Qt::Key_B, [&state]() -> void { state.resetBall(); });
    view_opengl.addButton("tog doors", Qt::Key_T, [&state]() -> void {
        using std::get;
        for (auto& door : state.doors)
        {
            auto& target = get<2>(door);
            target ++;
            target %= get<1>(door).size();
            qDebug() << "target" << target;
        }
        return;
    });
    view_opengl.addButton("drop water", Qt::Key_E, [&state, &rng]() -> void {
        state.addWater({ 0, 70 }, { 10, 10 }, rng());
    });
    view_opengl.addButton("drop crate", Qt::Key_Z, [&state, &rng]() -> void {
        std::uniform_real_distribution<double> dist_angle(0, 2 * M_PI);
        const auto angle = dist_angle(rng);
        std::normal_distribution<double> dist_normal(0, 10);
        const b2Vec2 velocity(dist_normal(rng), dist_normal(rng));
        state.addCrate({ 0, 10 }, velocity, angle);
        return;
    });

    return app.exec();
}

