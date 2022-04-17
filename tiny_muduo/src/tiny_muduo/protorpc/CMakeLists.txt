cmake_minimum_required (VERSION 3.8)
 
find_package(Protobuf 3 REQUIRED)
if(PROTOBUF_FOUND)
  message(STATUS "found protobuf")
endif()

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
  message(STATUS "found zlib")
endif()

#[[ 1.copy back to .proto file dir
#find all .proto file in current dir
file(GLOB protobuf_files
    *.proto
    )
    
#default output dir  
SET(PROTO_META_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR})
#use in .proto file, link to it's own import .proto file
LIST(APPEND PROTO_FLAGS -I${CMAKE_CURRENT_SOURCE_DIR})

FOREACH(FIL ${protobuf_files})
 
  GET_FILENAME_COMPONENT(FIL_WE ${FIL} NAME_WE)
 
  string(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FIL})
  string(REGEX REPLACE "(.+)\\${FILE_NAME}.*" "\\1" FILE_PATH ${FIL})
 
  #FILE_PATH : default output dir 
  string(REGEX MATCH "(/)" OUT_PATH ${FILE_PATH})
 
  set(PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}${OUT_PATH}${FIL_WE}.pb.cc")
  set(PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}${OUT_PATH}${FIL_WE}.pb.h")
 
  EXECUTE_PROCESS(
      COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} ${PROTO_FLAGS} --cpp_out=${PROTO_META_BASE_DIR} ${FIL}
  )
  message("Copying " ${PROTO_SRCS} " to " ${FILE_PATH})
  #copy back to .proto file dir
  file(COPY ${PROTO_SRCS} DESTINATION ${FILE_PATH})
  file(COPY ${PROTO_HDRS} DESTINATION ${FILE_PATH})
 
ENDFOREACH()
]]
#2. no copy back, include binary dir instead
add_custom_command(OUTPUT rpc.pb.cc rpc.pb.h
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS rpc.proto)

set_source_files_properties(rpc.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(protorpc_wire rpc.pb.cc rpccodec.cc)
set_target_properties(protorpc_wire PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")

add_library(protobufrpc_codec protobufcodecrpc_class.cc)
set_target_properties(protobufrpc_codec PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(protobufrpc_codec tiny_muduo_main protobuf z)

add_library(tiny_muduo_protorpc rpcchannel_class.cc rpcserver_class.cc)
set_target_properties(tiny_muduo_protorpc PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(tiny_muduo_protorpc protorpc_wire protobufrpc_codec tiny_muduo_main protobuf z)

#install header in usr/include so user can include it
#install(TARGETS protobuf_codec protorpc_wire tiny_muduo_protorpc DESTINATION lib)

#set(HEADERS
#  protobufcodecrpc_class.h
#  rpcchannel_class.h
#  rpcserver_class.h
#  rpc.proto
#  rpcservice.proto
#  ${PROJECT_BINARY_DIR}/tiny_muduo/protorpc/rpc.pb.h
#  )
#install(FILES ${HEADERS} DESTINATION include/tiny_muduo/protobuf)

#if (CMAKE_VERSION VERSION_GREATER 3.12) 
#set_property(TARGET tiny_muduo_protorpc PROPERTY CXX_STANDARD 20) 
#endif()

