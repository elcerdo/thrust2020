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

    view_opengl.addCheckbox("gravity", true, [&state](const bool checked) -> void {
        qDebug() << "gravity" << checked;
        state.world.SetGravity(checked ? b2Vec2 { 0, -10 } : b2Vec2 {0, 0});
    });

    view_opengl.addCheckbox("draw debug", true, [&view_opengl](const bool checked) -> void {
        const auto before = view_opengl.draw_debug;
        view_opengl.draw_debug = checked;
        qDebug() << "draw debug" << before << checked << &view_opengl << view_opengl.draw_debug;
    });

    view_opengl.addCheckbox("mute sfx", false, [&view_opengl](const bool checked) -> void {
        view_opengl.setMuted(checked);
    });

    return app.exec();
}

