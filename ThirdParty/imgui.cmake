set(imgui_SOURCE_DIR_ ${THIRD_PARTY}/imgui)

file(GLOB imgui_headers CONFIGURE_DEPENDS  "${imgui_SOURCE_DIR_}/*.h")
file(GLOB imgui_sources CONFIGURE_DEPENDS  "${imgui_SOURCE_DIR_}/*.cpp")
file(GLOB imgui_impl CONFIGURE_DEPENDS
"${imgui_SOURCE_DIR_}/backends/imgui_impl_glfw.cpp" 
"${imgui_SOURCE_DIR_}/backends/imgui_impl_glfw.h"
"${imgui_SOURCE_DIR_}/backends/imgui_impl_vulkan.cpp" 
"${imgui_SOURCE_DIR_}/backends/imgui_impl_vulkan.h")
add_library("imgui" STATIC ${imgui_headers} ${imgui_sources} ${imgui_impl})

target_include_directories(imgui PUBLIC $<BUILD_INTERFACE:${imgui_SOURCE_DIR_}>)

#glfw
target_link_directories("imgui" PRIVATE ${THIRD_PARTY}/glfwWin64/lib-vc2019)
target_link_libraries("imgui" PRIVATE ${THIRD_PARTY}/glfwWin64/lib-vc2019/glfw3.lib)
target_include_directories("imgui" PRIVATE ${THIRD_PARTY}/glfwWin64/include)

#vulkan
target_link_directories("imgui" PRIVATE ${THIRD_PARTY}/VulkanSDK/lib/Win64)
target_link_libraries("imgui" PRIVATE ${THIRD_PARTY}/VulkanSDK/lib/Win64/vulkan-1.lib)
target_include_directories("imgui" PRIVATE ${THIRD_PARTY}/VulkanSDK/include)