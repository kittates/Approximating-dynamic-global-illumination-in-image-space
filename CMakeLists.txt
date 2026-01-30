cmake_minimum_required(VERSION 3.10)

project(myOpenGLProject LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(app
    # main.cpp
    depth_testing.cpp
    glad.c
    shader.cpp
    stb_image.cpp
    
)

target_include_directories(app PRIVATE
    /usr/local/include
)

target_link_directories(app PRIVATE
    /usr/local/lib
)

target_link_libraries(app PRIVATE
    glfw3
    dl
    GL
    X11
    Xrandr
    pthread
    assimp
)

