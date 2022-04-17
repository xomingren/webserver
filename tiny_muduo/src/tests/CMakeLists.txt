add_executable(queue_bench queue_bench.cc)
target_link_libraries(queue_bench tiny_muduo_main)

add_executable(threadpool_test threadpool_test.cc)
target_link_libraries(threadpool_test tiny_muduo_main)
