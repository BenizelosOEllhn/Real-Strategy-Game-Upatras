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
set(EXT_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/external")
set(SRC_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(CORE_DIR "${SRC_DIR}/core")
set(GAME_DIR "${SRC_DIR}/game")
set(GUI_DIR  "${SRC_DIR}/gui")
set(RENDER_DIR "${SRC_DIR}/rendering")
set(TERRAIN_DIR "${RENDER_DIR}/terrain")
set(RAYCAST_DIR "${SRC_DIR}/raycast")
set(COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common")
set(TINYOBJ_DIR "${EXT_DIR}/tinyobjloader/include")

# ---------------------------------------------------------
# 3. Dependencies
# ---------------------------------------------------------

# GLFW 3.4
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${EXT_DIR}/glfw-3.4")

# GLM (header-only)
include_directories("${EXT_DIR}/glm-0.9.7.1")

# GLEW (static)
set(GLEW_INCLUDE_DIR "${EXT_DIR}/glew-1.13.0/include")

if(EXISTS "${EXT_DIR}/glew-1.13.0/src/glew.c")
    set(GLEW_SOURCE "${EXT_DIR}/glew-1.13.0/src/glew.c")
else()
    set(GLEW_SOURCE "${EXT_DIR}/glew-1.13.0/glew.c")
endif()

add_definitions(-DGLEW_STATIC)

# ---------------------------------------------------------
# 4. Game source files
# ---------------------------------------------------------
set(GAME_SOURCES
    # ---- entities ----
    ${GAME_DIR}/entities/GameEntity.cpp
    ${GAME_DIR}/entities/Unit.cpp
    ${GAME_DIR}/entities/Building.cpp

    # ---- units ----
    ${GAME_DIR}/units/Worker.cpp
    ${GAME_DIR}/units/Archer.cpp
    ${GAME_DIR}/units/Knight.cpp

    # ---- buildings ----
    ${GAME_DIR}/buildings/TownCenter.cpp
    ${GAME_DIR}/buildings/Barracks.cpp
    ${GAME_DIR}/buildings/Farm.cpp
    ${GAME_DIR}/buildings/House.cpp
    ${GAME_DIR}/buildings/Market.cpp
    ${GAME_DIR}/buildings/Storage.cpp

    # ---- managers ----
    ${GAME_DIR}/managers/BuildingManager.cpp

)

# ---------------------------------------------------------
# 5. Core + Engine sources
# ---------------------------------------------------------
set(CORE_SOURCES
    ${SRC_DIR}/main.cpp

    # core
    ${CORE_DIR}/Scene_Depth.cpp
    ${CORE_DIR}/Scene_Events.cpp
    ${CORE_DIR}/Scene_Renderer.cpp
    ${CORE_DIR}/Scene_Init.cpp
    ${CORE_DIR}/Scene_Water.cpp
    ${CORE_DIR}/Scene_Procedural.cpp
    ${CORE_DIR}/Camera.cpp

    # gui
    ${GUI_DIR}/UIManager.cpp
    ${GUI_DIR}/UIButton.cpp

    # rendering
    ${TERRAIN_DIR}/Terrain.cpp

    # raycast
    ${RAYCAST_DIR}/Raycaster.cpp

    # common
    ${COMMON_DIR}/Shader.cpp
    ${COMMON_DIR}/Texture.cpp
    ${COMMON_DIR}/Model.cpp
    ${COMMON_DIR}/FastNoiseLite.cpp

    # glew
    ${GLEW_SOURCE}
)

# ---------------------------------------------------------
# 6. Executable
# ---------------------------------------------------------
add_executable(cin
    ${CORE_SOURCES}
    ${GAME_SOURCES}
)

# ---------------------------------------------------------
# 7. Includes
# ---------------------------------------------------------
target_include_directories(cin PUBLIC
    ${GLEW_INCLUDE_DIR}
    "${EXT_DIR}/glfw-3.4/include"
    "${EXT_DIR}/glm-0.9.7.1"

    ${SRC_DIR}
    ${CORE_DIR}
    ${GAME_DIR}
    ${GAME_DIR}/entities
    ${GAME_DIR}/units
    ${GAME_DIR}/buildings
    ${GAME_DIR}/managers
    ${GAME_DIR}/data

    ${GUI_DIR}
    ${RENDER_DIR}
    ${TERRAIN_DIR}
    ${RAYCAST_DIR}
    ${COMMON_DIR}
    ${TINYOBJ_DIR}
)

# ---------------------------------------------------------
# 8. Linking
# ---------------------------------------------------------
target_link_libraries(cin
    OpenGL::GL
    glfw
)

# ---------------------------------------------------------
# 9. Asset Path Definition
# ---------------------------------------------------------
target_compile_definitions(cin PRIVATE
    ASSET_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
)
