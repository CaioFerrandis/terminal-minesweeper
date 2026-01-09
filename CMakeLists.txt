cmake_minimum_required(VERSION 3.20)

project(app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ENET_SHARED OFF CACHE BOOL "" FORCE)
set(ENET_STATIC ON CACHE BOOL "" FORCE)

add_subdirectory("external/enet")

target_include_directories(enet PUBLIC
    ${CMAKE_SOURCE_DIR}/external/enet/include
)

add_executable(app
src/main.cpp
)

target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/src)

find_package(Threads REQUIRED)

target_link_libraries(app PRIVATE enet Threads::Threads)

if (MSVC)
    target_compile_options(app PRIVATE /W4)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic)
endif()
