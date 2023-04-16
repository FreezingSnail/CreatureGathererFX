cmake_minimum_required(VERSION 3.10)
project(CreatureGathererFX)
if (USE_DEBUG)
  add_definitions(-DCLI)
  set(CMAKE_BUILD_TYPE Debug)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()


set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_subdirectory(src)
add_subdirectory(tst)
add_subdirectory(lib/googletest)