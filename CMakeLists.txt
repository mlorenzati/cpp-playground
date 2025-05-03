cmake_minimum_required(VERSION 3.5)
project(cpp-playground VERSION "0.1.0" LANGUAGES C CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fPIE ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fPIE -Wno-deprecated-declarations")
set(PROJECT_MAIN_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(PROJECT_MAIN_TEST_DIR ${CMAKE_SOURCE_DIR}/test)

add_subdirectory(test)