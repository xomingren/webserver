add_custom_command(OUTPUT rpc_example.pb.cc rpc_example.pb.h
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/rpc_example.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS rpc_example.proto)

set_source_files_properties(rpc_example.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(rpc_example rpc_example.pb.cc)
target_link_libraries(rpc_example protobuf pthread)

add_executable(rpcclient rpcclient.cc)
set_target_properties(rpcclient PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(rpcclient rpc_example tiny_muduo_protorpc)

add_executable(rpcserver rpcserver.cc)
set_target_properties(rpcserver PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(rpcserver rpc_example tiny_muduo_protorpc)
