cmake_minimum_required(VERSION 3.8)
project(EngineTest)

string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
message("Build type = ${CMAKE_BUILD_TYPE}")

set(CURRENT_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include_directories("common")
include_directories("D3D12Renderer/src/common")
include_directories("D3D12Renderer/src/Graphics")
include_directories("D3D12Renderer/src/Input")
include_directories("D3D12Renderer/src/Core")
include_directories("D3D12Renderer/src")

macro(make_filters _source_list)
    foreach(_source IN ITEMS ${_source_list})
        get_filename_component(_source_path "${_source}" PATH)
        string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_path}")
        string(REPLACE "/" "\\" _group_path "${_group_path}")
        source_group("${_group_path}" FILES "${_source}")
    endforeach()
endmacro()

add_subdirectory("D3D12Renderer")
add_subdirectory("EngineTest")
add_subdirectory("common")