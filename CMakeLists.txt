cmake_minimum_required(VERSION 3.28)

project(FluidSimulation CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER   ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

include(FetchContent)

# --- GLFW ---
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    ON
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)
set(GLFW_EXPOSE_NATIVE_WAYLAND OFF CACHE BOOL "" FORCE)

# --- WebGPU ---
FetchContent_Declare(
    webgpu_distribution
    GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution.git
    GIT_TAG        v0.3.0-gamma
    GIT_SHALLOW    ON
)
set(WEBGPU_BACKEND "WGPU" CACHE STRING "WebGPU backend" FORCE)

FetchContent_Declare(
    webgpu_cpp
    GIT_REPOSITORY https://github.com/eliemichel/WebGPU-Cpp.git
    GIT_TAG        wgpu-v24.0.3.1
    GIT_SHALLOW    ON
)

FetchContent_MakeAvailable(glfw webgpu_distribution webgpu_cpp)

add_executable(${PROJECT_NAME} main.cpp ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    webgpu
)

target_copy_webgpu_binaries(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${webgpu_cpp_SOURCE_DIR}
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wold-style-cast
    -Wconversion
    -march=native
    -mtune=native
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Configuring project ${PROJECT_NAME} for OPTIMIZATION mode (Release)")
    target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
    target_link_options(${PROJECT_NAME} PRIVATE -s)

elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring project ${PROJECT_NAME} for PROFILING/DEBUG mode (Debug)")
    target_compile_options(${PROJECT_NAME} PRIVATE -g -fno-omit-frame-pointer)

else()
    message(STATUS "Configuring project ${PROJECT_NAME} for default mode: ${CMAKE_BUILD_TYPE}")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
