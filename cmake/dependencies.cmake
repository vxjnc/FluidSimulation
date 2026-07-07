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

# --- ImGuiColorTextEdit ---
FetchContent_Declare(
    ImGuiColorTextEdit
    GIT_REPOSITORY https://github.com/goossens/ImGuiColorTextEdit.git
    GIT_TAG        a74fb090d2ea9276ae6c35c2f6ab39491c7d404f
    GIT_SHALLOW    ON
)

# --- imgui_markdown ---
FetchContent_Declare(
    imgui_markdown
    GIT_REPOSITORY https://github.com/enkisoftware/imgui_markdown.git
    GIT_TAG        4acbf80584753e15ea54eb271129995862daac8f
    GIT_SHALLOW    ON
)

# --- reproc ---
FetchContent_Declare(
    reproc
    GIT_REPOSITORY https://github.com/DaanDeMeyer/reproc.git
    GIT_TAG        v14.2.7
    GIT_SHALLOW    ON
)
set(REPROC++ ON CACHE BOOL "" FORCE)
set(REPROC_TEST OFF CACHE BOOL "" FORCE)
set(REPROC_EXAMPLES OFF CACHE BOOL "" FORCE)
set(REPROC_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(
    glfw
    webgpu_distribution
    imgui
    glfw3webgpu
    clip
    reproc
    stb
    nfd
    zpp_bits
    sigslot
    pfr
    wesl_distribution
    ImGuiColorTextEdit
    imgui_markdown
)
set_target_properties(glfw webgpu PROPERTIES SYSTEM TRUE)

# --- ImGui (compiled as static lib) ---
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

# --- clip (needs ZLIB) ---
find_package(ZLIB REQUIRED)
target_link_libraries(clip ZLIB::ZLIB)

# --- Header-only interface targets ---
add_library(stb_headers INTERFACE)
target_include_directories(stb_headers SYSTEM INTERFACE ${stb_SOURCE_DIR})

add_library(zpp_bits_headers INTERFACE)
target_include_directories(zpp_bits_headers SYSTEM INTERFACE ${zpp_bits_SOURCE_DIR})

# --- reproc++ ---
set_target_properties(reproc reproc++ PROPERTIES POSITION_INDEPENDENT_CODE ON)
