#include "GameWindow.h"

GameWindow::GameWindow(QWindow* parent)
    : RasterWindow(parent)
{
}

void GameWindow::render(QPainter *painter)
{
    painter->drawText(QRectF(0, 0, width(), height()), Qt::AlignCenter, QStringLiteral("QWindow"));
}

