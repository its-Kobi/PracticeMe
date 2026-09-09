cmake_minimum_required(VERSION 3.25)
project(KRecorder VERSION 1.0.0)
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
add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/Settings.cpp
    src/RecordingManager.cpp
    src/Overlay.cpp
    src/Hotkey.cpp
    src/capture/DesktopDuplication.cpp
    src/encoder/WMFEncoder.cpp
    src/encoder/EncoderFactory.cpp
    src/util/FileUtils.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE include src)
setup_geode_mod(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE mf mfplat mfuuid wmcodecdspuuid strmiids d3d11 dxgi)
