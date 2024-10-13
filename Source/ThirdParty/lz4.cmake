set(SOURCE_SIR_ ${THIRD_PARTY}/lz4)

file(GLOB sources_ CONFIGURE_DEPENDS
"${SOURCE_SIR_}/lz4.h" 
"${SOURCE_SIR_}/lz4.c"
"${SOURCE_SIR_}/lz4hc.h" 
"${SOURCE_SIR_}/lz4hc.c"
"${SOURCE_SIR_}/xxhash.c"
"${SOURCE_SIR_}/xxhash.c"
)
add_library("lz4" STATIC ${sources_})

target_include_directories(imgui PUBLIC $<BUILD_INTERFACE:${SOURCE_SIR_}>)