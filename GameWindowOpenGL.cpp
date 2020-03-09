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

const float camera_world_zoom = 1.5;
const QVector2D camera_world_center { 0, -120 };

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
        assert(load_ok);
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

std::unique_ptr<QOpenGLShaderProgram> GameWindowOpenGL::loadAndCompileProgram(const QString& vertex_filename, const QString& fragment_filename, const QString& geometry_filename)
{
    auto program = std::make_unique<QOpenGLShaderProgram>();

    const auto load_shader = [&program, &vertex_filename](const QOpenGLShader::ShaderType type, const QString& filename) -> bool
    {
        qDebug() << "compiling" << filename;
        QFile handle(filename);
        if (!handle.open(QIODevice::ReadOnly))
            return false;
        const auto& source = handle.readAll();
        return program->addShaderFromSourceCode(type, source);
    };

    const auto vertex_load_ok = load_shader(QOpenGLShader::Vertex, vertex_filename);
    const auto geometry_load_ok = geometry_filename.isNull() ? true : load_shader(QOpenGLShader::Geometry, geometry_filename);
    const auto fragment_load_ok = load_shader(QOpenGLShader::Fragment, fragment_filename);
    const auto link_ok = program->link();
    const auto gl_ok = glGetError() == GL_NO_ERROR;
    qDebug() << "loadAndCompileProgram" << link_ok << vertex_load_ok << fragment_load_ok << geometry_load_ok << gl_ok;

    const auto all_ok = vertex_load_ok && fragment_load_ok && geometry_load_ok && link_ok && gl_ok;
    if (!all_ok) {
        qDebug() << program->log();
        return nullptr;
    }
    assert(all_ok);

    return program;
};

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
    qDebug() << "currentContext" << QOpenGLContext::currentContext();

    initializeOpenGLFunctions();
    QtImGui::initialize(this);
    assert(glGetError() == GL_NO_ERROR);


    {
        assert(!main_program);
        main_program = loadAndCompileProgram(":main_vertex.glsl", ":main_fragment.glsl");

        assert(main_program);
        main_pos_attr = main_program->attributeLocation("posAttr");
        main_col_attr = main_program->attributeLocation("colAttr");
        main_mat_unif = main_program->uniformLocation("matrix");
        qDebug() << "main_locations" << main_pos_attr << main_col_attr << main_mat_unif;
        assert(main_pos_attr >= 0);
        assert(main_col_attr >= 0);
        assert(main_mat_unif >= 0);
        assert(glGetError() == GL_NO_ERROR);
    }

    {
        assert(!base_program);
        base_program = loadAndCompileProgram(":base_vertex.glsl", ":base_fragment.glsl");

        assert(base_program);
        base_pos_attr = base_program->attributeLocation("posAttr");
        base_mat_unif = base_program->uniformLocation("matrix");
        qDebug() << "base_locations" << base_pos_attr << base_mat_unif;
        assert(base_pos_attr >= 0);
        assert(base_mat_unif >= 0);
        assert(glGetError() == GL_NO_ERROR);
    }

    {
        assert(!particle_program);
        particle_program = loadAndCompileProgram(":particle_vertex.glsl", ":particle_fragment.glsl", ":particle_geometry.glsl");

        assert(particle_program);
        particle_pos_attr = particle_program->attributeLocation("posAttr");
        particle_col_attr = particle_program->attributeLocation("colAttr");
        particle_mat_unif = particle_program->uniformLocation("matrix");
        particle_color_unif = particle_program->uniformLocation("dotColor");
        particle_radius_unif = particle_program->uniformLocation("radius");
        particle_mode_unif = particle_program->uniformLocation("mode");
        qDebug() << "particle_locations" << particle_pos_attr << particle_col_attr << particle_mat_unif << particle_color_unif << particle_radius_unif << particle_mode_unif;
        assert(particle_pos_attr >= 0);
        assert(particle_col_attr >= 0);
        assert(particle_mat_unif >= 0);
        assert(particle_color_unif >= 0);
        assert(particle_radius_unif >= 0);
        assert(particle_mode_unif >= 0);
        assert(glGetError() == GL_NO_ERROR);
    }

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

        const auto load_indices = [this](const size_t kk, const std::vector<GLuint>& indices) -> void
        {
            assert(vbos.size() > kk);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[kk]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

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

        { // cube
            load_buffer3(6, {
                { -1, -1, 1 },
                { 1, -1, 1 },
                { 1, 1, 1 },
                { -1, 1, 1 },
                { -1, -1, -1 },
                { 1, -1, -1 },
                { 1, 1, -1 },
                { -1, 1, -1 },
            });
            load_indices(7, {
                0, 1, 3, 2, 7, 6, 4, 5,
                3, 7, 0, 4, 1, 5, 2, 6,
            });
        }

        // buffers 4 & 5 are free
    }

}

bool GameWindowOpenGL::isKeyFree(const int key) const
{
    if (key == Qt::Key_A) return false;
    if (button_states.find(key) != std::cend(button_states)) return false;
    if (checkbox_states.find(key) != std::cend(checkbox_states)) return false;
    return true;
}

void GameWindowOpenGL::addButton(const std::string& label, const int key, const VoidCallback& callback)
{
    assert(isKeyFree(key));
    button_states.emplace(key, ButtonState { button_states.size(), label, callback });
}

void GameWindowOpenGL::addCheckbox(const std::string& label, const int key, const bool& value, const BoolCallback& callback)
{
    assert(isKeyFree(key));
    auto state = std::make_tuple(checkbox_states.size(), label, value, callback);
    checkbox_states.emplace(key, state);
    std::get<3>(state)(std::get<2>(state));
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
        const QColor color = QColor::fromRgbF(water_color[0], water_color[1], water_color[2], water_color[3]);
        painter.setPen(QPen(color, 0));
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

    if (display_ui)
    {
        ImGui::SetNextWindowSize(ImVec2(350, 440), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Once);
        //ImGui::SetNextWindowSize(ImVec2(350,400), ImGuiCond_FirstUseEver);
        ImGui::Begin("~.: THRUST :.~", &display_ui, ImGuiWindowFlags_NoBackground);

        for (auto& state : float_states)
        {
            const auto prev = std::get<3>(state);
            ImGui::SliderFloat(std::get<0>(state).c_str(), &std::get<3>(state), std::get<1>(state), std::get<2>(state));
            if (prev != std::get<3>(state)) std::get<4>(state)(std::get<3>(state));
        }

        {
            using std::get;

            using Pair = std::tuple<size_t, int>;
            std::vector<Pair> pairs;
            for (const auto& pair : checkbox_states)
                pairs.emplace_back(get<0>(pair.second), pair.first);
            std::sort(std::begin(pairs), std::end(pairs), [](const Pair& aa, const Pair& bb) -> bool { return get<0>(aa) < get<0>(bb); });

            size_t kk = 0;
            for (auto& pair_ : pairs)
            {
                auto pair = checkbox_states.find(get<1>(pair_));
                assert(pair != std::end(checkbox_states));
                auto& state = pair->second;
                assert(get<1>(pair_) == pair->first);
                assert(get<0>(pair_) == get<0>(pair->second));
                const auto prev = get<2>(state);
                const std::string key_name = QKeySequence(pair->first).toString().toStdString();
                std::stringstream ss;
                ss << get<1>(state) << " (" << key_name << ")";
                ImGui::Checkbox(ss.str().c_str(), &get<2>(state));
                if (prev != std::get<2>(state)) std::get<3>(state)(std::get<2>(state));
            }
        }

        {
            using std::get;

            using Pair = std::tuple<size_t, int>;
            std::vector<Pair> pairs;
            for (const auto& pair : button_states)
                pairs.emplace_back(get<0>(pair.second), pair.first);
            std::sort(std::begin(pairs), std::end(pairs), [](const Pair& aa, const Pair& bb) -> bool { return get<0>(aa) < get<0>(bb); });

            size_t kk = 0;
            for (const auto& pair_ : pairs)
            {
                const auto& pair = button_states.find(get<1>(pair_));
                assert(pair != std::cend(button_states));
                const auto& state = pair->second;
                assert(get<1>(pair_) == pair->first);
                assert(get<0>(pair_) == get<0>(pair->second));
                const std::string key_name = QKeySequence(pair->first).toString().toStdString();
                std::stringstream ss;
                ss << get<1>(state) << " (" << key_name << ")";
                if (ImGui::Button(ss.str().c_str(), { 163, 19 }))
                    get<2>(state)();

                if (kk % 2 != 1) ImGui::SameLine();
                kk++;
            }
        }

        ImGui::Separator();
        ImGui::Text("application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("ship %.2f %.2f", state.ship->GetPosition().x, state.ship->GetPosition().y);
        if (state.system) ImGui::Text("particles %d", state.system->GetParticleCount());
        ImGui::Text("crates %d", static_cast<int>(state.crates.size()));
        ImGui::Text("contact %d", state.all_accum_contact);
        ImGui::Text(state.ship_touched_wall ? "!!!!BOOOM!!!!" : "<3<3<3<3");

        ImGui::End();
    }


    if (display_ui)
    {
        ImGui::SetNextWindowSize(ImVec2(330,100), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(width() - 330 - 5, 5), ImGuiCond_Once);
        ImGui::Begin("Shading", &display_ui, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

        //ImGui::Text("Hello, world!");
        //ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::ColorEdit3("water color", water_color.data());
        //if (ImGui::Button("Test Window")) show_test_window ^= 1;
        //if (ImGui::Button("Another Window")) show_another_window ^= 1;

        {
            const char* shader_names[3] = { "AAAA", "BBBB", "CCCC", };
            ImGui::Combo("shader", &shader_selection, shader_names,IM_ARRAYSIZE(shader_names));
        }

        ImGui::End();
    }

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
        painter.fillRect(0, 0, width(), height(), linearGrad);
    }

    { // world
        painter.save();
        painter.translate(width() / 2, height() / 2);
        painter.scale(1., -1);

        if (!is_zoom_out)
        {
            const auto& pos = state.ship->GetPosition();
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

        { // svg
            constexpr double scale = 600;
            painter.save();
            painter.scale(scale, scale);
            map_renderer.render(&painter, QRectF(-.5, -.75, 1, -1));
            painter.restore();
        }

        drawOrigin(painter);
        if (draw_debug)
            drawBody(painter, state.ground);

        for (auto& crate : state.crates)
            drawBody(painter, crate);

        for (auto& door : state.doors)
            drawBody(painter, std::get<0>(door), Qt::yellow);

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
            const auto& pos = state.ship->GetPosition();
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

    { // draw with main program
        glBindVertexArray(vao);

        assert(main_program);
        main_program->bind();
        assert(glGetError() == GL_NO_ERROR);

        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        const auto blit_triangle = [this]() -> void
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
            glVertexAttribPointer(main_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(main_pos_attr);
            assert(glGetError() == GL_NO_ERROR);

            glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
            glVertexAttribPointer(main_col_attr, 4, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(main_col_attr);
            assert(glGetError() == GL_NO_ERROR);

            glDrawArrays(GL_TRIANGLES, 0, 3);
            assert(glGetError() == GL_NO_ERROR);

            glDisableVertexAttribArray(main_col_attr);
            glDisableVertexAttribArray(main_pos_attr);
            assert(glGetError() == GL_NO_ERROR);
        };

        const auto blit_square = [this]() -> void
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
            glVertexAttribPointer(main_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(main_pos_attr);
            assert(glGetError() == GL_NO_ERROR);

            glBindBuffer(GL_ARRAY_BUFFER, vbos[3]);
            glVertexAttribPointer(main_col_attr, 4, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(main_col_attr);
            assert(glGetError() == GL_NO_ERROR);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            assert(glGetError() == GL_NO_ERROR);

            glDisableVertexAttribArray(main_col_attr);
            glDisableVertexAttribArray(main_pos_attr);
            assert(glGetError() == GL_NO_ERROR);
        };

        { // ship
            QMatrix4x4 matrix = world_matrix;

            const auto& pos = state.ship->GetPosition();
            matrix.translate(pos.x, pos.y);

            matrix.rotate(180. * state.ship->GetAngle() / M_PI, 0, 0, 1);
            matrix.rotate(frame_counter, 0, 1, 0);

            main_program->setUniformValue(main_mat_unif, matrix);
            assert(glGetError() == GL_NO_ERROR);

            blit_triangle();
            blit_square();

            matrix.rotate(90, 0, 1, 0);

            main_program->setUniformValue(main_mat_unif, matrix);
            assert(glGetError() == GL_NO_ERROR);

            blit_triangle();
        }

        glDisable(GL_DEPTH_TEST);

        main_program->release();
    }

    { // draw with particle program
        glBindVertexArray(vao);

        assert(particle_program);
        particle_program->bind();
        assert(glGetError() == GL_NO_ERROR);

        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        { // particle system
            const auto* system = state.system;
            assert(system);

            const b2Vec2* positions = system->GetPositionBuffer();
            const b2ParticleColor* colors = system->GetColorBuffer();
            const auto kk_max = system->GetParticleCount();
            const auto radius = system->GetRadius();

            particle_program->setUniformValue(particle_radius_unif, radius);
            particle_program->setUniformValue(particle_mode_unif, shader_selection);

//            const auto& color = QColor::fromRgb(0x6cu, 0xc3u, 0xf6u, 0xffu);
            const auto& color = QColor::fromRgbF(water_color[0], water_color[1], water_color[2], water_color[3]);
            particle_program->setUniformValue(particle_color_unif, color);
            particle_program->setUniformValue(particle_mat_unif, world_matrix);
            assert(glGetError() == GL_NO_ERROR);

            glBindBuffer(GL_ARRAY_BUFFER, vbos[4]);
            glBufferData(GL_ARRAY_BUFFER, kk_max * 2 * sizeof(GLfloat), positions, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(particle_pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(particle_pos_attr);
            assert(glGetError() == GL_NO_ERROR);

            static_assert(std::is_same<GLubyte, decltype(b2ParticleColor::r)>::value, "mismatching color types");

            glBindBuffer(GL_ARRAY_BUFFER, vbos[5]);
            glBufferData(GL_ARRAY_BUFFER, kk_max * 4 * sizeof(GLubyte), colors, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(particle_col_attr, 4,  GL_UNSIGNED_BYTE, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(particle_col_attr);
            assert(glGetError() == GL_NO_ERROR);

            glDrawArrays(GL_POINTS, 0, kk_max);
            assert(glGetError() == GL_NO_ERROR);

            //glDisableVertexAttribArray(particle_col_attr);
            glDisableVertexAttribArray(particle_pos_attr);
            assert(glGetError() == GL_NO_ERROR);
        }

        glDisable(GL_DEPTH_TEST);

        particle_program->release();
    }

    { // draw with base program
        glBindVertexArray(vao);

        assert(base_program);
        base_program->bind();
        assert(glGetError() == GL_NO_ERROR);

        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        const auto blit_cube = [this]() -> void
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[7]);
            assert(glGetError() == GL_NO_ERROR);

            glBindBuffer(GL_ARRAY_BUFFER, vbos[6]);
            glVertexAttribPointer(base_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(base_pos_attr);
            assert(glGetError() == GL_NO_ERROR);


            //glEnable(GL_CULL_FACE);
            glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, reinterpret_cast<void*>(0));
            glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, reinterpret_cast<void*>(8 * sizeof(unsigned int)));
            //glDisable(GL_CULL_FACE);
            assert(glGetError() == GL_NO_ERROR);

            glDisableVertexAttribArray(base_pos_attr);
            assert(glGetError() == GL_NO_ERROR);
        };

        {
            auto matrix = world_matrix;
            matrix.translate(0, 10);
            matrix.scale(3, 3, 3);
            matrix.rotate(frame_counter, 1, 1, 1);
            base_program->setUniformValue(base_mat_unif, matrix);

            blit_cube();
        }

        base_program->release();
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
    for (auto& pair : checkbox_states)
    {
        using std::get;
        auto& state = pair.second;
        if (event->key() == pair.first)
        {
            get<2>(state) ^= 1;
            get<3>(state)(get<2>(state));
            return;
        }
    }
    for (const auto& pair : button_states)
    {
        using std::get;
        const auto& state = pair.second;
        if (event->key() == pair.first)
        {
            get<2>(state)();
            return;
        }
    }
    if (event->key() == Qt::Key_Space)
    {
        if (state.joint) state.release();
        else if (state.canGrab()) state.grab();
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
