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
#include <sstream>
#include <unordered_set>
#include <bitset>

const float camera_world_zoom = 1.5;
const QVector2D camera_world_center { 0, -120 };

GameWindowOpenGL::GameWindowOpenGL(QWindow* parent)
    : RasterWindowOpenGL(parent)
{
    qDebug() << "========== levels";
    level_datas = levels::load(":levels.json");
    qDebug() << level_datas.size() << "levels";
    for (const auto& level_data : level_datas)
        qDebug() << "level" << QString::fromStdString(level_data.name) << QString::fromStdString(level_data.map_filename) << level_data.doors.size();

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
}

void GameWindowOpenGL::resetLevel(const int level)
{
    using std::get;
    qDebug() << "reset level" << level << level_current;

    if (level == level_current)
        return;

    if (level < 0)
    {
        state = std::make_unique<GameState>();
        state->dumpCollisionData();
        level_current = level;
        return;
    }

    assert(level >= 0 );
    assert(level < static_cast<int>(level_datas.size()));
    const auto& level_data = level_datas[level];

    qDebug() << "loading" << QString::fromStdString(level_data.name);

    state = std::make_unique<GameState>();
    state->resetGround(level_data.map_filename);
    loadBackground(level_data.map_filename);

    for (const auto& door : level_data.doors)
        state->addDoor(get<0>(door), get<1>(door), get<2>(door));

    for (const auto& path : level_data.paths)
        state->addPath(get<0>(path), get<1>(path));

    state->dumpCollisionData();
    level_current = level;
}

void GameWindowOpenGL::loadBackground(const std::string& map_filename)
{
    auto& renderer = map_renderer;
    const auto load_ok = renderer.load(QString::fromStdString(map_filename));
    //qDebug() << "renderer" << renderer.isValid() << load_ok;
    assert(renderer.isValid());
    assert(load_ok);
}

void GameWindowOpenGL::setMuted(const bool muted)
{
    is_muted = muted;

    if (!engine_sfx.isMuted() && is_muted) engine_sfx.setMuted(true);
    ship_click_sfx.setMuted(muted);
    //back_click_sfx.setMuted(muted);
}

void GameWindowOpenGL::initializePrograms()
{
    {
        assert(!base_program);
        base_program = loadAndCompileProgram(":base_vertex.glsl", ":base_fragment.glsl");

        assert(base_program);
        base_pos_attr = base_program->attributeLocation("posAttr");
        base_mat_unif = base_program->uniformLocation("matrix");
        qDebug() << "locations" << base_pos_attr << base_mat_unif;
        assert(base_pos_attr >= 0);
        assert(base_mat_unif >= 0);
        assertNoError();
    }

    {
        assert(!main_program);
        main_program = loadAndCompileProgram(":main_vertex.glsl", ":main_fragment.glsl");

        assert(main_program);
        main_pos_attr = main_program->attributeLocation("posAttr");
        main_col_attr = main_program->attributeLocation("colAttr");
        main_mat_unif = main_program->uniformLocation("matrix");
        qDebug() << "locations" << main_pos_attr << main_col_attr << main_mat_unif;
        assert(main_pos_attr >= 0);
        assert(main_col_attr >= 0);
        assert(main_mat_unif >= 0);
        assertNoError();
    }

    {
        assert(!ball_program);
        ball_program = loadAndCompileProgram(":ball_vertex.glsl", ":ball_fragment.glsl");

        assert(ball_program);
        ball_pos_attr = ball_program->attributeLocation("posAttr");
        ball_mat_unif = ball_program->uniformLocation("matrix");
        ball_angular_speed_unif = ball_program->uniformLocation("circularSpeed");
        qDebug() << "locations" << ball_pos_attr << ball_mat_unif << ball_angular_speed_unif;
        assert(ball_pos_attr >= 0);
        assert(ball_mat_unif >= 0);
        assert(ball_angular_speed_unif >= 0);
        assertNoError();
    }

    {
        assert(!particle_program);
        particle_program = loadAndCompileProgram(":particle_vertex.glsl", ":particle_fragment.glsl", ":particle_geometry.glsl");

        assert(particle_program);
        particle_pos_attr = particle_program->attributeLocation("posAttr");
        particle_col_attr = particle_program->attributeLocation("colAttr");
        particle_speed_attr = particle_program->attributeLocation("speedAttr");
        particle_flag_attr = particle_program->attributeLocation("flagAttr");
        particle_mat_unif = particle_program->uniformLocation("matrix");
        particle_water_color_unif = particle_program->uniformLocation("waterColor");
        particle_foam_color_unif = particle_program->uniformLocation("foamColor");
        particle_radius_unif = particle_program->uniformLocation("radius");
        particle_radius_factor_unif = particle_program->uniformLocation("radiusFactor");
        particle_mode_unif = particle_program->uniformLocation("mode");
        particle_poly_unif = particle_program->uniformLocation("poly");
        particle_max_speed_unif = particle_program->uniformLocation("maxSpeed");
        particle_alpha_unif = particle_program->uniformLocation("alpha");
        qDebug() << "attr_locations" << particle_pos_attr << particle_col_attr << particle_speed_attr << particle_flag_attr;
        assert(particle_pos_attr >= 0);
        assert(particle_col_attr >= 0);
        assert(particle_speed_attr >= 0);
        assert(particle_flag_attr >= 0);
        qDebug() << "unif_locations" << particle_mat_unif << particle_water_color_unif << particle_foam_color_unif << particle_radius_unif << particle_radius_factor_unif << particle_mode_unif << particle_poly_unif << particle_max_speed_unif << particle_alpha_unif;
        assert(particle_mat_unif >= 0);
        assert(particle_water_color_unif >= 0);
        assert(particle_foam_color_unif >= 0);
        assert(particle_radius_unif >= 0);
        assert(particle_radius_factor_unif >= 0);
        assert(particle_mode_unif >= 0);
        assert(particle_poly_unif >= 0);
        assert(particle_max_speed_unif >= 0);
        assert(particle_alpha_unif >= 0);
        assertNoError();
    }
}

void GameWindowOpenGL::initializeBuffers(BufferLoader& loader)
{
    loader.init(10);

    // ship
    loader.loadBuffer3(0, {
            { -1.8, 0, 0 },
            { 1.8, 0, 0 },
            { 0, 2*1.8, 0 },
            });
    loader.loadBuffer4(1, {
            { 1, 0, 0, 1 },
            { 0, 1, 0, 1 },
            { 0, 0, 1, 1 },
            });

    // square
    loader.loadBuffer3(2, {
            { -1, -1, 0 },
            { 1, -1, 0 },
            { -1, 1, 0 },
            { 1, 1, 0 },
            });
    loader.loadBuffer4(3, {
            { 0, 0, 1, 1 },
            { 0, 0, 1, 1 },
            { 0, 0, 1, 1 },
            { 0, 0, 1, 1 },
            });

    // cube
    loader.loadBuffer3(4, {
            { -1, -1, 1 },
            { 1, -1, 1 },
            { 1, 1, 1 },
            { -1, 1, 1 },
            { -1, -1, -1 },
            { 1, -1, -1 },
            { 1, 1, -1 },
            { -1, 1, -1 },
            });
    loader.loadIndices(5, {
            0, 1, 3, 2, 7, 6, 4, 5,
            3, 7, 0, 4, 1, 5, 2, 6,
            });

    // buffers 6, 7 & 8 are used by particle system
    loader.reserve(6); // position
    loader.reserve(7); // color
    loader.reserve(8); // speed
    loader.reserve(9); // flag
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

void GameWindowOpenGL::drawParticleSystem(QPainter& painter, const b2ParticleSystem& system, const QColor& color) const
{
    if (!draw_debug)
        return;

    const b2Vec2* positions = system.GetPositionBuffer();
    const auto kk_max = system.GetParticleCount();
    const auto radius = system.GetRadius();

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

void GameWindowOpenGL::drawBody(QPainter& painter, const b2Body& body, const QColor& color) const
{
    { // shape and origin
        const auto& position = body.GetPosition();
        const auto& angle = body.GetAngle();

        painter.save();

        painter.translate(position.x, position.y);
        painter.rotate(qRadiansToDegrees(angle));

        painter.setBrush(color);
        painter.setPen(QPen(Qt::white, 0));
        const auto* fixture = body.GetFixtureList();
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
        const auto& world_center = body.GetWorldCenter();
        const auto& linear_velocity = body.GetLinearVelocityFromWorldPoint(world_center);
        const auto& is_awake = body.IsAwake();

        painter.save();

        painter.translate(world_center.x, world_center.y);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Qt::red, 0));
        painter.drawLine(QPointF(0, 0), QPointF(linear_velocity.x, linear_velocity.y));

        painter.setBrush(is_awake ? Qt::blue : Qt::white);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(0, 0), .2, .2);

        painter.restore();
    }
}

void GameWindowOpenGL::drawFlame(QPainter& painter)
{
    assert(state);

    std::bernoulli_distribution dist_flicker;
    std::normal_distribution<float> dist_noise(0, .2);
    const auto randomPoint = [this, &dist_noise]() -> QPointF {
        return { dist_noise(flame_rng), dist_noise(flame_rng) };
    };

    const auto& body = state->ship;
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
    if (dist_flicker(flame_rng)) poly << QPointF(.5, -2.5) + randomPoint();
    poly << QPointF(0, -4) + randomPoint();
    if (dist_flicker(flame_rng)) poly << QPointF(-.5, -2.5) + randomPoint();
    poly << QPointF(-1, -3) + randomPoint();
    painter.scale(.8, .8);
    painter.translate(0, 1.5);
    painter.drawPolygon(poly);
    painter.restore();
}

void GameWindowOpenGL::drawShip(QPainter& painter)
{
    assert(state);

    const auto& body = state->ship;
    assert(body);

    if (state->ship_firing)
        drawFlame(painter);

    if (draw_debug)
        drawBody(painter, *body, Qt::black);

    if (state->canGrab() && frame_counter % 2 == 0)
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
        painter.drawLine(QPointF(0, 0), QPointF(-sin(state->ship_target_angle), cos(state->ship_target_angle)));
        painter.restore();
    }
}

void GameWindowOpenGL::paintUI()
{
    constexpr auto ui_window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

    {
        ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Once);
        //ImGui::SetNextWindowSize(ImVec2(350, 440), ImGuiCond_Once);
        //ImGui::SetNextWindowSize(ImVec2(350,400), ImGuiCond_FirstUseEver);
        //ImGui::SetNextWindowSize(ImVec2(330,100), ImGuiCond_Always);
        ImGui::Begin("~.: THRUST :.~", &display_ui, ui_window_flags);

        {
            std::vector<const char*> level_names;
            for (const auto& level_data : level_datas)
                level_names.emplace_back(level_data.name.c_str());

            int level_current_ = level_current;
            ImGui::Combo("level", &level_current_, level_names.data(), level_names.size());
            if (level_current_ != level_current)
                resetLevel(level_current_);
        }
        ImGui::Separator();

        ImGuiCallbacks();
        ImGui::Separator();

        assert(state);
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("ship %.2f %.2f", state->ship->GetPosition().x, state->ship->GetPosition().y);
        if (state->system) ImGui::Text("particles %d(%d)", state->system->GetParticleCount(), state->system->GetStuckCandidateCount());
        ImGui::Text("crates %d", static_cast<int>(state->crates.size()));
        ImGui::Text("contact %d", state->all_accum_contact);
        ImGui::Text("ship mass %f", state->ship->GetMass());
        ImGui::Text("ball mass %f", state->ball->GetMass());
        ImGui::Text(state->ship_touched_wall ? "!!!!BOOOM!!!!" : "<3<3<3<3");

        ImGui::End();
    }

    {
        ImGui::SetNextWindowPos(ImVec2(width() - 330 - 5, 5), ImGuiCond_Once);
        ImGui::Begin("Shading", &display_ui, ui_window_flags);

        //ImGui::Text("Hello, world!");
        ImGui::ColorEdit3("water color", water_color.data());
        ImGui::ColorEdit3("foam color", foam_color.data());

        {
            const char* shader_names[] = { "full grprng + center dot", "full grprng", "full uniform", "dot grprng", "dot uniform", "stuck", "default" };
            shader_selection %= IM_ARRAYSIZE(shader_names);
            ImGui::Combo("shader (Q)", &shader_selection, shader_names, IM_ARRAYSIZE(shader_names));
        }

        {
            const char* poly_names[] = { "octogon", "hexagon", "square", "triangle" };
            poly_selection %= IM_ARRAYSIZE(poly_names);
            ImGui::Combo("poly", &poly_selection, poly_names, IM_ARRAYSIZE(poly_names));
        }

        ImGui::SliderFloat("radius factor", &radius_factor, 0.0f, 1.0f);

        ImGui::SliderFloat("alpha", &shading_alpha, 0, 10);
        ImGui::SliderFloat("max speed", &shading_max_speed, 0, 100);

        ImGui::Separator();
        {
            assert(state);
            assert(state->system);
            const b2Vec2* speeds = state->system->GetVelocityBuffer();
            const auto kk_max = state->system->GetParticleCount();
            const auto max_speed = std::accumulate(speeds, speeds + kk_max, 0.f, [](const float& max_speed, const b2Vec2& speed) -> float {
                return std::max(max_speed, speed.Length());
            });
            ImGui::Text("max speed %f", max_speed);
        }

        ImGui::End();
    }

    {
        ImGui::SetNextWindowPos(ImVec2(width() - 330 - 5, 225), ImGuiCond_Once);
        ImGui::Begin("Liquid system", &display_ui, ui_window_flags);

        assert(state);
        assert(state->system);
        auto& system = *state->system;

        {
            static int value = 4;
            ImGui::SliderInt("stuck thresh", &value, 0, 10);
            system.SetStuckThreshold(value);
        }

        {
            static float value = 0;
            ImGui::SliderFloat("damping", &value, 0, 1);
            system.SetDamping(value);
        }

        {
            static float value = .1;
            ImGui::SliderFloat("density", &value, .1, 2);
            system.SetDensity(value);
        }

        ImGui::Checkbox("clean stuck in door", &state->clean_stuck_in_door);

        ImGui::Separator();

        {
            const auto flags = system.GetAllParticleFlags();
            const auto str = std::bitset<32>(flags).to_string();
            assert(str.size() == 32);
            ImGui::Text("all particle flags %d", flags);
            ImGui::Text("00-15 %s", str.substr(0, 16).c_str());
            ImGui::Text("16-31 %s", str.substr(16, 16).c_str());
        }

        ImGui::End();
    }
}

void GameWindowOpenGL::paintScene()
{
    const double dt_ = std::min(50e-3, 1. / ImGui::GetIO().Framerate);
    state->step(dt_);

    glBindVertexArray(0);

    if (!device)
        device = new QOpenGLPaintDevice;

    device->setSize(size() * devicePixelRatio());
    device->setDevicePixelRatio(devicePixelRatio());

    QPainter painter(device);
    painter.setRenderHint(QPainter::Antialiasing);

    { // background gradient
        QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, height()));
        linearGrad.setColorAt(0, QColor(0x33, 0x08, 0x67)); // morpheus den gradient
        linearGrad.setColorAt(1, QColor(0x30, 0xcf, 0xd0));
        painter.fillRect(0, 0, width(), height(), linearGrad);
    }

    { // world
        painter.save();
        painter.translate(width() / 2, height() / 2);
        painter.scale(1., -1);

        if (!is_zoom_out)
        {
            const auto& pos = state->ship->GetPosition();
            const int side = qMin(width(), height());
            const double ship_height =  75 * std::max(1., pos.y / 40.);
            painter.scale(side / ship_height, side / ship_height);
            painter.translate(-pos.x, -std::min(20.f, pos.y));
        }
        else
        {
            painter.scale(camera_world_zoom, camera_world_zoom);
            painter.translate(-camera_world_center.toPoint());
        }

        //{
        //    const QTransform tt = painter.worldTransform();
        //    const std::array<float, 9> tt_values {
        //        static_cast<float>(tt.m11()), static_cast<float>(tt.m21()), static_cast<float>(tt.m31()),
        //        static_cast<float>(tt.m12()), static_cast<float>(tt.m22()), static_cast<float>(tt.m32()),
        //        static_cast<float>(tt.m13()), static_cast<float>(tt.m23()), static_cast<float>(tt.m33())
        //    };
        //    const QMatrix3x3 foo(tt_values.data());
        //    qDebug() << foo << world_matrix;
        //}

        { // svg
            constexpr double scale = 600;
            painter.save();
            painter.scale(scale, scale);
            map_renderer.render(&painter, QRectF(-.5, -.75, 1, -1));
            painter.restore();
        }

        drawOrigin(painter);
        if (draw_debug)
            drawBody(painter, *state->ground);

        for (auto& crate : state->crates)
            drawBody(painter, *crate);

        for (auto& door : state->doors)
            drawBody(painter, *std::get<0>(door), Qt::yellow);

        //drawParticleSystem(painter, state->system);

        if (state->link)
        { // joint line
            assert(state->link);
            painter.save();
            const auto& anchor_aa = state->link->GetAnchorA();
            const auto& anchor_bb = state->link->GetAnchorB();
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(Qt::white, .5));
            painter.drawLine(QPointF(anchor_aa.x, anchor_aa.y), QPointF(anchor_bb.x, anchor_bb.y));
            painter.restore();
        }

        if (draw_debug)
        {
            assert(state->ball);
            const bool is_fast = state->ball->GetLinearVelocity().Length() > 30;
            drawBody(painter, *state->ball, is_fast ? QColor(0xfd, 0xa0, 0x85) : Qt::black);
        }

        drawShip(painter);

        painter.restore();
    }

    glClear(GL_DEPTH_BUFFER_BIT);

    const auto world_matrix = [this]() -> QMatrix4x4
    {
        QMatrix4x4 matrix;
        const auto ratio = static_cast<double>(width()) / height();
        const auto norm_width = ratio;
        const double norm_height = 1;
        matrix.ortho(-norm_width, norm_width, -norm_height, norm_height, 0, 100);
        //matrix.perspective(60.0f, width() / static_cast<float>(height()), 0.1f, 10.0f);

        matrix.translate(0, 0, -50);
        matrix.scale(2, 2, 2);

        if (!is_zoom_out)
        {
            const auto& pos = state->ship->GetPosition();
            const auto side = std::min(ratio, 1.);
            const double ship_height = 75 * std::max(1., pos.y / 40.);
            matrix.scale(side / ship_height, side / ship_height, 1);
            matrix.translate(-pos.x, -std::min(20.f, pos.y), 0);
        }
        else
        {
            const int side = qMin(width(), height());
            matrix.scale(camera_world_zoom/side, camera_world_zoom/side, camera_world_zoom);
            matrix.translate(-camera_world_center);
        }

        return matrix;
    }();

    { // draw with base program
        ProgramBinder binder(*this, base_program);

        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        const auto blit_cube = [this]() -> void
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[5]);
            assertNoError();

            glBindBuffer(GL_ARRAY_BUFFER, vbos[4]);
            glVertexAttribPointer(base_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(base_pos_attr);
            assertNoError();

            //glEnable(GL_CULL_FACE);
            glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, reinterpret_cast<void*>(0));
            glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, reinterpret_cast<void*>(8 * sizeof(unsigned int)));
            //glDisable(GL_CULL_FACE);
            assertNoError();

            glDisableVertexAttribArray(base_pos_attr);
            assertNoError();
        };

        {
            auto matrix = world_matrix;
            matrix.translate(0, 10);
            matrix.scale(3, 3, 3);
            matrix.rotate(frame_counter, 1, 1, 1);
            base_program->setUniformValue(base_mat_unif, matrix);

            blit_cube();
        }
    }

    { // draw with particle program
        ProgramBinder binder(*this, particle_program);

        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        { // particle system
            const auto& system = state->system;
            assert(system);

            const b2Vec2* positions = system->GetPositionBuffer();
            const b2ParticleColor* colors = system->GetColorBuffer();
            const b2Vec2* speeds = system->GetVelocityBuffer();
            const auto kk_max = system->GetParticleCount();
            const auto radius = system->GetRadius();

            std::vector<GLuint> flags(kk_max);
            {
                std::fill(std::begin(flags), std::end(flags), 0);
                const auto candidates = system->GetStuckCandidates();
                for (auto ll=0, ll_max=system->GetStuckCandidateCount(); ll < ll_max; ll++)
                    flags[candidates[ll]] = 1;
            }


            particle_program->setUniformValue(particle_radius_unif, radius);
            particle_program->setUniformValue(particle_radius_factor_unif, radius_factor);
            particle_program->setUniformValue(particle_mode_unif, shader_selection);
            particle_program->setUniformValue(particle_poly_unif, poly_selection);
            particle_program->setUniformValue(particle_max_speed_unif, shading_max_speed);
            particle_program->setUniformValue(particle_alpha_unif, shading_alpha);

            //            const auto& color = QColor::fromRgb(0x6cu, 0xc3u, 0xf6u, 0xffu);
            particle_program->setUniformValue(particle_water_color_unif, QColor::fromRgbF(water_color[0], water_color[1], water_color[2], water_color[3]));
            particle_program->setUniformValue(particle_foam_color_unif, QColor::fromRgbF(foam_color[0], foam_color[1], foam_color[2], foam_color[3]));
            particle_program->setUniformValue(particle_mat_unif, world_matrix);
            assertNoError();

            glBindBuffer(GL_ARRAY_BUFFER, vbos[6]);
            glBufferData(GL_ARRAY_BUFFER, kk_max * 2 * sizeof(GLfloat), positions, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(particle_pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(particle_pos_attr);
            assertNoError();

            static_assert(std::is_same<GLubyte, decltype(b2ParticleColor::r)>::value, "mismatching color types");

            glBindBuffer(GL_ARRAY_BUFFER, vbos[7]);
            glBufferData(GL_ARRAY_BUFFER, kk_max * 4 * sizeof(GLubyte), colors, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(particle_col_attr, 4,  GL_UNSIGNED_BYTE, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(particle_col_attr);
            assertNoError();

            glBindBuffer(GL_ARRAY_BUFFER, vbos[8]);
            glBufferData(GL_ARRAY_BUFFER, kk_max * 2 * sizeof(GLfloat), speeds, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(particle_speed_attr, 2,  GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(particle_speed_attr);
            assertNoError();

            glBindBuffer(GL_ARRAY_BUFFER, vbos[9]);
            glBufferData(GL_ARRAY_BUFFER, kk_max * sizeof(GLuint), flags.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(particle_flag_attr, 1,  GL_UNSIGNED_INT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(particle_flag_attr);
            assertNoError();

            glDrawArrays(GL_POINTS, 0, kk_max);
            assertNoError();

            glDisableVertexAttribArray(particle_flag_attr);
            glDisableVertexAttribArray(particle_speed_attr);
            glDisableVertexAttribArray(particle_col_attr);
            glDisableVertexAttribArray(particle_pos_attr);
            assertNoError();
        }
    }

    { // draw with main program
        ProgramBinder binder(*this, main_program);

        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        const auto blit_triangle = [this]() -> void
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
            glVertexAttribPointer(main_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(main_pos_attr);
            assertNoError();

            glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
            glVertexAttribPointer(main_col_attr, 4, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(main_col_attr);
            assertNoError();

            glDrawArrays(GL_TRIANGLES, 0, 3);
            assertNoError();

            glDisableVertexAttribArray(main_col_attr);
            glDisableVertexAttribArray(main_pos_attr);
            assertNoError();
        };

        const auto blit_square = [this]() -> void
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
            glVertexAttribPointer(main_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(main_pos_attr);
            assertNoError();

            glBindBuffer(GL_ARRAY_BUFFER, vbos[3]);
            glVertexAttribPointer(main_col_attr, 4, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(main_col_attr);
            assertNoError();

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            assertNoError();

            glDisableVertexAttribArray(main_col_attr);
            glDisableVertexAttribArray(main_pos_attr);
            assertNoError();
        };

        { // ship
            QMatrix4x4 matrix = world_matrix;

            const auto& pos = state->ship->GetPosition();
            matrix.translate(pos.x, pos.y);

            matrix.rotate(180. * state->ship->GetAngle() / M_PI, 0, 0, 1);
            matrix.rotate(frame_counter, 0, 1, 0);

            main_program->setUniformValue(main_mat_unif, matrix);
            assertNoError();

            blit_triangle();
            blit_square();

            matrix.rotate(90, 0, 1, 0);

            main_program->setUniformValue(main_mat_unif, matrix);
            assertNoError();

            blit_triangle();
        }
    }

    { // draw with ball program
        ProgramBinder binder(*this, ball_program);

        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        const auto blit_square = [this]() -> void
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
            glVertexAttribPointer(main_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(main_pos_attr);
            assertNoError();

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            assertNoError();

            glDisableVertexAttribArray(main_pos_attr);
            assertNoError();
        };

        {
            assert(state->ball);
            assert(state->ball->GetFixtureList());
            assert(state->ball->GetFixtureList()->GetShape());
            const auto& shape = static_cast<const b2CircleShape&>(*state->ball->GetFixtureList()->GetShape());

            auto matrix = world_matrix;
            const auto& pos = state->ball->GetWorldCenter();
            const auto& angle = state->ball->GetAngle();
            matrix.translate(pos.x, pos.y);
            matrix.rotate(qRadiansToDegrees(angle), 0, 0, 1);
            matrix.scale(shape.m_radius, shape.m_radius, shape.m_radius);
            //matrix.rotate(frame_counter, 1, 1, 1);
            ball_program->setUniformValue(ball_mat_unif, matrix);

            const auto& angular_speed = state->ball->GetAngularVelocity();
            ball_program->setUniformValue(ball_angular_speed_unif, angular_speed);

            blit_square();
        }
    }

    { // sfx
        if (state->ship_accum_contact > 0)
            ship_click_sfx.play();

        //back_click_sfx.setVolume(volume);
        //if (state->all_accum_contact > 0)
        //    back_click_sfx.play();
    }

    { // reset
        state->ship_accum_contact = 0;
        state->all_accum_contact = 0;
        state->all_energy = 0;
    }
}

void GameWindowOpenGL::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Q)
    {
        shader_selection++;
        return;
    }

    if (event->key() == Qt::Key_Space)
    {
        assert(state);
        if (state->link) state->release();
        else if (state->canGrab()) state->grab();
        return;
    }

    if (event->key() == Qt::Key_Up)
    {
        assert(state);
        engine_sfx.setMuted(is_muted);
        state->ship_firing = true;
        return;
    }
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
    {
        assert(state);
        state->ship_target_angular_velocity = (state->isGrabbed() ? 2. : 2.6) * M_PI / 2. * (event->key() == Qt::Key_Left ? 1. : -1.);
        return;
    }

    RasterWindowOpenGL::keyPressEvent(event);
}

void GameWindowOpenGL::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Up)
    {
        assert(state);
        engine_sfx.setMuted(true);
        state->ship_firing = false;
        return;
    }
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
    {
        assert(state);
        state->ship_target_angular_velocity = 0;
        return;
    }

    RasterWindowOpenGL::keyReleaseEvent(event);
}

