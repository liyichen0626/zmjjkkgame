cmake_minimum_required(VERSION 3.16)

project(game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(game
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/loginpage.h
    src/loginpage.cpp
    src/settingsdialog.h
    src/settingsdialog.cpp
    src/gameaudio.h
    src/gameaudio.cpp
    src/gamepage.h
    src/gamepage.cpp
)

target_link_libraries(game PRIVATE Qt6::Widgets)
