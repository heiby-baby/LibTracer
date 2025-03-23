cmake_minimum_required(VERSION 3.10)
project(MyProject LANGUAGES C)

set(CMAKE_C_STANDARD 11)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(test SHARED lib.c)

add_executable(main main.c)
target_link_libraries(main PRIVATE test)

add_library(inject SHARED inject.c)
set_target_properties(inject PROPERTIES
    OUTPUT_NAME "inject"  
    PREFIX ""             
)