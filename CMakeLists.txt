cmake_minimum_required(VERSION 3.10)
project(malloc C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -O2")

# Check if mdriver.c exists (OJ might provide it)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mdriver.c")
    add_executable(code
        mm.c
        memlib.c
        mdriver.c
    )
else()
    # Fallback: create a minimal executable
    add_executable(code
        mm.c
        memlib.c
    )
    # Define main in mm.c or create a stub
endif()

target_link_libraries(code m)
