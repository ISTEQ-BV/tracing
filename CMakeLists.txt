cmake_minimum_required(VERSION 3.15...3.27)

project(tracing VERSION 1.0.0 LANGUAGES C CXX)
add_compile_options(-Wall -Wextra)

add_library(tracing
    tracing.h
    tracing.c
    README.md
)
target_include_directories(tracing PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(tracing PUBLIC c_std_99)

add_executable(test_tracing_threads
    test_tracing_threads.cpp
)
target_link_libraries(test_tracing_threads PRIVATE tracing)
target_compile_features(test_tracing_threads PRIVATE cxx_std_17)
