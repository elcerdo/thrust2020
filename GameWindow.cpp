#include "GameWindow.h"

#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"

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
    const auto& local_center = body->GetLocalCenter();
    const auto& is_awake = body->IsAwake();
    painter.translate(position.x, position.y);
    painter.rotate(qRadiansToDegrees(angle));

    painter.setBrush(Qt::black);
    painter.setPen(QPen(Qt::white, 0));
    const auto* fixture = body->GetFixtureList();
    while (fixture)
    {
        const auto* shape = fixture->GetShape();
        if (const auto* poly = dynamic_cast<const b2PolygonShape*>(shape))
        {
            QPolygonF poly_;
            for (int kk=0; kk<poly->m_count; kk++)
                poly_ << QPointF(poly->m_vertices[kk].x, poly->m_vertices[kk].y);
            painter.drawPolygon(poly_);
        }
        //assert(shape);
        //qDebug() << shape->GetType();
        fixture = fixture->GetNext();
    }

    drawOrigin(painter);
    painter.restore();

    painter.save();
    const auto& world_center = body->GetWorldCenter();
    const auto& linear_velocity = body->GetLinearVelocityFromWorldPoint(world_center);
    painter.translate(world_center.x, world_center.y);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(Qt::blue, 0));
    painter.drawLine(QPointF(0, 0), QPointF(linear_velocity.x, linear_velocity.y));

    painter.setBrush(is_awake ? Qt::blue : Qt::white);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(0, 0), .2, .2);

    painter.restore();
}

void GameWindow::drawFlame(QPainter& painter)
{
    std::bernoulli_distribution dist_flicker;

    const auto* body = state.ship;

    assert(body);
    painter.save();
    const auto& position = body->GetPosition();
    const auto& angle = body->GetAngle();
    painter.translate(position.x, position.y);
    painter.rotate(qRadiansToDegrees(angle));

    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::red);
    QPolygonF poly;
    poly << QPointF(0, 0) << QPointF(1, -3) ;
    if (dist_flicker(rng)) poly << QPointF(.5, -2.5);
    poly << QPointF(0, -4);
    if (dist_flicker(rng)) poly << QPointF(-.5, -2.5);
    poly << QPointF(-1, -3);
    painter.scale(.8, .8);
    painter.translate(0, 1.5);
    painter.drawPolygon(poly);
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

    painter.translate(0, -20);
    drawOrigin(painter);
    drawBody(painter, state.ground);
    if (state.ship_firing) drawFlame(painter);
    drawBody(painter, state.ship);
}

void GameWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Up)
    {
        state.ship_firing = true;
        return;
    }
    RasterWindow::keyPressEvent(event);
}

void GameWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Up)
    {
        state.ship_firing = false;
        return;
    }
    RasterWindow::keyReleaseEvent(event);
}

