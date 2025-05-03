cmake_minimum_required(VERSION 3.5)
project(thread-test)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fPIE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fPIE -Wno-deprecated-declarations")

# When locally compiling ONLY this project (e.g: if we want to build for macOS or Linux)
if(NOT DEFINED PROJECT_MAIN_SRC_DIR)
    set(PROJECT_MAIN_SRC_DIR ${CMAKE_SOURCE_DIR}/../../src)
    message(STATUS "Individual build, setting PROJECT_MAIN_SRC_DIR to ${PROJECT_MAIN_SRC_DIR}")
endif()

# Project sources
set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_SOURCE_DIR}/src/thread/resultThreadPool.cpp
)

# Compilation include directories
set(INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src
    ${PROJECT_MAIN_SRC_DIR}
)

# Executable target definition
add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBRARIES} gtest_main)
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})