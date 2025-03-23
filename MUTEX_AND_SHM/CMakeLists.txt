cmake_minimum_required(VERSION 3.10)
project(SharedMemoryExample C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

if(NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation directory" FORCE)
endif()

add_executable(increment increment.c)
target_link_libraries(increment rt pthread)

add_executable(reset reset.c)
target_link_libraries(reset rt pthread)

install(TARGETS increment reset
        RUNTIME DESTINATION bin)

install(CODE "execute_process(COMMAND chmod 755 ${CMAKE_INSTALL_PREFIX}/bin/increment)")
install(CODE "execute_process(COMMAND chmod 755 ${CMAKE_INSTALL_PREFIX}/bin/reset)")