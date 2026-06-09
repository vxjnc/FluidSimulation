set(WGSL_SHADER_OUT "${GENERATED_DIR}/shaders")
file(MAKE_DIRECTORY "${WGSL_SHADER_OUT}")

function(compile_wesl_to_header SHADER_FILE OUT_HEADERS_VAR)
    get_filename_component(NAME ${SHADER_FILE} NAME_WE)
    get_filename_component(EXT ${SHADER_FILE} EXT)

    set(H_FILE "${WGSL_SHADER_OUT}/${NAME}.wgsl.h")
    set(VAR_NAME "${NAME}_wgsl")

    set(INPUT_FOR_HEADER_GEN ${SHADER_FILE})
    set(WESL_COMMANDS)
    set(WESL_DEPS)

    if(EXT STREQUAL ".wesl")
        set(INPUT_FOR_HEADER_GEN "${WGSL_SHADER_OUT}/${NAME}.wgsl")
        set(WESL_COMMANDS COMMAND wesl-cli compile ${SHADER_FILE} > ${INPUT_FOR_HEADER_GEN})
        set(WESL_DEPS wesl-cli)
    endif()

    add_custom_command(
        OUTPUT ${H_FILE}
        ${WESL_COMMANDS}
        COMMAND ${CMAKE_COMMAND} 
            -DINPUT_FILE=${INPUT_FOR_HEADER_GEN} 
            -DOUTPUT_FILE=${H_FILE} 
            -DVAR_NAME=${VAR_NAME}
            -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/wgsl_to_h.cmake"
        DEPENDS ${SHADER_FILE} ${WESL_DEPS} "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/wgsl_to_h.cmake"
        COMMENT "Generating header for ${NAME}"
        VERBATIM
    )

    set(${OUT_HEADERS_VAR} ${${OUT_HEADERS_VAR}} ${H_FILE} PARENT_SCOPE)
endfunction()

file(GLOB_RECURSE WGSL_SOURCES
    "${CMAKE_SOURCE_DIR}/src/render/shaders/*.wgsl"
    "${CMAKE_SOURCE_DIR}/src/compute/shaders/*.wgsl"
    "${CMAKE_SOURCE_DIR}/src/render/shaders/*.wesl"
    "${CMAKE_SOURCE_DIR}/src/compute/shaders/*.wesl"
)
set(WGSL_HEADERS "")

foreach(FILE ${WGSL_SOURCES})
    compile_wesl_to_header(${FILE} WGSL_HEADERS)
endforeach()

add_custom_target(wgsl_shaders ALL DEPENDS ${WGSL_HEADERS})
