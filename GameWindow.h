#pragma once

#include "RasterWindow.h"
#include "GameState.h"

#include <random>

class GameWindow : public RasterWindow
{
    Q_OBJECT
    public:
        GameWindow(QWindow* parent = nullptr);
        void render(QPainter& painter) override;
        void drawFlame(QPainter& painter);

    protected:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

    private:
        QTime time;
        GameState state;
        std::default_random_engine rng;
};


