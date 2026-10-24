cmake_minimum_required(VERSION 3.20)
project(hft LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include(CTest)
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(googletest)


# Tell CMake to treat 'include/' as the header directory
include_directories(${PROJECT_SOURCE_DIR}/include)

# Shared production code
add_library(hft_core
    src/Order.cpp
)

target_include_directories(hft_core
    PUBLIC include
)

# Main application
add_executable(hft_project
    src/main.cpp
)

# Link the shared library to the main application
target_link_libraries(hft_project
    PRIVATE hft_core
)

# Unit tests
add_executable(test_order
    tests/test_order.cpp
)

# Link the shared library and Google Test to the test executable
target_link_libraries(test_order
    PRIVATE hft_core GTest::gtest_main
)

# Enable testing
include(GoogleTest)
gtest_discover_tests(test_order)

# To build the project, run the following commands in your terminal:
# cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
# which says "run cmake with the source directory as '.' and the build directory as 'build'" 
# and "export the compile commands to a file called 'compile_commands.json' in the build directory"
#
# Then, to build the project, run:
# cmake --build build
# To run the tests, you can use the following command:
# ctest --test-dir build --output-on-failure
