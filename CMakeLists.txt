cmake_minimum_required(VERSION 2.8)

project(filescaner)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Widgets REQUIRED)

add_executable(filescaner main.cpp mainwindow.h mainwindow.cpp scanner.h scanner.cpp my_file.h my_file.cpp)
qt5_use_modules(filescaner Widgets)
