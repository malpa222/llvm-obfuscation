cmake_minimum_required(VERSION 3.24)
project(obfuscation-lib)

# Substitution
add_library(Substitution MODULE
        Substitution.cpp
        "${CMAKE_CURRENT_SOURCE_DIR}/../plugin/SubstitutionPlugin.cpp")

target_include_directories(Substitution PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../include")
