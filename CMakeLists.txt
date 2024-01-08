cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME interruptible_thread)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 14)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG /W4")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /W4 /Zi /EHsc")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto -Wall -Wextra -Werror")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall -Wextra -Werror -fsanitize=address")
endif()

set(INC_DIR ${PROJECT_SOURCE_DIR})
include_directories(${INC_DIR})

add_executable(${PROJECT_NAME} main.cpp)
