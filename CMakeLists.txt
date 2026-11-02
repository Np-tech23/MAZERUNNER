cmake_minimum_required(VERSION 3.15)
project(MazeRunner CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra -O3)
endif()

# Prefix path for MSYS2 UCRT64 dependencies if present
if(EXISTS "C:/msys64/ucrt64")
    list(APPEND CMAKE_PREFIX_PATH "C:/msys64/ucrt64")
    include_directories("C:/msys64/ucrt64/include")
    link_directories("C:/msys64/ucrt64/lib")
endif()

include_directories(src)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG QUIET)
find_package(GLEW QUIET)
find_package(glm QUIET)

set(SOURCES
    src/main.cpp
    src/maze.cpp
    src/pathfinding.cpp
    src/player.cpp
    src/monster.cpp
    src/database.cpp
    src/graphics.cpp
    src/game.cpp
    src/tests.cpp
)

add_executable(mazerunner ${SOURCES})

# Link libraries
target_link_libraries(mazerunner
    PRIVATE
    OpenGL::GL
    glfw
    glew32
    sqlite3
    gdi32
    user32
    kernel32
)

set_target_properties(mazerunner PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
