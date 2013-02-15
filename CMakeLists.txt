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

find_library(HAVE_PTHREAD  NAMES pthread)

add_subdirectory(kjson)

set(ACTOR_SRC actor.c lfqueue.c sched.c)
add_library(actor SHARED ${ACTOR_SRC})
target_link_libraries(actor kjson ${HAVE_PTHREAD})
## Test cases
enable_testing()
add_executable(test_queue test/test_queue.c)
target_link_libraries(test_queue actor)
add_test(test_queue test_queue)

add_executable(test_sched test/test_sched.c)
target_link_libraries(test_sched actor)
add_test(test_sched test_sched)

add_executable(test_actor test/test_actor.c)
target_link_libraries(test_actor actor)
add_test(test_actor test_actor)

add_executable(test_ring test/ring.c)
target_link_libraries(test_ring actor)
add_test(test_ring test_ring)
