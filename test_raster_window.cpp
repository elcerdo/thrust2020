#include "RasterWindowOpenGL.h"

#include <imgui.h>

#include <QApplication>
#include <QOpenGLPaintDevice>
#include <QPainter>

#include <iostream>
#include <cmath>

class TestWindowOpenGL : public RasterWindowOpenGL
{
    public:
        float cube_angle = 30;

        std::array<float, 2> camera_position = { 0, 0 };
        float camera_screen_height = 4;
        float camera_fov_angle = 60.;
        std::array<float, 2> camera_clip = { .1, 100 };
        float camera_ortho_ratio = 0;

        bool show_demo_window = false;

        std::unique_ptr<QOpenGLPaintDevice> device = nullptr;

    protected:
        std::unique_ptr<QOpenGLShaderProgram> base_program = nullptr;
        int base_pos_attr = -1;
        int base_camera_mat_unif = -1;
        int base_world_mat_unif = -1;

        void initializeUI() override
        {
            ImVec4* colors = ImGui::GetStyle().Colors;
            colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.67f);

            device = std::make_unique<QOpenGLPaintDevice>();
            device->setDevicePixelRatio(devicePixelRatio());
        }

        void initializePrograms() override
        {
            assert(!base_program);
            base_program = loadAndCompileProgram(":/shaders/base_vertex.glsl", ":/shaders/base_fragment.glsl");
            assert(base_program);

            base_pos_attr = base_program->attributeLocation("posAttr");
            base_camera_mat_unif = base_program->uniformLocation("cameraMatrix");
            base_world_mat_unif = base_program->uniformLocation("worldMatrix");
            qDebug() << "locations" << base_pos_attr << base_camera_mat_unif << base_world_mat_unif;
            assert(base_pos_attr >= 0);
            assert(base_camera_mat_unif >= 0);
            assert(base_world_mat_unif >= 0);
            assertNoError();
        }

        void initializeBuffers(BufferLoader& loader) override
        {
            loader.init(3);

            // cube
            loader.loadBuffer3(0, {
                { -1, -1, 1 },
                { 1, -1, 1 },
                { 1, 1, 1 },
                { -1, 1, 1 },
                { -1, -1, -1 },
                { 1, -1, -1 },
                { 1, 1, -1 },
                { -1, 1, -1 },
            });
            loader.loadIndices(1, {
                0, 1, 3, 2, 7, 6, 4, 5,
                3, 7, 0, 4, 1, 5, 2, 6,
            });

            // square
            loader.loadBuffer2(2, {
                { -1, -1 },
                { 1, -1 },
                { -1, 1 },
                { 1, 1 },
            });
        }

        void paintUI() override
        {
            ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Once);
            ImGui::Begin("test_raster_window", &display_ui);

            ImGui::SliderFloat("cube angle", &cube_angle, 0, 360, "%.1f°");
            ImGui::DragFloat2("camera pos", camera_position.data(), .1, -10, 10, "%.1fm");
            ImGui::SliderFloat("screen height", &camera_screen_height, .1, 10, "%.1fm");
            ImGui::Separator();

            ImGui::SliderFloat("camera fov", &camera_fov_angle, 5, 90, "%.1f°");
            ImGui::SliderFloat2("camera clip", camera_clip.data(), .1, 100, "%.1fm", 3);
            ImGui::SliderFloat("ortho ratio", &camera_ortho_ratio, 0, 1, "%.3f");
            ImGui::Separator();

            ImGuiCallbacks();
            ImGui::Separator();

            {
                const auto& io = ImGui::GetIO();
                ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                ImGui::Text("left %d right %d", io.KeysDown[ImGuiKey_LeftArrow], io.KeysDown[ImGuiKey_RightArrow]);
            }

            ImGui::End();

            if (show_demo_window)
                ImGui::ShowDemoWindow();
        }

        void paintScene() override
        {
            using std::get;

            const auto prepare_painter = [this](QPainter& painter) -> void
            {
                const auto foo = height() / camera_screen_height;
                painter.translate(width() / 2., height() / 2.);
                painter.scale(foo, -foo);
                painter.translate(-get<0>(camera_position), -get<1>(camera_position));
            };

            const auto camera_matrix = [this]() -> QMatrix4x4
            {
                const auto aspect_ratio = width() / static_cast<float>(height());
                const auto hh = camera_screen_height / 2;

                QMatrix4x4 perspective_matrix;
                perspective_matrix.perspective(camera_fov_angle, aspect_ratio, get<0>(camera_clip), get<1>(camera_clip));

                QMatrix4x4 ortho_matrix;
                ortho_matrix.ortho(-hh * aspect_ratio, hh * aspect_ratio, -hh, hh, get<0>(camera_clip), get<1>(camera_clip));

                auto mixed_matrix = camera_ortho_ratio * ortho_matrix + (1 - camera_ortho_ratio) * perspective_matrix;
                const auto camera_zz = camera_screen_height / tan(M_PI * camera_fov_angle / 180 / 2) / 2;
                mixed_matrix.translate(-get<0>(camera_position), -get<1>(camera_position), -camera_zz);

                return mixed_matrix;
            }();

            { // draw with base program
                ProgramBinder binder(*this, base_program);

                glDepthFunc(GL_LESS);
                glEnable(GL_DEPTH_TEST);

                const auto blit_cube = [this]() -> void
                {
                    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
                    glVertexAttribPointer(base_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
                    glEnableVertexAttribArray(base_pos_attr);
                    assertNoError();

                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[1]);
                    assertNoError();

                    glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, reinterpret_cast<void*>(0));
                    glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, reinterpret_cast<void*>(8 * sizeof(unsigned int)));
                    assertNoError();

                    glDisableVertexAttribArray(base_pos_attr);
                    assertNoError();
                };

                const auto blit_square = [this]() -> void
                {
                    glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
                    glVertexAttribPointer(base_pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
                    glEnableVertexAttribArray(base_pos_attr);
                    assertNoError();

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    assertNoError();

                    glDisableVertexAttribArray(base_pos_attr);
                    assertNoError();
                };

                base_program->setUniformValue(base_camera_mat_unif, camera_matrix);

                {
                    QMatrix4x4 world_matrix;
                    world_matrix.rotate(cube_angle, 0, 0, 1);
                    world_matrix.scale(.5, .5, .5);
                    world_matrix.translate(1, 1, 0);
                    base_program->setUniformValue(base_world_mat_unif, world_matrix);
                    blit_square();
                }

                {
                    QMatrix4x4 world_matrix;
                    world_matrix.translate(0, 1, 0);
                    world_matrix.scale(2, 2, 2);
                    world_matrix.rotate(90, 1, 0, 0);
                    base_program->setUniformValue(base_world_mat_unif, world_matrix);
                    blit_square();
                }

                {
                    QMatrix4x4 world_matrix;
                    world_matrix.translate(-1.5, 0, 0);
                    world_matrix.rotate(cube_angle, 1, 0, 0);
                    base_program->setUniformValue(base_world_mat_unif, world_matrix);
                    blit_cube();
                }
            }

            {
                assert(device);
                device->setSize(size() * devicePixelRatio());
                QPainter painter(device.get());

                prepare_painter(painter);

                { // background gradient
                    QLinearGradient linearGrad(QPointF(0, 0), QPointF(1, 1));
                    linearGrad.setColorAt(0, QColor(0x33, 0x08, 0x67)); // morpheus den gradient
                    linearGrad.setColorAt(1, QColor(0x30, 0xcf, 0xd0));
                    painter.fillRect(QRectF(-.5, -.5, 1, 1), linearGrad);
                }
            }
        }
};

int main(int argc, char* argv[])
{
    using std::cout;
    using std::endl;

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

    TestWindowOpenGL view;

    view.setAnimated(true);
    view.resize(1280, 720);
    view.show();

    view.addCheckbox("imgui demo", Qt::Key_Q, false, [&view](const bool checked) -> void {
        view.show_demo_window = checked;
    });
    view.addCheckbox("checkbox", Qt::Key_W, false, [&view](const bool checked) -> void {
        cout << "Checkbox " << checked << endl;
    });

    view.addButton("button0", Qt::Key_Z, []() -> void {
        cout << "button0" << endl;
    });
    view.addButton("button1", Qt::Key_E, []() -> void {
        cout << "button1" << endl;
    });

    return app.exec();
}

