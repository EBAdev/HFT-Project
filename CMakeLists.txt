cmake_minimum_required(VERSION 3.20)
project(hft LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake to treat 'include/' as the header directory
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add your source files
add_executable(hft 
    src/main.cpp
    src/OrderBook.cpp
    src/Order.cpp
)

# To build the project, run the following commands in your terminal:
# cmake -S . -B build
# which says "run cmake with the source directory as '.' and the build directory as 'build'"
