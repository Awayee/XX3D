# CMakeList.txt : CMake project for xxEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(TARGET_NAME "Engine")

file(GLOB_RECURSE H_FILES "*.h")
file(GLOB_RECURSE CPP_FILES "*.cpp")
file(GLOB_RECURSE HPP_FILES "*.hpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${H_FILES} ${CPP_FILES} ${HPP_FILES})

add_library(${TARGET_NAME} STATIC ${H_FILES} ${CPP_FILES} ${HPP_FILES})
target_include_directories(${TARGET_NAME} PUBLIC ${SOURCE_PATH}/Engine)

#glfw
target_link_libraries(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/glfwWin64/lib-vc2019/glfw3.lib)
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/glfwWin64/include)
#glm
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/glm)

#vulkan
target_link_libraries(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/VulkanSDK/lib/Win64/vulkan-1.lib)
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/VulkanSDK/include)

#vma
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/VulkanMemoryAllocator/include)

#rapidjson
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/rapidjson/include)

#stb image
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/stb_image)

#imgui
target_link_libraries(${TARGET_NAME} PRIVATE "imgui")

#tiny gltf
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/tinygltf)

#assimp
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/assimpWin64/include)
target_link_libraries(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/assimpWin64/lib/assimp-vc140-mt.lib)