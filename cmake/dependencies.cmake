if(DEFINED ENV{CPM_SOURCE_CACHE})
    set(_cpm_source_cache "$ENV{CPM_SOURCE_CACHE}")
else()
    set(_cpm_source_cache "${CMAKE_SOURCE_DIR}/.cache/cpm")
    set(CPM_SOURCE_CACHE "${_cpm_source_cache}" CACHE PATH "CPM source cache")
endif()

set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/build/_deps-shared" CACHE PATH "Shared FetchContent downloads")

set(CPM_DOWNLOAD_VERSION 0.43.1)
set(CPM_DOWNLOAD_LOCATION "${_cpm_source_cache}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT EXISTS ${CPM_DOWNLOAD_LOCATION})
    message(STATUS "Downloading CPM.cmake v${CPM_DOWNLOAD_VERSION}")
    file(DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
        ${CPM_DOWNLOAD_LOCATION}
        STATUS download_status
    )
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        list(GET download_status 1 status_message)
        file(REMOVE ${CPM_DOWNLOAD_LOCATION})
        message(FATAL_ERROR "Failed to download CPM.cmake: ${status_message}")
    endif()
endif()
include(${CPM_DOWNLOAD_LOCATION})

# --- GLFW ---
CPMAddPackage(
    NAME glfw
    URL  https://github.com/glfw/glfw/archive/refs/tags/3.4.zip
    OPTIONS
        "GLFW_BUILD_EXAMPLES OFF"
        "GLFW_BUILD_TESTS OFF"
        "GLFW_BUILD_DOCS OFF"
        "GLFW_INSTALL OFF"
        "GLFW_BUILD_WAYLAND OFF"
        "GLFW_EXPOSE_NATIVE_WAYLAND OFF"
)

# --- webgpu_distribution ---
CPMAddPackage(
    NAME   webgpu_distribution
    URL    https://github.com/eliemichel/WebGPU-distribution/archive/refs/tags/v0.3.0-gamma.zip
    SYSTEM TRUE
)

# --- wesl-distribution ---
CPMAddPackage(
    NAME wesl_distribution
    URL  https://github.com/vxjnc/wesl-distribution/archive/refs/tags/v0.4.0.zip
)

# --- Dear ImGui ---
CPMAddPackage(
    NAME imgui
    URL  https://github.com/ocornut/imgui/archive/refs/tags/v1.92.8-docking.zip
    DOWNLOAD_ONLY  YES
)

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

# --- glfw3webgpu ---
CPMAddPackage(
    NAME glfw3webgpu
    URL  https://github.com/eliemichel/glfw3webgpu/archive/fdcabcc54b56b50c12c10f5317abf8ae7ac32c29.zip
)

# --- zstd ---
CPMAddPackage(
    NAME zstd
    URL  https://github.com/facebook/zstd/archive/refs/tags/v1.5.7.zip
    SOURCE_SUBDIR build/cmake
    OPTIONS
        "ZSTD_BUILD_SHARED OFF"
        "ZSTD_BUILD_PROGRAMS OFF"
        "ZSTD_BUILD_TESTS OFF"
)

# --- clip ---
CPMAddPackage(
    NAME clip
    URL  https://github.com/dacap/clip/archive/refs/tags/v1.15.zip
    OPTIONS    
        "CLIP_ENABLE_IMAGE ON"
        "CLIP_ENABLE_LIST_FORMATS OFF"
        "CLIP_EXAMPLES OFF"
        "CLIP_TESTS OFF"
        "CLIP_INSTALL OFF"
        "CLIP_X11_WITH_PNG ON"
)

# --- stb ---
CPMAddPackage(
    NAME stb
    URL  https://github.com/nothings/stb/archive/refs/heads/master.zip
    DOWNLOAD_ONLY  YES
)

add_library(stb_headers INTERFACE)
target_include_directories(stb_headers SYSTEM INTERFACE ${stb_SOURCE_DIR})

# --- nativefiledialog-extended ---
CPMAddPackage(
    NAME nfd
    URL  https://github.com/btzy/nativefiledialog-extended/archive/refs/tags/v1.3.0.zip
    OPTIONS
        "NFD_BUILD_TESTS OFF"
        "NFD_PORTAL ON"
)

# --- zpp-bits ---
CPMAddPackage(
    NAME zpp_bits
    URL  https://github.com/eyalz800/zpp_bits/archive/refs/tags/v4.7.1.zip
    DOWNLOAD_ONLY  YES
)
add_library(zpp_bits_headers INTERFACE)
target_include_directories(zpp_bits_headers SYSTEM INTERFACE ${zpp_bits_SOURCE_DIR})

# --- sigslot ---
CPMAddPackage(
    NAME sigslot
    URL  https://github.com/palacaze/sigslot/archive/refs/tags/v1.2.3.zip
)

# --- pfr ---
CPMAddPackage(
    NAME pfr
    URL  https://github.com/apolukhin/pfr_non_boost/archive/refs/tags/2.3.2.zip
)

# --- ImGuiColorTextEdit ---
CPMAddPackage(
    NAME ImGuiColorTextEdit
    URL  https://github.com/goossens/ImGuiColorTextEdit/archive/a74fb090d2ea9276ae6c35c2f6ab39491c7d404f.zip
)

# --- imgui_markdown ---
CPMAddPackage(
    NAME imgui_markdown
    URL  https://github.com/enkisoftware/imgui_markdown/archive/4acbf80584753e15ea54eb271129995862daac8f.zip
)

# --- reproc ---
CPMAddPackage(
    NAME reproc
    URL  https://github.com/DaanDeMeyer/reproc/archive/refs/tags/v14.2.7.zip
    OPTIONS
        "REPROC++ ON"
        "REPROC_TEST OFF"
        "REPROC_EXAMPLES OFF"
        "REPROC_INSTALL OFF"
)

set_target_properties(reproc reproc++ PROPERTIES POSITION_INDEPENDENT_CODE ON)
