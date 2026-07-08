add_executable(${PROJECT_NAME}_benchmark 
    sim_benchmark.cpp 
    ${SOURCE_FILES}
)

target_link_libraries(${PROJECT_NAME}_benchmark PRIVATE
    fluid_gui
    benchmark::benchmark
)

if(NOT MSVC)
    target_compile_options(${PROJECT_NAME}_benchmark PRIVATE
        -Wall -Wextra -Wconversion
        -march=native -mtune=native
        -O3
    )
    set_property(TARGET ${PROJECT_NAME}_benchmark PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
endif()

target_copy_webgpu_binaries(${PROJECT_NAME}_benchmark)
