#include "RasterWindowOpenGL.h"

#include <QKeyEvent>

#include "imgui.h"
#include "QtImGui.h"

#include <iostream>
#include <unordered_set>
#include <sstream>

RasterWindowOpenGL::RasterWindowOpenGL(QWindow* parent)
    : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate, parent)
{
}

void RasterWindowOpenGL::assertNoError()
{
#if !defined(NDEBUG)
    const auto gl_error = glGetError();

    using std::cerr;
    using std::endl;

    switch (gl_error)
    {
        default:
        case GL_NO_ERROR:
            break;
        case GL_INVALID_ENUM:
            cerr << "GL_INVALID_ENUM" << endl;
            cerr << "An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag." << endl;
            break;
        case GL_INVALID_VALUE:
            cerr << "GL_INVALID_VALUE" << endl;
            cerr << "A numerir argument is out of range. The offending command is ignored and has no other side effect than to set the error flag." << endl;
            break;
        case GL_INVALID_OPERATION:
            cerr << "GL_INVALID_OPERATION" << endl;
            cerr << "The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag." << endl;
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            cerr << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl;
            cerr << "The framebuffer object is not complete. The offending command is ignored and has no other side effect than to set the error flag." << endl;
            break;
        case GL_OUT_OF_MEMORY:
            cerr << "GL_OUT_OF_MEMORY" << endl;
            cerr << "There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded." << endl;
            break;
        case GL_STACK_UNDERFLOW:
            cerr << "GL_STACK_UNDERFLOW" << endl;
            cerr << "An attempt has been made to perform an operation that would cause an internal stack to underflow." << endl;
            break;
        case GL_STACK_OVERFLOW:
            cerr << "GL_STACK_OVERFLOW" << endl;
            cerr << "An attempt has been made to perform an operation that would cause an internal stack to overflow." << endl;
            break;
    }
#endif

    assert(gl_error == GL_NO_ERROR);
}

void RasterWindowOpenGL::setAnimated(const bool value)
{
    is_animated = value;
    if (is_animated)
        update();
}

void RasterWindowOpenGL::addButton(const std::string& label, const int key, const VoidCallback& callback)
{
    assert(isKeyFree(key));
    button_states.emplace(key, ButtonState { button_states.size(), label, callback });
}

void RasterWindowOpenGL::addCheckbox(const std::string& label, const int key, const bool& value, const BoolCallback& callback)
{
    assert(isKeyFree(key));
    auto state = std::make_tuple(checkbox_states.size(), label, value, callback);
    checkbox_states.emplace(key, state);
    std::get<3>(state)(std::get<2>(state));
}

void RasterWindowOpenGL::addSlider(const std::string& label, const float& min, const float& max, const float& value, const FloatCallback& callback)
{
    auto state = std::make_tuple(label, min, max, value, callback);
    float_states.emplace_back(state);
    std::get<4>(state)(std::get<3>(state));
}

// keyboard controls

bool RasterWindowOpenGL::isKeyFree(const int key) const
{
    if (key == Qt::Key_A) return false;
    if (button_states.find(key) != std::cend(button_states)) return false;
    if (checkbox_states.find(key) != std::cend(checkbox_states)) return false;
    return true;
}

void RasterWindowOpenGL::keyPressEvent(QKeyEvent* event)
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
}

// opengl stuff

std::unique_ptr<QOpenGLShaderProgram> RasterWindowOpenGL::loadAndCompileProgram(const QString& vertex_filename, const QString& fragment_filename, const QString& geometry_filename)
{
    qDebug() << "========== shader";
    auto program = std::make_unique<QOpenGLShaderProgram>();

    const auto load_shader = [&program](const QOpenGLShader::ShaderType type, const QString& filename) -> bool
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
    assertNoError();
    assert(all_ok);

    return program;
}

void RasterWindowOpenGL::initializeGL()
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
    qDebug() << "currentOpenGLVersion" << QOpenGLContext::currentContext()->format().version();

    initializeOpenGLFunctions();
    QtImGui::initialize(this);
    assertNoError();

    {
        assert(!base_program);
        base_program = loadAndCompileProgram(":base_vertex.glsl", ":base_fragment.glsl");

        assert(base_program);
        base_pos_attr = base_program->attributeLocation("posAttr");
        base_mat_unif = base_program->uniformLocation("matrix");
        qDebug() << "base_locations" << base_pos_attr << base_mat_unif;
        assert(base_pos_attr >= 0);
        assert(base_mat_unif >= 0);
        assertNoError();
    }

    { // buffers
        using std::cout;
        using std::endl;

        glGenVertexArrays(1, &vao);
        qDebug() << "========== vao" << vao;
        glBindVertexArray(vao);

        glGenBuffers(vbos.size(), vbos.data());
        std::unordered_set<size_t> reserved_vbos;

        const auto is_available = [this, &reserved_vbos](const size_t kk) -> bool
        {
            if (kk >= vbos.size()) return false;
            if (reserved_vbos.find(kk) != std::cend(reserved_vbos)) return false;
            return true;
        };

        const auto reserve_vbo = [&reserved_vbos, &is_available](const size_t kk) -> void
        {
            assert(is_available(kk));
            reserved_vbos.emplace(kk);
        };

        /*
        const auto load_buffer3 = [this, &reserve_vbo](const size_t kk, const std::vector<b2Vec3>& vertices) -> void
        {
            reserve_vbo(kk);
            glBindBuffer(GL_ARRAY_BUFFER, vbos[kk]);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * 3 * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
        };

        const auto load_buffer4 = [this, &reserve_vbo](const size_t kk, const std::vector<b2Vec4>& vertices) -> void
        {
            reserve_vbo(kk);
            glBindBuffer(GL_ARRAY_BUFFER, vbos[kk]);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * 4 * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
        };

        const auto load_indices = [this, &reserve_vbo](const size_t kk, const std::vector<GLuint>& indices) -> void
        {
            reserve_vbo(kk);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[kk]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        };
        */

        /*
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

        // cube
        load_buffer3(4, {
            { -1, -1, 1 },
            { 1, -1, 1 },
            { 1, 1, 1 },
            { -1, 1, 1 },
            { -1, -1, -1 },
            { 1, -1, -1 },
            { 1, 1, -1 },
            { -1, 1, -1 },
        });
        load_indices(5, {
            0, 1, 3, 2, 7, 6, 4, 5,
            3, 7, 0, 4, 1, 5, 2, 6,
        });

        // buffers 6, 7 & 8 are used by particle system
        reserve_vbo(6); // position
        reserve_vbo(7); // color
        reserve_vbo(8); // speed
        reserve_vbo(9); // flag
        */

        cout << "vbos";
        size_t kk = 0;
        for (const auto& vbo : vbos)
        {
            cout << " " << vbo;
            if (reserved_vbos.find(kk++) != std::cend(reserved_vbos)) cout << "*";
        }
        cout << endl;
    }
}

void RasterWindowOpenGL::paintGL()
{
    assertNoError();

    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    assertNoError();

    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    assertNoError();

    QtImGui::newFrame();

    constexpr auto ui_window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

    if (display_ui)
    {
        //ImGui::SetNextWindowSize(ImVec2(350, 440), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Once);
        //ImGui::SetNextWindowSize(ImVec2(350,400), ImGuiCond_FirstUseEver);
        ImGui::Begin("raster", &display_ui, ui_window_flags);

        /*
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
        */

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
            const size_t kk_max = pairs.size();
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

                if (kk % 2 != 1 && kk != kk_max - 1) ImGui::SameLine();
                kk++;
            }
        }

        ImGui::Separator();
        ImGui::Text("prout");

        ImGui::End();
    }

    /*
    if (display_ui)
    {
        //ImGui::SetNextWindowSize(ImVec2(330,100), ImGuiCond_Always);
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

    if (display_ui)
    {
        //ImGui::SetNextWindowSize(ImVec2(330,100), ImGuiCond_Always);
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
            static float value = .2;
            ImGui::SliderFloat("damping", &value, 0, 1);
            system.SetDamping(value);
        }

        {
            static float value = .4;
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
*/

    /*
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
            painter.setPen(QPen(Qt::white, 0));
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
*/

    const auto world_matrix = [this]() -> QMatrix4x4
    {
        QMatrix4x4 matrix;
        const auto ratio = static_cast<double>(width()) / height();
        const auto norm_width = ratio;
        const double norm_height = 1;
        //matrix.ortho(-norm_width, norm_width, -norm_height, norm_height, 0, 100);
        matrix.perspective(60.0f, width() / static_cast<float>(height()), 0.1f, 10.0f);

        matrix.translate(0, 0, -50);
        matrix.scale(2, 2, 2);

        /*
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
        */

        return matrix;
    }();

    { // draw with base program
        glBindVertexArray(vao);

        assert(base_program);
        base_program->bind();
        assertNoError();

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

        base_program->release();

        glBindVertexArray(0);
    }

    ImGui::Render();

    frame_counter++;

    if (is_animated)
        update();
}
