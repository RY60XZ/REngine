cmake_minimum_required(VERSION 3.20)
project(cpp_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(rengine_core STATIC
        src/board.cpp
        src/fen.cpp
        src/magic_tables.cpp
        src/movegen.cpp
        src/make_move.cpp
        src/perft.cpp
)

target_include_directories(rengine_core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(rengine_core
        PRIVATE
        $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -Wpedantic>
)

add_executable(cpp_engine src/main.cpp)
target_link_libraries(cpp_engine PRIVATE rengine_core)

add_executable(test_movegen tests/test_movegen.cpp)
target_link_libraries(test_movegen PRIVATE rengine_core)

add_executable(test_make_unmake tests/test_make_unmake.cpp)
target_link_libraries(test_make_unmake PRIVATE rengine_core)

add_executable(perft_test tests/perft_test.cpp)
target_link_libraries(perft_test PRIVATE rengine_core)

add_executable(test_move_format tests/test_move_format.cpp)
target_link_libraries(test_move_format PRIVATE rengine_core)

add_executable(bench_perft bench/bench_perft.cpp)
target_link_libraries(bench_perft PRIVATE rengine_core)

enable_testing()

add_test(NAME movegen COMMAND test_movegen)
add_test(NAME make_unmake COMMAND test_make_unmake)
add_test(NAME perft COMMAND perft_test)
add_test(NAME move_format COMMAND test_move_format)
