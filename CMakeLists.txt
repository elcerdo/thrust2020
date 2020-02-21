cmake_minimum_required(VERSION 3.10.0)

project(rocket)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets Multimedia OpenGL Svg REQUIRED)

set(BOX2D_VERSION 2.3.0)
set(BOX2D_BUILD_STATIC TRUE)
add_subdirectory(liquidfun/liquidfun/Box2D/Box2D)
include_directories(liquidfun/liquidfun/Box2D)
include_directories(imgui)

add_executable(test_imgui
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_widgets.cpp
    QtImGui.cpp
    ImGuiRenderer.cpp
    test_imgui.cpp
    )
target_link_libraries(test_imgui
    Qt5::Widgets
    Qt5::OpenGL
    )

add_executable(test_svg_polygon
    test_svg_polygon.cpp
    rocket.qrc
    )
target_link_libraries(test_svg_polygon
    Qt5::Core
    Qt5::Svg
    )

add_executable(rocket
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    QtImGui.cpp
    ImGuiRenderer.cpp
    GameWindowOpenGL.cpp
    GameState.cpp
    main.cpp
    #mainwindow.ui
    #mainwindow.cpp
    rocket.qrc
    )
target_link_libraries(rocket
    Box2D
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::OpenGL
    Qt5::Svg
    )

