#include "GameWindow.h"

#include <QApplication>
#include <QMainWindow>
#include <QGridLayout>
#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <QSoundEffect>
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

    const auto pushButton = [&layout, &row](const QString& label, const std::function<void(bool)> setter) -> QPushButton*
    {
        qDebug() << "creating slider" << label;

        auto button = new QPushButton(label);
        button->setCheckable(true);
        QObject::connect(button, &QPushButton::clicked, setter);
        layout->addWidget(button, row, 0, 1, 2);

        row++;
        return button;
    };

    {
        auto foo = pushSlider("thrust", [&view](const int value) -> void {
            qDebug() << "change thrust" << value;
            view.state.ship_thrust_factor = value / 100.;
        });
        foo->setRange(50, 200);
        foo->setValue(100);
    }

    pushSlider("ball mass",  [&view](const int value) -> void {
        qDebug() << "change ball mass" << value;
        //view.state.ship_thrust_factor = value / 100.;
    });

    QSoundEffect engine_sfx;
    const auto engine_sound = QUrl::fromLocalFile(":engine.wav");
    qDebug() << "engine_sound" << engine_sound.isValid();
    engine_sfx.setSource(engine_sound);
    engine_sfx.setVolume(.5);
    engine_sfx.play();

    {
        auto foo = pushButton("gravity", [&view](const bool clicked) -> void {
            qDebug() << "gravity" << clicked;
            view.state.world.SetGravity(clicked ? b2Vec2 { 0, -10 } : b2Vec2 {0, 0});
        });
        foo->setChecked(true);
    }

    auto central = new QWidget;
    central->setLayout(layout);

    QMainWindow main;
    main.show();
    main.setCentralWidget(central);


    return app.exec();
}

