#include "Camera.h"
#include "RasterWindowOpenGL.h"

#include <imgui.h>

#include <QApplication>
#include <QOpenGLPaintDevice>
#include <QPainter>

#include <iostream>

class TestWindowOpenGL : public RasterWindowOpenGL
{
    public:
        float cube_angle = 30;
        bool show_demo_window = false;
        Camera camera;

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
        }

        void initializePrograms() override
        {
            device = std::make_unique<QOpenGLPaintDevice>();
            device->setDevicePixelRatio(devicePixelRatio());

            assert(!base_program);
            base_program = loadAndCompileProgram(":/shaders/base_vertex.glsl", ":/shaders/base_fragment.glsl");
            assert(base_program);
            const auto init_ok = initProgramLocations(*base_program, {
                { "posAttr", base_pos_attr },
            }, {
                { "cameraMatrix", base_camera_mat_unif },
                { "worldMatrix", base_world_mat_unif },
            });
            assert(init_ok);
            assertNoError();

            /*
            base_pos_attr = base_program->attributeLocation("posAttr");
            base_camera_mat_unif = base_program->uniformLocation("cameraMatrix");
            base_world_mat_unif = base_program->uniformLocation("worldMatrix");
            qDebug() << "locations" << base_pos_attr << base_camera_mat_unif << base_world_mat_unif;
            assert(base_pos_attr >= 0);
            assert(base_camera_mat_unif >= 0);
            assert(base_world_mat_unif >= 0);
            */
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
            ImGui::Separator();

            camera.paintUI();
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
            const auto camera_matrix = camera.cameraMatrix(*this);

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
                camera.preparePainter(*this, painter);

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

    view.camera.screen_height = 10;

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

