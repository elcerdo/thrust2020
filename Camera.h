#pragma once

#include "RasterWindowOpenGL.h"

#include <QPainter>

#include <array>

struct Camera
{
    std::array<float, 2> position = { 0, 0 };
    float screen_height = 100;
    float fov_angle = 50.;
    std::array<float, 2> clip = { .1, 1000 };
    float ortho_ratio = 0;

    void paintUI();
    void preparePainter(const RasterWindowOpenGL& view, QPainter& painter) const;
    QMatrix4x4 cameraMatrix(const RasterWindowOpenGL& view) const;
};

