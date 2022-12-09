cmake_minimum_required(VERSION 3.24)
project(obfuscation)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-fno-rtti")

find_package(LLVM REQUIRED CONFIG)

add_subdirectory(lib)