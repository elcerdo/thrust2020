#pragma once

#include <QOpenGLWindow>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <array>

class RasterWindowOpenGL : public QOpenGLWindow, public QOpenGLExtraFunctions
{
    Q_OBJECT
    public:
        using VoidCallback = std::function<void(void)>;
        using ButtonState = std::tuple<size_t, std::string, VoidCallback>;
        using ButtonStates = std::unordered_map<int, ButtonState>;

        using BoolCallback = std::function<void(bool)>;
        using BoolState = std::tuple<size_t, std::string, bool, BoolCallback>;
        using BoolStates = std::unordered_map<int, BoolState>;

        RasterWindowOpenGL(QWindow* parent = nullptr);
        void setAnimated(const bool value);
        void addCheckbox(const std::string& label, const int key, const bool& value, const BoolCallback& callback);
        void addButton(const std::string& label, const int key, const VoidCallback& callback);
        bool isKeyFree(const int key) const;
        void registerFreeKey(const int key);
        void enforceCallbackValues() const;

    protected:

        void assertNoError();
        void initializeGL() override final;
        void paintGL() override final;
        void keyPressEvent(QKeyEvent* event) override;

        class BufferLoader
        {
            public:
                BufferLoader(RasterWindowOpenGL& view);
                ~BufferLoader();

                void init(const size_t size);
                bool isAvailable(const size_t kk) const;
                void reserve(const size_t kk);
                void loadBuffer2(const size_t kk, const std::vector<std::array<GLfloat, 2>>& values);
                void loadBuffer3(const size_t kk, const std::vector<std::array<GLfloat, 3>>& values);
                void loadBuffer4(const size_t kk, const std::vector<std::array<GLfloat, 4>>& values);
                void loadIndices(const size_t kk, const std::vector<GLuint>& indices);

            protected:
                RasterWindowOpenGL& view;
                GLuint vao = 0;
                std::vector<GLuint> vbos;
                std::unordered_set<size_t> reserved_indices;
        };
        virtual void initializeBuffers(BufferLoader& loader) = 0;

        std::unique_ptr<QOpenGLShaderProgram> loadAndCompileProgram(const std::string& vertex_filename, const std::string& fragment_filename, const std::string& geometry_filename = "");

        using Locations = std::unordered_map<std::string, int&>;
        bool initLocations(const QOpenGLShaderProgram& program, const Locations& attr_locations, const Locations& unif_locations);

        virtual void initializePrograms() = 0;

        virtual void initializeUI() = 0;

        class ProgramBinder
        {
            public:
                ProgramBinder(RasterWindowOpenGL& view, std::unique_ptr<QOpenGLShaderProgram>& program);
                ~ProgramBinder();

            protected:
                RasterWindowOpenGL& view;
                std::unique_ptr<QOpenGLShaderProgram>& program;

        };
        virtual void paintScene() = 0;

        void ImGuiCallbacks();
        virtual void paintUI() = 0;

    public:
        bool is_animated = false;
        bool display_ui = true;

    protected:
        BoolStates checkbox_states;
        ButtonStates button_states;
        std::unordered_set<int> other_keys;

        GLuint vao = 0;
        std::vector<GLuint> vbos;
};


