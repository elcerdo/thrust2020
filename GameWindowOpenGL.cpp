#include "GameWindowOpenGL.h"

#include "Box2D/Dynamics/b2Fixture.h"
#include "Box2D/Collision/Shapes/b2PolygonShape.h"
#include "Box2D/Collision/Shapes/b2CircleShape.h"

#include "QtImGui.h"
#include <QDebug>
#include <QPainter>
#include <QKeyEvent>
#include <QtMath>
#include <QFontDatabase>
#include <QOpenGLShaderProgram>

#include <imgui.h>

#include <random>
#include <iostream>

GameWindowOpenGL::GameWindowOpenGL(QWindow* parent)
    : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate, parent)
{
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

    /*{
        auto& sfx = back_click_sfx;
        const auto click_sound = QUrl::fromLocalFile(":click00.wav");
        assert(click_sound.isValid());
        sfx.setSource(click_sound);
        sfx.setLoopCount(1);
        sfx.setVolume(.5);
        sfx.setMuted(false);
        sfx.stop();
    }*/

    {
        auto& renderer = map_renderer;
        const auto load_ok = renderer.load(QString(":map.svg"));
        //qDebug() << "renderer" << renderer.isValid() << load_ok;
        assert(renderer.isValid());
    }
}

void GameWindowOpenGL::setMuted(const bool muted)
{
    is_muted = muted;

    if (!engine_sfx.isMuted() && is_muted) engine_sfx.setMuted(true);
    ship_click_sfx.setMuted(muted);
    //back_click_sfx.setMuted(muted);
}

void GameWindowOpenGL::setAnimated(const bool value)
{
    is_animated = value;
    if (is_animated)
        update();
}

void GameWindowOpenGL::initializeGL()
{

    /*
    assert(!context);
    context = new QOpenGLContext(this);
    context->setFormat(requestedFormat());
    context->create();
    qDebug() << context->format();

    assert(context);
    context->makeCurrent(this);
    */
    qDebug() << QOpenGLContext::currentContext();

    initializeOpenGLFunctions();
    QtImGui::initialize(this);
    assert(glGetError() == GL_NO_ERROR);

    assert(!program);
    program = new QOpenGLShaderProgram;
    const auto vertex_compile_ok = [this]() -> bool {
        QFile handle(":vertex.glsl");
        if (!handle.open(QIODevice::ReadOnly))
            return false;
        const auto& source = handle.readAll();
        return program->addShaderFromSourceCode(QOpenGLShader::Vertex, source);
    }();
    const auto fragment_compile_ok = [this]() -> bool {
        QFile handle(":fragment.glsl");
        if (!handle.open(QIODevice::ReadOnly))
            return false;
        const auto& source = handle.readAll();
        return program->addShaderFromSourceCode(QOpenGLShader::Fragment, source);
    }();
    const auto link_ok = program->link();
    qDebug() << "ok" << link_ok << vertex_compile_ok << fragment_compile_ok;
    const auto all_ok = vertex_compile_ok && fragment_compile_ok && link_ok;
    qDebug() << program->log();
    assert(all_ok);

    /*
    */

    assert(program);
    pos_attr = program->attributeLocation("posAttr");
    col_attr = program->attributeLocation("colAttr");
    mat_unif = program->uniformLocation("matrix");
    dot_unif = program->uniformLocation("dotColor");
    qDebug() << "attrs" << pos_attr << col_attr << mat_unif << dot_unif;
    assert(pos_attr >= 0);
    assert(col_attr >= 0);
    assert(mat_unif >= 0);
    assert(dot_unif >= 0);
    assert(glGetError() == GL_NO_ERROR);

    { // ship vao
        using std::cout;
        using std::endl;

        glGenVertexArrays(1, &vao);
        qDebug() << "vao" << vao;
        glBindVertexArray(vao);

        glGenBuffers(vbos.size(), vbos.data());
        cout << "vbos";
        for (const auto& vbo : vbos) cout << " " << vbo;
        cout << endl;

        const auto load_buffer3 = [this](const size_t kk, const std::vector<b2Vec3>& vertices) -> void
        {
            assert(vbos.size() > kk);
            glBindBuffer(GL_ARRAY_BUFFER, vbos[kk]);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * 3 * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
        };

        const auto load_buffer4 = [this](const size_t kk, const std::vector<b2Vec4>& vertices) -> void
        {
            assert(vbos.size() > kk);
            glBindBuffer(GL_ARRAY_BUFFER, vbos[kk]);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * 4 * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
        };

        // ship
        load_buffer3(0, {
            { -1.8, 0, 0 },
            { 1.8, 0, 0 },
            { 0, 2*1.8, 0 },
        });
        load_buffer4(1, {
            { 1, 0, 0, 1 },
            { 0, 1, 0, 1 },
            { 0, 0, 1, 1 },
        });

        // square
        load_buffer3(2, {
            { -1, -1, 0 },
            { 1, -1, 0 },
            { -1, 1, 0 },
            { 1, 1, 0 },
        });
        load_buffer4(3, {
            { 0, 0, 1, 1 },
            { 0, 0, 1, 1 },
            { 0, 0, 1, 1 },
            { 0, 0, 1, 1 },
        });


        // octogon
        {
            using std::cout;
            using std::endl;
            std::vector<b2Vec3> positions;
            std::vector<b2Vec4> colors;

            positions.emplace_back(b2Vec3 { 0, 0, 0 });
            colors.emplace_back(b2Vec4 { 0, 0, 0, 1 });

            for (int kk=0; kk<=8; kk++)
            {
                const float theta = 2. * kk * M_PI / 8;
                positions.emplace_back(1.2f * b2Vec3 { cos(theta), sin(theta), 0 });
                colors.emplace_back(b2Vec4 { 0, 0, 0, 1 });
            }

            assert(positions.size() == 10);
            assert(colors.size() == 10);
            load_buffer3(4, positions);
            load_buffer4(5, colors);
        }

    }

}

void GameWindowOpenGL::addCheckbox(const std::string& label, const bool& value, const BoolCallback& callback)
{
    auto state = std::make_tuple(label, value, callback);
    bool_states.emplace_back(state);
    std::get<2>(state)(std::get<1>(state));
}

void GameWindowOpenGL::addSlider(const std::string& label, const float& min, const float& max, const float& value, const FloatCallback& callback)
{
    auto state = std::make_tuple(label, min, max, value, callback);
    float_states.emplace_back(state);
    std::get<4>(state)(std::get<3>(state));
}

void GameWindowOpenGL::drawOrigin(QPainter& painter) const
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

void GameWindowOpenGL::drawParticleSystem(QPainter& painter, const b2ParticleSystem* system, const QColor& color) const
{
    if (!draw_debug)
        return;

    assert(system);

    const b2Vec2* positions = system->GetPositionBuffer();
    const auto kk_max = system->GetParticleCount();
    const auto radius = system->GetRadius();

    painter.setBrush(color);
    painter.setPen(QPen(Qt::white, 0));

    for (auto kk=0; kk<kk_max; kk++)
    {
        painter.save();
        painter.translate(positions[kk].x, positions[kk].y);
        painter.drawEllipse(QPointF(0, 0), radius, radius);
        painter.restore();
    }
}

void GameWindowOpenGL::drawBody(QPainter& painter, const b2Body* body, const QColor& color) const
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
        const QColor speed_color_ = QColor::fromRgbF(speed_color[0], speed_color[1], speed_color[2], speed_color[3]);
        painter.setPen(QPen(speed_color_, 0));
        painter.drawLine(QPointF(0, 0), QPointF(linear_velocity.x, linear_velocity.y));

        painter.setBrush(is_awake ? Qt::blue : Qt::white);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(0, 0), .2, .2);

        painter.restore();
    }
}

void GameWindowOpenGL::drawFlame(QPainter& painter)
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

void GameWindowOpenGL::drawShip(QPainter& painter)
{
    const auto* body = state.ship;
    assert(body);

    if (state.ship_firing) drawFlame(painter);

    if (draw_debug)
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

void GameWindowOpenGL::paintGL()
{
    const double dt_ = std::min(50e-3, 1. / ImGui::GetIO().Framerate);
    state.step(dt_);

    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    assert(glGetError() == GL_NO_ERROR);

    QtImGui::newFrame();

    // 1. Show a simple window
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
    if (display_ui)
    {
        //static float f = 0.0f;
        //ImGui::Text("Hello, world!");
        //ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::ColorEdit3("speed color", speed_color.data());
        //if (ImGui::Button("Test Window")) show_test_window ^= 1;
        //if (ImGui::Button("Another Window")) show_another_window ^= 1;

        for (auto& state : float_states)
        {
            const auto prev = std::get<3>(state);
            ImGui::SliderFloat(std::get<0>(state).c_str(), &std::get<3>(state), std::get<1>(state), std::get<2>(state));
            if (prev != std::get<3>(state)) std::get<4>(state)(std::get<3>(state));
        }

        for (auto& state : bool_states)
        {
            const auto prev = std::get<1>(state);
            ImGui::Checkbox(std::get<0>(state).c_str(), &std::get<1>(state));
            if (prev != std::get<1>(state)) std::get<2>(state)(std::get<1>(state));
        }

        ImGui::Separator();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Ball density %.2f", state.ball->GetFixtureList()->GetDensity());
    }

    /*
    // 2. Show another simple window, this time using an explicit Begin/End pair
    {
        ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiCond_FirstUseEver);
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello");
        ImGui::End();
    }

    // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
    if (show_test_window)
    {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
        ImGui::ShowDemoWindow();
    }
    */

    glBindVertexArray(0);

    if (!device)
        device = new QOpenGLPaintDevice;

    device->setSize(size() * devicePixelRatio());
    device->setDevicePixelRatio(devicePixelRatio());

    static const QFont fixed_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    QPainter painter(device);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(fixed_font);

    { // background gradient
        QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, height()));
        linearGrad.setColorAt(0, QColor(0x33, 0x08, 0x67)); // morpheus den gradient
        linearGrad.setColorAt(1, QColor(0x30, 0xcf, 0xd0));
        painter.fillRect(0, 0, width() / 2, height(), linearGrad);
    }

    { // world
        painter.save();
        painter.translate(width() / 2, height() / 2);
        painter.scale(1., -1);

        const auto& pos = state.ship->GetPosition();
        const int side = qMin(width(), height());
        const double ship_height =  75 * std::max(1., pos.y / 40.);
        const double factor = is_zoom_out ? 2 : 1;
        painter.scale(factor * side / ship_height, factor * side / ship_height);
        //painter.scale(side / height, side / height);
        painter.translate(-pos.x, -std::min(20.f, pos.y));


        { // svg
            constexpr double scale = 600;
            painter.save();
            painter.scale(scale, scale);
            map_renderer.render(&painter, QRectF(-.5, -.75, 1, -1));
            painter.restore();
        }

        drawOrigin(painter);
        drawBody(painter, state.left_side);
        drawBody(painter, state.right_side);
        drawBody(painter, state.ground);

        for (auto& crate : state.crates)
            drawBody(painter, crate);

        drawParticleSystem(painter, state.system);

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

        if (state.system) print(QString("particles %1").arg(state.system->GetParticleCount(), 4, 10, fill));
        print(QString("  creates %1").arg(state.crates.size(), 4, 10, fill));
        print(QString("   thrust %1%").arg(state.ship_thrust_factor * 100, 4, 'f', 0, fill));
        print(QString("  contact %1").arg(state.all_accum_contact, 4, 10, fill));
        if (state.ship_touched_wall) print("!!!!BOOOM!!!!");

        painter.restore();
    }

    { // draw with custom shader
        glBindVertexArray(vao);

        glClear(GL_DEPTH_BUFFER_BIT);

        assert(program);
        assert(program->isLinked());
        program->bind();
        assert(glGetError() == GL_NO_ERROR);

        const auto blit_triangle = [this]() -> void
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
            glVertexAttribPointer(pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(pos_attr);
            assert(glGetError() == GL_NO_ERROR);

            glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
            glVertexAttribPointer(col_attr, 4, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(col_attr);
            assert(glGetError() == GL_NO_ERROR);

            //glEnable(GL_DEPTH_TEST);
            //glDepthFunc(GL_GREATER);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            //glDisable(GL_DEPTH_TEST);
            assert(glGetError() == GL_NO_ERROR);

            glDisableVertexAttribArray(col_attr);
            glDisableVertexAttribArray(pos_attr);
            assert(glGetError() == GL_NO_ERROR);
        };

        const auto blit_square = [this]() -> void
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
            glVertexAttribPointer(pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(pos_attr);
            assert(glGetError() == GL_NO_ERROR);

            glBindBuffer(GL_ARRAY_BUFFER, vbos[3]);
            glVertexAttribPointer(col_attr, 4, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(col_attr);
            assert(glGetError() == GL_NO_ERROR);

            //glEnable(GL_DEPTH_TEST);
            //glDepthFunc(GL_GREATER);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            //glDisable(GL_DEPTH_TEST);
            assert(glGetError() == GL_NO_ERROR);

            glDisableVertexAttribArray(col_attr);
            glDisableVertexAttribArray(pos_attr);
            assert(glGetError() == GL_NO_ERROR);
        };

        const auto blit_octogon = [this, &blit_square]() -> void
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbos[4]);
            glVertexAttribPointer(pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(pos_attr);
            assert(glGetError() == GL_NO_ERROR);

            glBindBuffer(GL_ARRAY_BUFFER, vbos[5]);
            glVertexAttribPointer(col_attr, 4, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(col_attr);
            assert(glGetError() == GL_NO_ERROR);

            //glEnable(GL_DEPTH_TEST);
            //glDepthFunc(GL_GREATER);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 10);
            //glDisable(GL_DEPTH_TEST);
            assert(glGetError() == GL_NO_ERROR);

            glDisableVertexAttribArray(col_attr);
            glDisableVertexAttribArray(pos_attr);
            assert(glGetError() == GL_NO_ERROR);
        };

        const auto world_matrix = [this]() -> QMatrix4x4
        {
            QMatrix4x4 matrix;
            const auto ratio = static_cast<double>(width()) / height();
            const auto norm_width = ratio;
            const double norm_height = 1;
            matrix.ortho(-norm_width, norm_width, -norm_height, norm_height, 0, 100);
            //matrix.perspective(60.0f, width() / static_cast<float>(height()), 0.1f, 10.0f);

            matrix.translate(0, 0, -50);

            const auto& pos = state.ship->GetPosition();
            const auto side = std::min(ratio, 1.);
            const double ship_height = 75 * std::max(1., pos.y / 40.);
            const double factor = is_zoom_out ? 2. : 1;
            matrix.scale(factor * side / ship_height, factor * side / ship_height, 1);
            matrix.scale(2, 2, 2);
            matrix.translate(-pos.x, -std::min(20.f, pos.y));

            return matrix;
        }();

        // ship
        {
            QMatrix4x4 matrix = world_matrix;

            const auto& pos = state.ship->GetPosition();
            matrix.translate(pos.x, pos.y);

            matrix.rotate(180. * state.ship->GetAngle() / M_PI, 0, 0, 1);
            matrix.rotate(frame_counter, 0, 1, 0);

            program->setUniformValue(mat_unif, matrix);
            program->setUniformValue(dot_unif, QVector4D(0, 0, 0, 1));
            assert(glGetError() == GL_NO_ERROR);

            blit_triangle();
            blit_square();

            matrix.rotate(90, 0, 1, 0);

            program->setUniformValue(mat_unif, matrix);
            assert(glGetError() == GL_NO_ERROR);

            blit_triangle();
        }

        {
            const auto* system = state.system;
            assert(system);

            const b2Vec2* positions = system->GetPositionBuffer();
            const b2ParticleColor* colors = system->GetColorBuffer();
            const auto kk_max = system->GetParticleCount();
            const auto radius = system->GetRadius();

            //program->setUniformValue(dot_unif, QVector4D(1, 1, 1, 1));

            for (auto kk=0; kk<kk_max; kk++)
            {
                QMatrix4x4 matrix = world_matrix;
                matrix.translate(positions[kk].x, positions[kk].y);
                program->setUniformValue(mat_unif, matrix);
                const auto& color = colors[kk];
                //const auto rr = ddstatic_cast<int>(color.r)
                const auto& color_ = QColor::fromRgb(color.r, color.g, color.b, color.a);
                program->setUniformValue(dot_unif, color_);
                blit_octogon();
                assert(glGetError() == GL_NO_ERROR);
            }
        }

        program->release();
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
        state.ship_accum_contact = 0;
        state.all_accum_contact = 0;
        state.all_energy = 0;
    }

    ImGui::Render();

    //context->swapBuffers(this);

    frame_counter++;

    if (is_animated)
        update();
}

void GameWindowOpenGL::keyPressEvent(QKeyEvent* event)
{

    if (event->key() == Qt::Key_A)
    {
        display_ui ^= 1;
        return;
    }
    if (event->key() == Qt::Key_Space)
    {
        if (state.joint) state.release();
        else if (state.canGrab()) state.grab();
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
    if (event->key() == Qt::Key_E)
    {
        state.flop();
        return;
    }
    if (event->key() == Qt::Key_Up)
    {
        engine_sfx.setMuted(is_muted);
        state.ship_firing = true;
        return;
    }
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
    {
        state.ship_target_angular_velocity = (state.isGrabbed() ? 2. : 2.6) * M_PI / 2. * (event->key() == Qt::Key_Left ? 1. : -1.);
        return;
    }
}

void GameWindowOpenGL::keyReleaseEvent(QKeyEvent* event)
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
}
