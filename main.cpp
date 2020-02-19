#include "GameWindowOpenGL.h"
#include "GameWindow.h"

#include <QApplication>
#include <QMainWindow>
#include <QGridLayout>
#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <iostream>

int main(int argc, char* argv[])
{
    using std::cout;
    cout << std::boolalpha;

    QSurfaceFormat glFormat;
    glFormat.setVersion(3, 3);
    glFormat.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(glFormat);

    QApplication app(argc, argv);

    GameWindowOpenGL view_opengl;
    view_opengl.setAnimated(true);
    view_opengl.resize(800, 600);
    view_opengl.show();

    GameWindow view;
    view.setAnimated(true);
    view.show();

    view_opengl.addSlider("thrust", .5, 2, 1, [&view](const float value) -> void {
        qDebug() << "change thrust" << value;
        view.state.ship_thrust_factor = value;
    });

    view_opengl.addSlider("ball mass", 0, 2, 1, [&view](const float value) -> void {
        qDebug() << "change ball mass" << value;
        //view.state.ship_thrust_factor = value;
    });

    view_opengl.addCheckbox("gravity", true, [&view](const bool clicked) -> void {
        qDebug() << "gravity" << clicked;
        view.state.world.SetGravity(clicked ? b2Vec2 { 0, -10 } : b2Vec2 {0, 0});
    });

    return app.exec();
}

