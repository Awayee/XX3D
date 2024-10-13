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
set(ENGINE_DIR ${${ENGINE_NAME}_SOURCE_DIR})
set(ENGINE_SOURCE_DIR ${ENGINE_DIR}/Source)
set(THIRD_PARTY ${ENGINE_SOURCE_DIR}/ThirdParty)
set(EXECUTABE_DIR ${ENGINE_DIR}/Binaries)

add_definitions(
    -D_CRT_SECURE_NO_WARNINGS
    -DENGINE_NAME="${ENGINE_NAME}"
)

macro (DEPENDENCY TARGET_NAME DEPENDENCY_NAME)
add_dependencies(${TARGET_NAME} ${DEPENDENCY_NAME})
target_link_libraries(${TARGET_NAME} PUBLIC ${DEPENDENCY_NAME})
endmacro(DEPENDENCY)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

# set compile output dir and work dir
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${EXECUTABE_DIR}) # lib output
Set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${EXECUTABE_DIR}) # debug exe output
Set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${EXECUTABE_DIR}) # release exe output
set(CMAKE_VS_DEBUGGER_WORKING_DIRECTORY ${EXECUTABE_DIR}) # executable dir

add_subdirectory("Source/Core")
add_subdirectory("Source/Engine")
add_subdirectory("Source/Editor")
add_subdirectory("Source/Runtime")
add_subdirectory("Source/ShaderCompiler")
add_subdirectory("Source/ThirdParty")
add_subdirectory("Shaders")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "Editor")
