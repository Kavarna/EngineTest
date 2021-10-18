cmake_minimum_required(VERSION 3.8)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(EngineTest)

FILE(GLOB SOURCES "src/*.cpp" "src/*.h")

add_executable(EngineTest ${SOURCES})

make_filters("${SOURCES}")


target_link_libraries(EngineTest PUBLIC D3D12Renderer)

set_property(TARGET EngineTest PROPERTY CXX_STANDARD 17)
set_property(TARGET EngineTest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CURRENT_WORKING_DIRECTORY}")

set(CMAKE_INSTALL_PREFIX ../bin)

