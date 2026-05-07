cmake_minimum_required(VERSION 3.21)
project(tucil3_stima LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TUCIL3_PORTABLE_BUILD "Build portable binaries with static MSVC runtime" ON)

if(MSVC AND TUCIL3_PORTABLE_BUILD)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_executable(solver_cli
    src/main.cpp
    src/core/Map.cpp
    src/core/Solver.cpp
)
target_include_directories(solver_cli PRIVATE src/core)

set(_raylib_ready FALSE)
find_package(raylib QUIET)
if(raylib_FOUND)
    set(_raylib_ready TRUE)
endif()

if(NOT _raylib_ready)
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)

    # Build raylib from source so users don't need global raylib installation.
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
    )
    FetchContent_MakeAvailable(raylib)
    if(TARGET raylib)
        set(_raylib_ready TRUE)
    endif()
endif()

if(_raylib_ready)
    add_executable(solver_gui
        src/gui/main.cpp
        src/gui/MainWindow.cpp
        src/gui/BoardWidget.cpp
        src/core/Map.cpp
        src/core/Solver.cpp
    )
    target_include_directories(solver_gui PRIVATE src/core src/gui)
    target_link_libraries(solver_gui PRIVATE raylib)
    if(WIN32)
        target_link_libraries(solver_gui PRIVATE winmm)
    endif()
else()
    message(WARNING "Raylib tetap tidak tersedia. Target solver_gui tidak dibangun.")
endif()
