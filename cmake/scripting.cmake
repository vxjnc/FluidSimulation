find_package(Python 3.12 COMPONENTS Interpreter Development QUIET OPTIONAL_COMPONENTS Development.SABIModule)

if(Python_FOUND)
    message(STATUS "Python ${Python_VERSION} found - scripting enabled")

    FetchContent_Declare(
        nanobind
        GIT_REPOSITORY https://github.com/wjakob/nanobind.git
        GIT_TAG        v2.13.0
        GIT_SHALLOW    ON
    )
    FetchContent_MakeAvailable(nanobind)

    function(configure_scripting_target target_name bindings_source output_dir)
        add_library(${target_name} SHARED
            src/scripting/scripting_engine.cpp
            ${bindings_source}
            src/utils/python_find.cpp
            ${nanobind_SOURCE_DIR}/src/nb_combined.cpp
        )
        target_include_directories(${target_name} PRIVATE
            ${CMAKE_SOURCE_DIR}
            ${nanobind_SOURCE_DIR}/include
            ${nanobind_SOURCE_DIR}/ext/robin_map/include
            ${Python_INCLUDE_DIRS}
        )
        target_link_libraries(${target_name} PRIVATE nfd reproc++)
        target_compile_definitions(${target_name} PRIVATE SCRIPTING_AVAILABLE Py_LIMITED_API=0x030C0000)
        if(MSVC)
            target_link_libraries(${target_name} PRIVATE Python::SABIModule)
        else()
            target_link_options(${target_name} PRIVATE -Wl,--allow-shlib-undefined -s)
        endif()
        set_target_properties(${target_name} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${output_dir}"
            RUNTIME_OUTPUT_DIRECTORY "${output_dir}"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${output_dir}"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${output_dir}"
        )
    endfunction()

    configure_scripting_target(fluid_scripting src/scripting/bindings.cpp "${CMAKE_SOURCE_DIR}")

    target_compile_definitions(fluid_gui PUBLIC SCRIPTING_AVAILABLE)
    if(UNIX)
        target_link_libraries(fluid_gui PUBLIC dl)
    endif()

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

    configure_scripting_target(fluid_scripting_stubs "${STUBS_BINDINGS}" "${CMAKE_BINARY_DIR}")
    set_target_properties(fluid_scripting_stubs PROPERTIES EXCLUDE_FROM_ALL TRUE)

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
