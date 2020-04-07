#include "Camera.h"

#include <cmath>
#include <imgui.h>

void
Camera::paintUI()
{
    ImGui::DragFloat2("camera pos", position.data(), .1, -10, 10, "%.1fm");
    ImGui::SliderFloat("screen height", &screen_height, .1, 1000, "%.1fm", 3);
    ImGui::SliderFloat("camera fov", &fov_angle, 10, 120, "%.1f°");
    ImGui::SliderFloat2("camera clip", clip.data(), .1, 1000, "%.1fm", 3);
    ImGui::SliderFloat("ortho ratio", &ortho_ratio, 0, 1, "%.3f");
}

void
Camera::preparePainter(const RasterWindowOpenGL& view, QPainter& painter) const
{
    using std::get;

    const auto foo = view.height() / screen_height;
    painter.translate(view.width() / 2., view.height() / 2.);
    painter.scale(foo, -foo);
    painter.translate(-get<0>(position), -get<1>(position));
}

QMatrix4x4
Camera::cameraMatrix(const RasterWindowOpenGL& view) const
{
    using std::get;

    const auto aspect_ratio = view.width() / static_cast<float>(view.height());
    const auto hh = screen_height / 2;

    QMatrix4x4 perspective_matrix;
    perspective_matrix.perspective(fov_angle, aspect_ratio, get<0>(clip), get<1>(clip));

    QMatrix4x4 ortho_matrix;
    ortho_matrix.ortho(-hh * aspect_ratio, hh * aspect_ratio, -hh, hh, get<0>(clip), get<1>(clip));

    auto mixed_matrix = ortho_ratio * ortho_matrix + (1 - ortho_ratio) * perspective_matrix;
    const auto zz = screen_height / tan(M_PI * fov_angle / 180 / 2) / 2;
    mixed_matrix.translate(-get<0>(position), -get<1>(position), -zz);

    return mixed_matrix;
}

