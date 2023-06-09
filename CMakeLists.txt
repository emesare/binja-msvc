﻿cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

project(
    binja-msvc
    LANGUAGES CXX
)

add_subdirectory(binaryninja-api)
add_subdirectory(llvm-demangle)

# Add source to this project's shared library.
file(
    GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS # Automatically reconfigure if source files are added/removed.
    ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/include/*.h
)
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
    target_link_options(binaryninjaapi PUBLIC "LINKER:/FORCE:UNRESOLVED")
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
    target_link_options(binaryninjaapi PUBLIC "LINKER:--allow-shlib-undefined")
endif()

target_link_libraries(${PROJECT_NAME} binaryninjaapi)
target_link_libraries(${PROJECT_NAME} llvm-demangle)

target_include_directories(binja-msvc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_VISIBILITY_PRESET hidden
    CXX_STANDARD_REQUIRED ON
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON)