
cmake_minimum_required(VERSION 3.10)
project(xLog VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)
include_directories(${SQLite3_INCLUDE_DIRS})

add_executable(xlog src/main.cpp)
target_link_libraries(xlog PRIVATE ${SQLite3_LIBRARIES})
