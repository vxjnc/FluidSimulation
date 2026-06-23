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

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")
list(FILTER SOURCE_FILES EXCLUDE REGEX "src/scripting/.*")

set(WEBGPU_BACKEND "DAWN" CACHE STRING "WebGPU backend")
set(WGPU_LINK_TYPE "STATIC" CACHE STRING "Link webgpu statically" FORCE)
set_property(CACHE WEBGPU_BACKEND PROPERTY STRINGS "DAWN" "WGPU")
if(WEBGPU_BACKEND STREQUAL "DAWN")
    set(IMGUI_WEBGPU_BACKEND "IMGUI_IMPL_WEBGPU_BACKEND_DAWN")
elseif(WEBGPU_BACKEND STREQUAL "WGPU")
    set(IMGUI_WEBGPU_BACKEND "IMGUI_IMPL_WEBGPU_BACKEND_WGPU")
else()
    message(FATAL_ERROR "Unknown WEBGPU_BACKEND: ${WEBGPU_BACKEND}")
endif()

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

# --- webgpu_distribution ---
FetchContent_Declare(
    webgpu_distribution
    GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution.git
    GIT_TAG        v0.3.0-gamma
    GIT_SHALLOW    ON
)

# --- wesl-distribution ---
FetchContent_Declare(wesl_distribution
    GIT_REPOSITORY https://github.com/vxjnc/wesl-distribution
    GIT_TAG        v0.4.0
    GIT_SHALLOW    ON
)

# --- Dear ImGui ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.8-docking
    GIT_SHALLOW    ON
)

# --- glfw3webgpu ---
FetchContent_Declare(
    glfw3webgpu
    GIT_REPOSITORY https://github.com/eliemichel/glfw3webgpu.git
    GIT_TAG        fdcabcc54b56b50c12c10f5317abf8ae7ac32c29
    GIT_SHALLOW    ON
)

# --- clip ---
FetchContent_Declare(
    clip
    GIT_REPOSITORY https://github.com/dacap/clip.git
    GIT_TAG        v1.15
    GIT_SHALLOW    ON
)
set(CLIP_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CLIP_TESTS    OFF CACHE BOOL "" FORCE)

# --- stb ---
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    ON
)

# --- nativefiledialog-extended ---
FetchContent_Declare(
    nfd
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
    GIT_TAG        v1.3.0
    GIT_SHALLOW    ON
)
set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NFD_PORTAL ON CACHE BOOL "" FORCE)

# --- zpp-bits ---
FetchContent_Declare(
    zpp_bits
    GIT_REPOSITORY https://github.com/eyalz800/zpp_bits.git
    GIT_TAG        v4.7.1
    GIT_SHALLOW    ON
)

# --- sigslot ---
FetchContent_Declare(
    sigslot
    GIT_REPOSITORY https://github.com/palacaze/sigslot
    GIT_TAG        v1.2.3
    GIT_SHALLOW    ON
)

# --- pfr ---
FetchContent_Declare(
    pfr
    GIT_REPOSITORY https://github.com/apolukhin/pfr_non_boost.git
    GIT_TAG        2.3.2
    GIT_SHALLOW    ON
)

FetchContent_MakeAvailable(glfw webgpu_distribution imgui glfw3webgpu clip stb nfd zpp_bits sigslot pfr wesl_distribution)
set_target_properties(glfw webgpu PROPERTIES SYSTEM TRUE)
find_package(Python 3.10 COMPONENTS Interpreter Development.Embed QUIET)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_wgpu.cpp
)
target_include_directories(imgui SYSTEM PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_compile_definitions(imgui PUBLIC ${IMGUI_WEBGPU_BACKEND})
target_link_libraries(imgui PUBLIC glfw webgpu)

find_package(ZLIB REQUIRED)
target_link_libraries(clip ZLIB::ZLIB)

add_library(stb_headers INTERFACE)
target_include_directories(stb_headers SYSTEM INTERFACE ${stb_SOURCE_DIR})

add_library(zpp_bits_headers INTERFACE)
target_include_directories(zpp_bits_headers SYSTEM INTERFACE ${zpp_bits_SOURCE_DIR})

set(GENERATED_ROOT "${CMAKE_BINARY_DIR}/gen_includes")
set(GENERATED_DIR  "${GENERATED_ROOT}/generated")

include(shaders)
include(version)

# --- fluid_compute ---
add_library(fluid_compute INTERFACE)
target_include_directories(fluid_compute INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${GENERATED_ROOT}
)
target_link_libraries(fluid_compute INTERFACE webgpu wesl)
add_dependencies(fluid_compute wgsl_shaders)

# --- fluid_gui ---
add_library(fluid_gui INTERFACE)
target_include_directories(fluid_gui INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${GENERATED_ROOT}
)
target_link_libraries(fluid_gui INTERFACE webgpu wesl glfw imgui glfw3webgpu clip stb_headers nfd zpp_bits_headers Pal::Sigslot pfr)
add_dependencies(fluid_gui wgsl_shaders)

# --- Executable ---
add_executable(${PROJECT_NAME} main.cpp ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    fluid_compute
    fluid_gui
)

if(NOT MSVC AND NOT DEFINED ENV{CI})
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wconversion
        -march=native -mtune=native
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Configuring project ${PROJECT_NAME} for OPTIMIZATION mode (Release)")
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /O2)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    endif()
    set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
    target_link_options(${PROJECT_NAME} PRIVATE -s)

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
)


if(Python_FOUND)
    message(STATUS "Python ${Python_VERSION} found at ${Python_EXECUTABLE} - scripting enabled")
    set(PYTHON_API_CPP "${CMAKE_CURRENT_BINARY_DIR}/python_api.cpp")
    add_custom_command(
        OUTPUT "${PYTHON_API_CPP}"
        COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/src/scripting/generate_loader.py" "${CMAKE_CURRENT_SOURCE_DIR}/src/scripting/python_api.hpp" "${PYTHON_API_CPP}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/scripting/python_api.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/scripting/generate_loader.py"
        VERBATIM
    )

    target_sources(${PROJECT_NAME} PRIVATE
        src/scripting/engine.cpp
        src/scripting/bindings.cpp
        "${PYTHON_API_CPP}"
    )

    target_include_directories(${PROJECT_NAME} PRIVATE ${Python_INCLUDE_DIRS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE SCRIPTING_AVAILABLE)

    if(APPLE)
        target_link_options(${PROJECT_NAME} PRIVATE -undefined dynamic_lookup)
    elseif(UNIX)
    endif()
else()
    message(STATUS "Python not found - scripting disabled (engine.cpp stub only)")
    target_sources(${PROJECT_NAME} PRIVATE src/scripting/engine.cpp)
endif()

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
