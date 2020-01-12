cmake_minimum_required(VERSION 3.10.0)

project(rocket)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_subdirectory(box2d/src)

add_executable(rocket
    RasterWindow.cpp
    GameWindow.cpp
    GameState.cpp
    main.cpp
    #mainwindow.ui
    #mainwindow.cpp
    #resources.qrc
    )

target_link_libraries(rocket
    box2d
    Qt5::Widgets
    )

