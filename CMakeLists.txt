cmake_minimum_required(VERSION 2.8)

project(renderfall C)

set(MAJOR_VERSION 0)
set(MINOR_VERSION 1)
set(PACKAGE renderfall)
set(VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION})
set(VERSION ${VERSION_STRING})
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -pedantic -Wextra")

add_subdirectory(src)
