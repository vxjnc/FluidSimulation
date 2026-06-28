cmake_minimum_required(VERSION 3.28)

project(FluidSimulation C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(BUILD_BENCHMARKS "Build performance benchmarks" OFF)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER   ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

set(WEBGPU_BACKEND "DAWN" CACHE STRING "WebGPU backend")
set_property(CACHE WEBGPU_BACKEND PROPERTY STRINGS "DAWN" "WGPU")
if(WEBGPU_BACKEND STREQUAL "DAWN")
    set(IMGUI_WEBGPU_BACKEND "IMGUI_IMPL_WEBGPU_BACKEND_DAWN")
elseif(WEBGPU_BACKEND STREQUAL "WGPU")
    set(IMGUI_WEBGPU_BACKEND "IMGUI_IMPL_WEBGPU_BACKEND_WGPU")
else()
    message(FATAL_ERROR "Unknown WEBGPU_BACKEND: ${WEBGPU_BACKEND}")
endif()

set(GENERATED_ROOT "${CMAKE_BINARY_DIR}/gen_includes")
set(GENERATED_DIR  "${GENERATED_ROOT}/generated")

include(dependencies)
include(shaders)
include(version)
include(embedded_resources)

# --- fluid_compute ---
file(GLOB_RECURSE COMPUTE_FILES "src/compute/*.cpp")

add_library(fluid_compute STATIC ${COMPUTE_FILES})
target_include_directories(fluid_compute PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${GENERATED_ROOT}
)
target_link_libraries(fluid_compute PUBLIC webgpu wesl)
add_dependencies(fluid_compute wgsl_shaders)

# --- fluid_gui ---
file(GLOB_RECURSE GUI_FILES "src/*.cpp")
list(FILTER GUI_FILES EXCLUDE REGEX "src/compute/.*")
list(FILTER GUI_FILES EXCLUDE REGEX "src/scripting/.*")
list(APPEND GUI_FILES "${CMAKE_SOURCE_DIR}/src/scripting/scripting_loader.cpp")
add_library(fluid_gui STATIC 
    ${GUI_FILES}
    ${imguicolortextedit_SOURCE_DIR}/TextEditor.cpp
)
target_include_directories(fluid_gui
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${GENERATED_ROOT}
    SYSTEM PUBLIC
        ${imguicolortextedit_SOURCE_DIR}
)
target_link_libraries(fluid_gui PUBLIC fluid_compute glfw imgui glfw3webgpu clip stb_headers nfd zpp_bits_headers Pal::Sigslot pfr)
add_dependencies(fluid_gui wgsl_shaders embedded_resources)

# --- Executable ---
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE fluid_gui)
target_copy_webgpu_binaries(${PROJECT_NAME})

if(NOT MSVC AND NOT DEFINED ENV{CI})
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wconversion
        -march=native -mtune=native
        -Wmismatched-tags
    )
    target_compile_options(fluid_compute PRIVATE -Wmismatched-tags)
    target_compile_options(fluid_gui PRIVATE -Wmismatched-tags)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Configuring project ${PROJECT_NAME} for OPTIMIZATION mode (Release)")
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /O2)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    endif()
    set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        STRIP_SYMBOLS ON
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring project ${PROJECT_NAME} for PROFILING/DEBUG mode (Debug)")
    target_compile_options(${PROJECT_NAME} PRIVATE -g -fno-omit-frame-pointer)

else()
    message(STATUS "Configuring project ${PROJECT_NAME} for default mode: ${CMAKE_BUILD_TYPE}")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}"
)

include(scripting)

# --- Google Benchmark ---
if(BUILD_BENCHMARKS)
    FetchContent_Declare(
        google_benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.5
        GIT_SHALLOW    ON
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(google_benchmark)

    add_subdirectory(benchmarks)
endif()

if(NOT "${CMAKE_BINARY_DIR}/compile_commands.json" STREQUAL "${CMAKE_SOURCE_DIR}/build/compile_commands.json")
    file(CREATE_LINK
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/build/compile_commands.json"
        COPY_ON_ERROR SYMBOLIC
    )
endif()
