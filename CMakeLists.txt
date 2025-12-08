cmake_minimum_required(VERSION 3.15)
project(cin)  
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------
# 1. Setup
# ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_language(OBJC)

if(APPLE)
    add_definitions(-Wno-deprecated-declarations)
endif()

find_package(OpenGL REQUIRED)

# ---------------------------------------------------------
# 2. Directories
# ---------------------------------------------------------
set(EXT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

include_directories(
    "${EXT_DIR}/glm-0.9.7.1"
    "${EXT_DIR}/tinyobjloader"
    "${EXT_DIR}/tinyobjloader/include"
    "${EXT_DIR}/tinyxml2"
    "${CMAKE_CURRENT_SOURCE_DIR}/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# ---------------------------------------------------------
# 3. Dependencies
# ---------------------------------------------------------

# GLFW 3.4
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${EXT_DIR}/glfw-3.4")

# GLM
include_directories("${EXT_DIR}/glm-0.9.7.1")

# GLEW 1.13.0
set(GLEW_INCLUDE_DIR "${EXT_DIR}/glew-1.13.0/include")

if(EXISTS "${EXT_DIR}/glew-1.13.0/src/glew.c")
    set(GLEW_SOURCE "${EXT_DIR}/glew-1.13.0/src/glew.c")
else()
    set(GLEW_SOURCE "${EXT_DIR}/glew-1.13.0/glew.c")
endif()

add_definitions(-DGLEW_STATIC)

# ---------------------------------------------------------
# 4. Executable
# ---------------------------------------------------------
add_executable(cin  
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/Terrain.cpp
    ${SRC_DIR}/Camera.cpp
    ${SRC_DIR}/Scene.cpp

    common/Shader.cpp
    common/Texture.cpp
    common/Model.cpp

    # ---------- NEW: Noise System ----------
    common/FastNoiseLite.cpp
    common/FastNoiseLite.h
    common/Noise.h

    ${GLEW_SOURCE}
)

# ---------------------------------------------------------
# 5. Includes
# ---------------------------------------------------------
target_include_directories(cin PUBLIC
    ${GLEW_INCLUDE_DIR}
    "${EXT_DIR}/glfw-3.4/include"
    "${EXT_DIR}/glm-0.9.7.1"
    "${EXT_DIR}/tinyobjloader"
    "${EXT_DIR}/tinyobjloader/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/common"
    ${SRC_DIR}
)

# ---------------------------------------------------------
# 6. Linking
# ---------------------------------------------------------
target_link_libraries(cin
    OpenGL::GL
    glfw
)

# ---------------------------------------------------------
# 7. Asset Path Definition
# ---------------------------------------------------------
target_compile_definitions(cin PRIVATE 
    ASSET_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
)
