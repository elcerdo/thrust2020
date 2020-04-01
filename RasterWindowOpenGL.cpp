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

        const auto load_buffer3 = [this, &reserve_vbo](const size_t kk, const std::vector<std::array<GLfloat, 3>>& vertices) -> void
        {
            reserve_vbo(kk);
            glBindBuffer(GL_ARRAY_BUFFER, vbos[kk]);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * 3 * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
        };

        const auto load_buffer4 = [this, &reserve_vbo](const size_t kk, const std::vector<std::array<GLfloat, 4>>& vertices) -> void
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
        */

        // cube
        load_buffer3(0, {
            { -1, -1, 1 },
            { 1, -1, 1 },
            { 1, 1, 1 },
            { -1, 1, 1 },
            { -1, -1, -1 },
            { 1, -1, -1 },
            { 1, 1, -1 },
            { -1, 1, -1 },
        });
        load_indices(1, {
            0, 1, 3, 2, 7, 6, 4, 5,
            3, 7, 0, 4, 1, 5, 2, 6,
        });

        /*
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

void RasterWindowOpenGL::paintUI()
{
    using std::get;

    constexpr auto ui_window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize;

    ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Once);
    //ImGui::SetNextWindowSize(ImVec2(350, 440), ImGuiCond_Once);
    //ImGui::SetNextWindowSize(ImVec2(350,400), ImGuiCond_FirstUseEver);
    ImGui::Begin("callbacks", &display_ui, ui_window_flags);

    for (auto& state : float_states)
    {
        const auto prev = get<3>(state);
        ImGui::SliderFloat(get<0>(state).c_str(), &get<3>(state), get<1>(state), get<2>(state));
        if (prev != get<3>(state)) get<4>(state)(get<3>(state));
    }

    {

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
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::End();
}

void RasterWindowOpenGL::paintGL()
{
    assertNoError();

    glClearColor(.8, .8, .8, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    assertNoError();

    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    assertNoError();

    /*
    const auto world_matrix = [this]() -> QMatrix4x4
    {
        QMatrix4x4 matrix;
        const auto ratio = static_cast<double>(width()) / height();
        const auto norm_width = ratio;
        const double norm_height = 1;
        matrix.ortho(-norm_width, norm_width, -norm_height, norm_height, 0, 100);
        //matrix.perspective(60.0f, width() / static_cast<float>(height()), 0.1f, 10.0f);

        matrix.translate(0, 0, -5);
        //matrix.scale(2, 2, 2);

        //if (!is_zoom_out)
        //{
        //    const auto& pos = state->ship->GetPosition();
        //    const auto side = std::min(ratio, 1.);
        //    const double ship_height = 75 * std::max(1., pos.y / 40.);
        //    matrix.scale(side / ship_height, side / ship_height, 1);
        //    matrix.translate(-pos.x, -std::min(20.f, pos.y), 0);
        //}
        //else
        //{
            const float camera_world_zoom = 1.5;
            //const QVector2D camera_world_center { 0, -120 };
            const int side = qMin(width(), height());
            matrix.scale(camera_world_zoom/side, camera_world_zoom/side, camera_world_zoom);
            //matrix.translate(-camera_world_center);
        //}

        return matrix;
    }();
    */

    const auto world_matrix = [this]() -> QMatrix4x4
    {
        QMatrix4x4 matrix;
        matrix.perspective(60.0f, width() / static_cast<float>(height()), 0.1f, 10.0f);
        matrix.translate(0, 0, -5);
        return matrix;
    }();

    { // draw with base program
        glBindVertexArray(vao);

        assert(base_program);
        base_program->bind();
        assertNoError();

        glDepthFunc(GL_LESS);
        glDepthMask(true);
        glEnable(GL_DEPTH_TEST);

        const auto blit_cube = [this]() -> void
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[1]);
            assertNoError();

            glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
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
            //matrix.translate(0, 10);
            //matrix.scale(3, 3, 3);
            matrix.rotate(frame_counter, 1, 1, 1);
            base_program->setUniformValue(base_mat_unif, matrix);

            blit_cube();
        }

        base_program->release();

        glBindVertexArray(0);
    }

    QtImGui::newFrame();
    if (display_ui)
        paintUI();
    ImGui::Render();

    frame_counter++;

    if (is_animated)
        update();
}
