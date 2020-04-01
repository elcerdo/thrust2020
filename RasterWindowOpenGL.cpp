#include "RasterWindowOpenGL.h"

#include <QKeyEvent>

#include "imgui.h"
#include "QtImGui.h"

#include <iostream>
#include <unordered_set>
#include <sstream>
#include <iomanip>

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

    QOpenGLWindow::keyPressEvent(event);
}

RasterWindowOpenGL::BufferLoader::BufferLoader(RasterWindowOpenGL& view_)
    : view(view_)
{
}

RasterWindowOpenGL::BufferLoader::~BufferLoader()
{
    assert(vao);
    view.assertNoError();

    using std::cout;
    using std::endl;

    cout << "vbos";
    size_t kk = 0;
    for (const auto& vbo : vbos)
    {
        cout << " " << vbo;
        if (reserved_indices.find(kk++) != std::cend(reserved_indices)) cout << "*";
    }
    cout << endl;

    view.vao = vao;
    view.vbos = vbos;
}

void RasterWindowOpenGL::BufferLoader::init(const size_t size)
{
    using std::cout;
    using std::endl;

    assert(vao == 0);
    assert(vbos.empty());
    assert(reserved_indices.empty());

    view.glGenVertexArrays(1, &vao);
    view.assertNoError();
    assert(vao > 0);

    cout << "========== vao " << vao << endl;
    vbos.resize(size);

    view.glBindVertexArray(vao);
    view.glGenBuffers(vbos.size(), vbos.data());
    view.glBindVertexArray(0);
    view.assertNoError();
}

bool RasterWindowOpenGL::BufferLoader::isAvailable(const size_t kk) const
{
    if (kk >= vbos.size()) return false;
    if (reserved_indices.find(kk) != std::cend(reserved_indices)) return false;
    return true;
}

void RasterWindowOpenGL::BufferLoader::reserve(const size_t kk)
{
    assert(isAvailable(kk));
    reserved_indices.emplace(kk);
}

void RasterWindowOpenGL::BufferLoader::loadBuffer2(const size_t kk, const std::vector<std::array<GLfloat, 2>>& values)
{
    reserve(kk);
    view.glBindVertexArray(vao);
    view.glBindBuffer(GL_ARRAY_BUFFER, vbos[kk]);
    view.glBufferData(GL_ARRAY_BUFFER, values.size() * 2 * sizeof(GLfloat), values.data(), GL_STATIC_DRAW);
    view.glBindVertexArray(0);
    view.assertNoError();
}

void RasterWindowOpenGL::BufferLoader::loadBuffer3(const size_t kk, const std::vector<std::array<GLfloat, 3>>& values)
{
    reserve(kk);
    view.glBindVertexArray(vao);
    view.glBindBuffer(GL_ARRAY_BUFFER, vbos[kk]);
    view.glBufferData(GL_ARRAY_BUFFER, values.size() * 3 * sizeof(GLfloat), values.data(), GL_STATIC_DRAW);
    view.glBindVertexArray(0);
    view.assertNoError();
}

void RasterWindowOpenGL::BufferLoader::loadBuffer4(const size_t kk, const std::vector<std::array<GLfloat, 4>>& values)
{
    reserve(kk);
    view.glBindVertexArray(vao);
    view.glBindBuffer(GL_ARRAY_BUFFER, vbos[kk]);
    view.glBufferData(GL_ARRAY_BUFFER, values.size() * 4 * sizeof(GLfloat), values.data(), GL_STATIC_DRAW);
    view.glBindVertexArray(0);
    view.assertNoError();
}

void RasterWindowOpenGL::BufferLoader::loadIndices(const size_t kk, const std::vector<GLuint>& indices)
{
    reserve(kk);
    view.glBindVertexArray(vao);
    view.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[kk]);
    view.glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    view.glBindVertexArray(0);
    view.assertNoError();
}

std::unique_ptr<QOpenGLShaderProgram> RasterWindowOpenGL::loadAndCompileProgram(const std::string& vertex_filename, const std::string& fragment_filename, const std::string& geometry_filename)
{
    using std::cerr;
    using std::cout;
    using std::endl;

    cout << "========== shader" << endl;
    auto program = std::make_unique<QOpenGLShaderProgram>();

    const auto load_shader = [&program](const QOpenGLShader::ShaderType type, const std::string& filename) -> bool
    {
        cout << "compiling ";
        switch (type)
        {
            case QOpenGLShader::Vertex:
                cout << "vertex ";
                break;
            case QOpenGLShader::Geometry:
                cout << "geometry ";
                break;
            case QOpenGLShader::Fragment:
                cout << "fragment ";
                break;
            default:
                assert(false);
                break;
        };
        cout << std::quoted(filename) << " ";
        cout.flush();

        QFile handle(QString::fromStdString(filename));
        if (!handle.open(QIODevice::ReadOnly))
            return false;
        const auto& source = handle.readAll();
        const auto compile_ok = program->addShaderFromSourceCode(type, source);

        cout << (compile_ok ? "OK" : "ERROR") << endl;

        return compile_ok;
    };

    const auto vertex_load_ok = load_shader(QOpenGLShader::Vertex, vertex_filename);
    const auto geometry_load_ok = geometry_filename.empty() ? true : load_shader(QOpenGLShader::Geometry, geometry_filename);
    const auto fragment_load_ok = load_shader(QOpenGLShader::Fragment, fragment_filename);

    cout << "linking ";
    cout.flush();
    const auto link_ok = program->link();
    cout << (link_ok ? "OK" : "ERROR") << endl;

    const auto gl_ok = glGetError() == GL_NO_ERROR;
    const auto all_ok = vertex_load_ok && fragment_load_ok && geometry_load_ok && link_ok && gl_ok;
    if (!all_ok) {
        cerr << program->log().toStdString() << endl;
        return nullptr;
    }
    assertNoError();
    assert(all_ok);

    return program;
}

void RasterWindowOpenGL::initializeGL()
{
    using std::cout;
    using std::endl;

    /*
    assert(!context);
    context = new QOpenGLContext(this);
    context->setFormat(requestedFormat());
    context->create();

    assert(context);
    context->makeCurrent(this);
    */

    {
        const auto version = QOpenGLContext::currentContext()->format().version();
        cout << "glversion " << version.first << "." << version.second << endl;
    }

    initializeOpenGLFunctions();
    QtImGui::initialize(this);
    assertNoError();

    initializePrograms();

    {
        BufferLoader loader(*this);
        initializeBuffers(loader);
    }
}

void RasterWindowOpenGL::ImGuiCallbacks()
{
    using std::get;

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
}

RasterWindowOpenGL::ProgramBinder::ProgramBinder(RasterWindowOpenGL& view_, std::unique_ptr<QOpenGLShaderProgram>& program_)
    : view(view_), program(program_)
{
    assert(program);
    program->bind();

    view.glBindVertexArray(view.vao);
    view.assertNoError();
}

RasterWindowOpenGL::ProgramBinder::~ProgramBinder()
{
    assert(program);
    program->release();

    view.glBindVertexArray(0);
    view.assertNoError();
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

    paintScene();

    QtImGui::newFrame();
    if (display_ui)
        paintUI();
    ImGui::Render();

    frame_counter++;

    if (is_animated)
        update();
}

