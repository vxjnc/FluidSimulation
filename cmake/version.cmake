# APP_VERSION -> git describe -> dev + дата
if(NOT DEFINED APP_VERSION)
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE APP_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    endif()
    if(NOT APP_VERSION)
        string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M")
        set(APP_VERSION "dev (${BUILD_DATE})")
    endif()
endif()

set(VERSION_HEADER "${GENERATED_DIR}/version.h")
file(WRITE ${VERSION_HEADER} "#pragma once\n")
file(APPEND ${VERSION_HEADER} "#include <string_view>\n\n")
file(APPEND ${VERSION_HEADER} "constexpr std::string_view APP_VERSION_STRING = \"${APP_VERSION}\";\n")

message(STATUS "Application version: ${APP_VERSION}")
