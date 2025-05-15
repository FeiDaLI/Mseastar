cmake_minimum_required(VERSION 3.10)
project(FuturePromiseTest)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find GTest package
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Add test executable
add_executable(future_promise_test
    test/future_promise_test.cc
)

# Link against GTest and pthread
target_link_libraries(future_promise_test
    GTest::GTest
    GTest::Main
    Threads::Threads
)

# Include directories
target_include_directories(future_promise_test PRIVATE
    ${CMAKE_SOURCE_DIR}
) 