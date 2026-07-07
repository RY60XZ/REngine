cmake_minimum_required(VERSION 3.23)
project(cpp_engine LANGUAGES CXX)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(RENGINE_CORE_SOURCES
        src/board.cpp
        src/fen.cpp
        src/magic_tables.cpp
        src/movegen.cpp
        src/make_move.cpp
        src/perft.cpp
        src/eval.cpp
        src/search.cpp
        src/move_ordering.cpp
        src/zobrist.cpp
        src/tt.cpp
        src/time.cpp
        src/uci.cpp
)

set(RENGINE_CORE_HEADERS
        include/rengine/attack.h
        include/rengine/board.h
        include/rengine/fen.h
        include/rengine/magic_tables.h
        include/rengine/make_move.h
        include/rengine/move.h
        include/rengine/move_format.h
        include/rengine/move_list.h
        include/rengine/move_ordering.h
        include/rengine/movegen.h
        include/rengine/perft.h
        include/rengine/square.h
        include/rengine/tt.h
        include/rengine/types.h
        include/rengine/eval.h
        include/rengine/search_types.h
        include/rengine/search_stack.h
        include/rengine/search.h
        include/rengine/draw.h
        include/rengine/zobrist.h
        include/rengine/time.h
        include/rengine/uci.h
)

add_library(rengine_core STATIC
        ${RENGINE_CORE_SOURCES}
)

target_sources(rengine_core
        PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS include
        FILES ${RENGINE_CORE_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include
        PREFIX "Header Files"
        FILES ${RENGINE_CORE_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
        PREFIX "Source Files"
        FILES ${RENGINE_CORE_SOURCES}
)

target_include_directories(rengine_core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rengine_core
        PUBLIC
        Threads::Threads
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

add_executable(test_move_ordering tests/test_move_ordering.cpp)
target_link_libraries(test_move_ordering PRIVATE rengine_core)

add_executable(test_zobrist tests/test_zobrist.cpp)
target_link_libraries(test_zobrist PRIVATE rengine_core)

add_executable(test_eval tests/test_eval.cpp)
target_link_libraries(test_eval PRIVATE rengine_core)

add_executable(test_search_smoke tests/test_search_smoke.cpp)
target_link_libraries(test_search_smoke PRIVATE rengine_core)

add_executable(test_uci tests/test_uci.cpp)
target_link_libraries(test_uci PRIVATE rengine_core)

add_executable(bench_perft bench/bench_perft.cpp)
target_link_libraries(bench_perft PRIVATE rengine_core)

add_executable(bench_search bench/bench_search.cpp)
target_link_libraries(bench_search PRIVATE rengine_core)

enable_testing()

add_test(NAME movegen COMMAND test_movegen)
add_test(NAME make_unmake COMMAND test_make_unmake)
add_test(NAME perft COMMAND perft_test)
add_test(NAME move_format COMMAND test_move_format)
add_test(NAME move_ordering COMMAND test_move_ordering)
add_test(NAME zobrist COMMAND test_zobrist)
add_test(NAME eval COMMAND test_eval)
add_test(NAME search_smoke COMMAND test_search_smoke)
add_test(NAME uci COMMAND test_uci)
