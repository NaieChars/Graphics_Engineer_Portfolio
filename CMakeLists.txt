cmake_minimum_required(VERSION 3.20)
project(OpenGL_Demos LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========================
# 第三方库
# ========================

# glfw（官方 CMake）
add_subdirectory(third_party/glfw)

# glad（手动建库）
add_library(glad
    third_party/glad/src/glad.c
)

# assimp
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)


target_include_directories(glad
    PUBLIC
        ${PROJECT_SOURCE_DIR}/third_party/glad/include
)

# glm / stb（纯头文件库）
# 统一把 third_party 加进 include 路径
include_directories(
    ${PROJECT_SOURCE_DIR}/third_party
)

# ========================
# demos
# ========================
add_subdirectory(demos/01_OpenGL_Triangle)
add_subdirectory(demos/02_OpenGL_Texture)
add_subdirectory(demos/03_OpenGL_3D_Space)
add_subdirectory(demos/04_OpenGL_Light)
add_subdirectory(demos/05_OpenGL_Advanced_Light)
add_subdirectory(demos/07_OpenGL_Advanced_GL)
add_subdirectory(demos/08_OpenGL_Advanced_Rendering)
add_subdirectory(demos/09_OpenGL_Advanced_Lighting2)
add_subdirectory(demos/10_OpenGL_PBR)