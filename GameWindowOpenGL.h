#pragma once

#include "GameState.h"

#include <QOpenGLWindow>
#include <QOpenGLPaintDevice>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>

#include <vector>
#include <tuple>
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

        void setAnimated(const bool value);
        void addSlider(const std::string& label, const float& min, const float& max, const float& value, const FloatCallback& callback);
        void addCheckbox(const std::string& label, const bool& value, const BoolCallback& callback);

    protected:
        void initializeGL() override;
        void paintGL() override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void drawOrigin(QPainter& painter) const;
        void drawBody(QPainter& painter, const b2Body* body, const QColor& color = Qt::black) const;
        void drawShip(QPainter& painter);
        void drawFlame(QPainter& painter);

    public:
        GameState state;
        std::default_random_engine rng;
        float clear_color[4] = { .2, .2, .2, 1 };
        bool is_animated = false;
        bool draw_debug = true;

    protected:
        //bool show_test_window = true;
        //bool show_another_window = false;
        BoolStates bool_states;
        FloatStates float_states;

        QOpenGLPaintDevice* device = nullptr;
        size_t frame_counter = 0;
};

