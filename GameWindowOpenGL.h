#pragma once

#include "GameState.h"

#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QOpenGLPaintDevice>
#include <QSoundEffect>
#include <QOpenGLShaderProgram>
#include <random>

class GameWindowOpenGL : public QOpenGLWindow, private QOpenGLFunctions
{
    Q_OBJECT
    public:
        using BoolCallback = std::function<void(bool)>;
        using BoolState = std::tuple<std::string, bool, BoolCallback>;
        using BoolStates = std::vector<BoolState>;

        using FloatCallback = std::function<void(float)>;
        using FloatState = std::tuple<std::string, float, float, float, FloatCallback>;
        using FloatStates = std::vector<FloatState>;

        GameWindowOpenGL(QWindow* parent = nullptr);
        void setAnimated(const bool value);
        void addSlider(const std::string& label, const float& min, const float& max, const float& value, const FloatCallback& callback);
        void addCheckbox(const std::string& label, const bool& value, const BoolCallback& callback);
        void setMuted(const bool muted);

    protected:
        void initializeGL() override;
        void paintGL() override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void drawOrigin(QPainter& painter) const;
        void drawBody(QPainter& painter, const b2Body* body, const QColor& color = Qt::black) const;
        void drawParticleSystem(QPainter& painter, const b2ParticleSystem* system, const QColor& color = Qt::black) const;
        void drawShip(QPainter& painter);
        void drawFlame(QPainter& painter);

    public:
        GameState state;
        std::default_random_engine rng;
        float speed_color[4] = { 0, 0, 1, 1 };
        bool is_animated = false;
        bool draw_debug = true;
        bool display_ui = true;

    protected:
        //bool show_test_window = true;
        //bool show_another_window = false;
        BoolStates bool_states;
        FloatStates float_states;

        QOpenGLPaintDevice* device = nullptr;
        size_t frame_counter = 0;

        QSoundEffect engine_sfx;
        QSoundEffect ship_click_sfx;
        //QSoundEffect back_click_sfx;

        bool is_muted = false;

        QOpenGLShaderProgram* program = nullptr;
        int pos_attr = -1;
        int col_attr = -1;
};


