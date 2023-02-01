cmake_minimum_required(VERSION 3.10)
project(ArdumonFX)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_subdirectory(src)
add_subdirectory(tst)
add_subdirectory(lib/googletest)