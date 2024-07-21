# CMakeList.txt : CMake project for xxEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(TARGET_NAME "Core")

#add source files
file(GLOB_RECURSE H_FILES "*.h")
file(GLOB_RECURSE CPP_FILES "*.cpp")
file(GLOB_RECURSE HPP_FILES "*.hpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${H_FILES} ${CPP_FILES} ${HPP_FILES})
add_library(${TARGET_NAME} STATIC ${H_FILES} ${CPP_FILES} ${HPP_FILES})
target_include_directories(${TARGET_NAME} PUBLIC ${SOURCE_PATH}/Core)

#glm
target_include_directories(${TARGET_NAME} PUBLIC ${THIRD_PARTY}/glm)

#rapidjson
target_include_directories(${TARGET_NAME} PUBLIC ${THIRD_PARTY}/rapidjson/include)
