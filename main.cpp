#include "GameWindowOpenGL.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char* argv[])
{
    QSurfaceFormat glFormat;
    glFormat.setVersion(3, 3);
    glFormat.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(glFormat);

    QApplication app(argc, argv);

    GameWindowOpenGL view_opengl;
    view_opengl.setAnimated(true);
    view_opengl.resize(1280, 720);
    view_opengl.show();

    GameState& state = view_opengl.state;

    view_opengl.addSlider("thrust", .5, 2, 1, [&state](const float value) -> void {
        qDebug() << "change thrust" << value;
        state.ship_thrust_factor = value;
    });

    view_opengl.addSlider("ball mass", 0, 2, 1, [&state](const float value) -> void {
        qDebug() << "change ball mass" << value;
        //state.ship_thrust_factor = value;
    });

    view_opengl.addCheckbox("gravity", true, [&state](const bool checked) -> void {
        qDebug() << "gravity" << checked;
        state.world.SetGravity(checked ? b2Vec2 { 0, -10 } : b2Vec2 {0, 0});
    });

    view_opengl.addCheckbox("draw debug", true, [&view_opengl](const bool checked) -> void {
        qDebug() << "draw debug" << checked;
        view_opengl.draw_debug = checked;
    });

    return app.exec();
}

