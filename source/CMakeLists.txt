cmake_minimum_required(VERSION 3.1.0)
project(GameEngineDemo VERSION 1.0)

# Set the output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

# source code
set(GAME_ENGINE_FILES main.cpp
					  utility.h
					  utility.cpp)
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

# ASSIMP
set(ASSIMP_PATH ../dependency/assimp)
file(GLOB_RECURSE ASSIMP_HEADERS ${ASSIMP_PATH}/include/assimp/*.h
								 ${ASSIMP_PATH}/include/assimp/*.hpp
								 ${ASSIMP_PATH}/include/assimp/*.inl)
source_group(lib\\assimp FILES ${ASSIMP_HEADERS})
include_directories(${ASSIMP_PATH}/include)

# STB
set(STB_PATH ../dependency/stb)
file(GLOB_RECURSE STB_FILES ${STB_PATH}/include/stb/*.h)
source_group(lib\\stb FILES ${STB_FILES})
include_directories(${STB_PATH}/include)

# Shader
set(SHADER_FILES model_shader.vert
				 model_shader.frag)
source_group(shader FILES ${SHADER_FILES})

# Make executable file
add_executable(GameEngineDemo
				${GAME_ENGINE_FILES}
				${GLAD_FILES}
				${IMGUI_FILES}
				${ASSIMP_HEADERS}
				${STB_FILES}
				${SHADER_FILES})

# Link glfw by finding the full path of it
find_library(GLFW_LIB NAMES glfw3 PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../dependency/glfw/lib)
target_link_libraries(GameEngineDemo ${GLFW_LIB})

find_library(ASSIMP_LIB NAMES assimp-vc142-mtd PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../../dependency/assimp/lib)
target_link_libraries(GameEngineDemo ${ASSIMP_LIB})

if(MSVC)

set(INSTALL_ADDITIONAL_PATH "$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>")

# https://stackoverflow.com/questions/23950887/does-cmake-offer-a-method-to-set-the-working-directory-for-a-given-build-system
set_property(TARGET GameEngineDemo PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${INSTALL_ADDITIONAL_PATH}")

else()

set(INSTALL_ADDITIONAL_PATH "")

endif()

# move the dlls into the executable location
list(APPEND DEPENDENCY_DLLS ../../dependency/assimp/lib/assimp-vc142-mtd.dll)
install(FILES ${DEPENDENCY_DLLS} DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${INSTALL_ADDITIONAL_PATH}")

# move resource files into the executable location
install(DIRECTORY ../../resource DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${INSTALL_ADDITIONAL_PATH}")

# move shaders
install(FILES ${SHADER_FILES} DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${INSTALL_ADDITIONAL_PATH}")