#include "GameWindow.h"

#include "box2d/b2_fixture.h"

GameWindow::GameWindow(QWindow* parent)
    : RasterWindow(parent)
{
    time.start();
}


void drawOrigin(QPainter& painter)
{
    painter.save();
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(Qt::red, 0));
    painter.drawLine(0, 0, 1, 0);
    painter.setPen(QPen(Qt::green, 0));
    painter.drawLine(0, 0, 0, 1);
    painter.restore();
}

void drawBody(QPainter& painter, const b2Body* body)
{
    assert(body);
    painter.save();
    const auto& position = body->GetPosition();
    const auto& angle = body->GetAngle();
    painter.translate(position.x, position.y);
    painter.rotate(qRadiansToDegrees(angle));
    drawOrigin(painter);

    const auto* fixture = body->GetFixtureList();
    while (fixture)
    {
        const auto* shape = fixture->GetShape();
        assert(shape);
        fixture = fixture->GetNext();
    }

    painter.restore();
}

void GameWindow::render(QPainter& painter)
{
    const double dt = time.elapsed() / 1e3;
    time.restart();

    state.step(dt);

    const int side = qMin(width(), height());
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(width() / 2, height() / 2);
    painter.scale(side / 50.0, -side / 50.0);

    drawOrigin(painter);
    drawBody(painter, state.ground);
    drawBody(painter, state.ship);

}

