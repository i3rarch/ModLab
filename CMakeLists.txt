cmake_minimum_required(VERSION 3.16)
project(diplom_v0 VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets SerialPort)

add_executable(${PROJECT_NAME}
    diplom_v0/main.cpp
    diplom_v0/diplom_v0.cpp
    diplom_v0/diplom_v0.h
    diplom_v0/diplom_v0.ui
    diplom_v0/diplom_v0.qrc
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::SerialPort
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()
