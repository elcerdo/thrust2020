#include "GameWindowOpenGL.h"
#include "QtImGui.h"

#include <imgui.h>
#include <QDebug>

GameWindowOpenGL::GameWindowOpenGL(QWindow* parent)
    : is_animated(false)
{
}

void GameWindowOpenGL::setAnimated(const bool value)
{
    is_animated = value;
    if (is_animated)
        update();
}

void GameWindowOpenGL::initializeGL()
{
    initializeOpenGLFunctions();
    QtImGui::initialize(this);
}

void GameWindowOpenGL::addCheckbox(const std::string& label, const bool& value, const BoolCallback& callback)
{
    auto state = std::make_tuple(label, value, callback);
    bool_states.emplace_back(state);
    std::get<2>(state)(std::get<1>(state));
}

void GameWindowOpenGL::paintGL()
{
    QtImGui::newFrame();

    // 1. Show a simple window
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
    {
        static float f = 0.0f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", clear_color);
        //if (ImGui::Button("Test Window")) show_test_window ^= 1;
        //if (ImGui::Button("Another Window")) show_another_window ^= 1;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Separator();

        for (auto& state : bool_states)
        {
            const auto prev = std::get<1>(state);
            ImGui::Checkbox(std::get<0>(state).c_str(), &std::get<1>(state));
            if (prev != std::get<1>(state)) std::get<2>(state)(std::get<1>(state));
        }
    }


    /*
    // 2. Show another simple window, this time using an explicit Begin/End pair
    {
        ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiCond_FirstUseEver);
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello");
        ImGui::End();
    }

    // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
    if (show_test_window)
    {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
        ImGui::ShowDemoWindow();
    }
    */

    // Do render before ImGui UI is rendered
    glViewport(0, 0, width(), height());
    glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::Render();

    if (is_animated)
        update();
}
