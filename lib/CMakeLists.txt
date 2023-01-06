cmake_minimum_required(VERSION 3.24)

set(LLVM_PASSES
        Substitution
        Flattening)

foreach(pass ${LLVM_PASSES})
    message("Building ${pass}")
    add_library(${pass} MODULE
            ${pass}.cpp
            "${CMAKE_CURRENT_SOURCE_DIR}/../plugin/${pass}Plugin.cpp")

    target_include_directories(${pass} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/../include")
endforeach()
