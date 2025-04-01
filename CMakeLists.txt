cmake_minimum_required(VERSION 3.12.0)
project(text_editor VERSION 0.1.0 LANGUAGES C CXX)
include(CTest)
enable_testing()

add_executable(text_editor src/main.c)
include_directories(include)
set_property(TARGET text_editor PROPERTY CXX_STANDARD 17)