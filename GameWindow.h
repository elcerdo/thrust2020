#pragma once

#include "RasterWindow.h"

class GameWindow : public RasterWindow
{
    Q_OBJECT
    public:
        GameWindow(QWindow* parent = nullptr);

        void render(QPainter *painter) override;

    private:
        QTime time;
};


