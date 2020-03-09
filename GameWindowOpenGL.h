#pragma once

#include "GameState.h"

#include <QOpenGLWindow>
#include <QOpenGLExtraFunctions>
#include <QOpenGLPaintDevice>
#include <QSoundEffect>
#include <QOpenGLShaderProgram>
#include <QSvgRenderer>

#include <random>
#include <array>
#include <unordered_map>

class GameWindowOpenGL : public QOpenGLWindow, private QOpenGLExtraFunctions
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

        GameWindowOpenGL(QWindow* parent = nullptr);
        void setAnimated(const bool value);
        void addSlider(const std::string& label, const float& min, const float& max, const float& value, const FloatCallback& callback);
        void addCheckbox(const std::string& label, const int key, const bool& value, const BoolCallback& callback);
        void addButton(const std::string& label, const int key, const VoidCallback& callback);
        void setMuted(const bool muted);
        bool isKeyFree(const int key) const;
				void loadBackground(const std::string& map_filename);

    protected:
        void initializeGL() override;
        void paintGL() override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void drawOrigin(QPainter& painter) const;
        void drawBody(QPainter& painter, const b2Body& body, const QColor& color = Qt::black) const;
        void drawParticleSystem(QPainter& painter, const b2ParticleSystem& system, const QColor& color = Qt::black) const;
        void drawShip(QPainter& painter);
        void drawFlame(QPainter& painter);

        std::unique_ptr<QOpenGLShaderProgram> loadAndCompileProgram(const QString& vertex_filename, const QString& fragment_filename, const QString& geometry_filename = QString());

    public:
        std::unique_ptr<GameState> state = nullptr;
        std::default_random_engine flame_rng;
        std::array<float, 4> water_color = { 108 / 255., 195 / 255., 246 / 255., 1 };
        bool is_animated = false;
        bool draw_debug = false;
        bool display_ui = true;
        bool is_zoom_out = true;
        int shader_selection = 2;

    protected:
        //bool show_test_window = true;
        //bool show_another_window = false;
        BoolStates checkbox_states;
        FloatStates float_states;
        ButtonStates button_states;

        QOpenGLPaintDevice* device = nullptr;
        //QOpenGLContext* context = nullptr;
        size_t frame_counter = 0;

        QSoundEffect engine_sfx;
        QSoundEffect ship_click_sfx;
        //QSoundEffect back_click_sfx;
        QSvgRenderer map_renderer;

        bool is_muted = false;

        std::unique_ptr<QOpenGLShaderProgram> base_program = nullptr;
        int base_pos_attr = -1;
        int base_mat_unif = -1;

        std::unique_ptr<QOpenGLShaderProgram> main_program = nullptr;
        int main_pos_attr = -1;
        int main_col_attr = -1;
        int main_mat_unif = -1;

        std::unique_ptr<QOpenGLShaderProgram> ball_program = nullptr;
        int ball_pos_attr = -1;
        int ball_mat_unif = -1;
        int ball_angular_speed_unif = -1;

        std::unique_ptr<QOpenGLShaderProgram> particle_program = nullptr;
        int particle_pos_attr = -1;
        int particle_col_attr = -1;
        int particle_mat_unif = -1;
        int particle_color_unif = -1;
        int particle_radius_unif = -1;
        int particle_mode_unif = -1;

        GLuint vao = 0;
        std::array<GLuint, 8> vbos = { 0, 0, 0, 0, 0, 0, 0, 0 };
};


