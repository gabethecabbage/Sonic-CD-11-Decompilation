cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)

project(SonicCDDecomp C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

find_package(SDL2 REQUIRED)
find_package(OggVorbis REQUIRED)
find_package(Theora REQUIRED)

add_subdirectory(SonicCDDecomp)
