#include "GameWindow.h"

GameWindow::GameWindow(QWindow* parent)
    : RasterWindow(parent)
{
    time.start();
}

void GameWindow::render(QPainter *painter)
{
    painter->drawText(QRectF(0, 0, width(), height()), Qt::AlignCenter, QStringLiteral("QWindow"));

    const double dt = time.elapsed() / 1e3;
    time.restart();
}

