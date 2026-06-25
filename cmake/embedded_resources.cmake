function(embed_bin_resource INPUT_FILE OUTPUT_DIR VAR_PREFIX)
    set(TEXT_MODE ${ARGV3})
    if(TEXT_MODE)
        set(TEXT_MODE_ARG "-DTEXT_MODE=1")
    endif()

    get_filename_component(RAW_NAME ${INPUT_FILE} NAME_WE)

    string(REPLACE " " "_" SAFE_NAME "${RAW_NAME}")
    string(REPLACE "-" "_" SAFE_NAME "${SAFE_NAME}")

    string(MAKE_C_IDENTIFIER "${SAFE_NAME}" VAR_IDENTIFIER)

    set(H_FILE "${OUTPUT_DIR}/${SAFE_NAME}.h")
    set(ARR_NAME "s_${VAR_PREFIX}_${VAR_IDENTIFIER}")

    file(MAKE_DIRECTORY "${OUTPUT_DIR}")

    add_custom_command(
        OUTPUT ${H_FILE}
        COMMAND ${CMAKE_COMMAND} -DINPUT_FILE=${INPUT_FILE} 
                                 -DOUTPUT_FILE=${H_FILE} 
                                 -DVAR_NAME=${ARR_NAME} 
                                 ${TEXT_MODE_ARG}
                                 -P "${CMAKE_MODULE_PATH}/bin2c.cmake"
        DEPENDS ${INPUT_FILE} "${CMAKE_MODULE_PATH}/bin2c.cmake"
        COMMENT "Embedding resource: ${RAW_NAME}"
        VERBATIM
    )

    set(G_RES_HEADERS ${G_RES_HEADERS} ${H_FILE} PARENT_SCOPE)
endfunction()

embed_bin_resource("${CMAKE_SOURCE_DIR}/assets/NotoSans-Regular.ttf" "${GENERATED_DIR}/fonts" "font")
embed_bin_resource("${CMAKE_SOURCE_DIR}/assets/JetBrainsMono-Regular.ttf" "${GENERATED_DIR}/fonts" "font")

add_custom_target(embedded_resources DEPENDS ${G_RES_HEADERS})
