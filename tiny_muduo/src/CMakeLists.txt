cmake_minimum_required (VERSION 3.8)

add_definitions(-std=c++20)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

project(tiny_muduo)

add_subdirectory(tiny_muduo)
add_subdirectory(examples/echoserver)
add_subdirectory(examples/protobuf/codec)
add_subdirectory(examples/protobuf/rpc)
add_subdirectory(tests)
