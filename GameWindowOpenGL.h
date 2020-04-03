#pragma once

#include "load_levels.h"
#include "GameState.h"
#include "RasterWindowOpenGL.h"

#include <QOpenGLPaintDevice>
#include <QSoundEffect>
#include <QSvgRenderer>

#include <random>

class GameWindowOpenGL : public RasterWindowOpenGL
{
    Q_OBJECT
    public:
        GameWindowOpenGL(QWindow* parent = nullptr);
        void setMuted(const bool muted);
        void loadBackground(const std::string& map_filename);
        void resetLevel(const int level);

    protected:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void drawOrigin(QPainter& painter) const;
        void drawBody(QPainter& painter, const b2Body& body, const QColor& color = Qt::black) const;
        void drawParticleSystem(QPainter& painter, const b2ParticleSystem& system, const QColor& color = Qt::black) const;
        void drawShip(QPainter& painter);
        void drawFlame(QPainter& painter);

        void initializeBuffers(BufferLoader& loader) override;
        void initializePrograms() override;
        void paintUI() override;
        void paintScene() override;

    public:
        levels::LevelDatas level_datas;
        std::unique_ptr<GameState> state = nullptr;
        std::default_random_engine flame_rng;
        std::array<float, 4> water_color = { 108 / 255., 195 / 255., 246 / 255., 1 };
        std::array<float, 4> foam_color = { 1, 1, 1, 1 };
        bool draw_debug = false;
        bool is_zoom_out = true;
        bool is_paused = false;
        int shader_selection = 6;
        int poly_selection = 3;
        float radius_factor = 1;
        float shading_max_speed = 60;
        float shading_alpha = -0.65;

    protected:
        QOpenGLPaintDevice* device = nullptr;

        QSoundEffect engine_sfx;
        QSoundEffect ship_click_sfx;
        //QSoundEffect back_click_sfx;

        QSvgRenderer map_renderer;

        bool is_muted = false;

        int level_current = -1;

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
        int particle_speed_attr = -1;
        int particle_flag_attr = -1;
        int particle_mat_unif = -1;
        int particle_water_color_unif = -1;
        int particle_foam_color_unif = -1;
        int particle_radius_unif = -1;
        int particle_radius_factor_unif = -1;
        int particle_mode_unif = -1;
        int particle_poly_unif = -1;
        int particle_max_speed_unif = -1;
        int particle_alpha_unif = -1;
};


