#pragma once

#include "RasterWindow.h"
#include "GameState.h"

#include <QSoundEffect>
#include <random>
#include <list>

class GameWindow : public RasterWindow
{
    Q_OBJECT
    public:
        GameWindow(QWindow* parent = nullptr);
        void render(QPainter& painter) override;
        void drawFlame(QPainter& painter);
        void drawShip(QPainter& painter);
        void drawBody(QPainter& painter, const b2Body* body, const QColor& color = Qt::black) const;
        void drawOrigin(QPainter& painter) const;
        GameState state;

    protected:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

    private:
        QTime time;
        std::default_random_engine rng;
        bool draw_debug;
        size_t frame_counter;
        std::list<double> dts;

        QSoundEffect engine_sfx;
        QSoundEffect click_sfx;
};


