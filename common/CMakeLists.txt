cmake_minimum_required(VERSION 3.8)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(Common)

FILE(GLOB SOURCES "*.cpp" "*.h")

add_library(Common ${SOURCES})

make_filters("${SOURCES}")


target_link_libraries(Common PUBLIC D3D12Renderer)

set_property(TARGET Common PROPERTY CXX_STANDARD 17)
set_property(TARGET Common PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CURRENT_WORKING_DIRECTORY}")

set(CMAKE_INSTALL_PREFIX ../bin)

