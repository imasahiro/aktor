include(CheckTypeSize)
include(CheckIncludeFiles)

cmake_minimum_required(VERSION 2.4)
project(actor)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

SET(CMAKE_C_FLAGS         "-Wall -Wno-variadic-macros")
SET(CMAKE_C_FLAGS_RELEASE "-O3 -g")
SET(CMAKE_C_FLAGS_DEBUG   "-O0 -g3")

add_subdirectory(kjson)

set(ACTOR_SRC actor.c lfqueue.c)
add_library(actor ${ACTOR_SRC})
target_link_libraries(actor kjson)

## Test cases
enable_testing()
add_executable(test_lfqueue test/test_lfqueue.c)
target_link_libraries(test_lfqueue actor)
add_test(test_lfqueue test_lfqueue)

##add_executable(test_stringbuilder test/test_string_builder.c)
##add_test(test_stringbuilder test_stringbuilder)
