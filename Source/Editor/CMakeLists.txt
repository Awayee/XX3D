# CMakeList.txt : CMake project for xxEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(TARGET_NAME "Editor")

file(GLOB_RECURSE H_FILES "*.h")
file(GLOB_RECURSE CPP_FILES "*.cpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${H_FILES} ${CPP_FILES})

add_executable(${TARGET_NAME} ${H_FILES} ${CPP_FILES})
target_link_libraries(${TARGET_NAME} PRIVATE "Engine")
target_include_directories(${TARGET_NAME} PRIVATE ${SOURCE_PATH}/Editor)

#stb image
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/stb_image)

#imgui
target_link_libraries(${TARGET_NAME} PRIVATE "imgui")

#tiny gltf
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/tinygltf)

#assimp
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/assimpWin64/include)
target_link_libraries(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/assimpWin64/lib/assimp-vc140-mt.lib)

#RenderDoc
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/RenderDoc)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${THIRD_PARTY}/assimpWin64/lib/assimp-vc140-mt.dll
    $<TARGET_FILE_DIR:${TARGET_NAME}>)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${THIRD_PARTY}/DirectXShaderCompiler/bin/x64/dxcompiler.dll
    $<TARGET_FILE_DIR:${TARGET_NAME}>)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${THIRD_PARTY}/zlib/lib/zlib.dll
    $<TARGET_FILE_DIR:${TARGET_NAME}>)
