cmake_minimum_required(VERSION 3.20)

project(blazingfastcpp_for_anton2920_stream LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(ISPC_EXECUTABLE ispc REQUIRED)
find_package(OpenMP REQUIRED)

set(ISPC_FASTA_TASK_COUNT 24 CACHE STRING "Task count for ispc_parallel_fasta.")
set(ISPC_FASTA_RENDERED_HEIGHT 576 CACHE STRING "Rows rendered by ispc_parallel_fasta for original-output comparison.")
set(ISPC_FASTA_TARGET "avx2-i32x8" CACHE STRING "ISPC target for ispc_parallel_fasta.")
set(ISPC_FASTA_EXTRA_OPTIONS "--opt=force-aligned-memory" CACHE STRING "Extra ISPC compiler options for ispc_parallel_fasta as a CMake list.")

function(enable_fast_math_and_avx2 target)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2 /fp:fast /Ot>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx2 -mfma -ffast-math>
    )
endfunction()

function(add_ispc_renderer target source_dir)
    set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/${source_dir}")
    set(header "${generated_dir}/shader.h")
    set(object "${generated_dir}/shader${CMAKE_CXX_OUTPUT_EXTENSION}")
    set(shader "${CMAKE_CURRENT_SOURCE_DIR}/${source_dir}/shader.ispc")
    set(ispc_target "avx2-i32x8")
    set(ispc_extra_options)
    if(target STREQUAL "ispc_parallel_fasta")
        file(MAKE_DIRECTORY "${generated_dir}")
        set(configured_shader "${generated_dir}/shader.configured.ispc")
        configure_file("${shader}" "${configured_shader}" @ONLY)
        set(shader_input "${configured_shader}")
        set(shader_dependency "${shader}")
        set(ispc_target "${ISPC_FASTA_TARGET}")
        set(ispc_extra_options ${ISPC_FASTA_EXTRA_OPTIONS})
    else()
        set(shader_input "${shader}")
        set(shader_dependency "${shader}")
    endif()

    add_custom_command(
        OUTPUT "${header}" "${object}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${generated_dir}"
        COMMAND "${ISPC_EXECUTABLE}"
            -O3
            --target=${ispc_target}
            ${ispc_extra_options}
            -h "${header}"
            -o "${object}"
            "${shader_input}"
        DEPENDS "${shader_dependency}"
        VERBATIM
        COMMENT "Compiling ${target} ISPC shader"
    )

    set_source_files_properties("${object}" PROPERTIES
        EXTERNAL_OBJECT TRUE
        GENERATED TRUE
    )

    add_executable(${target}
        "${source_dir}/main.cpp"
        ispc-parallel/tasksys.cpp
        "${object}"
    )
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${generated_dir}"
    )
    target_link_libraries(${target} PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(${target} PRIVATE ISPC_USE_OMP)
    enable_fast_math_and_avx2(${target})
endfunction()

add_ispc_renderer(ispc_parallel ispc-parallel)
add_ispc_renderer(ispc_parallel_fasta ispc-parallel_fasta)

add_executable(blazingfastcpp
    BlazingFastCPP_for_anton2920_stream/BlazingFastCPP_for_anton2920_stream.cpp
)
target_link_libraries(blazingfastcpp PRIVATE OpenMP::OpenMP_CXX)
target_include_directories(blazingfastcpp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
enable_fast_math_and_avx2(blazingfastcpp)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_custom_target(benchmark
        COMMAND Python3::Interpreter
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/benchmark.py"
            --build-dir "${CMAKE_BINARY_DIR}"
            --config "$<CONFIG>"
            --skip-build
        DEPENDS ispc_parallel ispc_parallel_fasta blazingfastcpp
        USES_TERMINAL
        COMMENT "Running benchmark comparison"
    )

    add_custom_target(render_images
        COMMAND Python3::Interpreter
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/benchmark.py"
            --build-dir "${CMAKE_BINARY_DIR}"
            --config "$<CONFIG>"
            --skip-build
            --images-only
            --images-dir "${CMAKE_CURRENT_SOURCE_DIR}"
        DEPENDS ispc_parallel ispc_parallel_fasta blazingfastcpp
        USES_TERMINAL
        COMMENT "Rendering comparison images to the project root"
    )
endif()
