find_package(Python 3.12 COMPONENTS Interpreter Development QUIET)

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
    target_link_libraries(fluid_scripting PRIVATE nfd)
    target_compile_definitions(fluid_scripting PRIVATE SCRIPTING_AVAILABLE)
    target_compile_definitions(fluid_gui PUBLIC SCRIPTING_AVAILABLE)
    if(MSVC)
        target_link_options(fluid_scripting PRIVATE /FORCE:UNRESOLVED)
    else()
        target_link_options(fluid_scripting PRIVATE -Wl,--allow-shlib-undefined)
    endif()
    if(UNIX)
        target_link_libraries(fluid_gui PUBLIC dl)
    endif()
    set_target_properties(fluid_scripting PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )

    # Generate stubs
    set(STUBS_BINDINGS "${CMAKE_BINARY_DIR}/bindings_stubs.cpp")

    add_custom_command(
        OUTPUT ${STUBS_BINDINGS}
        COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_stubs.py
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/src/scripting/bindings.cpp
            ${STUBS_BINDINGS}
        DEPENDS ${CMAKE_SOURCE_DIR}/src/scripting/bindings.cpp
        COMMENT "Generating bindings_stubs.cpp"
    )

    add_library(fluid_scripting_stubs SHARED
        src/scripting/scripting_engine.cpp
        ${STUBS_BINDINGS}
        src/utils/python_find.cpp
        ${nanobind_SOURCE_DIR}/src/nb_combined.cpp
    )
    target_include_directories(fluid_scripting_stubs PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${nanobind_SOURCE_DIR}/include
        ${nanobind_SOURCE_DIR}/ext/robin_map/include
        ${Python_INCLUDE_DIRS}
    )
    target_link_libraries(fluid_scripting_stubs PRIVATE nfd)
    target_compile_definitions(fluid_scripting_stubs PRIVATE SCRIPTING_AVAILABLE)
    if(MSVC)
        target_link_options(fluid_scripting_stubs PRIVATE /FORCE:UNRESOLVED)
    else()
        target_link_options(fluid_scripting_stubs PRIVATE -Wl,--allow-shlib-undefined)
    endif()
    set_target_properties(fluid_scripting_stubs PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        EXCLUDE_FROM_ALL TRUE
    )

    add_custom_target(generate_stubs
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:fluid_scripting_stubs>
            ${CMAKE_SOURCE_DIR}/fluidsim.so
        COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_SOURCE_DIR}
            ${Python_EXECUTABLE} -m nanobind.stubgen -m fluidsim -r -O ${CMAKE_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_SOURCE_DIR}/fluidsim.so
        DEPENDS fluid_scripting_stubs
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating .pyi stubs"
    )
else()
    message(STATUS "Python not found - scripting disabled")
endif()
