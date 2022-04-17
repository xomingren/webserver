cmake_minimum_required (VERSION 3.8)

aux_source_directory(. ALL_SRCS)

add_library(tiny_muduo_main ${ALL_SRCS})

#install(TARGETS tiny_muduo_main DESTINATION lib)

add_subdirectory(protorpc)

#if (CMAKE_VERSION VERSION_GREATER 3.12) 
#set_property(TARGET tiny_muduo_main PROPERTY CXX_STANDARD 20) 
#endif()

