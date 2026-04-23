cmake_minimum_required(VERSION 3.10)
project(malloc C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -O2")

add_executable(code
    mm.c
    memlib.c
    mdriver.c
)

target_link_libraries(code m)
