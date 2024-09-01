# CMakeList.txt : CMake project for xxEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(TARGET_NAME "Runtime")

file(GLOB_RECURSE H_FILES "*.h")
file(GLOB_RECURSE CPP_FILES "*.cpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${H_FILES} ${CPP_FILES})

add_executable(${TARGET_NAME} ${H_FILES} ${CPP_FILES})
target_link_libraries(${TARGET_NAME} PRIVATE "Engine")
target_include_directories(${TARGET_NAME} PRIVATE ${SOURCE_PATH}/Runtime)

#imgui
target_link_libraries(${TARGET_NAME} PRIVATE "imgui")

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${THIRD_PARTY}/zlib/lib/zlib.dll
    $<TARGET_FILE_DIR:${TARGET_NAME}>)