include(CheckTypeSize)
include(CheckIncludeFiles)

cmake_minimum_required(VERSION 2.4)
project(kjson)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

set(JSON_TEST
        "./test/test01.json"
        "./test/test02.json"
        "./test/test03.json"
        "./test/test04.json"
        "./test/test05.json"
        "./test/test06.json"
        "./test/test07.json"
        "./test/test08.json"
        "./test/test09.json"
        "./test/test10.json"
        "./test/test11.json")

set(JSON_BENCHMARK
        "./test/benchmark1.json"
        "./test/benchmark2.json"
        "./test/benchmark3.json"
        "./test/benchmark4.json"
        "./test/benchmark5.json"
        "./test/benchmark6.json"
        "./test/benchmark7.json")

SET(CMAKE_C_FLAGS         "-Wall -Wno-variadic-macros")
SET(CMAKE_C_FLAGS_RELEASE "-O3 -g")
SET(CMAKE_C_FLAGS_DEBUG   "-O0 -g3")

set(JSON_SRC ./kjson.c ./map.c ./stream.c)
add_library(kjson ${JSON_SRC})

## Test cases
set(JSON_TEST_CODE_LIST)
set(JSON_BENCH_CODE_LIST)
foreach(it ${JSON_TEST})
	set(JSON_TEST_CODE_LIST ${JSON_TEST_CODE_LIST}
            ${CMAKE_CURRENT_SOURCE_DIR}/${it})
endforeach()

foreach(it ${JSON_BENCH})
	set(JSON_BENCH_CODE_LIST ${JSON_BENCH_CODE_LIST}
            ${CMAKE_CURRENT_SOURCE_DIR}/${it})
endforeach()

enable_testing()
add_executable(test_kjson test/test_kjson.c)
target_link_libraries(test_kjson kjson)
add_test(test_kjson test_kjson ${JSON_TEST_CODE_LIST}
        ${JSON_BENCH_CODE_LIST})

add_executable(test_stream test/test_stream.c)
target_link_libraries(test_stream kjson)
add_test(test_stream test_stream ${JSON_TEST_CODE_LIST}
        ${JSON_BENCH_CODE_LIST})

add_executable(test_stringbuilder test/test_string_builder.c)
add_test(test_stringbuilder test_stringbuilder)

add_executable(test_tostring test/test_tostring.c)
target_link_libraries(test_tostring kjson)
add_test(test_tostring test_tostring)

