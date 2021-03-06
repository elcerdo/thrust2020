#pragma once

#include "load_levels.h"
#include "GameState.h"
#include "Camera.h"
#include "RasterWindowOpenGL.h"

#include <QOpenGLPaintDevice>
#include <QSoundEffect>
#include <QSvgRenderer>
#include <QOpenGLTexture>

#include <random>

class GameWindowOpenGL : public RasterWindowOpenGL
{
    Q_OBJECT
    public:
        GameWindowOpenGL(QWindow* parent = nullptr);
        void setMuted(const bool muted);
        void loadBackground(const std::string& map_filename);
        void resetLevel();

    protected:
        void keyPressEvent(QKeyEvent* event) override;

        void drawOrigin(QPainter& painter) const;
        void drawBody(QPainter& painter, const b2Body& body, const QColor& color = Qt::black) const;
        void drawParticleSystem(QPainter& painter, const b2ParticleSystem& system, const QColor& color = Qt::black) const;
        void drawShip(QPainter& painter);

        void initializeUI() override;
        void initializeBuffers(BufferLoader& loader) override;
        void initializePrograms() override;
        void paintUI() override;
        void paintScene() override;

    public:
        levels::MainData data;
        std::unique_ptr<GameState> state = nullptr;
        std::default_random_engine flame_rng;
        std::array<float, 4> water_color = { 108 / 255., 195 / 255., 246 / 255., 1 };
        std::array<float, 4> foam_color = { 1, 1, 1, 1 };
        std::array<float, 4> halo_out_color = { 0, 0, 1, .6 };
        std::array<float, 4> halo_in_color = { 0, 0, 1, .2 };
        std::array<float, 4> viscous_color = { 1, 0, 1, 1 };
        std::array<float, 4> tensible_color = { 0, 1, 1, 1 };
        std::array<float, 4> crate_color = { 1, 1, 1, 1 };
        int crate_max_tag = 10;
        float mix_ratio = .2;
        bool draw_debug = false;
        int shader_selection = 8;
        int poly_selection = 3;
        float radius_factor = 1;
        int current_level = -1;
        float shading_max_speed = 60;
        float shading_alpha = -.65;
        unsigned int water_flags = b2_viscousParticle | b2_tensileParticle;
        bool skip_state_step = false;
        bool use_painter = true;
        float world_time = 0;

        bool use_world_camera = false;
        Camera ship_camera;
        Camera world_camera;

        b2Vec2 ship_spawn = { 0, 0 };
        b2Vec2 ball_spawn = { 20, 0 };
        b2Vec2 crate_spawn = { 0, 20 };
        b2Vec2 water_spawn = { 0, 50 };
        b2Vec2 water_drop_size = { 10, 10 };

    protected:
        std::unique_ptr<QOpenGLPaintDevice> device = nullptr;

        QSoundEffect engine_sfx;
        QSoundEffect ship_click_sfx;
        QImage logo;
        //QSoundEffect back_click_sfx;

        QSvgRenderer map_renderer;

        bool is_muted = false;

        std::unique_ptr<QOpenGLShaderProgram> base_program = nullptr;
        int base_pos_attr = -1;
        int base_camera_mat_unif = -1;
        int base_world_mat_unif = -1;

        std::unique_ptr<QOpenGLShaderProgram> main_program = nullptr;
        int main_pos_attr = -1;
        int main_col_attr = -1;
        int main_camera_mat_unif = -1;
        int main_world_mat_unif = -1;

        std::unique_ptr<QOpenGLShaderProgram> ball_program = nullptr;
        int ball_pos_attr = -1;
        int ball_angular_speed_unif = -1;
        int ball_camera_mat_unif = -1;
        int ball_world_mat_unif = -1;

        std::unique_ptr<QOpenGLShaderProgram> grab_program = nullptr;
        int grab_pos_attr = -1;
        int grab_time_unif = -1;
        int grab_halo_out_color_unif = -1;
        int grab_halo_in_color_unif = -1;
        int grab_camera_mat_unif = -1;
        int grab_world_mat_unif = -1;

        std::unique_ptr<QOpenGLShaderProgram> particle_program = nullptr;
        int particle_pos_attr = -1;
        int particle_col_attr = -1;
        int particle_speed_attr = -1;
        int particle_flag_attr = -1;
        int particle_water_color_unif = -1;
        int particle_foam_color_unif = -1;
        int particle_radius_unif = -1;
        int particle_radius_factor_unif = -1;
        int particle_mode_unif = -1;
        int particle_poly_unif = -1;
        int particle_max_speed_unif = -1;
        int particle_alpha_unif = -1;
        int particle_viscous_color_unif = -1;
        int particle_tensible_color_unif = -1;
        int particle_mix_unif = -1;
        int particle_camera_mat_unif = -1;
        int particle_world_mat_unif = -1;

        std::unique_ptr<QOpenGLShaderProgram> crate_program = nullptr;
        int crate_pos_attr = -1;
        int crate_camera_mat_unif = -1;
        int crate_world_mat_unif = -1;
        int crate_texture_unif = -1;
        int crate_color_unif = -1;
        int crate_max_tag_unif = -1;
        int crate_tag_unif = -1;
        std::unique_ptr<QOpenGLTexture> crate_texture = nullptr;
};

