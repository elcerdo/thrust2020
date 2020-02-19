#include "GameWindow.h"

#include "Box2D/Dynamics/b2Fixture.h"
#include "Box2D/Collision/Shapes/b2PolygonShape.h"
#include "Box2D/Collision/Shapes/b2CircleShape.h"

GameWindow::GameWindow(QWindow* parent) :
    RasterWindow(parent),
    state(),
    rng(0x78549632),
    draw_debug(false),
    frame_counter(0)
{
    time.start();

    {
        auto& sfx = engine_sfx;
        const auto sound = QUrl::fromLocalFile(":engine.wav");
        assert(sound.isValid());
        sfx.setSource(sound);
        sfx.setLoopCount(QSoundEffect::Infinite);
        sfx.setVolume(.3);
        sfx.setMuted(true);
        sfx.play();
    }

    {
        auto& sfx = ship_click_sfx;
        const auto click_sound = QUrl::fromLocalFile(":click01.wav");
        assert(click_sound.isValid());
        sfx.setSource(click_sound);
        sfx.setLoopCount(1);
        sfx.setVolume(.2);
        sfx.setMuted(false);
        sfx.stop();
    }

    {
        auto& sfx = back_click_sfx;
        const auto click_sound = QUrl::fromLocalFile(":click00.wav");
        assert(click_sound.isValid());
        sfx.setSource(click_sound);
        sfx.setLoopCount(1);
        sfx.setVolume(.5);
        sfx.setMuted(false);
        sfx.stop();
    }
}

void GameWindow::drawOrigin(QPainter& painter) const
{
    if (!draw_debug)
        return;

    painter.save();
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(Qt::red, 0));
    painter.drawLine(0, 0, 1, 0);
    painter.setPen(QPen(Qt::green, 0));
    painter.drawLine(0, 0, 0, 1);
    painter.restore();
}

void GameWindow::drawBody(QPainter& painter, const b2Body* body, const QColor& color) const
{
    assert(body);

    { // shape and origin
        const auto& position = body->GetPosition();
        const auto& angle = body->GetAngle();

        painter.save();

        painter.translate(position.x, position.y);
        painter.rotate(qRadiansToDegrees(angle));

        painter.setBrush(color);
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
            if (const auto* circle = dynamic_cast<const b2CircleShape*>(shape))
            {
                painter.drawEllipse(QPointF(0, 0), circle->m_radius, circle->m_radius);
            }
            //assert(shape);
            //qDebug() << shape->GetType();
            fixture = fixture->GetNext();
        }

        drawOrigin(painter);

        painter.restore();
    }

    if (draw_debug)
    { // center of mass and velocity
        const auto& world_center = body->GetWorldCenter();
        const auto& linear_velocity = body->GetLinearVelocityFromWorldPoint(world_center);
        const auto& is_awake = body->IsAwake();

        painter.save();

        painter.translate(world_center.x, world_center.y);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Qt::blue, 0));
        painter.drawLine(QPointF(0, 0), QPointF(linear_velocity.x, linear_velocity.y));

        painter.setBrush(is_awake ? Qt::blue : Qt::white);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(0, 0), .2, .2);

        painter.restore();
    }
}

void GameWindow::drawFlame(QPainter& painter)
{
    std::bernoulli_distribution dist_flicker;
    std::normal_distribution<float> dist_noise(0, .2);
    const auto randomPoint = [this, &dist_noise]() -> QPointF {
        return { dist_noise(rng), dist_noise(rng) };
    };

    const auto* body = state.ship;
    assert(body);

    painter.save();
    const auto& position = body->GetPosition();
    const auto& angle = body->GetAngle();
    painter.translate(position.x, position.y);
    painter.rotate(qRadiansToDegrees(angle));
    painter.scale(2.5, 2.5);

    QLinearGradient grad(QPointF(0, 0), QPointF(0, -3));
    grad.setColorAt(0, QColor(0xfa, 0x70, 0x9a)); // true sunset
    grad.setColorAt(1, QColor(0xfe, 0xe1, 0x40));

    painter.setPen(Qt::NoPen);
    painter.setBrush(grad);
    QPolygonF poly;
    poly << QPointF(0, 0) << QPointF(1, -3) + randomPoint();
    if (dist_flicker(rng)) poly << QPointF(.5, -2.5) + randomPoint();
    poly << QPointF(0, -4) + randomPoint();
    if (dist_flicker(rng)) poly << QPointF(-.5, -2.5) + randomPoint();
    poly << QPointF(-1, -3) + randomPoint();
    painter.scale(.8, .8);
    painter.translate(0, 1.5);
    painter.drawPolygon(poly);
    painter.restore();
}

void GameWindow::drawShip(QPainter& painter)
{
    const auto* body = state.ship;
    assert(body);

    if (state.ship_firing) drawFlame(painter);

    drawBody(painter, body, Qt::black);

    if (state.canGrab() && frame_counter % 2 == 0)
    {
        painter.save();
        const auto& world_center = body->GetWorldCenter();
        painter.translate(world_center.x, world_center.y);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Qt::blue, 1));
        painter.drawEllipse(QPointF(0, 0), 3.5, 3.5);
        painter.restore();
    }

    if (draw_debug)
    { // direction hint
        painter.save();
        const auto& world_center = body->GetWorldCenter();
        painter.translate(world_center.x, world_center.y);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Qt::blue, 0));
        painter.drawLine(QPointF(0, 0), QPointF(-sin(state.ship_target_angle), cos(state.ship_target_angle)));
        painter.restore();
    }
}

void GameWindow::render(QPainter& painter)
{
    constexpr double max_step = 50e-3;

    frame_counter++;

    const double dt = time.elapsed() / 1e3;

    // dt_mean
    dts.push_front(dt);
    while (dts.size() > 30)
        dts.pop_back();

    double dt_mean = 0;
    double dt_mean_ = 0;
    for (const auto& dt : dts)
    {
        dt_mean += dt;
        dt_mean_ += std::min(max_step, dt);
    }
    assert(!dts.empty());
    dt_mean /= dts.size();
    dt_mean_ /= dts.size();

    const double fps = 1. / dt_mean;

    const double dt_ = std::min(max_step, dt);
    state.step(dt_);

    const double energy = state.all_accum_contact ? state.all_energy / state.all_accum_contact : 0;

    // energy_mean
    energies.push_front(energy);
    while (energies.size() > 30)
        energies.pop_back();

    double energy_mean = 0;
    for (const auto& energy : energies)
        energy_mean += energy;
    assert(!energies.empty());
    energy_mean /= energies.size();

    const auto power_mean = energy_mean / dt_mean_;
    //if (state.all_accum_contact >  0) qDebug() << state.all_accum_contact << energy << power_mean;

    const auto volume = [&power_mean]() -> double {
        constexpr double min_power = 100;
        constexpr double max_power = .5;
        constexpr double min_volume = 5e-2;
        constexpr double max_volume = 50e-2;

        constexpr double delta = max_power - min_power;
        static_assert(delta != 0, "delta must be non null");
        static_assert(max_volume > min_volume, "max_volume must be greater than min_volume");
        constexpr double aa = (max_volume - min_volume) / delta;
        constexpr double bb = (min_volume * max_power - max_volume * min_power) / delta;
        return std::max(min_volume, std::min(max_volume, aa * power_mean + bb));
    }();

    const int side = qMin(width(), height());
    painter.setRenderHint(QPainter::Antialiasing);

    { // world
        painter.save();
        painter.translate(width() / 2, height() / 2);

        const auto& pos = state.ship->GetPosition();
        const double height =  75 * std::max(1., pos.y / 40.);
        painter.scale(side / height, -side / height);
        painter.translate(-pos.x, -20);

        drawOrigin(painter);
        drawBody(painter, state.left_side);
        drawBody(painter, state.right_side);
        drawBody(painter, state.ground);

        for (auto& crate : state.crates)
            drawBody(painter, crate);

        if (state.joint)
        { // joint line
            painter.save();
            const auto& anchor_aa = state.joint->GetAnchorA();
            const auto& anchor_bb = state.joint->GetAnchorB();
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(Qt::white, 0));
            painter.drawLine(QPointF(anchor_aa.x, anchor_aa.y), QPointF(anchor_bb.x, anchor_bb.y));
            painter.restore();
        }

        assert(state.ball);
        const bool is_fast = state.ball->GetLinearVelocity().Length() > 30;
        drawBody(painter, state.ball, is_fast ? QColor(0xfd, 0xa0, 0x85) : Qt::black);
        drawShip(painter);

        painter.restore();
    }

    { // overlay
        painter.save();
        painter.translate(QPointF(0, height()));
        painter.setPen(QPen(Qt::red, 1));
        const auto print = [&painter](const QString& text) -> void
        {
            painter.translate(QPointF(0, -12));
            painter.drawText(0, 0, text);
        };

        constexpr QChar fill(' ');

        print(QString("creates %1").arg(state.crates.size(), 4, 10, fill));
        print(QString("     ms %1").arg(dt_mean * 1000, 7, 'f', 2, fill));
        print(QString("    fps %1").arg(fps, 7, 'f', 2, fill));
        print(QString(" thrust %1%").arg(state.ship_thrust_factor * 100, 4, 'f', 0, fill));
        print(QString("contact %1").arg(state.all_accum_contact, 4, 10, fill));
        print(QString("  power %1").arg(power_mean, 7, 'f', 2, fill));
        print(QString(" volume %1%").arg(volume * 100, 4, 'f', 0, fill));
        if (state.ship_touched_wall) print("boom");

        painter.restore();

    }

    { // sfx
        if (state.ship_accum_contact > 0)
            ship_click_sfx.play();

        /*
        back_click_sfx.setVolume(volume);
        if (state.all_accum_contact > 0)
            back_click_sfx.play();
            */
    }

    { // reset
        time.restart();
        state.ship_accum_contact = 0;
        state.all_accum_contact = 0;
        state.all_energy = 0;
    }
}

void GameWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_O)
    {
        renderLater();
        return;
    }
    if (event->key() == Qt::Key_P)
    {
        setAnimated(!isAnimated());
        return;
    }
    if (event->key() == Qt::Key_Space)
    {
        if (state.joint) state.release();
        else if (state.canGrab()) state.grab();
        return;
    }
    if (event->key() == Qt::Key_A)
    {
        draw_debug = !draw_debug;
        qDebug() << "draw_debug" << draw_debug;
        return;
    }
    if (event->key() == Qt::Key_Z)
    {
        std::uniform_real_distribution<double> dist_angle(0, 2 * M_PI);
        const auto angle = dist_angle(rng);

        std::normal_distribution<double> dist_normal(0, 10);
        const b2Vec2 velocity(dist_normal(rng), dist_normal(rng));
        state.addCrate({ 0, 10 }, velocity, angle);
        return;
    }
    if (event->key() == Qt::Key_Up)
    {
        engine_sfx.setMuted(false);
        state.ship_firing = true;
        return;
    }
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
    {
        state.ship_target_angular_velocity = (state.isGrabbed() ? 2. : 2.6) * M_PI / 2. * (event->key() == Qt::Key_Left ? 1. : -1.);
        return;
    }
    RasterWindow::keyPressEvent(event);
}

void GameWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Up)
    {
        engine_sfx.setMuted(true);
        state.ship_firing = false;
        return;
    }
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
    {
        state.ship_target_angular_velocity = 0;
        return;
    }
    RasterWindow::keyReleaseEvent(event);
}

