cmake_minimum_required(VERSION 3.10.0)

project(rocket)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets Multimedia OpenGL Svg REQUIRED)

set(BOX2D_VERSION 2.3.0)
set(BOX2D_BUILD_STATIC TRUE)
add_subdirectory(liquidfun/liquidfun/Box2D/Box2D)
include_directories(liquidfun/liquidfun/Box2D)
include_directories(imgui)

enable_testing()

add_library(acd2d STATIC
    acd2d/src/acd2d_data.cpp
    acd2d/src/acd2d_concavity.cpp
    acd2d/src/acd2d_core.cpp
    acd2d/src/hull/acd2d_hull.cpp
    )
target_include_directories(acd2d
    PUBLIC
    acd2d/src
    acd2d/src/mathtool
    acd2d/src/hull
    acd2d/src/edge_visibility
    )

add_executable(test_level
    load_levels.cpp
    test_level.cpp
    rocket.qrc
    )
target_link_libraries(test_level
    Qt5::Core
    )
add_test(test_level
    test_level
    )

add_executable(test_convex_decomposition
    data_polygons.cpp
    decompose_polygons.cpp
    test_convex_decomposition.cpp
    )
target_link_libraries(test_convex_decomposition
    acd2d
    )
add_test(test_convex_decomposition
    test_convex_decomposition
    )

add_executable(test_extract_polygons
    data_polygons.cpp
    extract_polygons.cpp
    decompose_polygons.cpp
    test_extract_polygons.cpp
    rocket.qrc
    )
target_link_libraries(test_extract_polygons
    Qt5::Core
    Qt5::Svg
    acd2d
    )
add_test(test_extract_polygons
    test_extract_polygons
    )

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

add_executable(test_raster_window
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    QtImGui.cpp
    ImGuiRenderer.cpp
    RasterWindowOpenGL.cpp
    test_raster_window.cpp
    rocket.qrc
    )
target_link_libraries(test_raster_window
    Qt5::Widgets
    Qt5::OpenGL
    )

add_executable(rocket
    load_levels.cpp
    data_polygons.cpp
    extract_polygons.cpp
    decompose_polygons.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    QtImGui.cpp
    ImGuiRenderer.cpp
    RasterWindowOpenGL.cpp
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
    acd2d
    )

