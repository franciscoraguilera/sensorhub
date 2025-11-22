cmake_minimum_required(VERSION 3.10)
project(advanced_multithread_sensorhub C)

# Set C standard to C11 for atomics support
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add pthread flag
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -Wall -Wextra")

# Source files
set(SOURCES
    src/main.c
    src/sensor.c
    src/data_processor.c
    src/queue.c
    src/network.c
    src/utils.c
    src/config.c
)

# Main executable
add_executable(sensorhub ${SOURCES})

# Link pthread library
target_link_libraries(sensorhub pthread)

# Enable testing
enable_testing()

# Test executables
add_executable(test_utils
    tests/test_utils.c
    src/utils.c
    src/queue.c
)
target_link_libraries(test_utils pthread)
add_test(NAME test_utils COMMAND test_utils)

add_executable(test_config
    tests/test_config.c
    src/config.c
)
add_test(NAME test_config COMMAND test_config)

add_executable(test_queue
    tests/test_queue.c
    src/queue.c
    src/utils.c
)
target_link_libraries(test_queue pthread)
add_test(NAME test_queue COMMAND test_queue)

# Custom target to run all tests
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_utils test_config test_queue
    COMMENT "Running all tests"
)
