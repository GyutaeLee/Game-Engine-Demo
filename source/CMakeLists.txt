cmake_minimum_required(VERSION 3.1.0)
project(GameEngineDemo VERSION 1.0)

# source code
set(GAME_ENGINE_FILES main.cpp)
source_group(source FILES ${GAME_ENGINE_FILES})

# GLAD
set(GLAD_FILES ../dependency/glad/include/glad/glad.h
				../dependency/glad/src/glad.c)
source_group(glad FILES ${GLAD_FILES})
include_directories(../dependency/glad/include)

# GLFW
include_directories(../dependency/glfw/include)

# GLM
include_directories(../dependency/glm/include)

# IMGUI
set(IMGUI_PATH ../dependency/imgui)
file(GLOB IMGUI_FILES ${IMGUI_PATH}/include/imgui/*.h
					  ${IMGUI_PATH}/src/*.cpp)
source_group(imgui FILES ${IMGUI_FILES})
include_directories(${IMGUI_PATH}/include)

# Make executable file
add_executable(GameEngineDemo
				${GAME_ENGINE_FILES}
				${GLAD_FILES}
				${IMGUI_FILES})

# Link glfw by finding the full path of it
find_library(GLFW_LIB NAMES glfw3 PATHS
${CMAKE_CURRENT_SOURCE_DIR}/../dependency/glfw/lib)
target_link_libraries(GameEngineDemo ${GLFW_LIB})