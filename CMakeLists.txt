cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

project(
    binja-msvc
    LANGUAGES CXX
)

find_path(
        BN_API_PATH
        NAMES binaryninjaapi.h
        # List of paths to search for the clone of the api
        HINTS ../.. binaryninjaapi binaryninja-api $ENV{BN_API_PATH}
        REQUIRED
)
add_subdirectory(${BN_API_PATH} binaryninjaapi)

# Add source to this project's shared library.
file(
    GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS # Automatically reconfigure if source files are added/removed.
    ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/include/*.h
)
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} binaryninjaapi)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_VISIBILITY_PRESET hidden
    CXX_STANDARD_REQUIRED ON
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON)