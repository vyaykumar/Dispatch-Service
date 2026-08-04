cmake_minimum_required(VERSION 3.20)
project(dispatch_service_smoketest CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(smoke_test src/main.cpp)
add_executable(worker src/worker.cpp src/utility/work/workloads.cpp
        src/utility/random/random_utils.cpp
        src/utility/random/random_utils.h)
add_executable(client src/client.cpp
        src/utility/random/random_utils.cpp
        src/utility/random/random_utils.h)

target_link_libraries(worker PRIVATE ws2_32)
target_link_libraries(client PRIVATE ws2_32)

if(WIN32)
    target_link_libraries(smoke_test PRIVATE ws2_32)
endif()
