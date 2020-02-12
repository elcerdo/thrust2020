#include "GameWindow.h"

#include <QApplication>
#include <QMainWindow>
#include <QGridLayout>
#include <QSlider>
#include <QLabel>
#include <iostream>

int main(int argc, char* argv[])
{
    using std::cout;
    cout << std::boolalpha;

    QApplication app(argc, argv);

    GameWindow view;
    view.setAnimated(true);
    view.show();
    int row = 0;
    auto layout = new QGridLayout;
    const auto pushSlider = [&layout, &row](const QString& label, const std::function<void(int)> setter) -> QSlider*
    {
        qDebug() << "creating slider" << label;

        auto slider = new QSlider(Qt::Horizontal);
        QObject::connect(slider, &QSlider::valueChanged, setter);
        layout->addWidget(slider, row, 1);

        auto label_ = new QLabel(label);
        layout->addWidget(label_, row, 0, Qt::AlignRight);

        row++;
        return slider;
    };

    auto foo = pushSlider("thrust", [&view](const int value) -> void {
        qDebug() << "change thrust" << value;
        view.state.ship_thrust_factor = value / 100.;
    });
    foo->setRange(50, 200);
    foo->setValue(100);

    pushSlider("ball mass",  [&view](const int value) -> void {
        qDebug() << "change ball mass" << value;
        //view.state.ship_thrust_factor = value / 100.;
    });


    auto central = new QWidget;
    central->setLayout(layout);

    QMainWindow main;
    main.show();
    main.setCentralWidget(central);


    return app.exec();
}

