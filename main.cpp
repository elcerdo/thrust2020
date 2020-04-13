#include "GameWindowOpenGL.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    { // default opengl format
        QSurfaceFormat format;
        format.setVersion(3, 3);
        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
        format.setAlphaBufferSize(0);
        format.setStencilBufferSize(0);
        format.setDepthBufferSize(16);
        format.setProfile(QSurfaceFormat::CoreProfile);
        QSurfaceFormat::setDefaultFormat(format);
    }

    QApplication app(argc, argv);
    GameWindowOpenGL view;

    view.setAnimated(true);
    view.resize(1280, 720);
    view.show();

    view.addCheckbox("gravity", Qt::Key_G, true, [&view](const bool checked) -> void {
        if (!view.state)
            return;
        assert(view.state);
        view.state->world.SetGravity(checked ? b2Vec2 { 0, -10 } : b2Vec2 {0, 0});
    });
    view.addCheckbox("draw debug", Qt::Key_P, false, [&view](const bool checked) -> void {
        view.draw_debug = checked;
    });
    view.addCheckbox("mute sfx", Qt::Key_M, true, [&view](const bool checked) -> void {
        view.setMuted(checked);
    });
    view.addCheckbox("world view", Qt::Key_W, false, [&view](const bool checked) -> void {
        view.use_world_camera = checked;
    });
    view.addCheckbox("pause", Qt::Key_X, false, [&view](const bool checked) -> void {
        view.skip_state_step = checked;
    });
    view.addCheckbox("painter", Qt::Key_O, true, [&view](const bool checked) -> void {
        view.use_painter = checked;
    });

    std::default_random_engine rng;
    int tag = 0;
    view.addButton("drop water", Qt::Key_E, [&view, &rng]() -> void {
        if (!view.state)
            return;
        assert(view.state);
        view.state->addWater(view.water_spawn, view.water_drop_size, rng(), view.water_flags);
    });
    view.addButton("drop crate", Qt::Key_R, [&view, &rng, &tag]() -> void {
        if (!view.state)
            return;
        assert(view.state);
        std::uniform_real_distribution<double> dist_angle(0, 2 * M_PI);
        const auto angle = dist_angle(rng);
        std::normal_distribution<double> dist_normal(0, 10);
        const b2Vec2 velocity(dist_normal(rng), dist_normal(rng));
        view.state->addCrate(view.crate_spawn, velocity, angle, tag++);
        return;
    });
    view.addButton("clear all water", Qt::Key_D, [&view]() -> void {
        if (!view.state)
            return;
        assert(view.state);
        view.state->clearWater(-1);
    });
    view.addButton("clear all crates", Qt::Key_F, [&view]() -> void {
        if (!view.state)
            return;
        assert(view.state);
        view.state->clearCrates();
    });
    view.addButton("clear last water", Qt::Key_C, [&view]() -> void {
        if (!view.state)
            return;
        assert(view.state);
        view.state->clearWater(1);
    });
    view.addButton("clear level", Qt::Key_Y, [&view]() -> void {
        view.current_level = -1;
        view.resetLevel();
    });
    view.addButton("reset ship", Qt::Key_S, [&view]() -> void {
        if (!view.state)
            return;
        assert(view.state);
        view.state->resetShip(view.ship_spawn);
    });
    view.addButton("reset ball", Qt::Key_B, [&view]() -> void {
        if (!view.state)
            return;
        assert(view.state);
        view.state->resetBall(view.ball_spawn);
    });
    view.addButton("toggle doors", Qt::Key_T, [&view]() -> void {
        if (!view.state)
            return;
        assert(view.state);
        using std::get;
        for (auto& door : view.state->doors)
        {
            auto& target = get<2>(door);
            target ++;
            target %= get<1>(door).size();
            qDebug() << "target" << target;
        }
        return;
    });

    return app.exec();
}

