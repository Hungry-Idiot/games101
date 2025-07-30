# CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)
project(PBF_Simulation VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_policy(SET CMP0072 NEW)

# --- ‼️ 核心修改部分 ---
# 1. 定义项目根目录变量 (不变)
set(PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR})
message(STATUS "Project root directory set to: ${PROJECT_ROOT_DIR}")

# 2. 使用 configure_file 生成 config.h
#    这会读取 config.h.in, 替换 @PROJECT_ROOT_DIR@, 
#    并把结果写入到构建目录(build/)下的 config.h 文件
configure_file(
    "${PROJECT_ROOT_DIR}/config.h.in"
    "${CMAKE_BINARY_DIR}/config.h"
)

# --- 查找依赖包 (不变) ---
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# --- 定义源文件 (不变) ---
set(SOURCES
    src/main.cpp
    src/pbf_solver.cpp
    lib/glad/src/glad.c
)

add_executable(pbf_sim ${SOURCES})

# --- ‼️ 核心修改部分 ---
# 3. 为我们的程序添加包含路径
target_include_directories(pbf_sim PRIVATE
    # 添加构建目录，这样我们就能 #include "config.h"
    ${CMAKE_BINARY_DIR} 
    # GLM 和 GLAD 的路径
    ${GLM_INCLUDE_DIRS}
    lib/glad/include
)

# --- 链接库到可执行文件 (不变) ---
target_link_libraries(pbf_sim PRIVATE
    ${OPENGL_LIBRARIES}
    glfw
    pthread
    dl
)