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
#include <iomanip>

const char* shader_names[] = { "out group + dot speed", "out speed + dot flag", "out flag + dot speed", "out flag + dot group", "dot group", "dot uniform", "dot stuck", "dot flag", "dot speed" };
const int shader_switch_key = Qt::Key_Q;
const int level_switch_key = Qt::Key_L;

constexpr auto ui_window_width = 330;
constexpr auto ui_window_spacing = 5;
constexpr auto ui_window_flags = ImGuiWindowFlags_AlwaysAutoResize | /*ImGuiWindowFlags_NoBackground |*/ ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

GameWindowOpenGL::GameWindowOpenGL(QWindow* parent)
    : RasterWindowOpenGL(parent)
{
    registerFreeKey(shader_switch_key);
    registerFreeKey(level_switch_key);
    registerFreeKey(Qt::Key_Space);
    registerFreeKey(Qt::Key_Left);
    registerFreeKey(Qt::Key_Right);
    registerFreeKey(Qt::Key_Up);

    {
        auto& sfx = engine_sfx;
        const auto sound = QUrl::fromLocalFile(":/sounds/engine.wav");
        assert(sound.isValid());
        sfx.setSource(sound);
        sfx.setLoopCount(QSoundEffect::Infinite);
        sfx.setVolume(.3);
        sfx.setMuted(true);
        sfx.play();
    }

    {
        auto& sfx = ship_click_sfx;
        const auto click_sound = QUrl::fromLocalFile(":/sounds/click01.wav");
        assert(click_sound.isValid());
        sfx.setSource(click_sound);
        sfx.setLoopCount(1);
        sfx.setVolume(.2);
        sfx.setMuted(false);
        sfx.stop();
    }

    {
        logo = QImage(":/images/logo.png");
        assert(!logo.size().isEmpty());
    }

    /*{
        auto& sfx = back_click_sfx;
        const auto click_sound = QUrl::fromLocalFile(":/sounds/click00.wav");
        assert(click_sound.isValid());
        sfx.setSource(click_sound);
        sfx.setLoopCount(1);
        sfx.setVolume(.5);
        sfx.setMuted(false);
        sfx.stop();
    }*/

    {
        qDebug() << "========== levels";
        data = levels::load(":/levels/levels.json");
        qDebug() << data.levels.size() << "levels";
        for (const auto& level : data.levels)
            qDebug() << "level" << QString::fromStdString(level.name) << QString::fromStdString(level.map_filename) << level.doors.size();
        current_level = data.default_level;
        resetLevel();
    }

}

void GameWindowOpenGL::resetLevel()
{
    using std::cout;
    using std::endl;
    using std::get;

    world_time = 0;
    world_camera = Camera();
    ship_camera = Camera();

    if (current_level < 0)
    {
        current_level = -1;
        state = nullptr;
        return;
    }

    assert(current_level >= 0 );
    assert(current_level < static_cast<int>(data.levels.size()));
    const auto& level = data.levels[current_level];

    cout << "========== loading " << std::quoted(level.name) << endl;

    {
        state = std::make_unique<GameState>();
        state->resetGround(level.map_filename);
        loadBackground(level.map_filename);

        for (const auto& door : level.doors)
            state->addDoor(get<0>(door), get<1>(door), get<2>(door));

        for (const auto& path : level.paths)
            state->addPath(get<0>(path), get<1>(path));

        state->dumpCollisionData();
    }

    {
        world_camera.screen_height = level.world_screen_height;
        world_camera.position = { level.world_camera_position.x, level.world_camera_position.y };
    }

    enforceCallbackValues();

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
        device = std::make_unique<QOpenGLPaintDevice>();
        device->setDevicePixelRatio(devicePixelRatio());
    }

    {
        assert(!base_program);
        base_program = loadAndCompileProgram(":/shaders/base_vertex.glsl", ":/shaders/base_fragment.glsl");

        assert(base_program);
        const auto init_ok = initLocations(*base_program, {
                { "posAttr", base_pos_attr },
                }, {
                { "cameraMatrix", base_camera_mat_unif },
                { "worldMatrix", base_world_mat_unif },
                });
        assert(init_ok);
        assertNoError();
    }

    {
        assert(!main_program);
        main_program = loadAndCompileProgram(":/shaders/main_vertex.glsl", ":/shaders/main_fragment.glsl");

        assert(main_program);
        const auto init_ok = initLocations(*main_program, {
                { "posAttr", main_pos_attr },
                { "colAttr", main_col_attr },
                }, {
                { "cameraMatrix", main_camera_mat_unif },
                { "worldMatrix", main_world_mat_unif },
                });
        assert(init_ok);
        assertNoError();
    }

    {
        assert(!ball_program);
        ball_program = loadAndCompileProgram(":/shaders/ball_vertex.glsl", ":/shaders/ball_fragment.glsl");

        assert(ball_program);
        const auto init_ok = initLocations(*ball_program, {
                { "posAttr", ball_pos_attr },
                }, {
                { "circularSpeed", ball_angular_speed_unif },
                { "cameraMatrix", ball_camera_mat_unif },
                { "worldMatrix", ball_world_mat_unif },
                });
        assert(init_ok);
        assertNoError();
    }

    {
        assert(!grab_program);
        grab_program = loadAndCompileProgram(":/shaders/grab_vertex.glsl", ":/shaders/grab_fragment.glsl");

        assert(grab_program);
        const auto init_ok = initLocations(*grab_program, {
                { "posAttr", grab_pos_attr },
                }, {
                { "time", grab_time_unif },
                { "haloOuterColor", grab_halo_out_color_unif },
                { "haloInnerColor", grab_halo_in_color_unif },
                { "cameraMatrix", grab_camera_mat_unif },
                { "worldMatrix", grab_world_mat_unif },
                });
        assert(init_ok);
        assertNoError();
    }

    {
        assert(!particle_program);
        particle_program = loadAndCompileProgram(":/shaders/particle_vertex.glsl", ":/shaders/particle_fragment.glsl", ":/shaders/particle_geometry.glsl");

        assert(particle_program);
        const auto init_ok = initLocations(*particle_program, {
                { "posAttr", particle_pos_attr },
                { "colAttr", particle_col_attr },
                { "speedAttr", particle_speed_attr },
                { "flagAttr", particle_flag_attr },
                }, {
                { "waterColor", particle_water_color_unif },
                { "foamColor", particle_foam_color_unif },
                { "radius", particle_radius_unif },
                { "radiusFactor", particle_radius_factor_unif },
                { "mode", particle_mode_unif },
                { "poly", particle_poly_unif },
                { "maxSpeed", particle_max_speed_unif },
                { "alpha", particle_alpha_unif },
                { "viscousColor", particle_viscous_color_unif },
                { "tensibleColor", particle_tensible_color_unif },
                { "mixColor", particle_mix_unif },
                { "cameraMatrix", particle_camera_mat_unif },
                { "worldMatrix", particle_world_mat_unif },
                });
        assert(init_ok);
        assertNoError();
    }

    {
        assert(!crate_texture);
        crate_texture = std::make_unique<QOpenGLTexture>(QImage(":/images/crate_texture.png").mirrored());
        assert(crate_texture->width());
        assert(crate_texture->height());

        assert(!crate_program);
        crate_program = loadAndCompileProgram(":/shaders/crate_vertex.glsl", ":/shaders/crate_fragment.glsl");

        assert(crate_program);
        const auto init_ok = initLocations(*crate_program, {
                { "posAttr", crate_pos_attr },
                }, {
                { "cameraMatrix", crate_camera_mat_unif },
                { "worldMatrix", crate_world_mat_unif },
                { "crateTexture", crate_texture_unif },
                { "baseColor", crate_color_unif },
                { "maxTag", crate_max_tag_unif },
                { "tag", crate_tag_unif },
                });
        assert(init_ok);
        assertNoError();
    }
}

void GameWindowOpenGL::initializeBuffers(BufferLoader& loader)
{
    loader.init(12);

    // ship
    loader.loadBuffer3(0, {
            { -1, 0, .1 },
            { 1, 0, .1 },
            { 0, 2, .1 },
            { -1, 0, -.1 },
            { 1, 0, -.1 },
            { 0, 2, -.1 },
            });
    loader.loadBuffer4(1, { // ship colors
            { 1, 1, 1, 1 },
            { 1, 1, 1, 1 },
            { .2, .2, 1, 1 },
            { 0, 0, 0, 1 },
            { 0, 0, 0, 1 },
            { 0, 0, 0, 1 },
            });
    loader.loadIndices(11, { // ship indices
            0, 1, 2,
            4, 3, 5,
            0, 3, 1,
            1, 3, 4,
            2, 5, 0,
            0, 5, 3,
            1, 4, 2,
            2, 4, 5,
            });

    // square
    loader.loadBuffer3(2, {
            { -1, -1, 0 },
            { 1, -1, 0 },
            { -1, 1, 0 },
            { 1, 1, 0 },
            });
    loader.loadBuffer4(10, { // morpheus den gradient colors
            { 0x30 / 255., 0xcf / 255., 0xd0 / 255., 1 },
            { 0x30 / 255., 0xcf / 255., 0xd0 / 255., 1 },
            { 0x33 / 255., 0x08 / 255., 0x67 / 255., 1 },
            { 0x33 / 255., 0x08 / 255., 0x67 / 255., 1 },
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
    loader.loadBuffer4(3, { // ship tail colors
            { 1, 1, 0, 1 },
            { 0, 1, 0, 1 },
            { 0, 0, 0, 1 },
            { 1, 1, 0, 1 },
            { 1, 1, 0, 1 },
            { 0, 1, 0, 1 },
            { 0, 1, 0, 1 },
            { 1, 1, 0, 1 },
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

void GameWindowOpenGL::drawShip(QPainter& painter)
{
    assert(state);

    const auto& body = state->ship;
    assert(body);

    if (state->ship_state.firing_thruster)
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

    if (draw_debug)
        drawBody(painter, *body, Qt::black);

    if (draw_debug && state->canGrab())
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
        const auto angle = state->ship_state.target_angle;
        painter.drawLine(QPointF(0, 0), QPointF(-4 * sin(angle), 4 * cos(angle)));
        painter.restore();
    }
}

void GameWindowOpenGL::initializeUI()
{
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.67f);

    ImGui::SetColorEditOptions(ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
}

void GameWindowOpenGL::paintUI()
{
    int left_offset = ui_window_spacing;
    const auto begin_left = [&left_offset, this](const std::string& title) -> void
    {
        ImGui::SetNextWindowPos(ImVec2(ui_window_spacing, left_offset), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(ui_window_width, -1), ImGuiCond_Always);
        ImGui::Begin(title.c_str(), &display_ui, ui_window_flags);
    };
    const auto end_left = [&left_offset]() -> void
    {
        left_offset += ImGui::GetWindowHeight();
        left_offset += ui_window_spacing;
        ImGui::End();
    };

    int right_offset = ui_window_spacing;
    const auto begin_right = [&right_offset, this](const std::string& title) -> void
    {
        ImGui::SetNextWindowPos(ImVec2(width() - ui_window_spacing - ui_window_width, right_offset), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(ui_window_width, -1), ImGuiCond_Always);
        ImGui::Begin(title.c_str(), &display_ui, ui_window_flags);
    };
    const auto end_right = [&right_offset]() -> void
    {
        right_offset += ImGui::GetWindowHeight();
        right_offset += ui_window_spacing;
        ImGui::End();
    };

    { // callbacks and general info window
        begin_left("~.: THRUST :.~");

        {
            std::vector<const char*> level_names;
            for (const auto& level : data.levels)
                level_names.emplace_back(level.name.c_str());

            const auto level_selection_prev = current_level;

            const std::string key_name = QKeySequence(level_switch_key).toString().toStdString();
            std::stringstream ss;
            ss << "level (" << key_name << ")";
            ImGui::Combo(ss.str().c_str(), &current_level, level_names.data(), level_names.size());
            if (level_selection_prev != current_level)
                resetLevel();
        }
        ImGui::Separator();

        if (state)
        {
            assert(state);

            ImGui::SliderFloat("ship thrust", &state->ship_state.thrust_factor, .5, 10);

            { // ship density
                const auto& body = state->ship;
                assert(body);
                const auto fixture = body->GetFixtureList();
                assert(fixture);

                const auto prev_value = fixture->GetDensity();
                static auto current_value = prev_value;
                ImGui::SliderFloat("ship density", &current_value, .1, 2);
                if (current_value != prev_value)
                {
                    fixture->SetDensity(current_value);
                    body->ResetMassData();
                }
            }

            { // ball density
                const auto& body = state->ball;
                assert(body);
                const auto fixture = body->GetFixtureList();
                assert(fixture);

                const auto prev_value = fixture->GetDensity();
                static auto current_value = prev_value;
                ImGui::SliderFloat("ball density", &current_value, .1, 2);
                if (current_value != prev_value)
                {
                    fixture->SetDensity(current_value);
                    body->ResetMassData();
                }
            }
        }
        ImGuiCallbacks();
        ImGui::Separator();

        {
            const auto& io = ImGui::GetIO();
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            //ImGui::Text("left %d right %d", io.KeysDown[ImGuiKey_LeftArrow], io.KeysDown[ImGuiKey_RightArrow]);
        }

        if (state)
        {
            assert(state);
            assert(state->ship);
            ImGui::Text("ship m=%.2f x=%.2f y=%.2f", state->ship->GetMass(), state->ship->GetPosition().x, state->ship->GetPosition().y);
            ImGui::Text("ball m=%.2f x=%.2f y=%.2f", state->ball->GetMass(), state->ball->GetPosition().x, state->ball->GetPosition().y);
            if (state->system) ImGui::Text("particles %d %d(%d)", state->system->GetParticleGroupCount(), state->system->GetParticleCount(), state->system->GetStuckCandidateCount());
            ImGui::Text("crates %d", static_cast<int>(state->crates.size()));

            std::stringstream ss;
            for (unsigned int kk=0, kk_max=std::min(state->all_accum_contact, 10u); kk<kk_max; kk++)
                ss << "*";
            ImGui::Text("contact %s %s", state->ship_state.touched_wall ? "!!BOOM!!" : "<3<3<3<3", ss.str().c_str());
        }

        end_left();
    }

    { // camera window
        begin_left("Cameras");

        auto& camera = use_world_camera ? world_camera : ship_camera;
        ImGui::Text("%s camera", use_world_camera ? "world" : "ship");
        camera.paintUI();

        end_left();
    }

    { // shading window
        begin_right("Shading");

        if (ImGui::BeginTabBar("##shading_tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Particle"))
            {
                //ImGui::Text("Hello, world!");
                ImGui::ColorEdit3("water color", water_color.data());
                ImGui::ColorEdit3("foam color", foam_color.data());
                ImGui::ColorEdit3("viscous color", viscous_color.data());
                ImGui::ColorEdit3("tensible color", tensible_color.data());
                ImGui::SliderFloat("mix ratio", &mix_ratio, 0, 1);

                {
                    shader_selection %= IM_ARRAYSIZE(shader_names);
                    const std::string key_name = QKeySequence(shader_switch_key).toString().toStdString();
                    std::stringstream ss;
                    ss << "shader (" << key_name << ")";
                    ImGui::Combo(ss.str().c_str(), &shader_selection, shader_names, IM_ARRAYSIZE(shader_names));
                    shader_selection %= IM_ARRAYSIZE(shader_names);
                }

                {
                    const char* poly_names[] = { "octogon", "hexagon", "square", "triangle" };
                    poly_selection %= IM_ARRAYSIZE(poly_names);
                    ImGui::Combo("poly", &poly_selection, poly_names, IM_ARRAYSIZE(poly_names));
                }

                ImGui::SliderFloat("radius factor", &radius_factor, 0.0f, 1.0f);
                ImGui::SliderFloat("alpha", &shading_alpha, -1, 1);
                ImGui::SliderFloat("max speed", &shading_max_speed, 0, 100);

                if (state && state->system)
                {
                    ImGui::Separator();

                    assert(state);
                    assert(state->system);
                    const b2Vec2* speeds = state->system->GetVelocityBuffer();
                    const auto kk_max = state->system->GetParticleCount();
                    const auto max_speed = std::accumulate(speeds, speeds + kk_max, 0.f, [](const float& max_speed, const b2Vec2& speed) -> float {
                            return std::max(max_speed, speed.Length());
                            });
                    ImGui::Text("max speed %f", max_speed);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Ship & crate"))
            {
                ImGui::ColorEdit4("halo out color", halo_out_color.data());
                ImGui::ColorEdit4("halo in color", halo_in_color.data());
                ImGui::ColorEdit3("crate color", crate_color.data());
                ImGui::SliderInt("crate max tag", &crate_max_tag, 1, 64);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        end_right();
    }

    if (state && state->system)
    { // particle system control
        begin_right("Particle system");

        assert(state);
        assert(state->system);
        auto& system = *state->system;

        {
            const auto ww = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.f;
            const auto ww_ = ww + ImGui::GetStyle().ItemSpacing.x + ImGui::GetStyle().WindowPadding.x;

            ImGui::CheckboxFlags("spring", &water_flags, b2_springParticle);
            ImGui::SameLine(ww_);
            ImGui::CheckboxFlags("elastic", &water_flags, b2_elasticParticle);

            ImGui::CheckboxFlags("viscous", &water_flags, b2_viscousParticle);
            ImGui::SameLine(ww_);
            ImGui::CheckboxFlags("powder", &water_flags, b2_powderParticle);

            ImGui::CheckboxFlags("tensible", &water_flags, b2_tensileParticle);
            ImGui::SameLine(ww_);
            ImGui::CheckboxFlags("color mixing", &water_flags, b2_colorMixingParticle);

            ImGui::CheckboxFlags("static pressure", &water_flags, b2_staticPressureParticle);
            ImGui::SameLine(ww_);
            ImGui::CheckboxFlags("repulsive", &water_flags, b2_repulsiveParticle);
        }


        {
            static int value = 4;
            ImGui::SliderInt("stuck thresh", &value, 0, 10);
            system.SetStuckThreshold(value);
        }

        {
            static float value = .5;
            ImGui::SliderFloat("damping", &value, 0, 1);
            system.SetDamping(value);
        }

        {
            static float value = .1;
            ImGui::SliderFloat("density", &value, .1, 2);
            system.SetDensity(value);
        }

        ImGui::SliderFloat2("drop size", water_drop_size.data(), 0, 20);
        ImGui::Checkbox("clean stuck in door", &state->clean_stuck_in_door);

        {
            ImGui::Separator();
            const auto& flags = water_flags;
            const auto str = std::bitset<32>(flags).to_string();
            assert(str.size() == 32);
            ImGui::Text("next drop flags %d", flags);
            ImGui::Text("      fedcba9876543210");
            ImGui::Text("31-16 %s", str.substr(0, 16).c_str());
            ImGui::Text("15-00 %s", str.substr(16, 16).c_str());
        }

        /*
        {
            ImGui::Separator();
            const auto& flags = system.GetAllParticleFlags();
            const auto str = std::bitset<32>(flags).to_string();
            assert(str.size() == 32);
            ImGui::Text("all particle flags %d", flags);
            ImGui::Text("      fedcba9876543210");
            ImGui::Text("31-16 %s", str.substr(0, 16).c_str());
            ImGui::Text("15-00 %s", str.substr(16, 16).c_str());
        }
        */

        end_right();
    }
}

void GameWindowOpenGL::paintScene()
{
    using std::get;

    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        { // draw with main program
            ProgramBinder binder(*this, main_program);

            const auto blit_square = [this]() -> void
            {
                glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
                glVertexAttribPointer(main_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(main_pos_attr);
                assertNoError();

                glBindBuffer(GL_ARRAY_BUFFER, vbos[10]);
                glVertexAttribPointer(main_col_attr, 4, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(main_col_attr);
                assertNoError();

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                assertNoError();

                glDisableVertexAttribArray(main_col_attr);
                glDisableVertexAttribArray(main_pos_attr);
                assertNoError();
            };

            // background gradient

            QMatrix4x4 screen_matrix;
            main_program->setUniformValue(main_camera_mat_unif, screen_matrix);

            QMatrix4x4 world_matrix;
            main_program->setUniformValue(main_world_mat_unif, world_matrix);
            assertNoError();

            blit_square();
        }

        glDisable(GL_BLEND);
    }

    if (use_painter && !state)
    { // draw with qt painter
        assert(device);
        device->setSize(size() * devicePixelRatio());
        QPainter painter(device.get());

        const auto scale = pow(std::min(1., world_time / .2), 1.35);
        painter.translate(width() / 2, height() / 2);
        painter.scale(scale, scale);
        painter.drawImage(QRect(QPoint(-logo.size().width() / 2, -logo.size().height() / 2), logo.size()), logo);
    }

    const auto& io = ImGui::GetIO();
    const double dt = std::min(50e-3, 1. / io.Framerate);
    world_time += dt;

    if (!state)
        return;

    { // ship state
        assert(state);
        state->ship_state.firing_thruster = io.KeysDown[ImGuiKey_UpArrow];

        const bool pressed_left = (!state->ship_state.turning_left && io.KeysDown[ImGuiKey_LeftArrow]);
        if (pressed_left) state->ship_state.turning_left_time = world_time;
        state->ship_state.turning_left = io.KeysDown[ImGuiKey_LeftArrow];

        const bool pressed_right = (!state->ship_state.turning_right && io.KeysDown[ImGuiKey_RightArrow]);
        if (pressed_right) state->ship_state.turning_right_time = world_time;
        state->ship_state.turning_right = io.KeysDown[ImGuiKey_RightArrow];
    }

    if (!skip_state_step)
        state->step(dt);

    {
        assert(state);
        assert(state->ship);
        const auto& position = state->ship->GetWorldCenter();
        ship_camera.position = { position.x, position.y };
    }

    const auto& camera = use_world_camera ?  world_camera : ship_camera;

    if (use_painter)
    { // draw with qt painter
        assert(device);
        device->setSize(size() * devicePixelRatio());
        QPainter painter(device.get());
        //painter.setRenderHint(QPainter::Antialiasing);

        { // world
            painter.save();
            camera.preparePainter(*this, painter);

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

            if (draw_debug)
                for (auto& crate : state->crates)
                    drawBody(painter, *get<0>(crate));

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
    }

    assertNoError();

    {
        //glClear(GL_DEPTH_BUFFER_BIT);

        //glEnable(GL_CULL_FACE);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        const auto camera_matrix = camera.cameraMatrix(*this);

        { // draw with particle program
            ProgramBinder binder(*this, particle_program);

            particle_program->setUniformValue(particle_camera_mat_unif, camera_matrix);

            { // particle system
                const auto& system = state->system;
                assert(system);

                const b2Vec2* positions = system->GetPositionBuffer();
                const b2ParticleColor* colors = system->GetColorBuffer();
                const b2Vec2* speeds = system->GetVelocityBuffer();

                const auto kk_max = system->GetParticleCount();
                std::vector<GLuint> flags(kk_max);
                {
                    static_assert(sizeof(GLuint) == sizeof(uint32), "mismatching size");
                    const GLuint* flags_ = system->GetFlagsBuffer();
                    std::copy(flags_, flags_ + kk_max, std::begin(flags));
                    const auto candidates = system->GetStuckCandidates();
                    for (auto ll=0, ll_max=system->GetStuckCandidateCount(); ll < ll_max; ll++)
                        flags[candidates[ll]] |= 1 << 31;
                }

                const auto radius = system->GetRadius();

                QMatrix4x4 world_matrix;
                world_matrix.translate(0, 0, 0);

                particle_program->setUniformValue(particle_radius_unif, radius);
                particle_program->setUniformValue(particle_radius_factor_unif, radius_factor);
                particle_program->setUniformValue(particle_mode_unif, shader_selection);
                particle_program->setUniformValue(particle_poly_unif, poly_selection);
                particle_program->setUniformValue(particle_max_speed_unif, shading_max_speed);
                particle_program->setUniformValue(particle_alpha_unif, shading_alpha);

                //            const auto& color = QColor::fromRgb(0x6cu, 0xc3u, 0xf6u, 0xffu);
                particle_program->setUniformValue(particle_water_color_unif, QColor::fromRgbF(water_color[0], water_color[1], water_color[2], water_color[3]));
                particle_program->setUniformValue(particle_foam_color_unif, QColor::fromRgbF(foam_color[0], foam_color[1], foam_color[2], foam_color[3]));
                particle_program->setUniformValue(particle_viscous_color_unif, QColor::fromRgbF(viscous_color[0], viscous_color[1], viscous_color[2], viscous_color[3]));
                particle_program->setUniformValue(particle_tensible_color_unif, QColor::fromRgbF(tensible_color[0], tensible_color[1], tensible_color[2], tensible_color[3]));
                particle_program->setUniformValue(particle_mix_unif, mix_ratio);
                particle_program->setUniformValue(particle_world_mat_unif, world_matrix);
                assertNoError();

                glBindBuffer(GL_ARRAY_BUFFER, vbos[6]);
                glBufferData(GL_ARRAY_BUFFER, kk_max * 2 * sizeof(GLfloat), positions, GL_DYNAMIC_DRAW);
                glVertexAttribPointer(particle_pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(particle_pos_attr);
                assertNoError();

                static_assert(std::is_same<GLubyte, decltype(b2ParticleColor::r)>::value, "mismatching color types");

                glBindBuffer(GL_ARRAY_BUFFER, vbos[7]);
                glBufferData(GL_ARRAY_BUFFER, kk_max * 4 * sizeof(GLubyte), colors, GL_DYNAMIC_DRAW);
                glVertexAttribPointer(particle_col_attr, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
                glEnableVertexAttribArray(particle_col_attr);
                assertNoError();

                glBindBuffer(GL_ARRAY_BUFFER, vbos[8]);
                glBufferData(GL_ARRAY_BUFFER, kk_max * 2 * sizeof(GLfloat), speeds, GL_DYNAMIC_DRAW);
                glVertexAttribPointer(particle_speed_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(particle_speed_attr);
                assertNoError();

                glBindBuffer(GL_ARRAY_BUFFER, vbos[9]);
                glBufferData(GL_ARRAY_BUFFER, kk_max * sizeof(GLuint), flags.data(), GL_DYNAMIC_DRAW);
                glVertexAttribIPointer(particle_flag_attr, 1, GL_UNSIGNED_INT, 0, 0);
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

        //glClear(GL_DEPTH_BUFFER_BIT);

        { // draw with base program
            ProgramBinder binder(*this, base_program);

            const auto blit_cube = [this]() -> void
            {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[5]);
                assertNoError();

                glBindBuffer(GL_ARRAY_BUFFER, vbos[4]);
                glVertexAttribPointer(base_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(base_pos_attr);
                assertNoError();

                glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, reinterpret_cast<void*>(0));
                glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, reinterpret_cast<void*>(8 * sizeof(unsigned int)));
                assertNoError();

                glDisableVertexAttribArray(base_pos_attr);
                assertNoError();
            };

            base_program->setUniformValue(base_camera_mat_unif, camera_matrix);

            {
                QMatrix4x4 world_matrix;
                world_matrix.translate(0, 10);
                world_matrix.scale(3, 3, 3);
                world_matrix.rotate(world_time * 60, 1, 1, 1);
                base_program->setUniformValue(base_world_mat_unif, world_matrix);
                blit_cube();
            }
        }

        { // draw with main program
            ProgramBinder binder(*this, main_program);

            const auto blit_ship = [this]() -> void
            {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[11]);
                assertNoError();

                glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
                glVertexAttribPointer(main_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(main_pos_attr);
                assertNoError();

                glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
                glVertexAttribPointer(main_col_attr, 4, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(main_col_attr);
                assertNoError();

                glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, nullptr);
                assertNoError();

                glDisableVertexAttribArray(main_col_attr);
                glDisableVertexAttribArray(main_pos_attr);
                assertNoError();
            };

            const auto blit_wing = [this]() -> void
            {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[5]);
                assertNoError();

                glBindBuffer(GL_ARRAY_BUFFER, vbos[4]);
                glVertexAttribPointer(main_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(main_pos_attr);
                assertNoError();

                glBindBuffer(GL_ARRAY_BUFFER, vbos[3]);
                glVertexAttribPointer(main_col_attr, 4, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(main_col_attr);
                assertNoError();

                glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, reinterpret_cast<void*>(0));
                glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, reinterpret_cast<void*>(8 * sizeof(unsigned int)));
                assertNoError();

                glDisableVertexAttribArray(main_col_attr);
                glDisableVertexAttribArray(main_pos_attr);
                assertNoError();
            };

            main_program->setUniformValue(base_camera_mat_unif, camera_matrix);

            { // ship
                QMatrix4x4 world_matrix;

                assert(state);
                assert(state->ship);
                const auto& pos = state->ship->GetPosition();
                world_matrix.translate(pos.x, pos.y);

                auto delta = state->ship_state.target_angle - state->ship->GetAngle();
                delta /= .1;
                delta *= 20;

                world_matrix.rotate(180. * state->ship->GetAngle() / M_PI, 0, 0, 1);
                world_matrix.rotate(delta, 0, 1, 0);
                world_matrix.scale(GameState::ship_scale, GameState::ship_scale, GameState::ship_scale);

                main_program->setUniformValue(main_world_mat_unif, world_matrix);
                assertNoError();

                blit_ship();

                world_matrix.rotate(90, 0, 1, 0);
                world_matrix.scale(.25, .4, .5);

                {
                    auto foo = world_matrix;
                    foo.translate(0, -.5, 1.2);
                    foo.scale(1, 1, .2);
                    foo.rotate(delta / 2, 1, 0, 0);
                    main_program->setUniformValue(main_world_mat_unif, foo);
                    assertNoError();
                    blit_wing();
                }

                {
                    auto foo = world_matrix;
                    foo.translate(0, -.5, -1.2);
                    foo.scale(1, 1, .2);
                    foo.rotate(delta / 2, 1, 0, 0);
                    main_program->setUniformValue(main_world_mat_unif, foo);
                    assertNoError();
                    blit_wing();
                }
            }
        }

        { // draw with crate program
            ProgramBinder binder(*this, crate_program);

            assert(crate_texture);
            crate_texture->bind(0);

            const auto blit_cube = [this]() -> void
            {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[5]);
                assertNoError();

                glBindBuffer(GL_ARRAY_BUFFER, vbos[4]);
                glVertexAttribPointer(crate_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(crate_pos_attr);
                assertNoError();

                glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, reinterpret_cast<void*>(0));
                glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, reinterpret_cast<void*>(8 * sizeof(unsigned int)));
                assertNoError();

                glDisableVertexAttribArray(crate_pos_attr);
                assertNoError();
            };

            crate_program->setUniformValue(crate_color_unif, QColor::fromRgbF(crate_color[0], crate_color[1], crate_color[2], crate_color[3]));
            crate_program->setUniformValue(crate_camera_mat_unif, camera_matrix);
            crate_program->setUniformValue(crate_texture_unif, 0);
            crate_program->setUniformValue(crate_max_tag_unif, crate_max_tag);

            for (auto& crate : state->crates)
            {
                const auto& pos = get<0>(crate)->GetWorldCenter();
                const auto& angle = get<0>(crate)->GetAngle();

                QMatrix4x4 world_matrix;
                world_matrix.translate(pos.x, pos.y, 0);
                world_matrix.rotate(180. * angle / M_PI, 0, 0, 1);
                world_matrix.scale(GameState::crate_scale, GameState::crate_scale, GameState::crate_scale);
                crate_program->setUniformValue(crate_world_mat_unif, world_matrix);
                crate_program->setUniformValue(crate_tag_unif, get<1>(crate));
                blit_cube();
            }

            crate_texture->release();
        }


        { // draw with ball program
            ProgramBinder binder(*this, ball_program);

            const auto blit_square = [this]() -> void
            {
                glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
                glVertexAttribPointer(ball_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(ball_pos_attr);
                assertNoError();

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                assertNoError();

                glDisableVertexAttribArray(ball_pos_attr);
                assertNoError();
            };

            ball_program->setUniformValue(base_camera_mat_unif, camera_matrix);

            {
                assert(state->ball);
                assert(state->ball->GetFixtureList());
                assert(state->ball->GetFixtureList()->GetShape());

                QMatrix4x4 world_matrix;
                const auto& pos = state->ball->GetWorldCenter();
                const auto& angle = state->ball->GetAngle();
                world_matrix.translate(pos.x, pos.y);
                world_matrix.rotate(qRadiansToDegrees(angle), 0, 0, 1);
                world_matrix.scale(GameState::ball_scale, GameState::ball_scale, GameState::ball_scale);
                ball_program->setUniformValue(ball_world_mat_unif, world_matrix);

                const auto& angular_speed = state->ball->GetAngularVelocity();
                ball_program->setUniformValue(ball_angular_speed_unif, angular_speed);

                blit_square();
            }
        }

        if (state && state->canGrab())
        { // draw with grab program
            ProgramBinder binder(*this, grab_program);

            assertNoError();

            const auto blit_square = [this]() -> void
            {
                glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
                glVertexAttribPointer(grab_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(grab_pos_attr);
                assertNoError();

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                assertNoError();

                glDisableVertexAttribArray(grab_pos_attr);
                assertNoError();
            };

            ball_program->setUniformValue(base_camera_mat_unif, camera_matrix);

            { // grab indicator
                QMatrix4x4 world_matrix;

                assert(state);
                assert(state->ship);
                const auto& pos = state->ship->GetWorldCenter();
                world_matrix.translate(pos.x, pos.y, 2);
                world_matrix.scale(6, 6, 1);

                assertNoError();
                grab_program->setUniformValue(grab_world_mat_unif, world_matrix);
                assertNoError();
                grab_program->setUniformValue(grab_time_unif, world_time);
                assertNoError();
                grab_program->setUniformValue(grab_halo_out_color_unif, QColor::fromRgbF(halo_out_color[0], halo_out_color[1], halo_out_color[2], halo_out_color[3]));
                assertNoError();
                grab_program->setUniformValue(grab_halo_in_color_unif, QColor::fromRgbF(halo_in_color[0], halo_in_color[1], halo_in_color[2], halo_in_color[3]));
                assertNoError();

                blit_square();

                assertNoError();
            }
        }

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
    }

    { // sfx
        assert(state);
        if (state->ship_state.accum_contact > 0)
            ship_click_sfx.play();


        if (!is_muted)
            engine_sfx.setMuted(!state->ship_state.firing_thruster);

        //back_click_sfx.setVolume(volume);
        //if (state->all_accum_contact > 0)
        //    back_click_sfx.play();
    }
}

void GameWindowOpenGL::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == shader_switch_key)
    {
        assert(IM_ARRAYSIZE(shader_names) > 0);
        const auto modifiers = event->modifiers();
        if (modifiers == Qt::ShiftModifier)
        {
            shader_selection += IM_ARRAYSIZE(shader_names) - 1;
            shader_selection %= IM_ARRAYSIZE(shader_names);
            return;
        }
        if (modifiers == Qt::NoModifier)
        {
            shader_selection++;
            shader_selection %= IM_ARRAYSIZE(shader_names);
            return;
        }
    }

    if (event->key() == level_switch_key)
    {
        assert(!data.levels.empty());
        const auto modifiers = event->modifiers();
        if (modifiers == Qt::ShiftModifier)
            if (current_level >= 0)
            {
                current_level += data.levels.size() - 1;
                current_level %= data.levels.size();
                resetLevel();
                return;
            }
            else
            {
                current_level = data.levels.size() - 1;
                resetLevel();
                return;
            }

        if (modifiers == Qt::NoModifier)
            if (current_level >= 0)
            {
                current_level++;
                current_level %= data.levels.size();
                resetLevel();
                return;
            }
            else
            {
                current_level = 0;
                resetLevel();
                return;
            }
    }

    if (event->key() == Qt::Key_Space)
    {
        assert(state);
        if (state->link) state->release();
        else if (state->canGrab()) state->grab();
        return;
    }

    RasterWindowOpenGL::keyPressEvent(event);
}

