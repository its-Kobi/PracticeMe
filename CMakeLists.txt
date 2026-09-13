cmake_minimum_required(VERSION 3.25)
project(PracticeMe VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

include(cmake/CPM.cmake)

if(DEFINED ENV{GEODE_SDK} AND EXISTS "$ENV{GEODE_SDK}/CMakeLists.txt")
  message(STATUS "Using GEODE_SDK at $ENV{GEODE_SDK}")
  add_subdirectory($ENV{GEODE_SDK} ${CMAKE_BINARY_DIR}/geode)
else()
  CPMAddPackage("gh:geode-sdk/geode#v5.10.1")
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)
setup_geode_mod(${PROJECT_NAME})
