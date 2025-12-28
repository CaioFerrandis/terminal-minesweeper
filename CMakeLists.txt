cmake_minimum_required(VERSION 3.20)

project(app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("${CMAKE_SOURCE_DIR}/external/enet")

target_include_directories(enet PUBLIC
    ${CMAKE_SOURCE_DIR}/external/enet/include
)

add_executable(app
src/main.cpp
)

target_link_libraries(app PUBLIC 
    enet
)

target_link_libraries(app PRIVATE enet)

if (MSVC)
    target_compile_options(app PRIVATE /W4)
else()
    target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic)
endif()
