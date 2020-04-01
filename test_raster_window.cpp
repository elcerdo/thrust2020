#include "RasterWindowOpenGL.h"

#include <QApplication>

#include <iostream>

int main(int argc, char* argv[])
{
    using std::cout;
    using std::endl;

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

    QApplication app(argc, argv);

    RasterWindowOpenGL view;

    view.setAnimated(true);
    view.resize(1280, 720);
    view.show();

    view.addSlider("slider", 0, 1, .5, [](const float value) -> void {
        cout << "slider " << value << endl;
    });

    view.addCheckbox("checkbox", Qt::Key_Q, true, [](const bool checked) -> void {
        cout << "checkbox " << checked << endl;
    });

    view.addButton("button0", Qt::Key_Z, []() -> void {
        cout << "button0" << endl;
    });
    view.addButton("button1", Qt::Key_E, []() -> void {
        cout << "button1" << endl;
    });

    return app.exec();
}

