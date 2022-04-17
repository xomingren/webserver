add_executable(echoclient echoclient.cc)
target_link_libraries(echoclient tiny_muduo_main)

add_executable(echoserver echoserver.cc)
target_link_libraries(echoserver tiny_muduo_main)