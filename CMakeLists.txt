cmake_minimum_required(VERSION 3.20)
project(dispatch_service_smoketest CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(workerpool
        src/worker_cli.cpp

        src/utility/workload/workload.cpp
        src/utility/worker_pool/worker_pool.cpp
        src/utility/worker_pool/worker_interface/worker_interface.cpp
        src/utility/random/random_utils.cpp
        src/utility/scope_logger/scope_logger.cpp
)

add_executable(synchronous_client
        src/client_cli.cpp

        src/utility/client/client_interface.cpp
        src/utility/client/client_state_machine.cpp

        src/utility/workload/workload.cpp
        src/utility/random/random_utils.cpp

        src/utility/scope_logger/scope_logger.cpp
)

add_executable(logger_validation
        src/utility/scope_logger/main.cpp
        src/utility/scope_logger/scope_logger.cpp
)

add_executable(run_test
        src/main.cpp

        src/utility/execution/execution_engine.cpp

        src/utility/scenario/scenarios.cpp

        src/utility/client/client_interface.cpp
        src/utility/client/client_state_machine.cpp

        src/utility/workload/workload.cpp
        src/utility/random/random_utils.cpp

        src/utility/scope_logger/scope_logger.cpp
)

target_link_libraries(workerpool PRIVATE ws2_32)
target_link_libraries(synchronous_client PRIVATE ws2_32)
target_link_libraries(run_test PRIVATE ws2_32)

if(WIN32)
    target_link_libraries(run_test PRIVATE ws2_32)
endif()
