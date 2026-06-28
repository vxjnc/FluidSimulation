find_package(Python 3.10 COMPONENTS Interpreter Development QUIET)

if(Python_FOUND)
    message(STATUS "Python ${Python_VERSION} found - scripting enabled")

    FetchContent_Declare(
        nanobind
        GIT_REPOSITORY https://github.com/wjakob/nanobind.git
        GIT_TAG        v2.13.0
        GIT_SHALLOW    ON
    )
    FetchContent_MakeAvailable(nanobind)

    add_library(fluid_scripting SHARED
        src/scripting/scripting_engine.cpp
        src/scripting/bindings.cpp
        src/utils/python_find.cpp
        ${nanobind_SOURCE_DIR}/src/nb_combined.cpp
    )
    target_include_directories(fluid_scripting PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${nanobind_SOURCE_DIR}/include
        ${nanobind_SOURCE_DIR}/ext/robin_map/include
        ${Python_INCLUDE_DIRS}
    )
    target_compile_definitions(fluid_scripting PRIVATE SCRIPTING_AVAILABLE)
    target_link_options(fluid_scripting PRIVATE -Wl,--allow-shlib-undefined)
    set_target_properties(fluid_scripting PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )

    target_compile_definitions(fluid_gui PUBLIC SCRIPTING_AVAILABLE)
    target_link_libraries(fluid_gui PUBLIC dl)

else()
    message(STATUS "Python not found - scripting disabled")
endif()
