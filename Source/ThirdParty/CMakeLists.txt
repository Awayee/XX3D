# CMakeList.txt : CMake project for XX3D, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
set(THIRD_PARTY_NAME "ThirdParty")

if(NOT TARGET "imgui")
    include(imgui.cmake)
    set_target_properties("imgui" PROPERTIES FOLDER ${THIRD_PARTY_NAME})
endif()

if(NOT TARGET "imnodes")
    include(imnodes.cmake)
    set_target_properties("imnodes" PROPERTIES FOLDER ${THIRD_PARTY_NAME})
endif()

if(NOT TARGET "lz4")
    include(lz4.cmake)
    set_target_properties("lz4" PROPERTIES FOLDER ${THIRD_PARTY_NAME})
endif()
