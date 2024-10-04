# CMakeList.txt : CMake project for xxEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(ENGINE_NAME "XX3D")

project (${ENGINE_NAME})


# c++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

# directory setup
## engine path
set(ENGINE_PATH ${${ENGINE_NAME}_SOURCE_DIR})
set(THIRD_PARTY ${ENGINE_PATH}/ThirdParty)
set(SOURCE_PATH ${ENGINE_PATH}/Source)
set(SHADER_PATH ${ENGINE_PATH}/Source/Shaders)
set(ENGINE_ASSETS ${ENGINE_PATH}/Assets) # inner assets
## project path
set(PROJECT_NAME "DemoProject")
set(PROJECT_PATH ${ENGINE_PATH}/${PROJECT_NAME}) # customize project path here
set(PROJECT_ASSETS ${PROJECT_PATH}/Assets)
set(PROJECT_CONFIG ${PROJECT_PATH}/Config)

add_definitions(
    -D_CRT_SECURE_NO_WARNINGS
    -DENGINE_NAME="${ENGINE_NAME}"
    -DSHADER_PATH="${SHADER_PATH}/"
    -DENGINE_ASSETS="${ENGINE_PATH}/Assets/"
    -DPROJECT_NAME="${PROJECT_NAME}"
    -DPROJECT_ASSETS="${PROJECT_ASSETS}/"
    -DPROJECT_CONFIG="${PROJECT_CONFIG}/"
)

macro (DEPENDENCY TARGET_NAME DEPENDENCY_NAME)
add_dependencies(${TARGET_NAME} ${DEPENDENCY_NAME})
target_link_libraries(${TARGET_NAME} PUBLIC ${DEPENDENCY_NAME})
endmacro(DEPENDENCY)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory("ThirdParty")
add_subdirectory("Source/Core")
add_subdirectory("Source/Engine")
add_subdirectory("Source/Editor")
add_subdirectory("Source/Runtime")
add_subdirectory("Source/ShaderCompiler")
add_subdirectory("Source/Shaders")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "Editor")
