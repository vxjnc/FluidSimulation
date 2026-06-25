find_package(Python 3.10 COMPONENTS Interpreter Development.Embed QUIET)

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
        src/scripting/scripting_engine.cpp
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
    target_sources(${PROJECT_NAME} PRIVATE src/scripting/scripting_engine.cpp)
endif()
