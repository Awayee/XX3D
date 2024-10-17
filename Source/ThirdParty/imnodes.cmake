set(imnodes_SOURCE_DIR_ ${THIRD_PARTY}/imnodes)
file(GLOB imnodes_headers CONFIGURE_DEPENDS  "${imnodes_SOURCE_DIR_}/*.h")
file(GLOB imnodes_sources CONFIGURE_DEPENDS  "${imnodes_SOURCE_DIR_}/*.cpp")
add_library("imnodes" STATIC ${imnodes_headers} ${imnodes_sources})

target_include_directories("imnodes" PUBLIC $<BUILD_INTERFACE:${imnodes_SOURCE_DIR_}>)

# imgui
target_link_libraries("imnodes" PRIVATE "imgui")