cmake_minimum_required(VERSION 3.20)

project(blazingfastcpp_for_anton2920_stream LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(ISPC_EXECUTABLE ispc REQUIRED)
find_package(OpenMP REQUIRED)

function(enable_fast_math_and_avx2 target)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2 /fp:fast /Ot>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx2 -mfma -ffast-math>
    )
endfunction()

set(ISPC_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/ispc-parallel")
set(ISPC_HEADER "${ISPC_GENERATED_DIR}/shader.h")
set(ISPC_OBJECT "${ISPC_GENERATED_DIR}/shader${CMAKE_CXX_OUTPUT_EXTENSION}")

add_custom_command(
    OUTPUT "${ISPC_HEADER}" "${ISPC_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${ISPC_GENERATED_DIR}"
    COMMAND "${ISPC_EXECUTABLE}"
        -O3
        --target=avx2-i32x8
        -h "${ISPC_HEADER}"
        -o "${ISPC_OBJECT}"
        "${CMAKE_CURRENT_SOURCE_DIR}/ispc-parallel/shader.ispc"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ispc-parallel/shader.ispc"
    VERBATIM
    COMMENT "Compiling ISPC shader"
)

add_custom_target(generate_ispc_shader DEPENDS "${ISPC_HEADER}" "${ISPC_OBJECT}")

set_source_files_properties("${ISPC_OBJECT}" PROPERTIES
    EXTERNAL_OBJECT TRUE
    GENERATED TRUE
)

add_executable(ispc_parallel
    ispc-parallel/main.cpp
    ispc-parallel/tasksys.cpp
    "${ISPC_OBJECT}"
)
add_dependencies(ispc_parallel generate_ispc_shader)
target_include_directories(ispc_parallel PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${ISPC_GENERATED_DIR}"
)
target_link_libraries(ispc_parallel PRIVATE OpenMP::OpenMP_CXX)
target_compile_definitions(ispc_parallel PRIVATE ISPC_USE_OMP)
enable_fast_math_and_avx2(ispc_parallel)

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
        DEPENDS ispc_parallel blazingfastcpp
        USES_TERMINAL
        COMMENT "Running benchmark comparison"
    )
endif()
