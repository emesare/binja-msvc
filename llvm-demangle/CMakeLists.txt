cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

project(llvm-demangle)

add_library(llvm-demangle STATIC
    lib/MicrosoftDemangle.cpp
    lib/MicrosoftDemangleNodes.cpp
)
set_property(TARGET llvm-demangle PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(llvm-demangle PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
