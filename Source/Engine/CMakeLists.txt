# CMakeList.txt : CMake project for xxEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(TARGET_NAME "Engine")

#add source files
file(GLOB_RECURSE H_FILES "*.h")
file(GLOB_RECURSE CPP_FILES "*.cpp")
file(GLOB_RECURSE HPP_FILES "*.hpp")
file(GLOB_RECURSE INL_FILES "*.inl")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${H_FILES} ${CPP_FILES} ${HPP_FILES} ${INL_FILES})
add_library(${TARGET_NAME} STATIC ${H_FILES} ${CPP_FILES} ${HPP_FILES} ${INL_FILES})

target_link_libraries(${TARGET_NAME} PUBLIC "Core")
target_link_libraries(${TARGET_NAME} PUBLIC "ShaderCompiler")
target_include_directories(${TARGET_NAME} PUBLIC ${SOURCE_PATH}/Engine)

# #add include files
# file(GLOB PUBLIC_DIRECTORY PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")
# foreach(folder ${PUBLIC_DIRECTORY})
#     target_include_directories(${TARGET_NAME} PRIVATE "${folder}/Public")
# endforeach()

#dx
target_link_libraries(${TARGET_NAME} PRIVATE
d3d12.lib
dxgi.lib
dxguid.lib
winmm.lib
)
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/DirectX/include)


#glfw
target_link_libraries(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/glfwWin64/lib-vc2019/glfw3.lib)
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/glfwWin64/include)

#vulkan
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/Vulkan/include)

#vma
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/VulkanMemoryAllocator/include)

#stb image
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/stb_image)

#imgui
target_link_libraries(${TARGET_NAME} PRIVATE "imgui")

#tiny gltf
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/tinygltf)

#assimp
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/assimpWin64/include)
target_link_libraries(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/assimpWin64/lib/assimp-vc140-mt.lib)

#lz4
target_link_libraries(${TARGET_NAME} PRIVATE "lz4")

#zlib
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/zlib/include)
target_link_libraries(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/zlib/lib/zlib.lib)
target_link_libraries(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/zlib/lib/zlibstatic.lib)

#RenderDoc
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/RenderDoc)

#WinPixEventRuntime
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/WinPixEventRuntime/Include)
target_link_libraries(${TARGET_NAME} PRIVATE ${THIRD_PARTY}/WinPixEventRuntime/bin/x64/WinPixEventRuntime.lib)
