#include "RasterWindowOpenGL.h"

#include <imgui.h>

#include <QApplication>

#include <iostream>

class TestWindowOpenGL : public RasterWindowOpenGL
{
    public:
        float angle = 0;

    protected:
        std::unique_ptr<QOpenGLShaderProgram> base_program = nullptr;
        int base_pos_attr = -1;
        int base_mat_unif = -1;

        void initializePrograms() override
        {
            assert(!base_program);
            base_program = loadAndCompileProgram(":/shaders/base_vertex.glsl", ":/shaders/base_fragment.glsl");
            assert(base_program);

            base_pos_attr = base_program->attributeLocation("posAttr");
            base_mat_unif = base_program->uniformLocation("matrix");
            qDebug() << "locations" << base_pos_attr << base_mat_unif;
            assert(base_pos_attr >= 0);
            assert(base_mat_unif >= 0);
            assertNoError();
        }

        void initializeBuffers(BufferLoader& loader) override
        {
            loader.init(2);

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
        }

        void paintUI() override
        {
            constexpr auto  ui_window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize;

            ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Once);
            ImGui::Begin("test_raster_window", &display_ui, ui_window_flags);
            ImGuiCallbacks();
            ImGui::Separator();

            {
                const auto& io = ImGui::GetIO();
                ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                ImGui::Text("left %d right %d", io.KeysDown[ImGuiKey_LeftArrow], io.KeysDown[ImGuiKey_RightArrow]);
            }

            ImGui::End();

        }

        void paintScene() override
        {
            const auto world_matrix = [this]() -> QMatrix4x4
            {
                QMatrix4x4 matrix;
                matrix.perspective(60.0f, width() / static_cast<float>(height()), 0.1f, 10.0f);
                matrix.translate(0, 0, -5);
                return matrix;
            }();

            { // draw with base program
                ProgramBinder binder(*this, base_program);

                glDepthFunc(GL_LESS);
                glEnable(GL_DEPTH_TEST);

                const auto blit_cube = [this]() -> void
                {
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[1]);
                    assertNoError();

                    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
                    glVertexAttribPointer(base_pos_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
                    glEnableVertexAttribArray(base_pos_attr);
                    assertNoError();

                    glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, reinterpret_cast<void*>(0));
                    glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, reinterpret_cast<void*>(8 * sizeof(unsigned int)));
                    assertNoError();

                    glDisableVertexAttribArray(base_pos_attr);
                    assertNoError();
                };

                {
                    auto matrix = world_matrix;
                    //matrix.translate(0, 10);
                    //matrix.scale(3, 3, 3);
                    matrix.rotate(angle, 1, 1, 1);
                    base_program->setUniformValue(base_mat_unif, matrix);

                    blit_cube();
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

    view.addSlider("slider", 0, 360, 30, [&view](const float value) -> void {
        cout << "slider " << value << endl;
        view.angle = value;
    });

    view.addCheckbox("checkbox", Qt::Key_Q, true, [](const bool checked) -> void {
        cout << "checkbox " << checked << endl;
    });

    view.addButton("button0", Qt::Key_Z, []() -> void {
        cout << "button0" << endl;
    });
    view.addButton("button1", Qt::Key_E, []() -> void {
        cout << "button1" << endl;
    });

    return app.exec();
}

