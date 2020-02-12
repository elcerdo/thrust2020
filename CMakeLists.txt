cmake_minimum_required(VERSION 3.10.0)

project(rocket)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets Multimedia REQUIRED)

set(BOX2D_VERSION 2.3.0)
set(BOX2D_BUILD_STATIC TRUE)
add_subdirectory(liquidfun/liquidfun/Box2D/Box2D)
include_directories(liquidfun/liquidfun/Box2D)

add_executable(rocket
    RasterWindow.cpp
    GameWindow.cpp
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
    )

