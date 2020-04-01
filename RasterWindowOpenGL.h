#pragma once

#include <QOpenGLWindow>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>

#include <unordered_map>
#include <memory>

class RasterWindowOpenGL : public QOpenGLWindow, private QOpenGLExtraFunctions
{
    Q_OBJECT
    public:
        using VoidCallback = std::function<void(void)>;
        using ButtonState = std::tuple<size_t, std::string, VoidCallback>;
        using ButtonStates = std::unordered_map<int, ButtonState>;

        using BoolCallback = std::function<void(bool)>;
        using BoolState = std::tuple<size_t, std::string, bool, BoolCallback>;
        using BoolStates = std::unordered_map<int, BoolState>;

        using FloatCallback = std::function<void(float)>;
        using FloatState = std::tuple<std::string, float, float, float, FloatCallback>;
        using FloatStates = std::vector<FloatState>;

        RasterWindowOpenGL(QWindow* parent = nullptr);
        void setAnimated(const bool value);
        void addSlider(const std::string& label, const float& min, const float& max, const float& value, const FloatCallback& callback);
        void addCheckbox(const std::string& label, const int key, const bool& value, const BoolCallback& callback);
        void addButton(const std::string& label, const int key, const VoidCallback& callback);
        bool isKeyFree(const int key) const;

    protected:
        void assertNoError();
        void initializeGL() override;
        void paintGL() override;
        void keyPressEvent(QKeyEvent* event) override;

        std::unique_ptr<QOpenGLShaderProgram> loadAndCompileProgram(const QString& vertex_filename, const QString& fragment_filename, const QString& geometry_filename = QString());

    public:
        bool is_animated = false;
        bool display_ui = true;

    protected:
        BoolStates checkbox_states;
        FloatStates float_states;
        ButtonStates button_states;

        size_t frame_counter = 0;

        std::unique_ptr<QOpenGLShaderProgram> base_program = nullptr;
        int base_pos_attr = -1;
        int base_mat_unif = -1;

        GLuint vao = 0;
        std::array<GLuint, 10> vbos = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
};


